#include "data_repository_service.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMutexLocker>
#include <QProcessEnvironment>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>

namespace {

QString normalizedBindKey(const QString& key) {
    const QString trimmed = key.trimmed();
    if (trimmed.startsWith(QLatin1Char(':'))
        || trimmed.startsWith(QLatin1Char('@'))
        || trimmed.startsWith(QLatin1Char('$'))) {
        return trimmed;
    }
    return QStringLiteral(":%1").arg(trimmed);
}

QString sqlErrorText(const QSqlError& error) {
    const QString text = error.text().trimmed();
    return text.isEmpty() ? QStringLiteral("未知数据库错误") : text;
}

bool isSqlIdentifier(const QString& value) {
    const QString text = value.trimmed();
    if (text.isEmpty()) {
        return false;
    }

    const auto isAlpha = [](const QChar ch) {
        const ushort code = ch.unicode();
        return (code >= 'A' && code <= 'Z') || (code >= 'a' && code <= 'z');
    };
    const auto isDigit = [](const QChar ch) {
        const ushort code = ch.unicode();
        return code >= '0' && code <= '9';
    };

    if (!isAlpha(text.at(0)) && text.at(0) != QLatin1Char('_')) {
        return false;
    }
    for (int i = 1; i < text.size(); ++i) {
        const QChar ch = text.at(i);
        if (!isAlpha(ch) && !isDigit(ch) && ch != QLatin1Char('_')) {
            return false;
        }
    }
    return true;
}

QString quotedIdentifier(const QString& value) {
    return QStringLiteral("`%1`").arg(value.trimmed());
}

bool appendValidatedIdentifier(QStringList* values, const QString& value, QString* errorMessage) {
    const QString trimmed = value.trimmed();
    if (!isSqlIdentifier(trimmed)) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("SQL 标识符不合法: %1").arg(value);
        }
        return false;
    }
    if (values != nullptr && !values->contains(trimmed)) {
        values->push_back(trimmed);
    }
    return true;
}

QString buildInsertSql(const QString& tableName, const QStringList& columns) {
    QStringList quotedColumns;
    QStringList placeholders;
    for (const QString& column : columns) {
        quotedColumns.push_back(quotedIdentifier(column));
        placeholders.push_back(QStringLiteral(":%1").arg(column));
    }

    return QStringLiteral("INSERT INTO %1 (%2) VALUES (%3)")
        .arg(quotedIdentifier(tableName), quotedColumns.join(QStringLiteral(", ")), placeholders.join(QStringLiteral(", ")));
}

QString buildSelectSql(const QString& tableName, const QStringList& columns, const QString& whereClause) {
    QString columnExpression = QStringLiteral("*");
    if (!columns.isEmpty()) {
        QStringList quotedColumns;
        for (const QString& column : columns) {
            quotedColumns.push_back(quotedIdentifier(column));
        }
        columnExpression = quotedColumns.join(QStringLiteral(", "));
    }

    QString sql = QStringLiteral("SELECT %1 FROM %2").arg(columnExpression, quotedIdentifier(tableName));
    const QString trimmedWhere = whereClause.trimmed();
    if (!trimmedWhere.isEmpty()) {
        if (trimmedWhere.startsWith(QStringLiteral("where "), Qt::CaseInsensitive)) {
            sql += QStringLiteral(" ") + trimmedWhere;
        } else {
            sql += QStringLiteral(" WHERE ") + trimmedWhere;
        }
    }
    return sql;
}

}  // namespace

DataRepositoryService::DataRepositoryService()
    : defaultConnectionName_(QStringLiteral("fusionmaster_mysql"))
    , defaultOptions_(buildDefaultOptions()) {
}

DataRepositoryService::~DataRepositoryService() {
    const QStringList connectionNames = openedConnectionNames_;
    for (const QString& connectionName : connectionNames) {
        close(connectionName);
    }
}

QString DataRepositoryService::defaultConnectionName() const {
    return defaultConnectionName_;
}

QVariantMap DataRepositoryService::defaultOptions() const {
    QMutexLocker locker(&mutex_);
    return defaultOptions_;
}

bool DataRepositoryService::initializeDefaultStorage(QString* errorMessage) {
    return open(defaultConnectionName_, defaultOptions_, errorMessage);
}

bool DataRepositoryService::isOpen(const QString& connectionName) const {
    QMutexLocker locker(&mutex_);
    const QString resolvedName = resolveConnectionName(connectionName);
    if (!QSqlDatabase::contains(resolvedName)) {
        return false;
    }

    const QSqlDatabase database = QSqlDatabase::database(resolvedName, false);
    return database.isValid() && database.isOpen();
}

