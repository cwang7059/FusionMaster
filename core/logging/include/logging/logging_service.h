#pragma once

#include <plugin_api/ilogger.h>

#include <QMessageLogContext>
#include <QMutex>
#include <QObject>
#include <QString>
#include <QVariantMap>

#include <memory>

namespace spdlog {
class logger;
}

class LoggingService final : public QObject, public ILogger {
    Q_OBJECT

public:
    explicit LoggingService(QObject* parent = nullptr);
    ~LoggingService() override;

    void initialize(const QString& logRootDir);
    void shutdown();

    QString logRootDir() const;

    void info(const QString& msg) override;
    void warn(const QString& msg) override;
    void error(const QString& msg) override;
    void debug(const QString& msg) override;

signals:
    void logRecordReady(const QVariantMap& record);

private:
    static void qtMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message);

    void handleQtMessage(QtMsgType type, const QMessageLogContext& context, const QString& message);
    void writeRecord(const QString& level, const QString& message, const QString& fallbackTag);
    void emitRecord(const QVariantMap& record);
    void ensureLoggerForTodayLocked();

    static QVariantMap buildRecord(const QString& level, const QString& rawMessage, const QString& fallbackTag);
    static QString normalizeTag(const QString& rawTag);

private:
    mutable QMutex mutex_;
    QString logRootDir_;
    QString currentDateFolder_;
    std::shared_ptr<spdlog::logger> logger_;
    QtMessageHandler previousMessageHandler_ = nullptr;
    bool installed_ = false;

    static LoggingService* instance_;
};
