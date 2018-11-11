#include "logger.h"
#include "logger-private.h"

#include <QCoreApplication>
#include <QSettings>
#include <QNetworkDatagram>
#include <QTime>
#include <QDir>
#include <stdio.h>

#include "ansi-colors.h"

// TODO: Decompose processCommand() function
// TODO: Add active filters to status
// TODO: Add app specific rc

using namespace qtlogger;

Logger::~Logger()
{}

void Logger::exec(const QString &command)
{
    instance().d_ptr->exec(command);
}

void Logger::debug(const QString &msg, const QMessageLogContext &context)
{
    if (!instance().d_ptr->passLevel(LoggerPrivate::Level::Debug)) { return; }
    if (!instance().d_ptr->passFile(QString(context.file))) { return; }
    if (!instance().d_ptr->passFunc(QString(context.function))) { return; }

    instance().d_ptr->log( QString(GRAY "[%1] DEBG <%2> %3" RESET).arg(QTime::currentTime().toString().toLocal8Bit().constData())
                                                                  .arg(LoggerPrivate::appNameString())
                                                                  .arg(msg));
}

void Logger::info(const QString &msg, const QMessageLogContext &context)
{
    if (!instance().d_ptr->passLevel(LoggerPrivate::Level::Info)) { return; }
    if (!instance().d_ptr->passFile(QString(context.file))) { return; }
    if (!instance().d_ptr->passFunc(QString(context.function))) { return; }

    instance().d_ptr->log(msg);
}

void Logger::warning(const QString &msg, const QMessageLogContext &context)
{
    if (!instance().d_ptr->passLevel(LoggerPrivate::Level::Warning)) { return; }
    if (!instance().d_ptr->passFile(QString(context.file))) { return; }
    if (!instance().d_ptr->passFunc(QString(context.function))) { return; }

    instance().d_ptr->log( QString("[%1] " YELLOW "WARN <%2> " RESET "%3").arg(QTime::currentTime().toString().toLocal8Bit().constData())
                                                                          .arg(LoggerPrivate::appNameString())
                                                                          .arg(msg));
}

void Logger::critical(const QString &msg, const QMessageLogContext &context)
{
    if (!instance().d_ptr->passLevel(LoggerPrivate::Level::Critical)) { return; }
    if (!instance().d_ptr->passFile(QString(context.file))) { return; }
    if (!instance().d_ptr->passFunc(QString(context.function))) { return; }

    instance().d_ptr->log( QString("[%1] " RED "CRIT <%2> " RESET "%3").arg(QTime::currentTime().toString().toLocal8Bit().constData())
                                                                       .arg(LoggerPrivate::appNameString())
                                                                       .arg(msg));
}

void Logger::fatal(const QString &msg, const QMessageLogContext &context)
{
    if (!instance().d_ptr->passLevel(LoggerPrivate::Level::Fatal)) { return; }
    if (!instance().d_ptr->passFile(QString(context.file))) { return; }
    if (!instance().d_ptr->passFunc(QString(context.function))) { return; }

    instance().d_ptr->log( QString(RED "[%1] FATL <%2> terminated at %3:%4 " RESET "%5").arg(QTime::currentTime().toString().toLocal8Bit().constData())
                                                                                        .arg(LoggerPrivate::appNameString())
                                                                                        .arg(context.file)
                                                                                        .arg(context.line)
                                                                                        .arg(msg));
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
    QSettings settings(".qtlogger-rc", QSettings::NativeFormat);
    commandPort = settings.value("command-port", 6060).toUInt();
    defaultDestPort = settings.value("default-dest-port", 6061).toUInt();
    defaultFlushPeriodMsec = settings.value("default-flush-period", 5000).toInt();

    connect(&flushTimer, &QTimer::timeout, this, &LoggerPrivate::flushEchoFile);

    connect(this, &LoggerPrivate::toggleStdErr, this, &LoggerPrivate::onEchoModeChanged);
    connect(this, &LoggerPrivate::toggleFile, this, &LoggerPrivate::onEchoModeChanged);
    connect(this, &LoggerPrivate::toggleUdp, this, &LoggerPrivate::onEchoModeChanged);
    connect(this, &LoggerPrivate::toggleMute, this, &LoggerPrivate::onEchoModeChanged);

    connect(this, &LoggerPrivate::toggleStdErr, this, &LoggerPrivate::switchToStdErr);
    connect(this, &LoggerPrivate::toggleFile, this, &LoggerPrivate::switchToFile);
    connect(this, &LoggerPrivate::toggleUdp, this, &LoggerPrivate::switchToUdp);
    connect(this, &LoggerPrivate::toggleMute, this, &LoggerPrivate::switchToMute);

    exec(settings.value("startup-command").toString());
    exec(argCommandString());

    commandSocket.bind(commandPort, QUdpSocket::ShareAddress);
    connect(&commandSocket, &QUdpSocket::readyRead, this, &LoggerPrivate::onCommandReceived);
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
            echoSocket.writeDatagram( (msg+"\n").toLocal8Bit().constData(),
                                      echoDestAddress,
                                      echoDestPort );
            break;
        default:
            break;
    }
}

