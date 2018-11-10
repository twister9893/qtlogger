#ifndef QTLOGGER_LOGGER_H
#define QTLOGGER_LOGGER_H

#include <QScopedPointer>

namespace qtlogger {

class LoggerPrivate;
class Logger {
public:
    enum class Echo {
        Mute,
        StdErr,
        File,
        Udp
    };
public:
    ~Logger();
public:
    static void exec(const QString &command);
public:
    static void debug(const QString &msg);
    static void warning(const QString &msg);
    static void critical(const QString &msg);

private:
    static Logger & instance();
private:
    Logger();
private:
    QScopedPointer<LoggerPrivate> d_ptr;
private:
    friend class LoggerPrivate;
};

}

#endif // QTLOGGER_LOGGER_H
