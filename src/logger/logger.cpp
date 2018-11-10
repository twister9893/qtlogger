#include "logger.h"
#include "logger-private.h"

#include <QCoreApplication>
#include <QNetworkDatagram>
#include <QTime>
#include <QDir>
#include <stdio.h>

#include "ansi-colors.h"

using namespace qtlogger;

Logger::~Logger()
{}

void Logger::exec(const QString &command)
{
    instance().d_ptr->processCommand((LoggerPrivate::appNameString() + " " + command).split(" "));
}

void Logger::debug(const QString &msg)
{
    instance().d_ptr->log( QString(GRAY "[%1] DEBG <%2> %3" RESET).arg(QTime::currentTime().toString().toLocal8Bit().constData())
                                                                  .arg(LoggerPrivate::appNameString())
                                                                  .arg(msg));
}

void Logger::warning(const QString &msg)
{
    instance().d_ptr->log( QString("[%1] " YELLOW "WARN <%2> " RESET "%3").arg(QTime::currentTime().toString().toLocal8Bit().constData())
                                                                          .arg(LoggerPrivate::appNameString())
                                                                          .arg(msg));
}

void Logger::critical(const QString &msg)
{
    instance().d_ptr->log( QString("[%1] " RED "CRIT <%2> " RESET "%3").arg(QTime::currentTime().toString().toLocal8Bit().constData())
                                                                       .arg(LoggerPrivate::appNameString())
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
    commandSocket.bind(QHostAddress::Any, commandPort);
    connect(&commandSocket, &QUdpSocket::readyRead, this, &LoggerPrivate::onCommandReceived);
}

void LoggerPrivate::log(const QString &msg)
{
    switch (echo)
    {
        case Logger::Echo::StdErr:
            fprintf(stderr, "%s\n", msg.toLocal8Bit().constData());
            break;
        case Logger::Echo::Udp:
            echoSocket.writeDatagram( (msg+"\n").toLocal8Bit().constData(),
                                      echoDestinationAddress,
                                      echoDestinationPort );
            break;
        default:
            break;
    }
}

void LoggerPrivate::processCommand(const QStringList &command, const QHostAddress &sender)
{
    if (command.size() < 2) {
        return;
    }
    const auto &action = command.at(1).simplified();

    if (action == "status")
    {
        QUdpSocket socket;
        const auto &port = ( command.size() < 3 ? commandPort : command.at(2).toUShort() );
        socket.writeDatagram( statusString().toLocal8Bit(), (sender.isNull() ? QHostAddress::Any : sender), port);
    }
    else if (action == "echo")
    {
        const auto &echoMode = command.at(2).simplified();
        if (echoMode == "stderr")
        {
            echo = Logger::Echo::StdErr;
        }
        else if (echoMode == "udp")
        {
            const auto &port = ( command.size() < 4 ? commandPort : command.at(3).toUShort() );
            echo = Logger::Echo::Udp;
            echoDestinationAddress = (sender.isNull() ? QHostAddress::Any : sender);
            echoDestinationPort = port;
        }
    }
    else if (action == "mute")
    {
        echo = Logger::Echo::Mute;
    }
}

QString LoggerPrivate::statusString() const
{
    QString string = QString("[%1] %2").arg(appNameString());
    switch (echo) {
        case Logger::Echo::Mute:   return string.arg( QString("Muted\n") );
        case Logger::Echo::StdErr: return string.arg( QString("Writing in stderr stream\n") );
        case Logger::Echo::File:   return string.arg( QString("Writing in file\n") );
        case Logger::Echo::Udp:    return string.arg( QString("Writing to %1:%2\n").arg(echoDestinationAddress.toString())
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

void LoggerPrivate::onCommandReceived()
{
    while (commandSocket.hasPendingDatagrams())
    {
        const auto &datagram = commandSocket.receiveDatagram();
        QStringList command = QString::fromLocal8Bit(datagram.data()).split(" ");

        QRegExp rx(command.first());
        if (rx.indexIn(LoggerPrivate::appNameString()) != -1) {
            processCommand(command, datagram.senderAddress());
        }
    }
}
