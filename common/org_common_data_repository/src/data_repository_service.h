#pragma once

#include <plugin_api/idata_repository.h>

#include <QMutex>
#include <QString>
#include <QStringList>
#include <QVariantMap>

class QSqlDatabase;
class QSqlQuery;

class DataRepositoryService : public IDataRepository {
public:
    DataRepositoryService();
    ~DataRepositoryService() override;

    QString defaultConnectionName() const override;
    QVariantMap defaultOptions() const override;

    bool initializeDefaultStorage(QString* errorMessage = nullptr);

    bool isOpen(const QString& connectionName = QString()) const override;
    bool open(const QString& connectionName, const QVariantMap& options, QString* errorMessage = nullptr) override;
    void close(const QString& connectionName = QString()) override;

    DataRepositoryResult execute(
        const QString& sql,
        const QVariantMap& namedValues = QVariantMap(),
        const QVariantList& positionalValues = QVariantList(),
        const QString& connectionName = QString()) override;

    DataRepositoryResult query(
        const QString& sql,
        const QVariantMap& namedValues = QVariantMap(),
        const QVariantList& positionalValues = QVariantList(),
        const QString& connectionName = QString()) override;

    bool beginTransaction(const QString& connectionName = QString(), QString* errorMessage = nullptr) override;
    bool commitTransaction(const QString& connectionName = QString(), QString* errorMessage = nullptr) override;
    bool rollbackTransaction(const QString& connectionName = QString(), QString* errorMessage = nullptr) override;

private:
    QString resolveConnectionName(const QString& connectionName) const;
    QSqlDatabase databaseFor(const QString& connectionName, QString* errorMessage) const;
    DataRepositoryResult runStatement(
        const QString& sql,
        const QVariantMap& namedValues,
        const QVariantList& positionalValues,
        const QString& connectionName,
        bool collectRows);
    bool bindValues(QSqlQuery& query, const QVariantMap& namedValues, const QVariantList& positionalValues, QString* errorMessage) const;
    bool initializeSchema(QSqlDatabase& database, QString* errorMessage) const;
    static QVariantMap buildDefaultOptions();
    static QVariantMap loadConfigFileOptions();
    static void mergeEnvironmentOptions(QVariantMap* options);
    static void mergeOption(QVariantMap* options, const QString& key, const QVariant& value);
    static DataRepositoryResult errorResult(const QString& message);
    static void setError(QString* errorMessage, const QString& message);

    QString defaultConnectionName_;
    QVariantMap defaultOptions_;
    QStringList openedConnectionNames_;
    mutable QMutex mutex_;
};
