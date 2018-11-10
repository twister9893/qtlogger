#include "logger.h"
#include "logger-private.h"

#include <QCoreApplication>
#include <QNetworkDatagram>
#include <QTime>
#include <QDir>
#include <stdio.h>

#include "ansi-colors.h"

// TODO: Add runtime config
// TODO: Add level filtering
// TODO: Add func filtering

using namespace qtlogger;

Logger::~Logger()
{}

void Logger::exec(const QString &command)
{
    instance().d_ptr->exec(command);
}

void Logger::info(const QString &msg, const QMessageLogContext &context)
{
    instance().d_ptr->log(msg);
}

void Logger::debug(const QString &msg, const QMessageLogContext &context)
{
    instance().d_ptr->log( QString(GRAY "[%1] DEBG <%2> %3" RESET).arg(QTime::currentTime().toString().toLocal8Bit().constData())
                                                                  .arg(LoggerPrivate::appNameString())
                                                                  .arg(msg));
}

void Logger::warning(const QString &msg, const QMessageLogContext &context)
{
    instance().d_ptr->log( QString("[%1] " YELLOW "WARN <%2> " RESET "%3").arg(QTime::currentTime().toString().toLocal8Bit().constData())
                                                                          .arg(LoggerPrivate::appNameString())
                                                                          .arg(msg));
}

void Logger::critical(const QString &msg, const QMessageLogContext &context)
{
    instance().d_ptr->log( QString("[%1] " RED "CRIT <%2> " RESET "%3").arg(QTime::currentTime().toString().toLocal8Bit().constData())
                                                                       .arg(LoggerPrivate::appNameString())
                                                                       .arg(msg));
}

void Logger::fatal(const QString &msg, const QMessageLogContext &context)
{
    instance().d_ptr->log( QString(RED "[%1] FATL <%2> terminated at %3:%4 " RESET "%5").arg(QTime::currentTime().toString().toLocal8Bit().constData())
                                                                                        .arg(LoggerPrivate::appNameString())
                                                                                        .arg(context.file)
                                                                                        .arg(context.line)
                                                                                        .arg(msg));
}

Logger::Logger() :
    d_ptr(new LoggerPrivate)
{}

Logger & Logger::instance()
{
    static Logger instance;
    return instance;
}

LoggerPrivate::LoggerPrivate()
{
    connect(&echoFileFlushTimer, &QTimer::timeout, this, &LoggerPrivate::flushEchoFile);

    connect(this, &LoggerPrivate::toggleStdErr, this, &LoggerPrivate::onEchoModeChanged);
    connect(this, &LoggerPrivate::toggleFile, this, &LoggerPrivate::onEchoModeChanged);
    connect(this, &LoggerPrivate::toggleUdp, this, &LoggerPrivate::onEchoModeChanged);
    connect(this, &LoggerPrivate::toggleMute, this, &LoggerPrivate::onEchoModeChanged);

    connect(this, &LoggerPrivate::toggleStdErr, this, &LoggerPrivate::switchToStdErr);
    connect(this, &LoggerPrivate::toggleFile, this, &LoggerPrivate::switchToFile);
    connect(this, &LoggerPrivate::toggleUdp, this, &LoggerPrivate::switchToUdp);
    connect(this, &LoggerPrivate::toggleMute, this, &LoggerPrivate::switchToMute);

    exec(argCommandString());

    commandSocket.bind(commandPort, QUdpSocket::ShareAddress);
    connect(&commandSocket, &QUdpSocket::readyRead, this, &LoggerPrivate::onCommandReceived);

}

void LoggerPrivate::log(const QString &msg)
{
    switch (echo)
    {
        case Echo::StdErr:
            fprintf(stderr, "%s\n", msg.toLocal8Bit().constData());
            break;
        case Echo::File:
            echoFileStream << msg << "\n";
            break;
        case Echo::Udp:
            echoSocket.writeDatagram( (msg+"\n").toLocal8Bit().constData(),
                                      echoDestinationAddress,
                                      echoDestinationPort );
            break;
        default:
            break;
    }
}

void LoggerPrivate::exec(const QString &command)
{
    processCommand((LoggerPrivate::appNameString() + " " + command).split(" ", QString::SkipEmptyParts));
}

