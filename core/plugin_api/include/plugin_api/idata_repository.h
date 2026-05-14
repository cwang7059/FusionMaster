#pragma once

#include <QString>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>

struct DataRepositoryResult {
    bool ok = false;
    QString errorMessage;
    QVariantList rows;
    int affectedRows = -1;
    QVariant lastInsertId;
};

class IDataRepository {
public:
    virtual ~IDataRepository() = default;

    virtual QString defaultConnectionName() const = 0;
    virtual QVariantMap defaultOptions() const = 0;

    virtual bool isOpen(const QString& connectionName = QString()) const = 0;
    virtual bool open(const QString& connectionName, const QVariantMap& options, QString* errorMessage = nullptr) = 0;
    virtual void close(const QString& connectionName = QString()) = 0;

    virtual DataRepositoryResult execute(
        const QString& sql,
        const QVariantMap& namedValues = QVariantMap(),
        const QVariantList& positionalValues = QVariantList(),
        const QString& connectionName = QString()) = 0;

    virtual DataRepositoryResult query(
        const QString& sql,
        const QVariantMap& namedValues = QVariantMap(),
        const QVariantList& positionalValues = QVariantList(),
        const QString& connectionName = QString()) = 0;

    virtual bool beginTransaction(const QString& connectionName = QString(), QString* errorMessage = nullptr) = 0;
    virtual bool commitTransaction(const QString& connectionName = QString(), QString* errorMessage = nullptr) = 0;
    virtual bool rollbackTransaction(const QString& connectionName = QString(), QString* errorMessage = nullptr) = 0;
};
