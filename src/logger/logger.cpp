#include "logger.h"
#include "logger-private.h"

#include <QCoreApplication>
#include <QThread>
#include <QSettings>
#include <QHostInfo>
#include <QTime>
#include <QDir>

#include <stdio.h>
#include <signal.h>

#include "ansi-colors.h"

// TODO: Decompose processCommand() function
// TODO: Add active filters to status

namespace qtlogger {

volatile bool LoggerPrivate::destroyed = false;

Logger::~Logger()
{
    LoggerPrivate::destroyed = true;
}

void Logger::exec(const QString &command)
{
    if (LoggerPrivate::destroyed) { return; }
    instance().d_ptr->exec(command, QHostAddress::LocalHost);
}

void Logger::debug(const QString &msg, const QMessageLogContext &context)
{
    if (LoggerPrivate::destroyed) { return; }
    if (!instance().d_ptr->passLevel(LoggerPrivate::Level::Debug)) { return; }
    if (!instance().d_ptr->passFile(QString(context.file)))        { return; }
    if (!instance().d_ptr->passFunc(QString(context.function)))    { return; }

    instance().d_ptr->log(LoggerPrivate::debugString(msg, context));
}

void Logger::info(const QString &msg, const QMessageLogContext &context)
{
    if (LoggerPrivate::destroyed) { return; }
    if (!instance().d_ptr->passLevel(LoggerPrivate::Level::Info)) { return; }
    if (!instance().d_ptr->passFile(QString(context.file)))       { return; }
    if (!instance().d_ptr->passFunc(QString(context.function)))   { return; }

    instance().d_ptr->log(LoggerPrivate::infoString(msg, context));
}

void Logger::warning(const QString &msg, const QMessageLogContext &context)
{
    if (LoggerPrivate::destroyed) { return; }
    if (!instance().d_ptr->passLevel(LoggerPrivate::Level::Warning)) { return; }
    if (!instance().d_ptr->passFile(QString(context.file)))          { return; }
    if (!instance().d_ptr->passFunc(QString(context.function)))      { return; }

    instance().d_ptr->log(LoggerPrivate::warningString(msg, context));
}

void Logger::critical(const QString &msg, const QMessageLogContext &context)
{
    if (LoggerPrivate::destroyed) { return; }
    if (!instance().d_ptr->passLevel(LoggerPrivate::Level::Critical)) { return; }
    if (!instance().d_ptr->passFile(QString(context.file)))           { return; }
    if (!instance().d_ptr->passFunc(QString(context.function)))       { return; }

    instance().d_ptr->log(LoggerPrivate::criticalString(msg, context));
}

void Logger::fatal(const QString &msg, const QMessageLogContext &context)
{
    if (LoggerPrivate::destroyed) { return; }
    if (!instance().d_ptr->passLevel(LoggerPrivate::Level::Fatal)) { return; }
    if (!instance().d_ptr->passFile(QString(context.file)))        { return; }
    if (!instance().d_ptr->passFunc(QString(context.function)))    { return; }

    instance().d_ptr->log(LoggerPrivate::fatalString(msg, context));
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
    Q_ASSERT_X(qApp != nullptr, "Logger", "instantiate QCoreApplication object first");
    Q_ASSERT_X(QThread::currentThread() == qApp->thread(), "Logger", "Logger should be created in the same thread with QCoreApplication");

    configure();

    originalSignalHandlers[SIGINT]  = signal(SIGINT,  LoggerPrivate::onAppTerminate);
    originalSignalHandlers[SIGTERM] = signal(SIGTERM, LoggerPrivate::onAppTerminate);
    originalSignalHandlers[SIGQUIT] = signal(SIGQUIT, LoggerPrivate::onAppTerminate);
    originalSignalHandlers[SIGSEGV] = signal(SIGSEGV, LoggerPrivate::onAppTerminate);

    connect(&flushTimer, &QTimer::timeout, this, &LoggerPrivate::flushEchoFile);

    connect(this, &LoggerPrivate::toggleStdErr, this, &LoggerPrivate::onEchoModeChanged);
    connect(this, &LoggerPrivate::toggleFile, this, &LoggerPrivate::onEchoModeChanged);
    connect(this, &LoggerPrivate::toggleUdp, this, &LoggerPrivate::onEchoModeChanged);
    connect(this, &LoggerPrivate::toggleMute, this, &LoggerPrivate::onEchoModeChanged);

    connect(this, &LoggerPrivate::toggleStdErr, this, &LoggerPrivate::switchToStdErr);
    connect(this, &LoggerPrivate::toggleFile, this, &LoggerPrivate::switchToFile);
    connect(this, &LoggerPrivate::toggleUdp, this, &LoggerPrivate::switchToUdp);
    connect(this, &LoggerPrivate::toggleMute, this, &LoggerPrivate::switchToMute);

    qRegisterMetaType<QHostAddress>("QHostAddress");
    connect(this, &LoggerPrivate::sendUdpMsg, this, &LoggerPrivate::writeUdpMsg, Qt::QueuedConnection);

    exec(appRcCommandString());
    exec(argCommandString());

    readSocket.bind(commandPort, QUdpSocket::ShareAddress);
    connect(&readSocket, &QUdpSocket::readyRead, this, &LoggerPrivate::onCommandReceived);
}

void LoggerPrivate::configure()
{
    QSettings settings(CONFIG_PATH "/.qtlogger-rc", QSettings::NativeFormat);
    commandPort = uint16_t(settings.value("command-port", 6060u).toUInt());
    defaultDestPort = uint16_t(settings.value("default-dest-port", 6061u).toUInt());
    defaultFlushPeriodMsec = settings.value("default-flush-period", 5000).toInt();
}

void LoggerPrivate::log(const QString &msg)
{
    switch (echo)
    {
        case Echo::StdErr:
            fprintf(stderr, "%s\n", msg.toLocal8Bit().constData());
            break;
        case Echo::File:
            echoFileStream << msg << "\n";
            break;
        case Echo::Udp:
            emit sendUdpMsg( msg + QString("\n"),
                             echoDestAddress,
                             echoDestPort );
            break;
        default:
            break;
    }
}

void LoggerPrivate::exec(const QString &command, const QHostAddress &sender)
{
    const auto &commands = command.split("|", QString::SkipEmptyParts);
    for (const auto &c : commands) {
        processCommand(c.split(" ", QString::SkipEmptyParts), sender);
    }
}

void LoggerPrivate::processCommand(const QStringList &command, const QHostAddress &sender)
{
    if (command.size() == 0) { return; }
    const auto &action = command.at(0).simplified();

    if (QString("status").startsWith(action))
    {
        QHostAddress address = (sender.isNull() ? QHostAddress::LocalHost : sender);
        quint16 port = defaultDestPort;
        if (command.size() == 2 ) {
            parseDestination(command.at(1), &address, &port);
        }
        emit sendUdpMsg(statusString(), address, port);
    }
    else if (QString("echo").startsWith(action))
    {
        if (command.size() < 2) { return; }
        const auto &echoMode = command.at(1).simplified();

        if (QString("stderr").startsWith(echoMode))
        {
            emit toggleStdErr();
        }
        else if (QString("file").startsWith(echoMode))
        {
            const auto &filePath = ( command.size() < 3 ? (appNameString() + ".log") : command.at(2) );
            const auto &flushPeriodMsec = ( command.size() < 4 ? defaultFlushPeriodMsec : command.at(3).toInt() );
            emit toggleFile(filePath, flushPeriodMsec);
        }
        else if (QString("udp").startsWith(echoMode))
        {
            QHostAddress address = (sender.isNull() ? QHostAddress::LocalHost : sender);
            quint16 port = defaultDestPort;
            if (command.size() == 3 ) {
                parseDestination(command.at(2), &address, &port);
            }
            emit toggleUdp(address, port);
        }
        else if (QString("mute").startsWith(echoMode))
        {
            emit toggleMute();
        }
    }
    else if (QString("filter").startsWith(action))
    {
        if (command.size() < 2) { return; }
        const auto &filterOperation = command.at(1).simplified();
        const auto &filterType = (command.size() < 3 ? QString("") : command.at(2).simplified());
        const auto &filterString = (command.size() < 4 ? QString("") : command.at(3).simplified());

        enum Operation { Add, Del, Clear };
        Operation operation = Clear;

        if (QString("add").startsWith(filterOperation)) {
            operation = Add;
        } else if (QString("del").startsWith(filterOperation)) {
            operation = Del;
        } else if (QString("clear").startsWith(filterOperation)) {
            operation = Clear;
        } else {
            return;
        }

        if (filterType.isEmpty() && operation == Clear)
        {
            levelFilter = quint32(Level::All);
            fileFilter.clear();
            funcFilter.clear();
        }
        else if (QString("level").startsWith(filterType))
        {
            if (operation == Clear) {
                levelFilter = quint32(Level::All);
                return;
            }

            if (filterString.isEmpty()) {
                return;
            }

            QRegExp rx(filterString);
            if (rx.indexIn("debug") != -1)    { (operation == Add) ? levelFilter |= quint32(Level::Debug)    : levelFilter &= ~quint32(Level::Debug); }
            if (rx.indexIn("info") != -1)     { (operation == Add) ? levelFilter |= quint32(Level::Info)     : levelFilter &= ~quint32(Level::Info); }
            if (rx.indexIn("warning") != -1)  { (operation == Add) ? levelFilter |= quint32(Level::Warning)  : levelFilter &= ~quint32(Level::Warning); }
            if (rx.indexIn("critical") != -1) { (operation == Add) ? levelFilter |= quint32(Level::Critical) : levelFilter &= ~quint32(Level::Critical); }
            if (rx.indexIn("fatal") != -1)    { (operation == Add) ? levelFilter |= quint32(Level::Fatal)    : levelFilter &= ~quint32(Level::Fatal); }
        }
        else if (QString("file").startsWith(filterType))
        {
            if (operation == Clear) {
                fileFilter.clear();
                return;
            }

            if (filterString.isEmpty()) {
                return;
            }

            if (operation == Add) {
                fileFilter.append(filterString);
            } else {
                QRegExp rx(filterString);
                for (auto it = fileFilter.begin(), ite = fileFilter.end(); it != ite;)
                {
                    if (rx.indexIn(*it) != -1) {
                        it = fileFilter.erase(it);
                        ite = fileFilter.end();
                    } else {
                        ++it;
                    }
                }
            }
        }
        else if (QString("function").startsWith(filterType))
        {
            if (operation == Clear) {
                funcFilter.clear();
                return;
            }

            if (filterString.isEmpty()) {
                return;
            }

            if (operation == Add) {
                funcFilter.append(filterString);
            } else {
                QRegExp rx(filterString);
                for (auto it = funcFilter.begin(), ite = funcFilter.end(); it != ite;)
                {
                    if (rx.indexIn(*it) != -1) {
                        it = funcFilter.erase(it);
                        ite = funcFilter.end();
                    } else {
                        ++it;
                    }
                }
            }
        }
    }
}

bool LoggerPrivate::passDestination(const QString & destination) const
{
    const auto &tupple = destination.split(":", QString::SkipEmptyParts);
    if (tupple.size() == 2)
    {
        QRegExp rx(tupple.first());
        if (rx.indexIn(LoggerPrivate::hostNameString()) == -1) {
            return false;
        }
    }

    if (tupple.size() > 0)
    {
        QRegExp rx(tupple.last());
        if (rx.indexIn(LoggerPrivate::appNameString()) == -1) {
            return false;
        }
    }
    return true;
}

bool LoggerPrivate::passLevel(LoggerPrivate::Level level) const
{
    return (levelFilter ? (levelFilter & quint32(level)) : true);
}

bool LoggerPrivate::passFile(const QString &file) const
{
    if (fileFilter.isEmpty()) {
        return true;
    }

    for (const QString &filter : fileFilter)
    {
        QRegExp rx(filter);
        if (rx.indexIn(file) != -1) {
            return true;
        }
    }
    return false;
}

bool LoggerPrivate::passFunc(const QString &func) const
{
    if (funcFilter.isEmpty()) {
        return true;
    }

    for (const QString &filter : funcFilter)
    {
        QRegExp rx(filter);
        if (rx.indexIn(func) != -1) {
            return true;
        }
    }
    return false;
}

QString LoggerPrivate::statusString() const
{
    QString string = QString(GREEN "<%1" RESET GRAY "@" RESET CYAN "%2>  " RESET "%3").arg(QHostInfo::localHostName()).arg(appNameString());
    switch (echo) {
        case Echo::Mute:   return string.arg( QString("Muted\n") );
        case Echo::StdErr: return string.arg( QString("Writing in stderr stream\n") );
        case Echo::File:   return string.arg( QString("Writing in file %1\n").arg(echoFileStream.device() ? static_cast<QFile*>(echoFileStream.device())->fileName() : "") );
        case Echo::Udp:    return string.arg( QString("Writing to %1:%2\n").arg(echoDestAddress.toString())
                                                                           .arg(echoDestPort) );
    }
    return string.arg( QString("N/D\n") );
}

void LoggerPrivate::resetSignals()
{
    signal(SIGINT,  originalSignalHandlers[SIGINT]);
    signal(SIGTERM, originalSignalHandlers[SIGTERM]);
    signal(SIGQUIT, originalSignalHandlers[SIGQUIT]);
    signal(SIGSEGV, originalSignalHandlers[SIGSEGV]);
}

QString LoggerPrivate::debugString(const QString &msg, const QMessageLogContext & context)
{
    Q_UNUSED(context)
    return QString(GRAY "[%1] DEBG <%2> %3" RESET).arg(QTime::currentTime().toString("hh:mm:ss").toLocal8Bit().constData())
                                                  .arg(LoggerPrivate::appNameString())
                                                  .arg(msg);
}

QString LoggerPrivate::infoString(const QString &msg, const QMessageLogContext & context)
{
    Q_UNUSED(context)
    return QString(GRAY "[%1] " RESET BLUE "INFO <%2> " RESET "%3").arg(QTime::currentTime().toString("hh:mm:ss").toLocal8Bit().constData())
                                                                   .arg(LoggerPrivate::appNameString())
                                                                   .arg(msg);
}

QString LoggerPrivate::warningString(const QString &msg, const QMessageLogContext & context)
{
    Q_UNUSED(context)
    return QString("[%1] " YELLOW "WARN <%2> " RESET "%3").arg(QTime::currentTime().toString("hh:mm:ss").toLocal8Bit().constData())
                                                          .arg(LoggerPrivate::appNameString())
                                                          .arg(msg);
}

QString LoggerPrivate::criticalString(const QString &msg, const QMessageLogContext & context)
{
    Q_UNUSED(context)
    return QString("[%1] " RED "CRIT <%2> " RESET "%3").arg(QTime::currentTime().toString("hh:mm:ss").toLocal8Bit().constData())
                                                       .arg(LoggerPrivate::appNameString())
                                                       .arg(msg);
}

QString LoggerPrivate::fatalString(const QString &msg, const QMessageLogContext & context)
{
    return QString(RED "[%1] FATL <%2> terminated at %3:%4 " RESET "%5").arg(QTime::currentTime().toString("hh:mm:ss").toLocal8Bit().constData())
                                                                        .arg(LoggerPrivate::appNameString())
                                                                        .arg(context.file)
                                                                        .arg(context.line)
                                                                        .arg(msg);
}

QString LoggerPrivate::hostNameString()
{
    return QHostInfo::localHostName();
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

QString LoggerPrivate::appRcCommandString()
{
    const QString &filePath = (QFile::exists(QString("qtlogger-%1-rc").arg(appNameString())) ? QString("qtlogger.d/qtlogger-%1-rc").arg(appNameString())
                                                                                             : QString(CONFIG_PATH "/qtlogger-default-rc"));

    QFile file(filePath);
    if(file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream textStream(&file);
        return textStream.readAll();
    }
    return QString();
}

QString LoggerPrivate::argCommandString()
{
    static QString string;
    static bool read = false;
    if (!read)
    {
        const auto *app = QCoreApplication::instance();
        if (app)
        {
            const auto &args = app->arguments();
            for(auto it = args.cbegin(), ite = args.cend(); it != ite; ++it)
            {
                if (it->contains("--qtlogger"))
                {
                    string = it->split("=", QString::SkipEmptyParts).last();
                }
            }
        }
        read = true;
    }
    return string;
}

bool LoggerPrivate::parseDestination(const QString &destination, QHostAddress * address, quint16 * port)
{
    const auto &tupple = destination.split(":", QString::SkipEmptyParts);
    if (tupple.size() == 2)
    {
        if (address) { address->setAddress(tupple.first()); }
        if (port) { *port = tupple.last().toUShort(); }
        return true;
    }
    else if (tupple.size() == 1)
    {
        QRegExp rx("\\d+");
        if (rx.indexIn(tupple.first()) != -1)
        {
            if (port) { *port = tupple.first().toUShort(); }
        }
        else
        {
            if (address) { address->setAddress(tupple.first()); }
        }
        return true;
    }
    return false;
}

void LoggerPrivate::switchToStdErr()
{
    echo = Echo::StdErr;
}

void LoggerPrivate::switchToFile(const QString &filePath, int flushPeriodMsec)
{
    static QFile file;
    file.setFileName(filePath);
    if (!file.open(QFile::WriteOnly))
    {
        qCritical() << Q_FUNC_INFO << "Echo file opening failed," << file.errorString();
        return;
    }

    echo = Echo::File;
    echoFileStream.setDevice(&file);
    flushTimer.start(flushPeriodMsec);
}

void LoggerPrivate::switchToUdp(const QHostAddress &address, quint16 port)
{
    echo = Echo::Udp;
    echoDestAddress = address;
    echoDestPort = port;
}

void LoggerPrivate::switchToMute()
{
    echo = Echo::Mute;
}

void LoggerPrivate::flushEchoFile()
{
    echoFileStream.flush();
}

void LoggerPrivate::writeUdpMsg(const QString &msg, const QHostAddress &address, quint16 port)
{
    writeSocket.writeDatagram(msg.toLocal8Bit(), address, port);
}

void LoggerPrivate::onEchoModeChanged()
{
    flushTimer.stop();
    if (echoFileStream.device()) {
        echoFileStream.device()->close();
    }
}

void LoggerPrivate::onCommandReceived()
{
    while (readSocket.hasPendingDatagrams())
    {
        QHostAddress address;
        QByteArray datagram(int(readSocket.pendingDatagramSize()), 0);
        readSocket.readDatagram(datagram.data(), datagram.size(), &address);
        QStringList command = QString::fromLocal8Bit(datagram.data()).split(" ", QString::SkipEmptyParts);

        if ( !command.isEmpty() && LoggerPrivate::passDestination(command.takeFirst()) )
        {
            exec(command.join(" "), address);
        }
    }
}

void LoggerPrivate::onAppTerminate(int signum)
{
    switch (signum)
    {
        case SIGINT:  Logger::instance().d_ptr->log(LoggerPrivate::fatalString(QString("Interrupt signal received")));
            break;
        case SIGTERM: Logger::instance().d_ptr->log(LoggerPrivate::fatalString(QString("Terminate signal received")));
            break;
        case SIGQUIT: Logger::instance().d_ptr->log(LoggerPrivate::fatalString(QString("Quit signal received")));
            break;
        case SIGSEGV: Logger::instance().d_ptr->log(LoggerPrivate::fatalString(QString("Segmentation fault signal received")));
            break;
    }

    if (Logger::instance().d_ptr->echo == Echo::File) {
        Logger::instance().d_ptr->flushEchoFile();
    }

    Logger::instance().d_ptr->resetSignals();
    raise(signum);
}

}
