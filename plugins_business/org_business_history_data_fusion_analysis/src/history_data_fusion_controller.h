#pragma once

#include <plugin_api/imessage_bus.h>

#include <QDateTime>
#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

class DataModelBase;
class QTimer;

class HistoryDataFusionController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString taskId READ taskId NOTIFY taskChanged)
    Q_PROPERTY(QString taskName READ taskName WRITE setTaskName NOTIFY taskSettingsChanged)
    Q_PROPERTY(QString owner READ owner WRITE setOwner NOTIFY taskSettingsChanged)
    Q_PROPERTY(QString dataSourceType READ dataSourceType WRITE setDataSourceType NOTIFY taskSettingsChanged)
    Q_PROPERTY(QString filterLocation READ filterLocation WRITE setFilterLocation NOTIFY taskSettingsChanged)
    Q_PROPERTY(QString filterTargetType READ filterTargetType WRITE setFilterTargetType NOTIFY taskSettingsChanged)
    Q_PROPERTY(QString filterResult READ filterResult WRITE setFilterResult NOTIFY taskSettingsChanged)
    Q_PROPERTY(int minBatchRequired READ minBatchRequired WRITE setMinBatchRequired NOTIFY taskSettingsChanged)
    Q_PROPERTY(QString taskStateText READ taskStateText NOTIFY runtimeChanged)
    Q_PROPERTY(QString feedback READ feedback NOTIFY runtimeChanged)
    Q_PROPERTY(double progress READ progress NOTIFY runtimeChanged)
    Q_PROPERTY(QString progressText READ progressText NOTIFY runtimeChanged)
    Q_PROPERTY(QString importPath READ importPath NOTIFY datasetChanged)
    Q_PROPERTY(int recordCount READ recordCount NOTIFY datasetChanged)
    Q_PROPERTY(int batchCount READ batchCount NOTIFY datasetChanged)
    Q_PROPERTY(QVariantList stageRows READ stageRows NOTIFY analysisChanged)
    Q_PROPERTY(QVariantList batchRows READ batchRows NOTIFY analysisChanged)
    Q_PROPERTY(QVariantList qualityRows READ qualityRows NOTIFY analysisChanged)
    Q_PROPERTY(QVariantList factorRows READ factorRows NOTIFY analysisChanged)
    Q_PROPERTY(QVariantList conditionRows READ conditionRows NOTIFY analysisChanged)
    Q_PROPERTY(QVariantList issueRows READ issueRows NOTIFY analysisChanged)
    Q_PROPERTY(QVariantList dataAccessRows READ dataAccessRows NOTIFY analysisChanged)
    Q_PROPERTY(QVariantList algorithmRows READ algorithmRows NOTIFY analysisChanged)
    Q_PROPERTY(QVariantList modelRows READ modelRows NOTIFY analysisChanged)
    Q_PROPERTY(QVariantList statisticsRows READ statisticsRows NOTIFY analysisChanged)
    Q_PROPERTY(QVariantList storageRows READ storageRows NOTIFY analysisChanged)
    Q_PROPERTY(QVariantList auditRows READ auditRows NOTIFY analysisChanged)
    Q_PROPERTY(QVariantList reportRows READ reportRows NOTIFY analysisChanged)
    Q_PROPERTY(QVariantList environmentRows READ environmentRows NOTIFY analysisChanged)
    Q_PROPERTY(QVariantList catalogRows READ catalogRows NOTIFY analysisChanged)
    Q_PROPERTY(QVariantList detailRows READ detailRows NOTIFY analysisChanged)
    Q_PROPERTY(QVariantList outboundRows READ outboundRows NOTIFY analysisChanged)
    Q_PROPERTY(QVariantMap summary READ summary NOTIFY analysisChanged)
    Q_PROPERTY(QString storageRoot READ storageRoot NOTIFY analysisChanged)
    Q_PROPERTY(bool running READ running NOTIFY runtimeChanged)

public:
    explicit HistoryDataFusionController(QObject* parent = nullptr);
    ~HistoryDataFusionController() override;

    void setMessageBus(IMessageBus* messageBus);
    void setDataModel(DataModelBase* dataModel);

    QString taskId() const;
    QString taskName() const;
    void setTaskName(const QString& value);
    QString owner() const;
    void setOwner(const QString& value);
    QString dataSourceType() const;
    void setDataSourceType(const QString& value);
    QString filterLocation() const;
    void setFilterLocation(const QString& value);
    QString filterTargetType() const;
    void setFilterTargetType(const QString& value);
    QString filterResult() const;
    void setFilterResult(const QString& value);
    int minBatchRequired() const;
    void setMinBatchRequired(int value);

    QString taskStateText() const;
    QString feedback() const;
    double progress() const;
    QString progressText() const;
    QString importPath() const;
    int recordCount() const;
    int batchCount() const;
    QVariantList stageRows() const;
    QVariantList batchRows() const;
    QVariantList qualityRows() const;
    QVariantList factorRows() const;
    QVariantList conditionRows() const;
    QVariantList issueRows() const;
    QVariantList dataAccessRows() const;
    QVariantList algorithmRows() const;
    QVariantList modelRows() const;
    QVariantList statisticsRows() const;
    QVariantList storageRows() const;
    QVariantList auditRows() const;
    QVariantList reportRows() const;
    QVariantList environmentRows() const;
    QVariantList catalogRows() const;
    QVariantList detailRows() const;
    QVariantList outboundRows() const;
    QVariantMap summary() const;
    QString storageRoot() const;
    bool running() const;

    Q_INVOKABLE void resetTask();
    Q_INVOKABLE QString chooseImportFile();
    Q_INVOKABLE bool importFile(const QString& filePath);
    Q_INVOKABLE void loadDemoDataset();
    Q_INVOKABLE bool startAnalysis();
    Q_INVOKABLE QString exportReport();
    Q_INVOKABLE bool refreshDataCatalog();
    Q_INVOKABLE bool runAlgorithmOptimization();
    Q_INVOKABLE bool runModelIdentification();
    Q_INVOKABLE bool runStatistics();
    Q_INVOKABLE bool archiveResult();
    Q_INVOKABLE bool publishResult();
    Q_INVOKABLE bool runEnvironmentSelfCheck();
    Q_INVOKABLE bool saveCurrentTask();
    Q_INVOKABLE bool loadArchivedTask(const QString& taskId);
    Q_INVOKABLE QString exportCurrentDataset();
    Q_INVOKABLE bool retryPendingPublications();
    Q_INVOKABLE QVariantMap taskContext() const;

