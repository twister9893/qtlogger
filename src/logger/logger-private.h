#ifndef LOGGERPRIVATE_H
#define LOGGERPRIVATE_H

#include <QUdpSocket>
#include <QTextStream>
#include <QTimer>

#include "logger.h"

namespace qtlogger {

class LoggerPrivate : public QObject {
    Q_OBJECT
public:
    enum class Echo {
        Mute,
        StdErr,
        File,
        Udp
    };

public:
    Echo echo = Echo::StdErr;

    QUdpSocket commandSocket;
    quint16 commandPort = 6060;

    QTextStream echoFileStream;
    QTimer echoFileFlushTimer;

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
public:
    QString statusString() const;
    static QString appNameString();
    static QString argCommandString();

signals:
    void toggleStdErr();
    void toggleFile(const QString &filePath, int flushPeriodMsec);
    void toggleUdp(const QHostAddress &address, quint16 port);
    void toggleMute();

private slots:
    void switchToStdErr();
    void switchToFile(const QString &filePath, int flushPeriodMsec);
    void switchToUdp(const QHostAddress &address, quint16 port);
    void switchToMute();

    void flushEchoFile();

private slots:
    void onEchoModeChanged();
    void onCommandReceived();
};

}

#endif // LOGGERPRIVATE_H
