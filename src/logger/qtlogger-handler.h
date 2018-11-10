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
            Logger::debug(msg);
            break;
        case QtInfoMsg:
//            Logger::debug(msg);
            break;
        case QtWarningMsg:
            Logger::warning(msg);
            break;
        case QtCriticalMsg:
            Logger::critical(msg);
            break;
        case QtFatalMsg:
//            fprintf(stderr, "Fatal: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
            abort();
    }
}

}

#endif // QTLOGGER_QTLOGGERMSGHANDLER_H
