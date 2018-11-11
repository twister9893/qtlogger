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
    enum class Level : quint32 {
        All =      0,
        Debug =    1,
        Info =     1 << 1,
        Warning =  1 << 2,
        Critical = 1 << 3,
        Fatal =    1 << 4
    };
    enum class Echo {
        Mute,
        StdErr,
        File,
        Udp
    };

public:
    Echo echo = Echo::StdErr;

    QUdpSocket commandSocket;
    quint16 commandPort = 0;

    QTextStream echoFileStream;
    QTimer flushTimer;
    int defaultFlushPeriodMsec = 0;

    QUdpSocket echoSocket;
    QHostAddress echoDestAddress;
    quint16 echoDestPort = 0;
    quint16 defaultDestPort = 0;

    quint32 levelFilter = quint32(Level::All);
    QStringList funcFilter;

public:
    LoggerPrivate();
public:
    void log(const QString &msg);
    void exec(const QString &command);
    void processCommand(const QStringList &command, const QHostAddress &sender = QHostAddress());
public:
    void writeStatus(const QHostAddress &address, quint16 port) const;
public:
    bool pass(Level level);
    bool pass(const QString &func);
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
