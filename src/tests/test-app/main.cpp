#include <QCoreApplication>
#include <QTimer>
#include <QDebug>

#include <qtlogger.h>

int main(int argc, char *argv[])
{
    qInstallMessageHandler(qtlogger::qtLoggerHandler);
    QCoreApplication app(argc, argv);

    QTimer timerDebug;
    QObject::connect(&timerDebug, &QTimer::timeout, []() -> void { static int i = 0; qDebug() << "Some debug" << i++; qInfo() << "Some info" << i++; });

    QTimer timerWarning;
    QObject::connect(&timerWarning, &QTimer::timeout, []() -> void { static int i = 0; qWarning() << "Some warning" << i++; if (i == 5) { qFatal("Memory corruption"); } });

    QTimer timerCritical;
    QObject::connect(&timerCritical, &QTimer::timeout, []() -> void { static int i = 0; qCritical() << "Some critical" << i++; });

    timerDebug.start(100);
    timerWarning.start(1000);
    timerCritical.start(4000);

    return app.exec();
}