void LoggerPrivate::exec(const QString &command)
{
    processCommand((LoggerPrivate::appNameString() + " " + command).split(" ", QString::SkipEmptyParts));
}

void LoggerPrivate::processCommand(const QStringList &command, const QHostAddress &sender)
{
    if (command.size() < 2) { return; }
    const auto &action = command.at(1).simplified();

    if (QString("status").startsWith(action))
    {
        const auto &address = (sender.isNull() ? QHostAddress::Broadcast : sender);
        const auto &port = (command.size() < 3 ? defaultDestPort : command.at(2).toUShort());
        writeStatus(address, port);
    }
    else if (QString("echo").startsWith(action))
    {
        if (command.size() < 3) { return; }
        const auto &echoMode = command.at(2).simplified();

        if (QString("stderr").startsWith(echoMode))
        {
            emit toggleStdErr();
        }
        else if (QString("file").startsWith(echoMode))
        {
            const auto &filePath = ( command.size() < 4 ? (appNameString() + ".log") : command.at(3) );
            const auto &flushPeriodMsec = ( command.size() < 5 ? defaultFlushPeriodMsec : command.at(4).toInt() );
            emit toggleFile(filePath, flushPeriodMsec);
        }
        else if (QString("udp").startsWith(echoMode))
        {
            const auto &address = (sender.isNull() ? QHostAddress::Broadcast : sender);
            const auto &port = ( command.size() < 4 ? defaultDestPort : command.at(3).toUShort() );
            emit toggleUdp(address, port);
        }
        else if (QString("mute").startsWith(echoMode))
        {
            emit toggleMute();
        }
    }
    else if (QString("filter").startsWith(action))
    {
        if (command.size() < 3) { return; }
        const auto &filterOperation = command.at(2).simplified();
        const auto &filterType = (command.size() < 4 ? QString("") : command.at(3).simplified());
        const auto &filterString = (command.size() < 5 ? QString("") : command.at(4).simplified());

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

void LoggerPrivate::writeStatus(const QHostAddress &address, quint16 port) const
{
    static QUdpSocket socket;
    socket.writeDatagram( statusString().toLocal8Bit(), address, port);
}

bool LoggerPrivate::passLevel(LoggerPrivate::Level level)
{
    return (levelFilter ? (levelFilter & quint32(level)) : true);
}

bool LoggerPrivate::passFile(const QString &file)
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

bool LoggerPrivate::passFunc(const QString &func)
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
    QString string = QString("[%1] %2").arg(appNameString());
    switch (echo) {
        case Echo::Mute:   return string.arg( QString("Muted\n") );
        case Echo::StdErr: return string.arg( QString("Writing in stderr stream\n") );
        case Echo::File:   return string.arg( QString("Writing in file %1\n").arg(echoFileStream.device() ? static_cast<QFile*>(echoFileStream.device())->fileName() : "") );
        case Echo::Udp:    return string.arg( QString("Writing to %1:%2\n").arg(echoDestAddress.toString())
                                                                           .arg(echoDestPort) );
        default:
            break;
    }
    return string.arg( QString("N/D\n") );
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

void LoggerPrivate::switchToStdErr()
{
    echo = Echo::StdErr;
}

void LoggerPrivate::switchToFile(const QString &filePath, int flushPeriodMsec)
{
    echo = Echo::File;

    static QFile file;
    file.setFileName(filePath);
    if (!file.open(QFile::WriteOnly))
    {
        emit toggleStdErr();
        qCritical() << Q_FUNC_INFO << "Echo file opening failed," << file.errorString();
        return;
    }

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

void LoggerPrivate::onEchoModeChanged()
{
    flushTimer.stop();
    if (echoFileStream.device()) {
        echoFileStream.device()->close();
    }
}

void LoggerPrivate::onCommandReceived()
{
    while (commandSocket.hasPendingDatagrams())
    {
        const auto &datagram = commandSocket.receiveDatagram();
        QStringList command = QString::fromLocal8Bit(datagram.data()).split(" ", QString::SkipEmptyParts);

        QRegExp rx(command.first());
        if (rx.indexIn(LoggerPrivate::appNameString()) != -1) {
            processCommand(command, datagram.senderAddress());
        }
    }
}