bool DataRepositoryService::open(const QString& connectionName, const QVariantMap& options, QString* errorMessage) {
    QMutexLocker locker(&mutex_);

    const QString resolvedName = resolveConnectionName(connectionName);
    const QString driver = options.value(QStringLiteral("driver"), QStringLiteral("QMYSQL")).toString().trimmed();
    const QString driverName = driver.isEmpty() ? QStringLiteral("QMYSQL") : driver;

    QString databaseName = options.value(QStringLiteral("databaseName")).toString().trimmed();
    if (databaseName.isEmpty()) {
        databaseName = options.value(QStringLiteral("path")).toString().trimmed();
    }

    QSqlDatabase database = QSqlDatabase::contains(resolvedName)
        ? QSqlDatabase::database(resolvedName, false)
        : QSqlDatabase::addDatabase(driverName, resolvedName);

    if (!database.isValid()) {
        const QString message = QStringLiteral("数据库驱动不可用: %1").arg(driverName);
        setError(errorMessage, message);
        return false;
    }

    if (!databaseName.isEmpty()) {
        database.setDatabaseName(databaseName);
    }
    const QString hostName = options.value(QStringLiteral("hostName")).toString().trimmed();
    if (!hostName.isEmpty()) {
        database.setHostName(hostName);
    }
    const int port = options.value(QStringLiteral("port"), -1).toInt();
    if (port >= 0) {
        database.setPort(port);
    }
    const QString userName = options.value(QStringLiteral("userName")).toString();
    if (!userName.isEmpty()) {
        database.setUserName(userName);
    }
    const QString password = options.value(QStringLiteral("password")).toString();
    if (!password.isEmpty()) {
        database.setPassword(password);
    }
    const QString connectOptions = options.value(QStringLiteral("connectOptions")).toString();
    if (!connectOptions.isEmpty()) {
        database.setConnectOptions(connectOptions);
    }

    if (!database.isOpen() && !database.open()) {
        const QString message = sqlErrorText(database.lastError());
        setError(errorMessage, message);
        return false;
    }

    if (!openedConnectionNames_.contains(resolvedName)) {
        openedConnectionNames_.push_back(resolvedName);
    }

    if (resolvedName == defaultConnectionName_ && !initializeSchema(database, errorMessage)) {
        return false;
    }

    setError(errorMessage, QString());
    return true;
}

void DataRepositoryService::close(const QString& connectionName) {
    QMutexLocker locker(&mutex_);

    const QString resolvedName = resolveConnectionName(connectionName);
    if (!QSqlDatabase::contains(resolvedName)) {
        openedConnectionNames_.removeAll(resolvedName);
        return;
    }

    {
        QSqlDatabase database = QSqlDatabase::database(resolvedName, false);
        if (database.isValid() && database.isOpen()) {
            database.close();
        }
    }
    QSqlDatabase::removeDatabase(resolvedName);
    openedConnectionNames_.removeAll(resolvedName);
}

DataRepositoryResult DataRepositoryService::execute(
    const QString& sql,
    const QVariantMap& namedValues,
    const QVariantList& positionalValues,
    const QString& connectionName) {
    return runStatement(sql, namedValues, positionalValues, connectionName, false);
}

DataRepositoryResult DataRepositoryService::query(
    const QString& sql,
    const QVariantMap& namedValues,
    const QVariantList& positionalValues,
    const QString& connectionName) {
    return runStatement(sql, namedValues, positionalValues, connectionName, true);
}