signals:
    void taskChanged();
    void taskSettingsChanged();
    void datasetChanged();
    void analysisChanged();
    void runtimeChanged();

private slots:
    void advanceAnalysisStage();

private:
    struct TrialRecord {
        QString recordId;
        QString batchNo;
        QString targetType;
        QString location;
        QString resultLabel;
        QString modality;
        QString sourceType;
        QString weather;
        QDateTime eventTime;
        double distance = 0.0;
        double height = 0.0;
        double measurement = 0.0;
        double confidence = 0.0;
        double visibility = 0.0;
        bool hasDistance = false;
        bool hasHeight = false;
        bool hasMeasurement = false;
        bool hasConfidence = false;
        bool hasVisibility = false;
        bool resultSuccess = false;
        bool resultKnown = false;
        double qualityScore = 0.0;
        QStringList issues;
    };

    struct StageMetric {
        QString name;
        QString status;
        QString output;
        qint64 durationMs = 0;
    };

    void ensureTaskId();
    void resetAnalysisOutputs();
    void rebuildStageRows();
    void setStageStatus(int index, const QString& status, const QString& output, qint64 durationMs);
    void setStateText(const QString& stateText, const QString& feedbackText);
    void setProgress(double value);
    void publishStatus(const QString& eventName) const;
    void syncModel();

    bool parseCsv(const QString& text, QVector<TrialRecord>* records, QStringList* errors) const;
    bool parseJson(const QByteArray& data, QVector<TrialRecord>* records, QStringList* errors) const;
    TrialRecord recordFromMap(const QVariantMap& raw, int index) const;
    bool applyFilters(const TrialRecord& record) const;

    void normalizeRecords();
    void evaluateQuality();
    void runFusionAnalysis();
    void summarizeResults();
    void rebuildDataAccessRows();
    void rebuildAlgorithmRows();
    void rebuildModelRows();
    void rebuildStatisticsRows();
    void rebuildStorageRows();
    void rebuildReportRows();
    void rebuildEnvironmentRows(bool checked);
    void rebuildCatalogRows();
    void rebuildDetailRows();
    void rebuildOutboundRows();
    void appendAuditRow(const QString& action, const QString& objectId, const QString& status, const QString& detail);
    bool persistTaskSnapshot(const QString& reason) const;
    bool writeJsonFile(const QString& path, const QVariant& payload, QString* error) const;
    QVariant readJsonFile(const QString& path, QString* error) const;
    QString taskStorageDir(const QString& taskId) const;
    QString outboxDir() const;
    QVariantMap storageManifest() const;
    QVariantMap recordToVariant(const TrialRecord& record) const;
    TrialRecord recordFromStoredVariant(const QVariantMap& value, int index) const;

    QVariantMap makeIssueRow(const TrialRecord& record, const QString& issue) const;
    QString makeReportMarkdown() const;

    static QString normalizeHeader(const QString& header);
    static QStringList splitCsvLine(const QString& line);
    static double parseNumber(const QVariant& value, bool* ok);
    static QDateTime parseDateTimeValue(const QVariant& value);
    static QString distanceBand(double distance, bool valid);
    static QString heightBand(double height, bool valid);
    static QString qualityGrade(double score);
    static QString formatDuration(qint64 milliseconds);

private:
    IMessageBus* messageBus_ = nullptr;
    DataModelBase* dataModel_ = nullptr;
    QTimer* stageTimer_ = nullptr;
    int stageIndex_ = 0;
    bool running_ = false;

    QString taskId_;
    QString taskName_ = QStringLiteral("历史数据融合分析任务");
    QString owner_ = QStringLiteral("试验分析员");
    QString dataSourceType_ = QStringLiteral("文件导入");
    QString filterLocation_;
    QString filterTargetType_;
    QString filterResult_;
    int minBatchRequired_ = 5;

    QString taskStateText_ = QStringLiteral("已创建");
    QString feedback_ = QStringLiteral("就绪");
    double progress_ = 0.0;
    QString importPath_;

    QVector<TrialRecord> rawRecords_;
    QVector<TrialRecord> standardRecords_;
    QVector<TrialRecord> filteredRecords_;

    QVector<StageMetric> stages_;
    QVariantList stageRows_;
    QVariantList batchRows_;
    QVariantList qualityRows_;
    QVariantList factorRows_;
    QVariantList conditionRows_;
    QVariantList issueRows_;
    QVariantList dataAccessRows_;
    QVariantList algorithmRows_;
    QVariantList modelRows_;
    QVariantList statisticsRows_;
    QVariantList storageRows_;
    QVariantList auditRows_;
    QVariantList reportRows_;
    QVariantList environmentRows_;
    QVariantList catalogRows_;
    QVariantList detailRows_;
    QVariantList outboundRows_;
    QVariantMap summary_;
};
