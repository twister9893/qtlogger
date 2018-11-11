#ifndef QTLOGGER_LOGGER_H
#define QTLOGGER_LOGGER_H

#include <QScopedPointer>

namespace qtlogger {

class LoggerPrivate;
class Logger {
public:
    ~Logger();
public:
    static void exec(const QString &command);
public:
    static void debug(const QString &msg, const QMessageLogContext &context);
    static void info(const QString &msg, const QMessageLogContext &context);
    static void warning(const QString &msg, const QMessageLogContext &context);
    static void critical(const QString &msg, const QMessageLogContext &context);
    static void fatal(const QString &msg, const QMessageLogContext &context);

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