DataRepositoryResult DataRepositoryService::importRows(
    const QString& tableName,
    const QVariantList& rows,
    const QString& connectionName) {
    QString errorMessage;
    if (!appendValidatedIdentifier(nullptr, tableName, &errorMessage)) {
        return errorResult(errorMessage);
    }

    if (rows.isEmpty()) {
        DataRepositoryResult result;
        result.ok = true;
        result.affectedRows = 0;
        return result;
    }

    QList<QVariantMap> rowMaps;
    QStringList columns;
    for (const QVariant& value : rows) {
        const QVariantMap row = value.toMap();
        if (row.isEmpty()) {
            return errorResult(QStringLiteral("导入行不能为空，且必须为对象结构"));
        }

        for (auto it = row.constBegin(); it != row.constEnd(); ++it) {
            if (!appendValidatedIdentifier(&columns, it.key(), &errorMessage)) {
                return errorResult(errorMessage);
            }
        }
        rowMaps.push_back(row);
    }

    if (columns.isEmpty()) {
        return errorResult(QStringLiteral("导入字段不能为空"));
    }

    QMutexLocker locker(&mutex_);
    QSqlDatabase database = databaseFor(connectionName, &errorMessage);
    if (!database.isValid()) {
        return errorResult(errorMessage);
    }

    if (!database.transaction()) {
        return errorResult(sqlErrorText(database.lastError()));
    }

    const QString sql = buildInsertSql(tableName.trimmed(), columns);
    int affectedRows = 0;
    QVariant lastInsertId;

    for (const QVariantMap& row : rowMaps) {
        QSqlQuery statement(database);
        if (!statement.prepare(sql)) {
            database.rollback();
            return errorResult(sqlErrorText(statement.lastError()));
        }

        QVariantMap values;
        for (const QString& column : columns) {
            values.insert(column, row.value(column));
        }

        if (!bindValues(statement, values, QVariantList(), &errorMessage)) {
            database.rollback();
            return errorResult(errorMessage);
        }

        if (!statement.exec()) {
            database.rollback();
            return errorResult(sqlErrorText(statement.lastError()));
        }

        const int rowAffected = statement.numRowsAffected();
        if (rowAffected > 0) {
            affectedRows += rowAffected;
        }
        lastInsertId = statement.lastInsertId();
    }

    if (!database.commit()) {
        return errorResult(sqlErrorText(database.lastError()));
    }

    DataRepositoryResult result;
    result.ok = true;
    result.affectedRows = affectedRows;
    result.lastInsertId = lastInsertId;
    return result;
}

DataRepositoryResult DataRepositoryService::exportRows(
    const QString& tableName,
    const QStringList& columns,
    const QString& whereClause,
    const QVariantMap& namedValues,
    const QVariantList& positionalValues,
    const QString& connectionName) {
    QString errorMessage;
    if (!appendValidatedIdentifier(nullptr, tableName, &errorMessage)) {
        return errorResult(errorMessage);
    }

    QStringList validatedColumns;
    for (const QString& column : columns) {
        if (!appendValidatedIdentifier(&validatedColumns, column, &errorMessage)) {
            return errorResult(errorMessage);
        }
    }

    const QString sql = buildSelectSql(tableName.trimmed(), validatedColumns, whereClause);
    return query(sql, namedValues, positionalValues, connectionName);
}

bool DataRepositoryService::beginTransaction(const QString& connectionName, QString* errorMessage) {
    QMutexLocker locker(&mutex_);
    QSqlDatabase database = databaseFor(connectionName, errorMessage);
    if (!database.isValid()) {
        return false;
    }

    if (!database.transaction()) {
        setError(errorMessage, sqlErrorText(database.lastError()));
        return false;
    }

    setError(errorMessage, QString());
    return true;
}

bool DataRepositoryService::commitTransaction(const QString& connectionName, QString* errorMessage) {
    QMutexLocker locker(&mutex_);
    QSqlDatabase database = databaseFor(connectionName, errorMessage);
    if (!database.isValid()) {
        return false;
    }

    if (!database.commit()) {
        setError(errorMessage, sqlErrorText(database.lastError()));
        return false;
    }

    setError(errorMessage, QString());
    return true;
}

bool DataRepositoryService::rollbackTransaction(const QString& connectionName, QString* errorMessage) {
    QMutexLocker locker(&mutex_);
    QSqlDatabase database = databaseFor(connectionName, errorMessage);
    if (!database.isValid()) {
        return false;
    }

    if (!database.rollback()) {
        setError(errorMessage, sqlErrorText(database.lastError()));
        return false;
    }

    setError(errorMessage, QString());
    return true;
}

QString DataRepositoryService::resolveConnectionName(const QString& connectionName) const {
    const QString trimmed = connectionName.trimmed();
    return trimmed.isEmpty() ? defaultConnectionName_ : trimmed;
}

QSqlDatabase DataRepositoryService::databaseFor(const QString& connectionName, QString* errorMessage) const {
    const QString resolvedName = resolveConnectionName(connectionName);
    if (!QSqlDatabase::contains(resolvedName)) {
        setError(errorMessage, QStringLiteral("数据库连接不存在: %1").arg(resolvedName));
        return QSqlDatabase();
    }

    QSqlDatabase database = QSqlDatabase::database(resolvedName, false);
    if (!database.isValid() || !database.isOpen()) {
        setError(errorMessage, QStringLiteral("数据库连接未打开: %1").arg(resolvedName));
        return QSqlDatabase();
    }

    setError(errorMessage, QString());
    return database;
}

