#include <QCoreApplication>
#include <QUdpSocket>
#include <QSettings>

void listen(QUdpSocket * socket, const QStringList & args)
{
    Q_ASSERT(socket);
    quint16 port = ( (args.count() < 3) ? quint16(QSettings(CONFIG_PATH "/.qtlogger-rc", QSettings::NativeFormat).value("default-dest-port").toInt())
                                        : args.at(2).toUShort() );

    socket->bind(port, QUdpSocket::ShareAddress);

    qInfo(" ***** Listening on %u *****", port);

    QObject::connect(socket, &QUdpSocket::readyRead, [socket]() -> void
    {
        QHostAddress address;
        QByteArray datagram(int(socket->pendingDatagramSize()), 0);
        socket->readDatagram(datagram.data(), datagram.size(), &address);

        qInfo().noquote() << " *"
                          << address.toString().split(":", QString::SkipEmptyParts).last()
                          << ">>"
                          << QString::fromLocal8Bit(datagram.data()).simplified();
    });
}

qint64 send(QUdpSocket * socket, const QStringList & args)
{
    Q_ASSERT(socket);
    return socket->writeDatagram( args.mid(1).join(" ").toLocal8Bit(),
                                  QHostAddress(QHostAddress::Broadcast),
                                  quint16(QSettings(CONFIG_PATH "/.qtlogger-rc", QSettings::NativeFormat).value("command-port").toInt()) );
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        qInfo("Usage: %s <command> [args]\n\n"
              "LISTENER MODE\n"
              "  listen [port]\n"
              "      Listen on port [port] or default dest port from .qtlogger-rc\n\n"
              "COMMAND MODE\n"
              "  \"[hostname:]<app>\" <command> [args]\n"
              "  \"[hostname:]<app>\" is a client endpoint string\n"
              "    [hostname] and <app> can be specified by regular expression\n\n"
              "  commands:\n"
              "    status [address:][port]\n"
              "      Request for clients status on [address:][port] or on sender\n"
              "      address and default dest port from .qtlogger-rc\n\n"
              "  redirecting commands:\n"
              "    echo <mode> [args]\n"
              "    <mode> = mute | stderr | file | udp\n"
              "      mute\n"
              "        Mute client\n"
              "      stderr\n"
              "        Redirect client output to stderr stream\n"
              "      file [file-path] [flush-period-msec]\n"
              "        Redirect client output to file [file-path] with flush period [flush-period-msec]\n"
              "        or %s.log with default flush period from .qtlogger-rc\n"
              "      udp [address:][port]\n"
              "        Redirect client output to [address:][port] or on sender\n"
              "        address and default dest port from .qtlogger-rc\n\n"
              "  filtering commands:\n"
              "    filter <operation> [type] [arg]\n"
              "    <operation> = add | del | clear\n"
              "      add\n"
              "        Add filter [type]\n"
              "      del\n"
              "        Delete filter [type]\n"
              "      clear\n"
              "        Clear all filters with [type] or all if no one specified\n"
              "    <type> = level | file | function\n"
              "      level <name>\n"
              "        <name> = info | debug | warning | critical | fatal\n"
              "      file <name>\n"
              "        Filter by file <name> where log message has been posted\n"
              "      function <name>\n"
              "        Filter by function <name> where log message has been posted\n\n"
              "EXAMPLES\n"
              "  Request for all clients status\n"
              "    %s \".*\" status\n"
              "  Redirect \"myamazyngtask\" output from \"179U1\" host to udp\n"
              "    %s \"179U1:myamaz\" echo udp\n"
              "  Show only debug messages from \"shittyFunc\"\n"
              "    %s \"179U1:mydaemon\" filter add level debug\n"
              "    %s \"179U1:mydaemon\" filter add function shittyFunc\n"
              "  You can use such short names as possible\n"
              "    %s \"ArmKK:taskbar\" f a l d\n"
              "    f a l d = filter add level debug\n", argv[0], argv[0], argv[0], argv[0], argv[0], argv[0], argv[0]);
        return EXIT_FAILURE;
    }

    QCoreApplication app(argc, argv);
    const auto & args = app.arguments();

    QUdpSocket socket;

    if (args.at(1) != QString("listen")) {
        const auto result =  ( (send(&socket, args) < 0) ? EXIT_FAILURE
                                                         : 0 );
        socket.waitForBytesWritten();
        return result;
    }

    listen(&socket, args);
    return app.exec();
}