void LoggerPrivate::processCommand(const QStringList &command, const QHostAddress &sender)
{
    if (command.size() < 2) { return; }
    const auto &action = command.at(1).simplified();

    if (QString("status").startsWith(action))
    {
        const auto &address = (sender.isNull() ? QHostAddress::Broadcast : sender);
        const auto &port = (command.size() < 3 ? commandPort : command.at(2).toUShort());
        writeStatus(address, port);
    }
    else if (QString("echo").startsWith(action))
    {
        const auto &echoMode = command.at(2).simplified();

        if (QString("stderr").startsWith(echoMode))
        {
            emit toggleStdErr();
        }
        else if (QString("file").startsWith(echoMode))
        {
            const auto &filePath = ( command.size() < 4 ? (appNameString() + ".log") : command.at(3) );
            const auto &flushPeriodMsec = ( command.size() < 5 ? 5000 : command.at(4).toInt() );
            emit toggleFile(filePath, flushPeriodMsec);
        }
        else if (QString("udp").startsWith(echoMode))
        {
            const auto &address = (sender.isNull() ? QHostAddress::Broadcast : sender);
            const auto &port = ( command.size() < 4 ? commandPort : command.at(3).toUShort() );
            emit toggleUdp(address, port);
        }
    }
    else if (QString("mute").startsWith(action))
    {
        emit toggleMute();
    }
}

void LoggerPrivate::writeStatus(const QHostAddress &address, quint16 port) const
{
    static QUdpSocket socket;
    socket.writeDatagram( statusString().toLocal8Bit(), address, port);
}

QString LoggerPrivate::statusString() const
{
    QString string = QString("[%1] %2").arg(appNameString());
    switch (echo) {
        case Echo::Mute:   return string.arg( QString("Muted\n") );
        case Echo::StdErr: return string.arg( QString("Writing in stderr stream\n") );
        case Echo::File:   return string.arg( QString("Writing in file\n") );
        case Echo::Udp:    return string.arg( QString("Writing to %1:%2\n").arg(echoDestinationAddress.toString())
                                                                                   .arg(echoDestinationPort) );
        default:
            break;
    }
    return string.arg( QString("N/D\n") );
}

QString LoggerPrivate::appNameString()
{
    static QString string;
    if (string.isEmpty())
    {
        const auto *app = QCoreApplication::instance();
        string = (app ? app->arguments().first().split(QDir::separator()).last()
                      : "unknown");
    }
    return string;
}

QString LoggerPrivate::argCommandString()
{
    static QString string;
    static bool read = false;
    if (!read)
    {
        const auto *app = QCoreApplication::instance();
        if (app)
        {
            const auto &args = app->arguments();
            for(auto it = args.cbegin(), ite = args.cend(); it != ite; ++it)
            {
                if (it->contains("--qtlogger"))
                {
                    string = it->split("=", QString::SkipEmptyParts).last();
                }
            }
        }
        read = true;
    }
    return string;
}

void LoggerPrivate::switchToStdErr()
{
    echo = Echo::StdErr;
}

void LoggerPrivate::switchToFile(const QString &filePath, int flushPeriodMsec)
{
    echo = Echo::File;

    static QFile file;
    file.setFileName(filePath);
    if (!file.open(QFile::WriteOnly))
    {
        emit toggleStdErr();
        qCritical() << Q_FUNC_INFO << "Echo file opening failed," << file.errorString();
        return;
    }

    echoFileStream.setDevice(&file);
    echoFileFlushTimer.start(flushPeriodMsec);
}

void LoggerPrivate::switchToUdp(const QHostAddress &address, quint16 port)
{
    echo = Echo::Udp;
    echoDestinationAddress = address;
    echoDestinationPort = port;
}

void LoggerPrivate::switchToMute()
{
    echo = Echo::Mute;
}

void LoggerPrivate::flushEchoFile()
{
    echoFileStream.flush();
}

void LoggerPrivate::onEchoModeChanged()
{
    echoFileFlushTimer.stop();
    if (echoFileStream.device()) {
        echoFileStream.device()->close();
    }
}

void LoggerPrivate::onCommandReceived()
{
    while (commandSocket.hasPendingDatagrams())
    {
        const auto &datagram = commandSocket.receiveDatagram();
        QStringList command = QString::fromLocal8Bit(datagram.data()).split(" ", QString::SkipEmptyParts);

        QRegExp rx(command.first());
        if (rx.indexIn(LoggerPrivate::appNameString()) != -1) {
            processCommand(command, datagram.senderAddress());
        }
    }
}
