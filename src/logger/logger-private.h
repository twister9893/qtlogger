#ifndef QTLOGGER_LOGGERPRIVATE_H
#define QTLOGGER_LOGGERPRIVATE_H

#include <QUdpSocket>
#include <QTextStream>
#include <QTimer>

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

    QUdpSocket readSocket;
    QUdpSocket writeSocket;

    quint16 commandPort = 0;

    QTextStream echoFileStream;
    QTimer flushTimer;
    int defaultFlushPeriodMsec = 0;

    QHostAddress echoDestAddress;
    quint16 echoDestPort = 0;
    quint16 defaultDestPort = 0;

    quint32 levelFilter = quint32(Level::All);
    QStringList fileFilter;
    QStringList funcFilter;

    typedef void(*SignalHandler)(int);
    QMap<int,SignalHandler> originalSignalHandlers;

    volatile static bool destroyed;

public:
    LoggerPrivate();
public:
    void configure();

    void log(const QString &msg);
    void exec(const QString &command, const QHostAddress &sender = QHostAddress());
    void processCommand(const QStringList &command, const QHostAddress &sender = QHostAddress());
public:
    bool passDestination(const QString &destination) const;
    bool passLevel(Level level) const;
    bool passFile(const QString &file) const;
    bool passFunc(const QString &func) const;
    QString statusString() const;

    void resetSignals();

    static QString debugString(const QString & msg, const QMessageLogContext &context = QMessageLogContext());
    static QString infoString(const QString & msg, const QMessageLogContext &context = QMessageLogContext());
    static QString warningString(const QString & msg, const QMessageLogContext &context = QMessageLogContext());
    static QString criticalString(const QString & msg, const QMessageLogContext &context = QMessageLogContext());
    static QString fatalString(const QString & msg, const QMessageLogContext &context = QMessageLogContext());

    static QString hostNameString();
    static QString appNameString();
    static QString appRcCommandString();
    static QString argCommandString();

    static bool parseDestination(const QString &destination, QHostAddress *address, quint16 *port);

signals:
    void toggleStdErr();
    void toggleFile(const QString &filePath, int flushPeriodMsec);
    void toggleUdp(const QHostAddress &address, quint16 port);
    void toggleMute();

    void sendUdpMsg(const QString & msg, const QHostAddress &address, quint16 port);

private slots:
    void switchToStdErr();
    void switchToFile(const QString &filePath, int flushPeriodMsec);
    void switchToUdp(const QHostAddress &address, quint16 port);
    void switchToMute();

    void flushEchoFile();
    void writeUdpMsg(const QString & msg, const QHostAddress &address, quint16 port);

private slots:
    void onEchoModeChanged();
    void onCommandReceived();

private:
    static void onAppTerminate(int signum);
};

}

#endif // QTLOGGER_LOGGERPRIVATE_H
