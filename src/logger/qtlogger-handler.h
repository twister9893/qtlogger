#ifndef QTLOGGER_QTLOGGERMSGHANDLER_H
#define QTLOGGER_QTLOGGERMSGHANDLER_H

#include <qapplication.h>
#include "logger.h"

namespace qtlogger {

void qtLoggerHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    switch (type)
    {
        case QtDebugMsg:
            Logger::debug(msg, context);
            break;
        case QtInfoMsg:
            Logger::info(msg, context);
            break;
        case QtWarningMsg:
            Logger::warning(msg, context);
            break;
        case QtCriticalMsg:
            Logger::critical(msg, context);
            break;
        case QtFatalMsg:
            Logger::fatal(msg, context);
            abort();
    }
}

}

#endif // QTLOGGER_QTLOGGERMSGHANDLER_H