DataRepositoryResult DataRepositoryService::runStatement(
    const QString& sql,
    const QVariantMap& namedValues,
    const QVariantList& positionalValues,
    const QString& connectionName,
    bool collectRows) {
    QMutexLocker locker(&mutex_);

    const QString trimmedSql = sql.trimmed();
    if (trimmedSql.isEmpty()) {
        return errorResult(QStringLiteral("SQL 语句不能为空"));
    }

    QString errorMessage;
    QSqlDatabase database = databaseFor(connectionName, &errorMessage);
    if (!database.isValid()) {
        return errorResult(errorMessage);
    }

    QSqlQuery statement(database);
    if (!statement.prepare(trimmedSql)) {
        return errorResult(sqlErrorText(statement.lastError()));
    }

    if (!bindValues(statement, namedValues, positionalValues, &errorMessage)) {
        return errorResult(errorMessage);
    }

    if (!statement.exec()) {
        return errorResult(sqlErrorText(statement.lastError()));
    }

    DataRepositoryResult result;
    result.ok = true;
    result.affectedRows = statement.numRowsAffected();
    result.lastInsertId = statement.lastInsertId();

    if (collectRows) {
        const QSqlRecord record = statement.record();
        while (statement.next()) {
            QVariantMap row;
            for (int i = 0; i < record.count(); ++i) {
                QString fieldName = record.fieldName(i).trimmed();
                if (fieldName.isEmpty()) {
                    fieldName = QStringLiteral("column%1").arg(i);
                }
                row.insert(fieldName, statement.value(i));
            }
            result.rows.push_back(row);
        }
    }

    return result;
}

bool DataRepositoryService::bindValues(
    QSqlQuery& query,
    const QVariantMap& namedValues,
    const QVariantList& positionalValues,
    QString* errorMessage) const {
    if (!namedValues.isEmpty() && !positionalValues.isEmpty()) {
        setError(errorMessage, QStringLiteral("不能同时使用命名参数和位置参数"));
        return false;
    }

    for (auto it = namedValues.constBegin(); it != namedValues.constEnd(); ++it) {
        const QString key = normalizedBindKey(it.key());
        if (key == QStringLiteral(":")) {
            setError(errorMessage, QStringLiteral("SQL 命名参数不能为空"));
            return false;
        }
        query.bindValue(key, it.value());
    }

    for (const QVariant& value : positionalValues) {
        query.addBindValue(value);
    }

    setError(errorMessage, QString());
    return true;
}

