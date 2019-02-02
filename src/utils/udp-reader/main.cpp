#include <QCoreApplication>
#include <QUdpSocket>

int main(int argc, char *argv[])
{
    if (argc < 2) {
        qDebug("Usage: %s <port>", argv[0]);
        return EXIT_FAILURE;
    }

    QCoreApplication app(argc, argv);

    QUdpSocket socket;
    socket.bind(QString(argv[1]).toUShort(), QUdpSocket::ShareAddress);

    QObject::connect(&socket, &QUdpSocket::readyRead, [&socket]() -> void 
    {
        QHostAddress address;
        QByteArray datagram(socket.pendingDatagramSize(), 0);
        socket.readDatagram(datagram.data(), datagram.size(), &address);
        qDebug("%s > %s", address.toString().split(":", QString::SkipEmptyParts).last().toStdString().c_str(),
                          QString::fromLocal8Bit(datagram.data()).simplified().toStdString().c_str());
    });

    return app.exec();
}
