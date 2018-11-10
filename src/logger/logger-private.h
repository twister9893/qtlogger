#ifndef LOGGERPRIVATE_H
#define LOGGERPRIVATE_H

#include <QUdpSocket>
#include "logger.h"

namespace qtlogger {

class LoggerPrivate : public QObject {
public:
    Logger::Echo echo = Logger::Echo::StdErr;

    QUdpSocket commandSocket;
    quint16 commandPort = 6060;

    QUdpSocket echoSocket;
    QHostAddress echoDestinationAddress;
    quint16 echoDestinationPort = 0;

public:
    LoggerPrivate();
public:
    void log(const QString &msg);
    void exec(const QString &command);
    void processCommand(const QStringList &command, const QHostAddress &sender = QHostAddress());
public:
    void writeStatus(const QHostAddress &address, quint16 port) const;
    void switchToStdErr();
    void switchToUdp(const QHostAddress &address, quint16 port);
    void switchToMute();
public:
    QString statusString() const;
    static QString appNameString();
    static QString argCommandString();

private slots:
    void onCommandReceived();
};

}

#endif // LOGGERPRIVATE_H