bool DataRepositoryService::initializeSchema(QSqlDatabase& database, QString* errorMessage) const {
    const QString driverName = database.driverName().toUpper();
    const bool mysqlDriver = driverName.contains(QStringLiteral("MYSQL"));

    QSqlQuery createMeta(database);
    const QString createMetaSql = mysqlDriver
        ? QStringLiteral(
            "CREATE TABLE IF NOT EXISTS fusionmaster_repository_meta ("
            "`key` VARCHAR(128) PRIMARY KEY,"
            "`value` TEXT NOT NULL,"
            "updated_at VARCHAR(64) NOT NULL"
            ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4")
        : QStringLiteral(
            "CREATE TABLE IF NOT EXISTS fusionmaster_repository_meta ("
            "key TEXT PRIMARY KEY,"
            "value TEXT NOT NULL,"
            "updated_at TEXT NOT NULL"
            ")");
    if (!createMeta.exec(createMetaSql)) {
        setError(errorMessage, sqlErrorText(createMeta.lastError()));
        return false;
    }

    QSqlQuery upsertMeta(database);
    const QString upsertMetaSql = mysqlDriver
        ? QStringLiteral(
            "INSERT INTO fusionmaster_repository_meta(`key`, `value`, updated_at) "
            "VALUES(:key, :value, :updated_at) "
            "ON DUPLICATE KEY UPDATE `value` = VALUES(`value`), updated_at = VALUES(updated_at)")
        : QStringLiteral(
            "INSERT OR REPLACE INTO fusionmaster_repository_meta(key, value, updated_at) "
            "VALUES(:key, :value, :updated_at)");
    if (!upsertMeta.prepare(upsertMetaSql)) {
        setError(errorMessage, sqlErrorText(upsertMeta.lastError()));
        return false;
    }

    upsertMeta.bindValue(QStringLiteral(":key"), QStringLiteral("schema.version"));
    upsertMeta.bindValue(QStringLiteral(":value"), QStringLiteral("1"));
    upsertMeta.bindValue(QStringLiteral(":updated_at"), QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
    if (!upsertMeta.exec()) {
        setError(errorMessage, sqlErrorText(upsertMeta.lastError()));
        return false;
    }

    setError(errorMessage, QString());
    return true;
}

QVariantMap DataRepositoryService::buildDefaultOptions() {
    QVariantMap options;
    options.insert(QStringLiteral("driver"), QStringLiteral("QMYSQL"));
    options.insert(QStringLiteral("hostName"), QStringLiteral("127.0.0.1"));
    options.insert(QStringLiteral("port"), 3306);
    options.insert(QStringLiteral("databaseName"), QStringLiteral("fusionmaster"));
    options.insert(QStringLiteral("userName"), QStringLiteral("root"));
    options.insert(QStringLiteral("password"), QString());
    options.insert(QStringLiteral("connectOptions"), QStringLiteral("MYSQL_OPT_RECONNECT=1"));

    const QVariantMap fileOptions = loadConfigFileOptions();
    for (auto it = fileOptions.constBegin(); it != fileOptions.constEnd(); ++it) {
        mergeOption(&options, it.key(), it.value());
    }

    mergeEnvironmentOptions(&options);
    return options;
}

QVariantMap DataRepositoryService::loadConfigFileOptions() {
    QStringList candidates;

    const QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    const QString configuredPath = environment.value(QStringLiteral("FUSIONMASTER_DB_CONFIG")).trimmed();
    if (!configuredPath.isEmpty()) {
        candidates.push_back(configuredPath);
    }

    candidates.push_back(QDir::current().filePath(QStringLiteral("config/database.json")));
    candidates.push_back(QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("config/database.json")));

    QDir appDir(QCoreApplication::applicationDirPath());
    if (appDir.cdUp()) {
        candidates.push_back(appDir.filePath(QStringLiteral("config/database.json")));
    }
    if (appDir.cdUp()) {
        candidates.push_back(appDir.filePath(QStringLiteral("config/database.json")));
    }
    if (appDir.cdUp()) {
        candidates.push_back(appDir.filePath(QStringLiteral("config/database.json")));
    }

    for (const QString& rawPath : candidates) {
        const QString path = QFileInfo(rawPath).absoluteFilePath();
        if (!QFileInfo::exists(path)) {
            continue;
        }

        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            continue;
        }

        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            continue;
        }

        const QJsonObject root = document.object();
        const QJsonObject databaseObject = root.value(QStringLiteral("database")).isObject()
            ? root.value(QStringLiteral("database")).toObject()
            : root;
        return databaseObject.toVariantMap();
    }

    return QVariantMap();
}

void DataRepositoryService::mergeEnvironmentOptions(QVariantMap* options) {
    if (options == nullptr) {
        return;
    }

    const QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    mergeOption(options, QStringLiteral("driver"), environment.value(QStringLiteral("FUSIONMASTER_DB_DRIVER")));
    mergeOption(options, QStringLiteral("hostName"), environment.value(QStringLiteral("FUSIONMASTER_DB_HOST")));
    mergeOption(options, QStringLiteral("databaseName"), environment.value(QStringLiteral("FUSIONMASTER_DB_NAME")));
    mergeOption(options, QStringLiteral("userName"), environment.value(QStringLiteral("FUSIONMASTER_DB_USER")));
    mergeOption(options, QStringLiteral("password"), environment.value(QStringLiteral("FUSIONMASTER_DB_PASSWORD")));
    mergeOption(options, QStringLiteral("connectOptions"), environment.value(QStringLiteral("FUSIONMASTER_DB_CONNECT_OPTIONS")));

    const QString portText = environment.value(QStringLiteral("FUSIONMASTER_DB_PORT")).trimmed();
    if (!portText.isEmpty()) {
        bool ok = false;
        const int port = portText.toInt(&ok);
        if (ok && port > 0) {
            options->insert(QStringLiteral("port"), port);
        }
    }
}

void DataRepositoryService::mergeOption(QVariantMap* options, const QString& key, const QVariant& value) {
    if (options == nullptr) {
        return;
    }

    const QString normalizedKey = key.trimmed();
    if (normalizedKey.isEmpty() || !value.isValid() || value.isNull()) {
        return;
    }

    if (value.type() == QVariant::String && value.toString().trimmed().isEmpty()) {
        return;
    }

    options->insert(normalizedKey, value);
}

DataRepositoryResult DataRepositoryService::errorResult(const QString& message) {
    DataRepositoryResult result;
    result.ok = false;
    result.errorMessage = message;
    return result;
}

void DataRepositoryService::setError(QString* errorMessage, const QString& message) {
    if (errorMessage != nullptr) {
        *errorMessage = message;
    }
}
