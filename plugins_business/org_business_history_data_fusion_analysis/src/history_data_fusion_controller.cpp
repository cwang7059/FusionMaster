#include "history_data_fusion_controller.h"

#include <plugin_api/idata_model.h>

#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QRegularExpression>
#include <QSet>
#include <QStandardPaths>
#include <QTextStream>
#include <QTimer>

#include <algorithm>
#include <cmath>

namespace {

constexpr double kEpsilon = 0.000001;

double clampScore(double value) {
    return std::max(0.0, std::min(100.0, value));
}

double averageOrZero(double sum, int count) {
    return count <= 0 ? 0.0 : sum / static_cast<double>(count);
}

QString percentText(double ratio) {
    return QStringLiteral("%1%").arg(QString::number(ratio * 100.0, 'f', 1));
}

QString numberText(double value, int precision = 2) {
    return QString::number(value, 'f', precision);
}

QString stableTaskId() {
    return QStringLiteral("HFA-%1").arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMddhhmmsszzz")));
}

QString firstNonEmpty(const QString& value, const QString& fallback) {
    const QString trimmed = value.trimmed();
    return trimmed.isEmpty() ? fallback : trimmed;
}

}  // namespace

HistoryDataFusionController::HistoryDataFusionController(QObject* parent)
    : QObject(parent)
    , stageTimer_(new QTimer(this)) {
    stageTimer_->setInterval(260);
    connect(stageTimer_, &QTimer::timeout, this, &HistoryDataFusionController::advanceAnalysisStage);
    resetTask();
}

HistoryDataFusionController::~HistoryDataFusionController() {
    if (stageTimer_ != nullptr) {
        stageTimer_->stop();
    }
}

void HistoryDataFusionController::setMessageBus(IMessageBus* messageBus) {
    messageBus_ = messageBus;
}

void HistoryDataFusionController::setDataModel(DataModelBase* dataModel) {
    dataModel_ = dataModel;
    syncModel();
}

QString HistoryDataFusionController::taskId() const {
    return taskId_;
}

QString HistoryDataFusionController::taskName() const {
    return taskName_;
}

void HistoryDataFusionController::setTaskName(const QString& value) {
    const QString normalized = firstNonEmpty(value, QStringLiteral("历史数据融合分析任务"));
    if (taskName_ == normalized) {
        return;
    }
    taskName_ = normalized;
    emit taskSettingsChanged();
    syncModel();
}

QString HistoryDataFusionController::owner() const {
    return owner_;
}

void HistoryDataFusionController::setOwner(const QString& value) {
    const QString normalized = firstNonEmpty(value, QStringLiteral("试验分析员"));
    if (owner_ == normalized) {
        return;
    }
    owner_ = normalized;
    emit taskSettingsChanged();
    syncModel();
}

QString HistoryDataFusionController::dataSourceType() const {
    return dataSourceType_;
}

void HistoryDataFusionController::setDataSourceType(const QString& value) {
    const QString normalized = firstNonEmpty(value, QStringLiteral("文件导入"));
    if (dataSourceType_ == normalized) {
        return;
    }
    dataSourceType_ = normalized;
    emit taskSettingsChanged();
    syncModel();
}

QString HistoryDataFusionController::filterLocation() const {
    return filterLocation_;
}

void HistoryDataFusionController::setFilterLocation(const QString& value) {
    const QString normalized = value.trimmed();
    if (filterLocation_ == normalized) {
        return;
    }
    filterLocation_ = normalized;
    emit taskSettingsChanged();
    syncModel();
}

QString HistoryDataFusionController::filterTargetType() const {
    return filterTargetType_;
}

void HistoryDataFusionController::setFilterTargetType(const QString& value) {
    const QString normalized = value.trimmed();
    if (filterTargetType_ == normalized) {
        return;
    }
    filterTargetType_ = normalized;
    emit taskSettingsChanged();
    syncModel();
}

QString HistoryDataFusionController::filterResult() const {
    return filterResult_;
}

void HistoryDataFusionController::setFilterResult(const QString& value) {
    const QString normalized = value.trimmed();
    if (filterResult_ == normalized) {
        return;
    }
    filterResult_ = normalized;
    emit taskSettingsChanged();
    syncModel();
}

int HistoryDataFusionController::minBatchRequired() const {
    return minBatchRequired_;
}

void HistoryDataFusionController::setMinBatchRequired(int value) {
    const int normalized = std::max(1, std::min(50, value));
    if (minBatchRequired_ == normalized) {
        return;
    }
    minBatchRequired_ = normalized;
    emit taskSettingsChanged();
    syncModel();
}

QString HistoryDataFusionController::taskStateText() const {
    return taskStateText_;
}

QString HistoryDataFusionController::feedback() const {
    return feedback_;
}

double HistoryDataFusionController::progress() const {
    return progress_;
}

QString HistoryDataFusionController::progressText() const {
    return QStringLiteral("%1%").arg(QString::number(progress_ * 100.0, 'f', 0));
}

QString HistoryDataFusionController::importPath() const {
    return importPath_;
}

int HistoryDataFusionController::recordCount() const {
    return rawRecords_.size();
}

int HistoryDataFusionController::batchCount() const {
    QSet<QString> batches;
    for (const TrialRecord& record : rawRecords_) {
        if (!record.batchNo.trimmed().isEmpty()) {
            batches.insert(record.batchNo.trimmed());
        }
    }
    return batches.size();
}

QVariantList HistoryDataFusionController::stageRows() const {
    return stageRows_;
}

QVariantList HistoryDataFusionController::batchRows() const {
    return batchRows_;
}

QVariantList HistoryDataFusionController::qualityRows() const {
    return qualityRows_;
}

QVariantList HistoryDataFusionController::factorRows() const {
    return factorRows_;
}

QVariantList HistoryDataFusionController::conditionRows() const {
    return conditionRows_;
}

QVariantList HistoryDataFusionController::issueRows() const {
    return issueRows_;
}

QVariantList HistoryDataFusionController::dataAccessRows() const {
    return dataAccessRows_;
}

QVariantList HistoryDataFusionController::algorithmRows() const {
    return algorithmRows_;
}

QVariantList HistoryDataFusionController::modelRows() const {
    return modelRows_;
}

QVariantList HistoryDataFusionController::statisticsRows() const {
    return statisticsRows_;
}

QVariantList HistoryDataFusionController::storageRows() const {
    return storageRows_;
}

QVariantList HistoryDataFusionController::auditRows() const {
    return auditRows_;
}

QVariantList HistoryDataFusionController::reportRows() const {
    return reportRows_;
}

QVariantList HistoryDataFusionController::environmentRows() const {
    return environmentRows_;
}

QVariantList HistoryDataFusionController::catalogRows() const {
    return catalogRows_;
}

QVariantList HistoryDataFusionController::detailRows() const {
    return detailRows_;
}

QVariantList HistoryDataFusionController::outboundRows() const {
    return outboundRows_;
}

QVariantMap HistoryDataFusionController::summary() const {
    return summary_;
}

QString HistoryDataFusionController::storageRoot() const {
    QString root = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (root.trimmed().isEmpty()) {
        root = QDir::current().filePath(QStringLiteral("runtime_data"));
    }
    return QDir(root).filePath(QStringLiteral("history_data_fusion"));
}

bool HistoryDataFusionController::running() const {
    return running_;
}

void HistoryDataFusionController::resetTask() {
    if (stageTimer_ != nullptr) {
        stageTimer_->stop();
    }
    running_ = false;
    stageIndex_ = 0;
    taskId_.clear();
    importPath_.clear();
    rawRecords_.clear();
    filteredRecords_.clear();
    standardRecords_.clear();
    auditRows_.clear();
    resetAnalysisOutputs();
    appendAuditRow(
        QStringLiteral("任务重置"),
        QStringLiteral("history_fusion.task"),
        QStringLiteral("完成"),
        QStringLiteral("清空任务上下文、数据集和分析结果"));
    rebuildStorageRows();
    setStateText(QStringLiteral("已创建"), QStringLiteral("就绪"));
    setProgress(0.0);
    emit taskChanged();
    emit datasetChanged();
    emit analysisChanged();
    syncModel();
}

QString HistoryDataFusionController::chooseImportFile() {
    const QString defaultDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    return QFileDialog::getOpenFileName(
        nullptr,
        QStringLiteral("选择历史试验数据"),
        defaultDir,
        QStringLiteral("试验数据 (*.csv *.tsv *.json);;CSV 文件 (*.csv *.tsv);;JSON 文件 (*.json);;所有文件 (*.*)"));
}

bool HistoryDataFusionController::importFile(const QString& filePath) {
    const QString normalizedPath = filePath.trimmed();
    if (normalizedPath.isEmpty()) {
        setStateText(QStringLiteral("待加载"), QStringLiteral("未选择导入文件"));
        return false;
    }

    QFile file(normalizedPath);
    if (!file.open(QIODevice::ReadOnly)) {
        setStateText(QStringLiteral("待加载"), QStringLiteral("导入失败：文件无法读取"));
        return false;
    }

    QVector<TrialRecord> records;
    QStringList errors;
    const QByteArray data = file.readAll();
    const QString suffix = QFileInfo(normalizedPath).suffix().toLower();
    bool ok = false;

    if (suffix == QStringLiteral("json")) {
        ok = parseJson(data, &records, &errors);
    } else if (suffix == QStringLiteral("csv") || suffix == QStringLiteral("tsv") || suffix == QStringLiteral("txt")) {
        ok = parseCsv(QString::fromUtf8(data), &records, &errors);
    } else {
        errors.push_back(QStringLiteral("暂不支持该格式，请导入 CSV/TSV/JSON 数据"));
    }

    if (!ok || records.isEmpty()) {
        setStateText(
            QStringLiteral("待加载"),
            errors.isEmpty() ? QStringLiteral("导入失败：未解析到有效记录") : errors.join(QStringLiteral("；")));
        return false;
    }

    rawRecords_ = records;
    importPath_ = QDir::cleanPath(normalizedPath);
    auditRows_.clear();
    resetAnalysisOutputs();
    appendAuditRow(
        QStringLiteral("数据导入"),
        importPath_,
        QStringLiteral("完成"),
        QStringLiteral("解析 %1 条记录、%2 个批次").arg(recordCount()).arg(batchCount()));
    rebuildStorageRows();
    persistTaskSnapshot(QStringLiteral("数据导入"));
    rebuildCatalogRows();
    rebuildDetailRows();
    setStateText(QStringLiteral("待加载"), QStringLiteral("已导入 %1 条记录，%2 个批次").arg(recordCount()).arg(batchCount()));
    setProgress(0.0);
    emit datasetChanged();
    emit analysisChanged();
    syncModel();
    publishStatus(QStringLiteral("dataset_imported"));
    return true;
}

void HistoryDataFusionController::loadDemoDataset() {
    QVector<TrialRecord> records;
    const QStringList batches = {
        QStringLiteral("B20260401"),
        QStringLiteral("B20260402"),
        QStringLiteral("B20260403"),
        QStringLiteral("B20260404"),
        QStringLiteral("B20260405"),
        QStringLiteral("B20260406")
    };
    const QStringList targets = {
        QStringLiteral("无人机"),
        QStringLiteral("车辆"),
        QStringLiteral("人员")
    };
    const QStringList locations = {
        QStringLiteral("东区靶场"),
        QStringLiteral("北区靶场"),
        QStringLiteral("山地试验区")
    };
    const QStringList modalities = {
        QStringLiteral("可见光"),
        QStringLiteral("红外"),
        QStringLiteral("激光")
    };
    const QStringList weather = {
        QStringLiteral("晴"),
        QStringLiteral("雾"),
        QStringLiteral("雨后")
    };

    const QDateTime baseTime = QDateTime::currentDateTime().addDays(-30);
    int index = 0;
    for (int batchIndex = 0; batchIndex < batches.size(); ++batchIndex) {
        for (int sample = 0; sample < 6; ++sample) {
            TrialRecord record;
            record.recordId = QStringLiteral("DEMO-%1").arg(++index, 3, 10, QLatin1Char('0'));
            record.batchNo = batches.at(batchIndex);
            record.targetType = targets.at((batchIndex + sample) % targets.size());
            record.location = locations.at((batchIndex + sample / 2) % locations.size());
            record.modality = modalities.at(sample % modalities.size());
            record.weather = weather.at((batchIndex + sample) % weather.size());
            record.sourceType = QStringLiteral("demo");
            record.eventTime = baseTime.addDays(batchIndex).addSecs(sample * 340);
            record.distance = 650.0 + batchIndex * 420.0 + sample * 85.0;
            record.height = 35.0 + ((batchIndex + sample) % 5) * 45.0;
            record.measurement = 0.54 + batchIndex * 0.035 + sample * 0.018;
            record.visibility = 4.5 + ((batchIndex + sample) % 4) * 1.4;
            record.confidence = 0.62 + ((batchIndex + sample) % 5) * 0.06;
            record.resultSuccess = record.measurement > 0.68
                || (record.distance < 1800.0 && record.visibility > 5.0);
            record.resultKnown = true;
            record.resultLabel = record.resultSuccess ? QStringLiteral("成功") : QStringLiteral("未命中");
            record.hasDistance = true;
            record.hasHeight = true;
            record.hasMeasurement = true;
            record.hasConfidence = true;
            record.hasVisibility = true;
            records.push_back(record);
        }
    }

    if (!records.isEmpty()) {
        records[4].hasVisibility = false;
        records[4].issues.push_back(QStringLiteral("能见度缺失"));
        records[17].confidence = 1.24;
        records[17].issues.push_back(QStringLiteral("置信度超出范围"));
        records[25].distance = 6200.0;
        records[25].issues.push_back(QStringLiteral("距离疑似离群"));
    }

    rawRecords_ = records;
    importPath_ = QStringLiteral("内置示例数据集");
    auditRows_.clear();
    resetAnalysisOutputs();
    appendAuditRow(
        QStringLiteral("载入示例数据"),
        importPath_,
        QStringLiteral("完成"),
        QStringLiteral("生成 %1 条记录、%2 个批次").arg(recordCount()).arg(batchCount()));
    rebuildStorageRows();
    persistTaskSnapshot(QStringLiteral("载入示例数据"));
    rebuildCatalogRows();
    rebuildDetailRows();
    setStateText(QStringLiteral("待加载"), QStringLiteral("已载入示例数据 %1 条，%2 个批次").arg(recordCount()).arg(batchCount()));
    setProgress(0.0);
    emit datasetChanged();
    emit analysisChanged();
    syncModel();
    publishStatus(QStringLiteral("demo_dataset_loaded"));
}

bool HistoryDataFusionController::startAnalysis() {
    if (running_) {
        return false;
    }
    if (rawRecords_.isEmpty()) {
        setStateText(QStringLiteral("待加载"), QStringLiteral("请先导入历史数据或载入示例数据"));
        return false;
    }

    ensureTaskId();
    resetAnalysisOutputs();
    appendAuditRow(
        QStringLiteral("启动分析"),
        taskId_,
        QStringLiteral("运行中"),
        QStringLiteral("任务进入状态机，最小批次数要求 %1").arg(minBatchRequired_));
    rebuildStorageRows();
    running_ = true;
    stageIndex_ = 0;
    setStateText(QStringLiteral("已创建"), QStringLiteral("分析任务已进入阶段状态机"));
    setProgress(0.01);
    publishStatus(QStringLiteral("analysis_started"));
    emit runtimeChanged();
    syncModel();

    QTimer::singleShot(0, this, &HistoryDataFusionController::advanceAnalysisStage);
    stageTimer_->start();
    return true;
}

QString HistoryDataFusionController::exportReport() {
    if (summary_.isEmpty() || !summary_.contains(QStringLiteral("conclusion"))) {
        setStateText(taskStateText_, QStringLiteral("尚无可导出的融合分析结果"));
        return QString();
    }

    ensureTaskId();
    const QString defaultDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    const QString defaultPath = QDir(defaultDir).filePath(QStringLiteral("%1_%2.md").arg(taskId_, taskName_));
    const QString reportPath = QFileDialog::getSaveFileName(
        nullptr,
        QStringLiteral("导出融合分析报告"),
        defaultPath,
        QStringLiteral("Markdown 报告 (*.md);;文本文件 (*.txt)"));
    if (reportPath.trimmed().isEmpty()) {
        return QString();
    }

    QFile file(reportPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        setStateText(taskStateText_, QStringLiteral("报告导出失败：路径不可写"));
        return QString();
    }

    QTextStream out(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    out.setCodec("UTF-8");
#endif
    out << makeReportMarkdown();
    file.close();

    summary_.insert(QStringLiteral("reportPath"), QDir::cleanPath(reportPath));
    rebuildReportRows();
    rebuildStorageRows();
    appendAuditRow(
        QStringLiteral("导出报告"),
        QDir::cleanPath(reportPath),
        QStringLiteral("完成"),
        QStringLiteral("按模板 RPT-2026.04 生成 Markdown 报告"));
    rebuildStorageRows();
    setStateText(taskStateText_, QStringLiteral("报告已导出：%1").arg(QDir::cleanPath(reportPath)));
    emit analysisChanged();
    syncModel();
    publishStatus(QStringLiteral("report_exported"));
    return QDir::cleanPath(reportPath);
}

bool HistoryDataFusionController::refreshDataCatalog() {
    rebuildCatalogRows();
    rebuildDetailRows();
    rebuildOutboundRows();
    rebuildDataAccessRows();
    appendAuditRow(
        QStringLiteral("刷新数据目录"),
        taskId_.isEmpty() ? QStringLiteral("未建任务") : taskId_,
        QStringLiteral("完成"),
        QStringLiteral("扫描本地仓储 %1 项，生成数据调取接口状态").arg(catalogRows_.size()));
    rebuildStorageRows();
    setStateText(taskStateText_, QStringLiteral("数据目录已刷新，可按批次、图表和结果继续调取"));
    emit analysisChanged();
    syncModel();
    publishStatus(QStringLiteral("data_catalog_refreshed"));
    return true;
}

bool HistoryDataFusionController::runAlgorithmOptimization() {
    rebuildAlgorithmRows();
    summary_.insert(QStringLiteral("algorithmInterfaceState"), QStringLiteral("接口预留"));
    appendAuditRow(
        QStringLiteral("算法自主优化接口"),
        taskId_.isEmpty() ? QStringLiteral("algorithm.contract") : taskId_,
        QStringLiteral("接口预留"),
        QStringLiteral("仅固化候选算法、评分输入、回退策略和审计字段，不执行算法优化"));
    rebuildReportRows();
    rebuildStorageRows();
    setStateText(taskStateText_, QStringLiteral("算法优化相关能力已按接口预留，当前版本不执行算法切换"));
    emit analysisChanged();
    syncModel();
    publishStatus(QStringLiteral("algorithm_interface_reserved"));
    return true;
}

bool HistoryDataFusionController::runModelIdentification() {
    rebuildModelRows();
    summary_.insert(QStringLiteral("modelInterfaceState"), QStringLiteral("接口预留"));
    appendAuditRow(
        QStringLiteral("模型辅助辨识接口"),
        taskId_.isEmpty() ? QStringLiteral("model.contract") : taskId_,
        QStringLiteral("接口预留"),
        QStringLiteral("仅固化特征输入、模型版本、置信度输出和人工复核字段"));
    rebuildReportRows();
    rebuildStorageRows();
    setStateText(taskStateText_, QStringLiteral("模型辨识相关能力已按接口预留，等待后续模型组件接入"));
    emit analysisChanged();
    syncModel();
    publishStatus(QStringLiteral("model_interface_reserved"));
    return true;
}

bool HistoryDataFusionController::runStatistics() {
    rebuildStatisticsRows();
    appendAuditRow(
        QStringLiteral("执行统计分析"),
        taskId_.isEmpty() ? QStringLiteral("statistics.preview") : taskId_,
        statisticsRows_.isEmpty() ? QStringLiteral("无数据") : QStringLiteral("完成"),
        QStringLiteral("按任务、批次、目标和工况口径生成统计结果"));
    rebuildReportRows();
    rebuildStorageRows();
    setStateText(taskStateText_, statisticsRows_.isEmpty()
        ? QStringLiteral("暂无可统计数据，请先导入或分析")
        : QStringLiteral("统计结果已刷新，报告将引用当前统计口径"));
    emit analysisChanged();
    syncModel();
    publishStatus(QStringLiteral("statistics_refreshed"));
    return !statisticsRows_.isEmpty();
}

bool HistoryDataFusionController::archiveResult() {
    if (summary_.isEmpty() || !summary_.contains(QStringLiteral("conclusion"))) {
        setStateText(taskStateText_, QStringLiteral("尚无可归档结果"));
        return false;
    }

    ensureTaskId();
    summary_.insert(QStringLiteral("archiveState"), QStringLiteral("已归档"));
    summary_.insert(QStringLiteral("archivedAt"), QDateTime::currentDateTime().toString(Qt::ISODate));
    summary_.insert(QStringLiteral("archiveCode"), QStringLiteral("ARCH-%1").arg(taskId_));
    summary_.insert(QStringLiteral("storagePath"), taskStorageDir(taskId_));
    rebuildStorageRows();
    rebuildReportRows();
    appendAuditRow(
        QStringLiteral("结果归档"),
        taskId_,
        QStringLiteral("完成"),
        QStringLiteral("结果层、图表索引、审计记录已形成归档状态"));
    rebuildStorageRows();
    setStateText(QStringLiteral("已归档"), QStringLiteral("分析结果已归档，历史版本保持只读可追溯"));
    persistTaskSnapshot(QStringLiteral("结果归档"));
    rebuildCatalogRows();
    emit taskChanged();
    emit analysisChanged();
    syncModel();
    publishStatus(QStringLiteral("result_archived"));
    return true;
}

bool HistoryDataFusionController::publishResult() {
    if (summary_.isEmpty() || !summary_.contains(QStringLiteral("conclusion"))) {
        setStateText(taskStateText_, QStringLiteral("尚无可发布结果"));
        return false;
    }

    ensureTaskId();
    const QString receipt = QStringLiteral("RCPT-%1").arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMddhhmmss")));
    summary_.insert(QStringLiteral("publishState"), QStringLiteral("本地回执已登记"));
    summary_.insert(QStringLiteral("receiptCode"), receipt);
    summary_.insert(QStringLiteral("publishedAt"), QDateTime::currentDateTime().toString(Qt::ISODate));
    QVariantMap outbound;
    outbound.insert(QStringLiteral("receiptCode"), receipt);
    outbound.insert(QStringLiteral("taskId"), taskId_);
    outbound.insert(QStringLiteral("target"), QStringLiteral("外部结果接收实体"));
    outbound.insert(QStringLiteral("status"), QStringLiteral("待发送"));
    outbound.insert(QStringLiteral("retryCount"), 0);
    outbound.insert(QStringLiteral("createdAt"), summary_.value(QStringLiteral("publishedAt")).toString());
    outbound.insert(QStringLiteral("payload"), summary_);
    QString error;
    writeJsonFile(QDir(outboxDir()).filePath(QStringLiteral("%1.json").arg(receipt)), outbound, &error);
    rebuildReportRows();
    rebuildStorageRows();
    rebuildOutboundRows();
    appendAuditRow(
        QStringLiteral("结果发布"),
        taskId_,
        error.isEmpty() ? QStringLiteral("待发送") : QStringLiteral("失败"),
        error.isEmpty()
            ? QStringLiteral("生成发布回执 %1，已进入本地待发队列").arg(receipt)
            : QStringLiteral("生成待发记录失败：%1").arg(error));
    rebuildStorageRows();
    persistTaskSnapshot(QStringLiteral("结果发布"));
    setStateText(taskStateText_, error.isEmpty()
        ? QStringLiteral("发布回执已登记，已进入本地待发队列")
        : QStringLiteral("发布回执登记失败，请检查 outbox 目录权限"));
    emit analysisChanged();
    syncModel();
    publishStatus(QStringLiteral("result_published"));
    return true;
}

bool HistoryDataFusionController::runEnvironmentSelfCheck() {
    rebuildEnvironmentRows(true);
    appendAuditRow(
        QStringLiteral("环境自检"),
        QStringLiteral("runtime.environment"),
        QStringLiteral("完成"),
        QStringLiteral("完成目录、版本、消息总线和配置项自检"));
    rebuildStorageRows();
    setStateText(taskStateText_, QStringLiteral("运行环境自检完成，可查看环境运维页"));
    emit analysisChanged();
    syncModel();
    publishStatus(QStringLiteral("environment_checked"));
    return true;
}

bool HistoryDataFusionController::saveCurrentTask() {
    if (rawRecords_.isEmpty() && summary_.isEmpty()) {
        setStateText(taskStateText_, QStringLiteral("当前没有可保存的任务数据"));
        return false;
    }
    ensureTaskId();
    appendAuditRow(
        QStringLiteral("手动保存"),
        taskId_,
        QStringLiteral("执行中"),
        QStringLiteral("写入本地仓储快照"));
    const bool ok = persistTaskSnapshot(QStringLiteral("手动保存"));
    rebuildCatalogRows();
    rebuildStorageRows();
    rebuildDetailRows();
    setStateText(taskStateText_, ok
        ? QStringLiteral("任务快照已保存至本地仓储")
        : QStringLiteral("任务快照保存失败，请检查目录权限"));
    emit analysisChanged();
    syncModel();
    publishStatus(ok ? QStringLiteral("task_saved") : QStringLiteral("task_save_failed"));
    return ok;
}

bool HistoryDataFusionController::loadArchivedTask(const QString& taskId) {
    const QString normalizedTaskId = taskId.trimmed();
    if (normalizedTaskId.isEmpty()) {
        setStateText(taskStateText_, QStringLiteral("请选择要调取的任务编号"));
        return false;
    }

    QString error;
    const QString dir = taskStorageDir(normalizedTaskId);
    const QVariant rawPayload = readJsonFile(QDir(dir).filePath(QStringLiteral("raw_records.json")), &error);
    const QVariant stdPayload = readJsonFile(QDir(dir).filePath(QStringLiteral("standard_records.json")), nullptr);
    const QVariant summaryPayload = readJsonFile(QDir(dir).filePath(QStringLiteral("summary.json")), nullptr);
    const QVariant auditPayload = readJsonFile(QDir(dir).filePath(QStringLiteral("audit.json")), nullptr);
    const QVariant manifestPayload = readJsonFile(QDir(dir).filePath(QStringLiteral("manifest.json")), nullptr);

    const QVariantList rawList = rawPayload.toList();
    if (!error.isEmpty() || rawList.isEmpty()) {
        setStateText(taskStateText_, QStringLiteral("调取失败：未找到任务 %1 的原始记录").arg(normalizedTaskId));
        return false;
    }

    if (stageTimer_ != nullptr) {
        stageTimer_->stop();
    }
    running_ = false;
    stageIndex_ = 0;
    taskId_ = normalizedTaskId;
    rawRecords_.clear();
    standardRecords_.clear();
    filteredRecords_.clear();

    int index = 0;
    for (const QVariant& item : rawList) {
        rawRecords_.push_back(recordFromStoredVariant(item.toMap(), ++index));
    }
    const QVariantList stdList = stdPayload.toList();
    index = 0;
    for (const QVariant& item : stdList) {
        TrialRecord record = recordFromStoredVariant(item.toMap(), ++index);
        record.qualityScore = item.toMap().value(QStringLiteral("qualityScore")).toDouble();
        standardRecords_.push_back(record);
    }
    filteredRecords_ = standardRecords_.isEmpty() ? rawRecords_ : standardRecords_;
    const QVariantMap restoredSummary = summaryPayload.toMap();
    summary_ = restoredSummary;
    auditRows_ = auditPayload.toList();
    const QVariantMap manifest = manifestPayload.toMap();
    taskName_ = firstNonEmpty(manifest.value(QStringLiteral("taskName")).toString(), taskName_);
    owner_ = firstNonEmpty(manifest.value(QStringLiteral("owner")).toString(), owner_);
    importPath_ = firstNonEmpty(manifest.value(QStringLiteral("importPath")).toString(), QStringLiteral("本地归档:%1").arg(normalizedTaskId));

    resetAnalysisOutputs();
    summary_ = restoredSummary;
    auditRows_ = auditPayload.toList();
    if (!standardRecords_.isEmpty()) {
        evaluateQuality();
        runFusionAnalysis();
        for (auto it = restoredSummary.cbegin(); it != restoredSummary.cend(); ++it) {
            summary_.insert(it.key(), it.value());
        }
    }
    rebuildDataAccessRows();
    rebuildStatisticsRows();
    rebuildStorageRows();
    rebuildReportRows();
    rebuildCatalogRows();
    rebuildDetailRows();
    rebuildOutboundRows();
    rebuildEnvironmentRows(false);
    appendAuditRow(
        QStringLiteral("调取归档任务"),
        normalizedTaskId,
        QStringLiteral("完成"),
        QStringLiteral("从本地仓储恢复任务上下文和记录"));
    setStateText(QStringLiteral("已调取"), QStringLiteral("已调取归档任务 %1，可继续分析或导出").arg(normalizedTaskId));
    setProgress(summary_.contains(QStringLiteral("conclusion")) ? 1.0 : 0.0);
    emit taskChanged();
    emit datasetChanged();
    emit analysisChanged();
    syncModel();
    publishStatus(QStringLiteral("archived_task_loaded"));
    return true;
}

QString HistoryDataFusionController::exportCurrentDataset() {
    const QVector<TrialRecord>* records = &standardRecords_;
    if (records->isEmpty()) {
        records = filteredRecords_.isEmpty() ? &rawRecords_ : &filteredRecords_;
    }
    if (records->isEmpty()) {
        setStateText(taskStateText_, QStringLiteral("当前没有可导出的调取数据"));
        return QString();
    }

    ensureTaskId();
    const QString defaultDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    const QString defaultPath = QDir(defaultDir).filePath(QStringLiteral("%1_dataset.csv").arg(taskId_));
    const QString exportPath = QFileDialog::getSaveFileName(
        nullptr,
        QStringLiteral("导出当前调取数据集"),
        defaultPath,
        QStringLiteral("CSV 文件 (*.csv);;JSON 文件 (*.json)"));
    if (exportPath.trimmed().isEmpty()) {
        return QString();
    }

    QFile file(exportPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        setStateText(taskStateText_, QStringLiteral("数据集导出失败：路径不可写"));
        return QString();
    }

    const bool json = QFileInfo(exportPath).suffix().compare(QStringLiteral("json"), Qt::CaseInsensitive) == 0;
    if (json) {
        QVariantList rows;
        for (const TrialRecord& record : *records) {
            rows.push_back(recordToVariant(record));
        }
        file.write(QJsonDocument::fromVariant(rows).toJson(QJsonDocument::Indented));
    } else {
        QTextStream out(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        out.setCodec("UTF-8");
#endif
        out << "record_id,batch_no,target_type,location,result_label,modality,source,event_time,distance,height,measurement,confidence,visibility,quality_score\n";
        for (const TrialRecord& record : *records) {
            out << record.recordId << ','
                << record.batchNo << ','
                << record.targetType << ','
                << record.location << ','
                << record.resultLabel << ','
                << record.modality << ','
                << record.sourceType << ','
                << record.eventTime.toString(Qt::ISODate) << ','
                << numberText(record.distance, 3) << ','
                << numberText(record.height, 3) << ','
                << numberText(record.measurement, 6) << ','
                << numberText(record.confidence, 6) << ','
                << numberText(record.visibility, 3) << ','
                << numberText(record.qualityScore, 2) << '\n';
        }
    }
    file.close();

    summary_.insert(QStringLiteral("datasetExportPath"), QDir::cleanPath(exportPath));
    appendAuditRow(
        QStringLiteral("导出数据集"),
        QDir::cleanPath(exportPath),
        QStringLiteral("完成"),
        QStringLiteral("导出 %1 条调取记录").arg(records->size()));
    rebuildReportRows();
    rebuildStorageRows();
    persistTaskSnapshot(QStringLiteral("导出数据集"));
    setStateText(taskStateText_, QStringLiteral("数据集已导出：%1").arg(QDir::cleanPath(exportPath)));
    emit analysisChanged();
    syncModel();
    publishStatus(QStringLiteral("dataset_exported"));
    return QDir::cleanPath(exportPath);
}

bool HistoryDataFusionController::retryPendingPublications() {
    QDir dir(outboxDir());
    const QFileInfoList files = dir.entryInfoList(QStringList() << QStringLiteral("*.json"), QDir::Files, QDir::Time);
    int touched = 0;
    for (const QFileInfo& info : files) {
        QString error;
        QVariantMap payload = readJsonFile(info.absoluteFilePath(), &error).toMap();
        if (!error.isEmpty() || payload.isEmpty()) {
            continue;
        }
        const QString status = payload.value(QStringLiteral("status")).toString();
        if (status == QStringLiteral("已发送")) {
            continue;
        }
        payload.insert(QStringLiteral("status"), QStringLiteral("待发送"));
        payload.insert(QStringLiteral("retryCount"), payload.value(QStringLiteral("retryCount")).toInt() + 1);
        payload.insert(QStringLiteral("lastRetryAt"), QDateTime::currentDateTime().toString(Qt::ISODate));
        writeJsonFile(info.absoluteFilePath(), payload, nullptr);
        ++touched;
    }

    rebuildOutboundRows();
    appendAuditRow(
        QStringLiteral("补发待发队列"),
        QStringLiteral("outbox"),
        touched > 0 ? QStringLiteral("完成") : QStringLiteral("无待发"),
        QStringLiteral("刷新 %1 条待发回执，等待外部通信组件发送").arg(touched));
    rebuildStorageRows();
    setStateText(taskStateText_, QStringLiteral("待发队列已刷新：%1 条").arg(touched));
    emit analysisChanged();
    syncModel();
    publishStatus(QStringLiteral("outbox_retry_refreshed"));
    return true;
}

QVariantMap HistoryDataFusionController::taskContext() const {
    QVariantMap context;
    context.insert(QStringLiteral("taskId"), taskId_);
    context.insert(QStringLiteral("taskName"), taskName_);
    context.insert(QStringLiteral("owner"), owner_);
    context.insert(QStringLiteral("dataSourceType"), dataSourceType_);
    context.insert(QStringLiteral("importPath"), importPath_);
    context.insert(QStringLiteral("storageRoot"), storageRoot());
    context.insert(QStringLiteral("filterLocation"), filterLocation_);
    context.insert(QStringLiteral("filterTargetType"), filterTargetType_);
    context.insert(QStringLiteral("filterResult"), filterResult_);
    context.insert(QStringLiteral("minBatchRequired"), minBatchRequired_);
    context.insert(QStringLiteral("fieldMappingVersion"), QStringLiteral("FM-2026.04"));
    context.insert(QStringLiteral("qualityRuleVersion"), QStringLiteral("QR-2026.04"));
    context.insert(QStringLiteral("fusionAlgorithmVersion"), QStringLiteral("FA-2026.04"));
    context.insert(QStringLiteral("statisticsVersion"), QStringLiteral("STAT-2026.04"));
    context.insert(QStringLiteral("reportTemplateVersion"), QStringLiteral("RPT-2026.04"));
    return context;
}

void HistoryDataFusionController::advanceAnalysisStage() {
    if (!running_) {
        return;
    }

    if (stageIndex_ < 0 || stageIndex_ >= stages_.size()) {
        if (stageTimer_ != nullptr) {
            stageTimer_->stop();
        }
        running_ = false;
        return;
    }

    stages_[stageIndex_].status = QStringLiteral("执行中");
    rebuildStageRows();
    emit analysisChanged();

    const qint64 durationMs = 40 + rawRecords_.size() * 2 + stageIndex_ * 17;
    switch (stageIndex_) {
    case 0:
        ensureTaskId();
        setStageStatus(stageIndex_, QStringLiteral("完成"), taskId_, durationMs);
        setStateText(QStringLiteral("待加载"), QStringLiteral("任务上下文已固化，规则版本已登记"));
        setProgress(0.12);
        break;
    case 1:
        filteredRecords_.clear();
        for (const TrialRecord& record : rawRecords_) {
            if (applyFilters(record)) {
                filteredRecords_.push_back(record);
            }
        }
        if (filteredRecords_.isEmpty()) {
            setStageStatus(stageIndex_, QStringLiteral("失败"), QStringLiteral("筛选条件下无有效记录"), durationMs);
            running_ = false;
            stageTimer_->stop();
            setStateText(QStringLiteral("已失败"), QStringLiteral("筛选条件过窄，未获得可分析样本"));
            publishStatus(QStringLiteral("analysis_failed"));
            break;
        }
        setStageStatus(stageIndex_, QStringLiteral("完成"), QStringLiteral("接入 %1 条记录").arg(filteredRecords_.size()), durationMs);
        setStateText(QStringLiteral("预处理中"), QStringLiteral("数据接入完成，进入字段标准化"));
        setProgress(0.28);
        break;
    case 2:
        normalizeRecords();
        rebuildDetailRows();
        setStageStatus(stageIndex_, QStringLiteral("完成"), QStringLiteral("标准化 %1 条记录").arg(standardRecords_.size()), durationMs);
        setStateText(QStringLiteral("预处理中"), QStringLiteral("统一字段集转换、单位归一与时间对齐完成"));
        setProgress(0.44);
        break;
    case 3:
        evaluateQuality();
        setStageStatus(
            stageIndex_,
            QStringLiteral("完成"),
            QStringLiteral("质量分 %1，问题 %2 项")
                .arg(numberText(summary_.value(QStringLiteral("qualityScore")).toDouble(), 1))
                .arg(issueRows_.size()),
            durationMs);
        setStateText(QStringLiteral("融合分析中"), QStringLiteral("质量评估完成，进入多批次融合分析"));
        setProgress(0.62);
        break;
    case 4:
        runFusionAnalysis();
        setStageStatus(
            stageIndex_,
            summary_.value(QStringLiteral("usedBatchCount")).toInt() < minBatchRequired_ ? QStringLiteral("待复核") : QStringLiteral("完成"),
            QStringLiteral("批次 %1，关键因子 %2")
                .arg(summary_.value(QStringLiteral("usedBatchCount")).toInt())
                .arg(summary_.value(QStringLiteral("keyFactor"), QStringLiteral("-")).toString()),
            durationMs);
        setStateText(QStringLiteral("结果汇总中"), QStringLiteral("融合指标、关键因子与工况排序已生成"));
        setProgress(0.82);
        break;
    case 5:
        summarizeResults();
        persistTaskSnapshot(QStringLiteral("分析完成"));
        rebuildCatalogRows();
        rebuildDetailRows();
        setStageStatus(stageIndex_, QStringLiteral("完成"), summary_.value(QStringLiteral("conclusion")).toString(), durationMs);
        running_ = false;
        stageTimer_->stop();
        setProgress(1.0);
        if (summary_.value(QStringLiteral("reviewRequired")).toBool()) {
            setStateText(QStringLiteral("待人工复核"), summary_.value(QStringLiteral("reviewReason")).toString());
            publishStatus(QStringLiteral("analysis_waiting_review"));
        } else {
            setStateText(QStringLiteral("已完成"), QStringLiteral("融合分析完成，可导出报告"));
            publishStatus(QStringLiteral("analysis_completed"));
        }
        break;
    default:
        break;
    }

    ++stageIndex_;
    emit runtimeChanged();
    emit analysisChanged();
    syncModel();
}

void HistoryDataFusionController::ensureTaskId() {
    if (taskId_.isEmpty()) {
        taskId_ = stableTaskId();
        emit taskChanged();
    }
}

void HistoryDataFusionController::resetAnalysisOutputs() {
    stages_ = {
        {QStringLiteral("任务创建"), QStringLiteral("待执行"), QStringLiteral("固化任务上下文"), 0},
        {QStringLiteral("数据接入"), QStringLiteral("待执行"), QStringLiteral("实时/数据库/文件记录接入"), 0},
        {QStringLiteral("标准化处理"), QStringLiteral("待执行"), QStringLiteral("统一字段集、单位和时间基准"), 0},
        {QStringLiteral("质量评估"), QStringLiteral("待执行"), QStringLiteral("完整性、一致性、时序稳定性评分"), 0},
        {QStringLiteral("融合分析"), QStringLiteral("待执行"), QStringLiteral("多批次、多模态、多条件综合分析"), 0},
        {QStringLiteral("结果汇总"), QStringLiteral("待执行"), QStringLiteral("结论、建议、图表数据与报告摘要"), 0}
    };
    batchRows_.clear();
    qualityRows_.clear();
    factorRows_.clear();
    conditionRows_.clear();
    issueRows_.clear();
    dataAccessRows_.clear();
    algorithmRows_.clear();
    modelRows_.clear();
    statisticsRows_.clear();
    storageRows_.clear();
    reportRows_.clear();
    environmentRows_.clear();
    catalogRows_.clear();
    detailRows_.clear();
    outboundRows_.clear();
    summary_.clear();
    rebuildStageRows();
    rebuildDataAccessRows();
    rebuildAlgorithmRows();
    rebuildModelRows();
    rebuildStatisticsRows();
    rebuildStorageRows();
    rebuildReportRows();
    rebuildEnvironmentRows(false);
    rebuildCatalogRows();
    rebuildDetailRows();
    rebuildOutboundRows();
}

void HistoryDataFusionController::rebuildStageRows() {
    QVariantList rows;
    for (const StageMetric& stage : stages_) {
        QVariantMap row;
        row.insert(QStringLiteral("name"), stage.name);
        row.insert(QStringLiteral("status"), stage.status);
        row.insert(QStringLiteral("output"), stage.output);
        row.insert(QStringLiteral("durationMs"), stage.durationMs);
        row.insert(QStringLiteral("durationText"), formatDuration(stage.durationMs));
        rows.push_back(row);
    }
    stageRows_ = rows;
}

void HistoryDataFusionController::setStageStatus(
    int index,
    const QString& status,
    const QString& output,
    qint64 durationMs) {
    if (index < 0 || index >= stages_.size()) {
        return;
    }
    stages_[index].status = status;
    stages_[index].output = output;
    stages_[index].durationMs = durationMs;
    rebuildStageRows();
}

void HistoryDataFusionController::setStateText(const QString& stateText, const QString& feedbackText) {
    taskStateText_ = stateText;
    feedback_ = feedbackText;
    emit runtimeChanged();
    syncModel();
}

void HistoryDataFusionController::setProgress(double value) {
    progress_ = std::max(0.0, std::min(1.0, value));
    emit runtimeChanged();
}

void HistoryDataFusionController::publishStatus(const QString& eventName) const {
    if (messageBus_ == nullptr) {
        return;
    }

    QVariantMap payload;
    payload.insert(QStringLiteral("event"), eventName);
    payload.insert(QStringLiteral("taskId"), taskId_);
    payload.insert(QStringLiteral("state"), taskStateText_);
    payload.insert(QStringLiteral("progress"), progress_);
    payload.insert(QStringLiteral("recordCount"), recordCount());
    payload.insert(QStringLiteral("batchCount"), batchCount());
    payload.insert(QStringLiteral("timestampMs"), QDateTime::currentMSecsSinceEpoch());
    messageBus_->publish(
        QStringLiteral("history_fusion/status"),
        QJsonDocument::fromVariant(payload).toJson(QJsonDocument::Compact));
}

void HistoryDataFusionController::syncModel() {
    if (dataModel_ == nullptr) {
        return;
    }
    dataModel_->setValue(QStringLiteral("task.context"), taskContext());
    dataModel_->setValue(QStringLiteral("task.state"), taskStateText_);
    dataModel_->setValue(QStringLiteral("task.feedback"), feedback_);
    dataModel_->setValue(QStringLiteral("task.progress"), progress_);
    dataModel_->setValue(QStringLiteral("dataset.importPath"), importPath_);
    dataModel_->setValue(QStringLiteral("dataset.recordCount"), recordCount());
    dataModel_->setValue(QStringLiteral("dataset.batchCount"), batchCount());
    dataModel_->setValue(QStringLiteral("analysis.stages"), stageRows_);
    dataModel_->setValue(QStringLiteral("analysis.batches"), batchRows_);
    dataModel_->setValue(QStringLiteral("analysis.quality"), qualityRows_);
    dataModel_->setValue(QStringLiteral("analysis.factors"), factorRows_);
    dataModel_->setValue(QStringLiteral("analysis.conditions"), conditionRows_);
    dataModel_->setValue(QStringLiteral("analysis.issues"), issueRows_);
    dataModel_->setValue(QStringLiteral("analysis.dataAccess"), dataAccessRows_);
    dataModel_->setValue(QStringLiteral("analysis.algorithms"), algorithmRows_);
    dataModel_->setValue(QStringLiteral("analysis.models"), modelRows_);
    dataModel_->setValue(QStringLiteral("analysis.statistics"), statisticsRows_);
    dataModel_->setValue(QStringLiteral("analysis.storage"), storageRows_);
    dataModel_->setValue(QStringLiteral("analysis.audit"), auditRows_);
    dataModel_->setValue(QStringLiteral("analysis.reports"), reportRows_);
    dataModel_->setValue(QStringLiteral("analysis.environment"), environmentRows_);
    dataModel_->setValue(QStringLiteral("analysis.catalog"), catalogRows_);
    dataModel_->setValue(QStringLiteral("analysis.details"), detailRows_);
    dataModel_->setValue(QStringLiteral("analysis.outbound"), outboundRows_);
    dataModel_->setValue(QStringLiteral("analysis.summary"), summary_);
    dataModel_->setValue(QStringLiteral("storage.root"), storageRoot());
}

bool HistoryDataFusionController::parseCsv(const QString& text, QVector<TrialRecord>* records, QStringList* errors) const {
    if (records == nullptr || errors == nullptr) {
        return false;
    }

    QString normalizedText = text;
    normalizedText.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    normalizedText.replace(QChar('\r'), QChar('\n'));
    const QStringList lines = normalizedText.split(QChar('\n'), Qt::SkipEmptyParts);
    if (lines.size() < 2) {
        errors->push_back(QStringLiteral("CSV 至少需要表头和一行数据"));
        return false;
    }

    const QStringList headers = splitCsvLine(lines.first());
    QStringList normalizedHeaders;
    for (const QString& header : headers) {
        normalizedHeaders.push_back(normalizeHeader(header));
    }

    for (int rowIndex = 1; rowIndex < lines.size(); ++rowIndex) {
        const QString line = lines.at(rowIndex).trimmed();
        if (line.isEmpty()) {
            continue;
        }

        const QStringList cells = splitCsvLine(line);
        QVariantMap row;
        for (int column = 0; column < normalizedHeaders.size() && column < cells.size(); ++column) {
            if (!normalizedHeaders.at(column).isEmpty()) {
                row.insert(normalizedHeaders.at(column), cells.at(column).trimmed());
            }
        }
        records->push_back(recordFromMap(row, rowIndex));
    }

    return !records->isEmpty();
}

bool HistoryDataFusionController::parseJson(
    const QByteArray& data,
    QVector<TrialRecord>* records,
    QStringList* errors) const {
    if (records == nullptr || errors == nullptr) {
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        errors->push_back(QStringLiteral("JSON 解析失败：%1").arg(parseError.errorString()));
        return false;
    }

    QJsonArray array;
    if (doc.isArray()) {
        array = doc.array();
    } else if (doc.isObject()) {
        const QJsonObject root = doc.object();
        const QStringList candidates = {
            QStringLiteral("records"),
            QStringLiteral("data"),
            QStringLiteral("items")
        };
        for (const QString& key : candidates) {
            if (root.value(key).isArray()) {
                array = root.value(key).toArray();
                break;
            }
        }
    }

    if (array.isEmpty()) {
        errors->push_back(QStringLiteral("JSON 中未找到 records/data/items 记录数组"));
        return false;
    }

    int index = 0;
    for (const QJsonValue& value : array) {
        if (!value.isObject()) {
            continue;
        }
        QVariantMap row;
        const QJsonObject object = value.toObject();
        for (auto it = object.begin(); it != object.end(); ++it) {
            row.insert(normalizeHeader(it.key()), it.value().toVariant());
        }
        records->push_back(recordFromMap(row, ++index));
    }

    return !records->isEmpty();
}

HistoryDataFusionController::TrialRecord HistoryDataFusionController::recordFromMap(
    const QVariantMap& raw,
    int index) const {
    auto valueOf = [&raw](const QStringList& aliases) -> QVariant {
        for (const QString& alias : aliases) {
            const QString key = normalizeHeader(alias);
            if (raw.contains(key)) {
                return raw.value(key);
            }
        }
        return QVariant();
    };
    auto stringOf = [&valueOf](const QStringList& aliases) -> QString {
        return valueOf(aliases).toString().trimmed();
    };

    TrialRecord record;
    record.recordId = firstNonEmpty(
        stringOf({QStringLiteral("记录编号"), QStringLiteral("record_id"), QStringLiteral("id")}),
        QStringLiteral("REC-%1").arg(index, 5, 10, QLatin1Char('0')));
    record.batchNo = stringOf({QStringLiteral("批次编号"), QStringLiteral("批次号"), QStringLiteral("batch_no"), QStringLiteral("batch"), QStringLiteral("batch_id")});
    record.targetType = stringOf({QStringLiteral("目标类型"), QStringLiteral("target_type"), QStringLiteral("target"), QStringLiteral("target_class")});
    record.location = stringOf({QStringLiteral("地点"), QStringLiteral("试验地点"), QStringLiteral("location"), QStringLiteral("site")});
    record.resultLabel = stringOf({QStringLiteral("试验结果"), QStringLiteral("结果标签"), QStringLiteral("result"), QStringLiteral("result_label"), QStringLiteral("result_code")});
    record.modality = stringOf({QStringLiteral("模态"), QStringLiteral("通道"), QStringLiteral("modality"), QStringLiteral("channel")});
    record.sourceType = stringOf({QStringLiteral("来源"), QStringLiteral("数据来源"), QStringLiteral("source"), QStringLiteral("source_type")});
    record.weather = stringOf({QStringLiteral("天气"), QStringLiteral("weather"), QStringLiteral("weather_condition")});
    record.eventTime = parseDateTimeValue(valueOf({QStringLiteral("事件时间"), QStringLiteral("时间"), QStringLiteral("event_time"), QStringLiteral("time"), QStringLiteral("timestamp")}));

    bool ok = false;
    record.distance = parseNumber(valueOf({QStringLiteral("距离"), QStringLiteral("distance"), QStringLiteral("distance_m")}), &ok);
    record.hasDistance = ok;
    record.height = parseNumber(valueOf({QStringLiteral("高度"), QStringLiteral("height"), QStringLiteral("height_m")}), &ok);
    record.hasHeight = ok;
    record.measurement = parseNumber(valueOf({QStringLiteral("测量值"), QStringLiteral("探测值"), QStringLiteral("measurement"), QStringLiteral("metric"), QStringLiteral("score")}), &ok);
    record.hasMeasurement = ok;
    record.confidence = parseNumber(valueOf({QStringLiteral("置信度"), QStringLiteral("confidence"), QStringLiteral("confidence_score")}), &ok);
    record.hasConfidence = ok;
    if (record.hasConfidence && record.confidence > 1.0 && record.confidence <= 100.0) {
        record.confidence /= 100.0;
    }
    record.visibility = parseNumber(valueOf({QStringLiteral("能见度"), QStringLiteral("visibility"), QStringLiteral("visibility_km")}), &ok);
    record.hasVisibility = ok;

    record.batchNo = firstNonEmpty(record.batchNo, QStringLiteral("未分批"));
    record.targetType = firstNonEmpty(record.targetType, QStringLiteral("未知目标"));
    record.location = firstNonEmpty(record.location, QStringLiteral("未知地点"));
    record.modality = firstNonEmpty(record.modality, QStringLiteral("未知模态"));
    record.sourceType = firstNonEmpty(record.sourceType, dataSourceType_);
    record.weather = firstNonEmpty(record.weather, QStringLiteral("未记录"));

    const QString resultLower = record.resultLabel.trimmed().toLower();
    if (resultLower.contains(QStringLiteral("成功"))
        || resultLower.contains(QStringLiteral("命中"))
        || resultLower.contains(QStringLiteral("通过"))
        || resultLower.contains(QStringLiteral("success"))
        || resultLower.contains(QStringLiteral("pass"))
        || resultLower.contains(QStringLiteral("hit"))
        || resultLower == QStringLiteral("1")) {
        record.resultKnown = true;
        record.resultSuccess = true;
    } else if (resultLower.contains(QStringLiteral("失败"))
               || resultLower.contains(QStringLiteral("未命中"))
               || resultLower.contains(QStringLiteral("fail"))
               || resultLower.contains(QStringLiteral("miss"))
               || resultLower == QStringLiteral("0")) {
        record.resultKnown = true;
        record.resultSuccess = false;
    }

    if (record.batchNo == QStringLiteral("未分批")) {
        record.issues.push_back(QStringLiteral("批次编号缺失"));
    }
    if (!record.eventTime.isValid()) {
        record.issues.push_back(QStringLiteral("事件时间缺失或格式异常"));
    }
    if (!record.hasDistance) {
        record.issues.push_back(QStringLiteral("距离缺失"));
    }
    if (!record.hasHeight) {
        record.issues.push_back(QStringLiteral("高度缺失"));
    }
    if (!record.hasMeasurement) {
        record.issues.push_back(QStringLiteral("测量值缺失"));
    }
    if (!record.hasConfidence) {
        record.issues.push_back(QStringLiteral("置信度缺失"));
    }
    if (!record.hasVisibility) {
        record.issues.push_back(QStringLiteral("能见度缺失"));
    }
    if (!record.resultKnown) {
        record.issues.push_back(QStringLiteral("试验结果无法识别"));
    }
    if (record.hasDistance && (record.distance < 0.0 || record.distance > 5000.0)) {
        record.issues.push_back(QStringLiteral("距离超出常规范围"));
    }
    if (record.hasHeight && (record.height < 0.0 || record.height > 800.0)) {
        record.issues.push_back(QStringLiteral("高度超出常规范围"));
    }
    if (record.hasConfidence && (record.confidence < 0.0 || record.confidence > 1.0)) {
        record.issues.push_back(QStringLiteral("置信度超出范围"));
    }

    return record;
}

bool HistoryDataFusionController::applyFilters(const TrialRecord& record) const {
    if (!filterLocation_.isEmpty() && !record.location.contains(filterLocation_, Qt::CaseInsensitive)) {
        return false;
    }
    if (!filterTargetType_.isEmpty() && !record.targetType.contains(filterTargetType_, Qt::CaseInsensitive)) {
        return false;
    }
    if (!filterResult_.isEmpty() && !record.resultLabel.contains(filterResult_, Qt::CaseInsensitive)) {
        return false;
    }
    return true;
}

void HistoryDataFusionController::normalizeRecords() {
    standardRecords_.clear();
    standardRecords_.reserve(filteredRecords_.size());
    for (TrialRecord record : filteredRecords_) {
        record.batchNo = firstNonEmpty(record.batchNo, QStringLiteral("未分批"));
        record.targetType = firstNonEmpty(record.targetType, QStringLiteral("未知目标"));
        record.location = firstNonEmpty(record.location, QStringLiteral("未知地点"));
        record.modality = firstNonEmpty(record.modality, QStringLiteral("未知模态"));
        record.sourceType = firstNonEmpty(record.sourceType, dataSourceType_);

        if (record.hasDistance && record.distance < 0.0) {
            record.issues.push_back(QStringLiteral("距离为负值，已保留原值并标记"));
        }
        if (record.hasMeasurement && (record.measurement < -kEpsilon || record.measurement > 10.0)) {
            record.issues.push_back(QStringLiteral("测量值疑似异常"));
        }
        standardRecords_.push_back(record);
    }

    std::sort(standardRecords_.begin(), standardRecords_.end(), [](const TrialRecord& lhs, const TrialRecord& rhs) {
        if (lhs.batchNo != rhs.batchNo) {
            return lhs.batchNo < rhs.batchNo;
        }
        return lhs.eventTime < rhs.eventTime;
    });
}

void HistoryDataFusionController::evaluateQuality() {
    issueRows_.clear();
    if (standardRecords_.isEmpty()) {
        return;
    }

    int missingFieldCount = 0;
    int issueCount = 0;
    int invalidTimeCount = 0;
    int knownSourceCount = 0;
    int usableCount = 0;
    int manualCount = 0;
    double totalQuality = 0.0;

    QMap<QString, QDateTime> lastTimeByBatch;
    int reverseTimeCount = 0;

    for (TrialRecord& record : standardRecords_) {
        double score = 100.0;
        const int missingForRecord =
            (record.hasDistance ? 0 : 1)
            + (record.hasHeight ? 0 : 1)
            + (record.hasMeasurement ? 0 : 1)
            + (record.hasConfidence ? 0 : 1)
            + (record.hasVisibility ? 0 : 1)
            + (record.resultKnown ? 0 : 1);
        missingFieldCount += missingForRecord;
        score -= missingForRecord * 7.0;

        if (!record.eventTime.isValid()) {
            ++invalidTimeCount;
            score -= 8.0;
        } else if (lastTimeByBatch.contains(record.batchNo)
                   && record.eventTime < lastTimeByBatch.value(record.batchNo)) {
            ++reverseTimeCount;
            score -= 8.0;
            record.issues.push_back(QStringLiteral("同批次时间倒序"));
        }
        if (record.eventTime.isValid()) {
            lastTimeByBatch.insert(record.batchNo, record.eventTime);
        }

        if (!record.sourceType.trimmed().isEmpty()) {
            ++knownSourceCount;
        } else {
            score -= 5.0;
        }
        if (record.sourceType.contains(QStringLiteral("人工")) || record.sourceType.contains(QStringLiteral("manual"), Qt::CaseInsensitive)) {
            ++manualCount;
            score -= 4.0;
        }
        if (record.hasMeasurement && record.resultKnown) {
            ++usableCount;
        }

        issueCount += record.issues.size();
        score -= record.issues.size() * 3.0;
        record.qualityScore = clampScore(score);
        totalQuality += record.qualityScore;

        for (const QString& issue : record.issues) {
            issueRows_.push_back(makeIssueRow(record, issue));
        }
    }

    const int count = standardRecords_.size();
    const double completenessScore = clampScore(100.0 - (missingFieldCount * 100.0 / std::max(1, count * 6)));
    const double consistencyScore = clampScore(100.0 - issueCount * 3.0);
    const double temporalScore = clampScore(100.0 - invalidTimeCount * 8.0 - reverseTimeCount * 10.0);
    const double sourceScore = knownSourceCount * 100.0 / count;
    const double usabilityScore = usableCount * 100.0 / count;
    const double manualScore = clampScore(100.0 - manualCount * 5.0);
    const double totalScore = totalQuality / count;

    auto appendQualityRow = [this](const QString& dimension, double score, const QString& detail) {
        QVariantMap row;
        row.insert(QStringLiteral("dimension"), dimension);
        row.insert(QStringLiteral("score"), score);
        row.insert(QStringLiteral("scoreText"), QString::number(score, 'f', 1));
        row.insert(QStringLiteral("grade"), qualityGrade(score));
        row.insert(QStringLiteral("detail"), detail);
        qualityRows_.push_back(row);
    };

    appendQualityRow(QStringLiteral("完整性"), completenessScore, QStringLiteral("缺失字段 %1 项").arg(missingFieldCount));
    appendQualityRow(QStringLiteral("一致性"), consistencyScore, QStringLiteral("异常/冲突 %1 项").arg(issueCount));
    appendQualityRow(QStringLiteral("时序稳定性"), temporalScore, QStringLiteral("时间异常 %1 项，倒序 %2 项").arg(invalidTimeCount).arg(reverseTimeCount));
    appendQualityRow(QStringLiteral("来源可信度"), sourceScore, QStringLiteral("来源完整 %1/%2").arg(knownSourceCount).arg(count));
    appendQualityRow(QStringLiteral("可用性"), usabilityScore, QStringLiteral("可参与分析 %1/%2").arg(usableCount).arg(count));
    appendQualityRow(QStringLiteral("人工修正影响"), manualScore, QStringLiteral("人工补录/修正 %1 条").arg(manualCount));

    summary_.insert(QStringLiteral("qualityScore"), totalScore);
    summary_.insert(QStringLiteral("qualityGrade"), qualityGrade(totalScore));
    summary_.insert(QStringLiteral("issueCount"), issueRows_.size());
}

void HistoryDataFusionController::runFusionAnalysis() {
    batchRows_.clear();
    factorRows_.clear();
    conditionRows_.clear();
    if (standardRecords_.isEmpty()) {
        return;
    }

    QMap<QString, QVector<int>> batchIndexes;
    int successCount = 0;
    int knownResultCount = 0;
    double qualitySum = 0.0;
    for (int i = 0; i < standardRecords_.size(); ++i) {
        const TrialRecord& record = standardRecords_.at(i);
        batchIndexes[record.batchNo].push_back(i);
        qualitySum += record.qualityScore;
        if (record.resultKnown) {
            ++knownResultCount;
            if (record.resultSuccess) {
                ++successCount;
            }
        }
    }
    const double overallSuccessRate = knownResultCount == 0 ? 0.0 : successCount / static_cast<double>(knownResultCount);
    const double overallQuality = qualitySum / standardRecords_.size();

    for (auto it = batchIndexes.cbegin(); it != batchIndexes.cend(); ++it) {
        double distanceSum = 0.0;
        double heightSum = 0.0;
        double measurementSum = 0.0;
        double batchQualitySum = 0.0;
        int distanceCount = 0;
        int heightCount = 0;
        int measurementCount = 0;
        int batchKnown = 0;
        int batchSuccess = 0;
        QSet<QString> modalities;
        QSet<QString> targets;
        for (int index : it.value()) {
            const TrialRecord& record = standardRecords_.at(index);
            modalities.insert(record.modality);
            targets.insert(record.targetType);
            batchQualitySum += record.qualityScore;
            if (record.hasDistance) {
                distanceSum += record.distance;
                ++distanceCount;
            }
            if (record.hasHeight) {
                heightSum += record.height;
                ++heightCount;
            }
            if (record.hasMeasurement) {
                measurementSum += record.measurement;
                ++measurementCount;
            }
            if (record.resultKnown) {
                ++batchKnown;
                if (record.resultSuccess) {
                    ++batchSuccess;
                }
            }
        }

        QStringList modalityList = modalities.values();
        QStringList targetList = targets.values();
        modalityList.sort();
        targetList.sort();

        QVariantMap row;
        row.insert(QStringLiteral("batchNo"), it.key());
        row.insert(QStringLiteral("sampleCount"), it.value().size());
        row.insert(QStringLiteral("targetTypes"), targetList.join(QStringLiteral("/")));
        row.insert(QStringLiteral("modalities"), modalityList.join(QStringLiteral("/")));
        row.insert(QStringLiteral("avgDistance"), averageOrZero(distanceSum, distanceCount));
        row.insert(QStringLiteral("avgHeight"), averageOrZero(heightSum, heightCount));
        row.insert(QStringLiteral("avgMeasurement"), averageOrZero(measurementSum, measurementCount));
        row.insert(QStringLiteral("successRate"), batchKnown == 0 ? 0.0 : batchSuccess / static_cast<double>(batchKnown));
        row.insert(QStringLiteral("qualityScore"), averageOrZero(batchQualitySum, it.value().size()));
        batchRows_.push_back(row);
    }

    struct FactorCandidate {
        QVariantMap row;
        double impact = 0.0;
    };
    QVector<FactorCandidate> candidates;
    auto addFactorRows = [&](const QString& factorName, const QMap<QString, QVector<int>>& groups) {
        for (auto it = groups.cbegin(); it != groups.cend(); ++it) {
            if (it.value().size() < 2) {
                continue;
            }
            int known = 0;
            int success = 0;
            double measurementSum = 0.0;
            int measurementCount = 0;
            double qSum = 0.0;
            for (int index : it.value()) {
                const TrialRecord& record = standardRecords_.at(index);
                qSum += record.qualityScore;
                if (record.resultKnown) {
                    ++known;
                    if (record.resultSuccess) {
                        ++success;
                    }
                }
                if (record.hasMeasurement) {
                    measurementSum += record.measurement;
                    ++measurementCount;
                }
            }
            const double rate = known == 0 ? 0.0 : success / static_cast<double>(known);
            const double impact = std::abs(rate - overallSuccessRate) * 100.0 + std::log(1.0 + it.value().size()) * 2.0;
            QVariantMap row;
            row.insert(QStringLiteral("factor"), factorName);
            row.insert(QStringLiteral("level"), it.key());
            row.insert(QStringLiteral("sampleCount"), it.value().size());
            row.insert(QStringLiteral("successRate"), rate);
            row.insert(QStringLiteral("avgMeasurement"), averageOrZero(measurementSum, measurementCount));
            row.insert(QStringLiteral("qualityScore"), averageOrZero(qSum, it.value().size()));
            row.insert(QStringLiteral("impactScore"), impact);
            row.insert(QStringLiteral("suggestion"), rate >= overallSuccessRate ? QStringLiteral("优先保持") : QStringLiteral("重点复核"));
            candidates.push_back({row, impact});
        }
    };

    QMap<QString, QVector<int>> byLocation;
    QMap<QString, QVector<int>> byTarget;
    QMap<QString, QVector<int>> byDistance;
    QMap<QString, QVector<int>> byHeight;
    QMap<QString, QVector<int>> byWeather;
    QMap<QString, QVector<int>> byModality;
    QMap<QString, QVector<int>> byCondition;
    for (int i = 0; i < standardRecords_.size(); ++i) {
        const TrialRecord& record = standardRecords_.at(i);
        byLocation[record.location].push_back(i);
        byTarget[record.targetType].push_back(i);
        byDistance[distanceBand(record.distance, record.hasDistance)].push_back(i);
        byHeight[heightBand(record.height, record.hasHeight)].push_back(i);
        byWeather[record.weather].push_back(i);
        byModality[record.modality].push_back(i);
        const QString condition = QStringLiteral("%1 / %2 / %3 / %4")
            .arg(record.location, record.targetType, distanceBand(record.distance, record.hasDistance), heightBand(record.height, record.hasHeight));
        byCondition[condition].push_back(i);
    }

    addFactorRows(QStringLiteral("地点"), byLocation);
    addFactorRows(QStringLiteral("目标类型"), byTarget);
    addFactorRows(QStringLiteral("距离分层"), byDistance);
    addFactorRows(QStringLiteral("高度分层"), byHeight);
    addFactorRows(QStringLiteral("天气"), byWeather);
    addFactorRows(QStringLiteral("模态"), byModality);

    std::sort(candidates.begin(), candidates.end(), [](const FactorCandidate& lhs, const FactorCandidate& rhs) {
        return lhs.impact > rhs.impact;
    });
    double topImpactTotal = 0.0;
    for (int i = 0; i < std::min(8, candidates.size()); ++i) {
        topImpactTotal += candidates.at(i).impact;
    }
    for (int i = 0; i < std::min(8, candidates.size()); ++i) {
        QVariantMap row = candidates.at(i).row;
        row.insert(QStringLiteral("contribution"), topImpactTotal <= kEpsilon ? 0.0 : candidates.at(i).impact * 100.0 / topImpactTotal);
        factorRows_.push_back(row);
    }

    QVector<QVariantMap> conditionCandidates;
    for (auto it = byCondition.cbegin(); it != byCondition.cend(); ++it) {
        if (it.value().size() < 2) {
            continue;
        }
        int known = 0;
        int success = 0;
        double confidenceSum = 0.0;
        int confidenceCount = 0;
        double qSum = 0.0;
        for (int index : it.value()) {
            const TrialRecord& record = standardRecords_.at(index);
            qSum += record.qualityScore;
            if (record.resultKnown) {
                ++known;
                if (record.resultSuccess) {
                    ++success;
                }
            }
            if (record.hasConfidence) {
                confidenceSum += record.confidence;
                ++confidenceCount;
            }
        }
        const double rate = known == 0 ? 0.0 : success / static_cast<double>(known);
        const double quality = averageOrZero(qSum, it.value().size()) / 100.0;
        const double confidence = averageOrZero(confidenceSum, confidenceCount);
        const double score = (rate * 0.45 + quality * 0.35 + confidence * 0.20) * 100.0;
        QVariantMap row;
        row.insert(QStringLiteral("condition"), it.key());
        row.insert(QStringLiteral("sampleCount"), it.value().size());
        row.insert(QStringLiteral("successRate"), rate);
        row.insert(QStringLiteral("qualityScore"), quality * 100.0);
        row.insert(QStringLiteral("confidence"), confidence);
        row.insert(QStringLiteral("fusionScore"), score);
        row.insert(QStringLiteral("conclusion"), score >= 75.0 ? QStringLiteral("推荐工况") : QStringLiteral("可比对样本"));
        conditionCandidates.push_back(row);
    }
    std::sort(conditionCandidates.begin(), conditionCandidates.end(), [](const QVariantMap& lhs, const QVariantMap& rhs) {
        return lhs.value(QStringLiteral("fusionScore")).toDouble() > rhs.value(QStringLiteral("fusionScore")).toDouble();
    });
    for (int i = 0; i < std::min(8, conditionCandidates.size()); ++i) {
        conditionRows_.push_back(conditionCandidates.at(i));
    }

    summary_.insert(QStringLiteral("usedRecordCount"), standardRecords_.size());
    summary_.insert(QStringLiteral("usedBatchCount"), batchIndexes.size());
    summary_.insert(QStringLiteral("overallSuccessRate"), overallSuccessRate);
    summary_.insert(QStringLiteral("overallQualityScore"), overallQuality);
    if (!factorRows_.isEmpty()) {
        const QVariantMap row = factorRows_.first().toMap();
        summary_.insert(QStringLiteral("keyFactor"), QStringLiteral("%1:%2").arg(row.value(QStringLiteral("factor")).toString(), row.value(QStringLiteral("level")).toString()));
    }
    if (!conditionRows_.isEmpty()) {
        summary_.insert(QStringLiteral("bestCondition"), conditionRows_.first().toMap().value(QStringLiteral("condition")).toString());
    }
}

void HistoryDataFusionController::summarizeResults() {
    const int usedBatchCount = summary_.value(QStringLiteral("usedBatchCount")).toInt();
    const double qualityScore = summary_.value(QStringLiteral("qualityScore")).toDouble();
    const int issueCount = summary_.value(QStringLiteral("issueCount")).toInt();
    const bool batchReview = usedBatchCount < minBatchRequired_;
    const bool qualityReview = qualityScore < 70.0;
    const bool issueReview = issueCount > standardRecords_.size();
    const bool reviewRequired = batchReview || qualityReview || issueReview;

    QStringList reviewReasons;
    if (batchReview) {
        reviewReasons.push_back(QStringLiteral("有效批次数低于 %1 批次要求").arg(minBatchRequired_));
    }
    if (qualityReview) {
        reviewReasons.push_back(QStringLiteral("质量评分低于自动归档阈值"));
    }
    if (issueReview) {
        reviewReasons.push_back(QStringLiteral("异常标记数量较多"));
    }

    const QString keyFactor = summary_.value(QStringLiteral("keyFactor"), QStringLiteral("暂无")).toString();
    const QString bestCondition = summary_.value(QStringLiteral("bestCondition"), QStringLiteral("暂无")).toString();
    const QString conclusion = reviewRequired
        ? QStringLiteral("分析完成，但建议人工复核后归档")
        : QStringLiteral("分析完成，结果满足自动归档条件");
    const QString suggestion = QStringLiteral("重点关注 %1；推荐工况：%2").arg(keyFactor, bestCondition);

    summary_.insert(QStringLiteral("reviewRequired"), reviewRequired);
    summary_.insert(QStringLiteral("reviewReason"), reviewReasons.isEmpty() ? QStringLiteral("无需人工复核") : reviewReasons.join(QStringLiteral("；")));
    summary_.insert(QStringLiteral("conclusion"), conclusion);
    summary_.insert(QStringLiteral("suggestion"), suggestion);
    summary_.insert(QStringLiteral("finishedAt"), QDateTime::currentDateTime().toString(Qt::ISODate));
    summary_.insert(QStringLiteral("resultVersion"), QStringLiteral("RES-2026.04"));
    summary_.insert(QStringLiteral("confidenceLevel"), qualityScore >= 85.0 ? QStringLiteral("高") : (qualityScore >= 70.0 ? QStringLiteral("中") : QStringLiteral("待复核")));

    rebuildDataAccessRows();
    rebuildAlgorithmRows();
    rebuildModelRows();
    rebuildStatisticsRows();
    rebuildStorageRows();
    rebuildReportRows();
    rebuildEnvironmentRows(false);
    appendAuditRow(
        QStringLiteral("结果汇总"),
        taskId_,
        reviewRequired ? QStringLiteral("待复核") : QStringLiteral("完成"),
        QStringLiteral("生成结论、建议、统计口径和报告摘要"));
    rebuildStorageRows();
}

void HistoryDataFusionController::rebuildDataAccessRows() {
    dataAccessRows_.clear();

    auto appendRow = [this](const QString& name,
                            const QString& input,
                            const QString& output,
                            const QString& status,
                            const QString& control) {
        QVariantMap row;
        row.insert(QStringLiteral("name"), name);
        row.insert(QStringLiteral("input"), input);
        row.insert(QStringLiteral("output"), output);
        row.insert(QStringLiteral("status"), status);
        row.insert(QStringLiteral("control"), control);
        dataAccessRows_.push_back(row);
    };

    appendRow(
        QStringLiteral("加载数据集"),
        QStringLiteral("数据源标识、筛选条件、分页参数"),
        QStringLiteral("数据集列表、记录总数、页信息"),
        rawRecords_.isEmpty() ? QStringLiteral("待接入") : QStringLiteral("可用"),
        QStringLiteral("按任务和批次过滤"));
    appendRow(
        QStringLiteral("查询明细记录"),
        QStringLiteral("批次标识、字段条件、排序条件、分页参数"),
        QStringLiteral("目标记录列表、质量标记、来源索引"),
        standardRecords_.isEmpty() ? QStringLiteral("待标准化") : QStringLiteral("可用"),
        QStringLiteral("分页与排序，支撑问题定位"));
    appendRow(
        QStringLiteral("模糊查询记录"),
        QStringLiteral("关键字、模糊字段、时间范围"),
        QStringLiteral("匹配记录列表、匹配摘要"),
        rawRecords_.isEmpty() ? QStringLiteral("待接入") : QStringLiteral("可用"),
        QStringLiteral("限制返回条数"));
    appendRow(
        QStringLiteral("调取图像/附件"),
        QStringLiteral("文件标识、关联批次、导出策略"),
        QStringLiteral("文件路径、缩略图路径或文件流"),
        QStringLiteral("接口预留"),
        QStringLiteral("校验文件存在性，后续接入附件服务"));
    appendRow(
        QStringLiteral("查询测量与状态数据"),
        QStringLiteral("批次标识、时间窗口、数据类型"),
        QStringLiteral("测量数据集、状态数据集"),
        rawRecords_.isEmpty() ? QStringLiteral("待接入") : QStringLiteral("可用"),
        QStringLiteral("按窗口读取，支撑趋势分析"));
    appendRow(
        QStringLiteral("查询分析结果"),
        QStringLiteral("任务标识、结果类型、版本号"),
        QStringLiteral("统计结果、融合结果、辨识结果"),
        summary_.contains(QStringLiteral("conclusion")) ? QStringLiteral("可用") : QStringLiteral("待分析"),
        QStringLiteral("默认返回最新有效版本"));
    appendRow(
        QStringLiteral("导出数据集"),
        QStringLiteral("导出范围、导出格式、压缩策略"),
        QStringLiteral("导出文件、导出回执、校验摘要"),
        summary_.contains(QStringLiteral("conclusion")) ? QStringLiteral("待权限校验") : QStringLiteral("待分析"),
        QStringLiteral("导出前校验权限、范围和敏感字段"));
}

void HistoryDataFusionController::rebuildAlgorithmRows() {
    algorithmRows_.clear();

    auto appendRow = [this](const QString& name,
                            const QString& input,
                            const QString& output,
                            const QString& version,
                            const QString& note) {
        QVariantMap row;
        row.insert(QStringLiteral("name"), name);
        row.insert(QStringLiteral("input"), input);
        row.insert(QStringLiteral("output"), output);
        row.insert(QStringLiteral("version"), version);
        row.insert(QStringLiteral("status"), QStringLiteral("接口预留"));
        row.insert(QStringLiteral("note"), note);
        algorithmRows_.push_back(row);
    };

    appendRow(
        QStringLiteral("候选算法登记接口"),
        QStringLiteral("算法标识、适用数据类型、参数模板"),
        QStringLiteral("候选算法清单、版本摘要"),
        QStringLiteral("ALG-CONTRACT-2026.04"),
        QStringLiteral("仅保留接口，算法组件后续独立接入"));
    appendRow(
        QStringLiteral("算法匹配评分接口"),
        QStringLiteral("数据质量、样本规模、工况维度、目标类型"),
        QStringLiteral("匹配分、推荐原因、约束说明"),
        QStringLiteral("ALG-SCORE-2026.04"),
        QStringLiteral("当前不执行自动优选，仅固化评分输入输出"));
    appendRow(
        QStringLiteral("算法回退策略接口"),
        QStringLiteral("失败原因、上次可用版本、阈值策略"),
        QStringLiteral("回退目标、阻断原因、人工确认项"),
        QStringLiteral("ALG-FALLBACK-2026.04"),
        QStringLiteral("回退链路由算法服务实现"));
    appendRow(
        QStringLiteral("算法审计接口"),
        QStringLiteral("任务标识、算法版本、切换原因、操作者"),
        QStringLiteral("审计记录、验证结论、影响范围"),
        QStringLiteral("ALG-AUDIT-2026.04"),
        QStringLiteral("对算法切换留痕，不参与当前计算"));
}

void HistoryDataFusionController::rebuildModelRows() {
    modelRows_.clear();

    auto appendRow = [this](const QString& name,
                            const QString& input,
                            const QString& output,
                            const QString& confidence,
                            const QString& note) {
        QVariantMap row;
        row.insert(QStringLiteral("name"), name);
        row.insert(QStringLiteral("input"), input);
        row.insert(QStringLiteral("output"), output);
        row.insert(QStringLiteral("confidence"), confidence);
        row.insert(QStringLiteral("status"), QStringLiteral("接口预留"));
        row.insert(QStringLiteral("note"), note);
        modelRows_.push_back(row);
    };

    appendRow(
        QStringLiteral("特征抽取输入接口"),
        QStringLiteral("标准记录、距离/高度/天气/模态分层"),
        QStringLiteral("特征向量、字段版本、样本引用"),
        QStringLiteral("-"),
        QStringLiteral("保留模型输入边界，当前不生成真实特征向量"));
    appendRow(
        QStringLiteral("模型辨识调用接口"),
        QStringLiteral("特征向量、模型版本、阈值配置"),
        QStringLiteral("辨识标签、置信度、解释摘要"),
        QStringLiteral("-"),
        QStringLiteral("模型服务后续接入"));
    appendRow(
        QStringLiteral("影响因子解释接口"),
        QStringLiteral("模型输出、融合因子、统计口径"),
        QStringLiteral("因子显著性、复核建议"),
        summary_.value(QStringLiteral("confidenceLevel"), QStringLiteral("-")).toString(),
        QStringLiteral("当前复用融合因子作为界面占位说明"));
    appendRow(
        QStringLiteral("人工复核接口"),
        QStringLiteral("待复核原因、样本引用、模型输出"),
        QStringLiteral("复核结论、干预记录、结果版本"),
        summary_.value(QStringLiteral("reviewRequired")).toBool() ? QStringLiteral("需复核") : QStringLiteral("-"),
        QStringLiteral("用于后续闭环人工确认"));
}

void HistoryDataFusionController::rebuildStatisticsRows() {
    statisticsRows_.clear();

    const QVector<TrialRecord>* records = &standardRecords_;
    QString basis = QStringLiteral("标准记录");
    if (records->isEmpty() && !filteredRecords_.isEmpty()) {
        records = &filteredRecords_;
        basis = QStringLiteral("筛选记录");
    }
    if (records->isEmpty() && !rawRecords_.isEmpty()) {
        records = &rawRecords_;
        basis = QStringLiteral("原始记录");
    }
    if (records->isEmpty()) {
        return;
    }

    QSet<QString> batches;
    QSet<QString> targets;
    int known = 0;
    int success = 0;
    int missingFieldCount = 0;
    int issueCount = 0;
    int usableCount = 0;
    double qualitySum = 0.0;
    int qualityCount = 0;

    for (const TrialRecord& record : *records) {
        batches.insert(record.batchNo);
        targets.insert(record.targetType);
        if (record.resultKnown) {
            ++known;
            if (record.resultSuccess) {
                ++success;
            }
        }
        missingFieldCount += (record.hasDistance ? 0 : 1)
            + (record.hasHeight ? 0 : 1)
            + (record.hasMeasurement ? 0 : 1)
            + (record.hasConfidence ? 0 : 1)
            + (record.hasVisibility ? 0 : 1)
            + (record.resultKnown ? 0 : 1);
        issueCount += record.issues.size();
        if (record.hasMeasurement && record.resultKnown) {
            ++usableCount;
        }
        if (record.qualityScore > kEpsilon) {
            qualitySum += record.qualityScore;
            ++qualityCount;
        }
    }

    qint64 stageTotalMs = 0;
    qint64 stageMaxMs = 0;
    for (const StageMetric& stage : stages_) {
        stageTotalMs += stage.durationMs;
        stageMaxMs = std::max(stageMaxMs, stage.durationMs);
    }

    auto appendRow = [this, &basis](const QString& category,
                                    const QString& metric,
                                    const QString& valueText,
                                    const QString& dimension,
                                    const QString& chart,
                                    const QString& note) {
        QVariantMap row;
        row.insert(QStringLiteral("category"), category);
        row.insert(QStringLiteral("metric"), metric);
        row.insert(QStringLiteral("value"), valueText);
        row.insert(QStringLiteral("dimension"), dimension);
        row.insert(QStringLiteral("chart"), chart);
        row.insert(QStringLiteral("basis"), basis);
        row.insert(QStringLiteral("note"), note);
        statisticsRows_.push_back(row);
    };

    const int count = records->size();
    const double successRate = known <= 0 ? 0.0 : success / static_cast<double>(known);
    const double usableRate = usableCount / static_cast<double>(std::max(1, count));
    const double passRate = 1.0 - issueCount / static_cast<double>(std::max(1, count + issueCount));
    const double missRate = known <= 0 ? 0.0 : (known - success) / static_cast<double>(known);
    const double avgQuality = qualityCount <= 0
        ? summary_.value(QStringLiteral("qualityScore"), 0.0).toDouble()
        : qualitySum / qualityCount;

    appendRow(QStringLiteral("数据质量统计"), QStringLiteral("缺失字段数量"), QString::number(missingFieldCount), QStringLiteral("批次/来源"), QStringLiteral("柱状图"), QStringLiteral("按标准记录字段完整性汇总"));
    appendRow(QStringLiteral("数据质量统计"), QStringLiteral("一致性校验通过率"), percentText(clampScore(passRate * 100.0) / 100.0), QStringLiteral("批次/时间"), QStringLiteral("表格"), QStringLiteral("异常越少通过率越高"));
    appendRow(QStringLiteral("处置结果统计"), QStringLiteral("命中数/样本数"), QStringLiteral("%1/%2").arg(success).arg(count), QStringLiteral("批次/目标类型"), QStringLiteral("柱状图"), QStringLiteral("按结果标签直接聚合"));
    appendRow(QStringLiteral("样本规模统计"), QStringLiteral("批次数/目标类型数"), QStringLiteral("%1/%2").arg(batches.size()).arg(targets.size()), QStringLiteral("任务/批次"), QStringLiteral("堆叠图"), QStringLiteral("用于判断样本充分性"));
    appendRow(QStringLiteral("成功性指标"), QStringLiteral("成功率"), percentText(successRate), QStringLiteral("距离/高度区间"), QStringLiteral("对比图"), QStringLiteral("依据命中与目标数计算"));
    appendRow(QStringLiteral("风险性指标"), QStringLiteral("漏判率"), percentText(missRate), QStringLiteral("地点/目标类型"), QStringLiteral("雷达图"), QStringLiteral("失败样本占已知结果比例"));
    appendRow(QStringLiteral("时延指标"), QStringLiteral("阶段耗时"), QStringLiteral("%1 / 最大 %2").arg(formatDuration(stageTotalMs), formatDuration(stageMaxMs)), QStringLiteral("任务/阶段"), QStringLiteral("折线图"), QStringLiteral("由状态机阶段耗时汇总"));
    appendRow(QStringLiteral("融合衍生指标"), QStringLiteral("质量分/有效样本率"), QStringLiteral("%1 / %2").arg(numberText(avgQuality, 1), percentText(usableRate)), QStringLiteral("条件模板/批次组合"), QStringLiteral("排行表"), QStringLiteral("供工况排序和报告引用"));
}

void HistoryDataFusionController::rebuildStorageRows() {
    storageRows_.clear();

    auto appendRow = [this](const QString& table,
                            const QString& fields,
                            const QString& index,
                            const QString& state,
                            const QString& policy) {
        QVariantMap row;
        row.insert(QStringLiteral("table"), table);
        row.insert(QStringLiteral("fields"), fields);
        row.insert(QStringLiteral("index"), index);
        row.insert(QStringLiteral("state"), state);
        row.insert(QStringLiteral("policy"), policy);
        storageRows_.push_back(row);
    };

    appendRow(
        QStringLiteral("任务表"),
        QStringLiteral("task_id、task_status、owner、start_time、end_time"),
        QStringLiteral("task_id 主键 + 状态/时间组合索引"),
        taskId_.isEmpty() ? QStringLiteral("待创建") : QStringLiteral("已登记"),
        QStringLiteral("状态变更更新，切换必须留痕"));
    appendRow(
        QStringLiteral("试验批次表"),
        QStringLiteral("batch_id、task_id、location、distance、height、target_type、result_label"),
        QStringLiteral("批次号 + 时间范围组合索引"),
        rawRecords_.isEmpty() ? QStringLiteral("待写入") : QStringLiteral("可写入 %1 批").arg(batchCount()),
        QStringLiteral("支持追加更新，供融合与统计使用"));
    appendRow(
        QStringLiteral("目标记录表"),
        QStringLiteral("record_id、batch_id、measurement、position、result_label、quality_flag"),
        QStringLiteral("批次 + 目标联合索引"),
        standardRecords_.isEmpty() ? QStringLiteral("待标准化") : QStringLiteral("可写入 %1 条").arg(standardRecords_.size()),
        QStringLiteral("原则上仅追加，保留版本"));
    appendRow(
        QStringLiteral("测量数据表"),
        QStringLiteral("timestamp、measurement_type、value、unit、source"),
        QStringLiteral("时间索引 + 类型索引"),
        rawRecords_.isEmpty() ? QStringLiteral("待接入") : QStringLiteral("可批量写入"),
        QStringLiteral("按批次批量写入，供时序分析"));
    appendRow(
        QStringLiteral("文件元数据表"),
        QStringLiteral("file_id、path、type、checksum、batch_id"),
        QStringLiteral("文件标识索引 + 关联索引"),
        importPath_.isEmpty() ? QStringLiteral("待登记") : QStringLiteral("已登记来源"),
        QStringLiteral("不直接存储大文件内容"));
    appendRow(
        QStringLiteral("分析结果表"),
        QStringLiteral("analysis_result_id、task_id、result_type、version、generated_at、confidence"),
        QStringLiteral("任务标识 + 结果类型/版本索引"),
        summary_.contains(QStringLiteral("conclusion")) ? QStringLiteral("可写入") : QStringLiteral("待分析"),
        QStringLiteral("版本化写入，禁止覆盖历史版本"));
    appendRow(
        QStringLiteral("图表索引表"),
        QStringLiteral("chart_id、result_id、chart_type、path、generated_at"),
        QStringLiteral("result_id 索引"),
        summary_.contains(QStringLiteral("conclusion")) ? QStringLiteral("可生成") : QStringLiteral("待结果"),
        QStringLiteral("图表可重新生成，报告引用需锁定版本"));
    appendRow(
        QStringLiteral("审计日志表"),
        QStringLiteral("operator、action、object_id、time、status、error_code"),
        QStringLiteral("操作时间 + 对象索引"),
        auditRows_.isEmpty() ? QStringLiteral("待写入") : QStringLiteral("已记录 %1 条").arg(auditRows_.size()),
        QStringLiteral("只追加不修改，支撑验收追踪"));
    appendRow(
        QStringLiteral("本地归档目录"),
        QStringLiteral("manifest、raw_records、standard_records、summary、audit"),
        QStringLiteral("任务编号目录索引"),
        catalogRows_.isEmpty() ? QStringLiteral("待扫描") : QStringLiteral("已发现 %1 项").arg(catalogRows_.size()),
        QStringLiteral("作为真实数据库接入前的可运行仓储"));
    appendRow(
        QStringLiteral("待发队列表"),
        QStringLiteral("receipt_code、task_id、target、status、retry_count、payload"),
        QStringLiteral("回执编号索引 + 状态索引"),
        outboundRows_.isEmpty() ? QStringLiteral("无待发") : QStringLiteral("待发 %1 项").arg(outboundRows_.size()),
        QStringLiteral("网络不可用时保留本地待发记录"));
}

void HistoryDataFusionController::rebuildReportRows() {
    reportRows_.clear();

    auto appendRow = [this](const QString& section,
                            const QString& source,
                            const QString& state,
                            const QString& templateVersion,
                            const QString& note) {
        QVariantMap row;
        row.insert(QStringLiteral("section"), section);
        row.insert(QStringLiteral("source"), source);
        row.insert(QStringLiteral("state"), state);
        row.insert(QStringLiteral("template"), templateVersion);
        row.insert(QStringLiteral("note"), note);
        reportRows_.push_back(row);
    };

    const bool hasResult = summary_.contains(QStringLiteral("conclusion"));
    appendRow(QStringLiteral("封面与任务摘要"), QStringLiteral("任务对象、责任人、数据范围"), taskId_.isEmpty() ? QStringLiteral("待创建") : QStringLiteral("已就绪"), QStringLiteral("RPT-2026.04"), QStringLiteral("包含任务编号、版本与导出时间"));
    appendRow(QStringLiteral("质量评估"), QStringLiteral("qualityRows"), qualityRows_.isEmpty() ? QStringLiteral("待评估") : QStringLiteral("已生成"), QStringLiteral("RPT-2026.04"), QStringLiteral("报告引用当前质量口径"));
    appendRow(QStringLiteral("批次对比"), QStringLiteral("batchRows"), batchRows_.isEmpty() ? QStringLiteral("待分析") : QStringLiteral("已生成"), QStringLiteral("RPT-2026.04"), QStringLiteral("支持多批次横向比较"));
    appendRow(QStringLiteral("关键因子与工况排序"), QStringLiteral("factorRows/conditionRows"), factorRows_.isEmpty() ? QStringLiteral("待分析") : QStringLiteral("已生成"), QStringLiteral("RPT-2026.04"), QStringLiteral("用于结论建议"));
    appendRow(QStringLiteral("算法与模型接口说明"), QStringLiteral("algorithmRows/modelRows"), QStringLiteral("接口预留"), QStringLiteral("RPT-2026.04"), QStringLiteral("算法不在当前版本实现，仅记录接口边界"));
    appendRow(QStringLiteral("统计口径"), QStringLiteral("statisticsRows"), statisticsRows_.isEmpty() ? QStringLiteral("待统计") : QStringLiteral("已生成"), QStringLiteral("STAT-2026.04"), QStringLiteral("报告生成优先读取固化统计结果"));
    appendRow(QStringLiteral("导出回执"), QStringLiteral("reportPath"), summary_.contains(QStringLiteral("reportPath")) ? QStringLiteral("已导出") : (hasResult ? QStringLiteral("可导出") : QStringLiteral("待结果")), QStringLiteral("RPT-2026.04"), summary_.value(QStringLiteral("reportPath"), QStringLiteral("未生成")).toString());
    appendRow(QStringLiteral("发布回执"), QStringLiteral("receiptCode"), summary_.contains(QStringLiteral("receiptCode")) ? QStringLiteral("已登记") : (hasResult ? QStringLiteral("可发布") : QStringLiteral("待结果")), QStringLiteral("PUB-2026.04"), summary_.value(QStringLiteral("receiptCode"), QStringLiteral("等待发布")).toString());
}

void HistoryDataFusionController::rebuildEnvironmentRows(bool checked) {
    environmentRows_.clear();

    const QString documentsDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    const bool documentWritable = !documentsDir.isEmpty() && QFileInfo(documentsDir).isWritable();

    auto appendRow = [this](const QString& item,
                            const QString& requirement,
                            const QString& config,
                            const QString& status,
                            const QString& handling) {
        QVariantMap row;
        row.insert(QStringLiteral("item"), item);
        row.insert(QStringLiteral("requirement"), requirement);
        row.insert(QStringLiteral("config"), config);
        row.insert(QStringLiteral("status"), status);
        row.insert(QStringLiteral("handling"), handling);
        environmentRows_.push_back(row);
    };

    appendRow(
        QStringLiteral("服务器部署"),
        QStringLiteral("单机部署、服务分层、后台任务执行"),
        QDir::currentPath(),
        checked ? QStringLiteral("通过") : QStringLiteral("待自检"),
        QStringLiteral("安装目录、运行目录和缓存目录配置化"));
    appendRow(
        QStringLiteral("数据库运行环境"),
        QStringLiteral("统一封装数据库访问接口和连接参数"),
        QStringLiteral("data_model_center"),
        checked ? QStringLiteral("接口可用") : QStringLiteral("待自检"),
        QStringLiteral("真实数据库连接由数据中心/部署配置接入"));
    appendRow(
        QStringLiteral("文件系统规则"),
        QStringLiteral("导入、导出、归档目录可配置"),
        storageRoot(),
        checked ? (QDir().mkpath(storageRoot()) ? QStringLiteral("通过") : QStringLiteral("需处理")) : QStringLiteral("待自检"),
        documentWritable ? QStringLiteral("文档目录和本地仓储可用于导出归档") : QStringLiteral("需检查目录权限"));
    appendRow(
        QStringLiteral("网络与消息链路"),
        QStringLiteral("超时、重试、回执通道可配置"),
        QStringLiteral("history_fusion/status"),
        checked ? (messageBus_ == nullptr ? QStringLiteral("降级") : QStringLiteral("通过")) : QStringLiteral("待自检"),
        messageBus_ == nullptr ? QStringLiteral("无消息总线时保留本地分析") : QStringLiteral("可发布状态事件"));
    appendRow(
        QStringLiteral("日志与审计"),
        QStringLiteral("日志级别、滚动策略和审计策略配置化"),
        QStringLiteral("auditRows / org_common_data_center"),
        checked ? QStringLiteral("通过") : QStringLiteral("待自检"),
        QStringLiteral("关键操作写入审计行，后续可接入持久化日志"));
    appendRow(
        QStringLiteral("模板和规则版本"),
        QStringLiteral("字段映射、质量阈值、统计口径、报告模板需固化版本"),
        QStringLiteral("FM/QR/STAT/RPT-2026.04"),
        checked ? QStringLiteral("通过") : QStringLiteral("待自检"),
        QStringLiteral("任务启动时写入上下文，便于回放"));
}

void HistoryDataFusionController::rebuildCatalogRows() {
    catalogRows_.clear();

    QDir root(storageRoot());
    if (!root.exists()) {
        root.mkpath(QStringLiteral("."));
    }
    const QFileInfoList dirs = root.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Time);
    for (const QFileInfo& info : dirs) {
        const QString manifestPath = QDir(info.absoluteFilePath()).filePath(QStringLiteral("manifest.json"));
        QString error;
        const QVariantMap manifest = readJsonFile(manifestPath, &error).toMap();
        if (!error.isEmpty() || manifest.isEmpty()) {
            continue;
        }
        QVariantMap row;
        row.insert(QStringLiteral("taskId"), manifest.value(QStringLiteral("taskId")).toString());
        row.insert(QStringLiteral("taskName"), manifest.value(QStringLiteral("taskName")).toString());
        row.insert(QStringLiteral("state"), manifest.value(QStringLiteral("state")).toString());
        row.insert(QStringLiteral("recordCount"), manifest.value(QStringLiteral("recordCount")).toInt());
        row.insert(QStringLiteral("batchCount"), manifest.value(QStringLiteral("batchCount")).toInt());
        row.insert(QStringLiteral("updatedAt"), manifest.value(QStringLiteral("updatedAt")).toString());
        row.insert(QStringLiteral("storagePath"), QDir::toNativeSeparators(info.absoluteFilePath()));
        row.insert(QStringLiteral("reason"), manifest.value(QStringLiteral("reason")).toString());
        catalogRows_.push_back(row);
    }
}

void HistoryDataFusionController::rebuildDetailRows() {
    detailRows_.clear();

    const QVector<TrialRecord>* records = &standardRecords_;
    QString layer = QStringLiteral("标准层");
    if (records->isEmpty()) {
        records = filteredRecords_.isEmpty() ? &rawRecords_ : &filteredRecords_;
        layer = filteredRecords_.isEmpty() ? QStringLiteral("原始层") : QStringLiteral("筛选层");
    }

    const int maxRows = std::min(80, records->size());
    for (int i = 0; i < maxRows; ++i) {
        const TrialRecord& record = records->at(i);
        QVariantMap row;
        row.insert(QStringLiteral("layer"), layer);
        row.insert(QStringLiteral("recordId"), record.recordId);
        row.insert(QStringLiteral("batchNo"), record.batchNo);
        row.insert(QStringLiteral("targetType"), record.targetType);
        row.insert(QStringLiteral("location"), record.location);
        row.insert(QStringLiteral("resultLabel"), record.resultLabel);
        row.insert(QStringLiteral("eventTime"), record.eventTime.isValid() ? record.eventTime.toString(QStringLiteral("yyyy-MM-dd hh:mm:ss")) : QStringLiteral("-"));
        row.insert(QStringLiteral("distance"), record.hasDistance ? numberText(record.distance, 1) : QStringLiteral("-"));
        row.insert(QStringLiteral("height"), record.hasHeight ? numberText(record.height, 1) : QStringLiteral("-"));
        row.insert(QStringLiteral("measurement"), record.hasMeasurement ? numberText(record.measurement, 3) : QStringLiteral("-"));
        row.insert(QStringLiteral("qualityScore"), numberText(record.qualityScore, 1));
        row.insert(QStringLiteral("issueCount"), record.issues.size());
        detailRows_.push_back(row);
    }
}

void HistoryDataFusionController::rebuildOutboundRows() {
    outboundRows_.clear();
    QDir dir(outboxDir());
    if (!dir.exists()) {
        dir.mkpath(QStringLiteral("."));
    }
    const QFileInfoList files = dir.entryInfoList(QStringList() << QStringLiteral("*.json"), QDir::Files, QDir::Time);
    for (const QFileInfo& info : files) {
        QString error;
        const QVariantMap payload = readJsonFile(info.absoluteFilePath(), &error).toMap();
        if (!error.isEmpty() || payload.isEmpty()) {
            continue;
        }
        QVariantMap row;
        row.insert(QStringLiteral("receiptCode"), payload.value(QStringLiteral("receiptCode")).toString());
        row.insert(QStringLiteral("taskId"), payload.value(QStringLiteral("taskId")).toString());
        row.insert(QStringLiteral("target"), payload.value(QStringLiteral("target")).toString());
        row.insert(QStringLiteral("status"), payload.value(QStringLiteral("status")).toString());
        row.insert(QStringLiteral("retryCount"), payload.value(QStringLiteral("retryCount")).toInt());
        row.insert(QStringLiteral("createdAt"), payload.value(QStringLiteral("createdAt")).toString());
        row.insert(QStringLiteral("lastRetryAt"), payload.value(QStringLiteral("lastRetryAt")).toString());
        row.insert(QStringLiteral("path"), QDir::toNativeSeparators(info.absoluteFilePath()));
        outboundRows_.push_back(row);
    }
}

void HistoryDataFusionController::appendAuditRow(
    const QString& action,
    const QString& objectId,
    const QString& status,
    const QString& detail) {
    QVariantMap row;
    row.insert(QStringLiteral("time"), QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss")));
    row.insert(QStringLiteral("operator"), owner_);
    row.insert(QStringLiteral("action"), action);
    row.insert(QStringLiteral("objectId"), firstNonEmpty(objectId, QStringLiteral("-")));
    row.insert(QStringLiteral("status"), status);
    row.insert(QStringLiteral("errorCode"), status == QStringLiteral("失败") ? QStringLiteral("HFA-ERR") : QStringLiteral("-"));
    row.insert(QStringLiteral("detail"), detail);
    auditRows_.prepend(row);
    while (auditRows_.size() > 80) {
        auditRows_.removeLast();
    }
}

bool HistoryDataFusionController::persistTaskSnapshot(const QString& reason) const {
    if (taskId_.trimmed().isEmpty()) {
        return false;
    }
    QString error;
    const QString dirPath = taskStorageDir(taskId_);
    QDir dir(dirPath);
    if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
        return false;
    }

    QVariantList rawRows;
    for (const TrialRecord& record : rawRecords_) {
        rawRows.push_back(recordToVariant(record));
    }
    QVariantList standardRows;
    for (const TrialRecord& record : standardRecords_) {
        standardRows.push_back(recordToVariant(record));
    }

    QVariantMap manifest = storageManifest();
    manifest.insert(QStringLiteral("reason"), reason);
    const bool ok = writeJsonFile(dir.filePath(QStringLiteral("manifest.json")), manifest, &error)
        && writeJsonFile(dir.filePath(QStringLiteral("raw_records.json")), rawRows, &error)
        && writeJsonFile(dir.filePath(QStringLiteral("standard_records.json")), standardRows, &error)
        && writeJsonFile(dir.filePath(QStringLiteral("summary.json")), summary_, &error)
        && writeJsonFile(dir.filePath(QStringLiteral("audit.json")), auditRows_, &error)
        && writeJsonFile(dir.filePath(QStringLiteral("stages.json")), stageRows_, &error)
        && writeJsonFile(dir.filePath(QStringLiteral("quality.json")), qualityRows_, &error)
        && writeJsonFile(dir.filePath(QStringLiteral("statistics.json")), statisticsRows_, &error);
    return ok;
}

bool HistoryDataFusionController::writeJsonFile(const QString& path, const QVariant& payload, QString* error) const {
    QFileInfo info(path);
    QDir dir = info.dir();
    if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
        if (error != nullptr) {
            *error = QStringLiteral("无法创建目录 %1").arg(dir.absolutePath());
        }
        return false;
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (error != nullptr) {
            *error = file.errorString();
        }
        return false;
    }
    file.write(QJsonDocument::fromVariant(payload).toJson(QJsonDocument::Indented));
    file.close();
    if (error != nullptr) {
        error->clear();
    }
    return true;
}

QVariant HistoryDataFusionController::readJsonFile(const QString& path, QString* error) const {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error != nullptr) {
            *error = file.errorString();
        }
        return QVariant();
    }
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        if (error != nullptr) {
            *error = parseError.errorString();
        }
        return QVariant();
    }
    if (error != nullptr) {
        error->clear();
    }
    return doc.toVariant();
}

QString HistoryDataFusionController::taskStorageDir(const QString& taskId) const {
    return QDir(storageRoot()).filePath(taskId.trimmed());
}

QString HistoryDataFusionController::outboxDir() const {
    return QDir(storageRoot()).filePath(QStringLiteral("_outbox"));
}

QVariantMap HistoryDataFusionController::storageManifest() const {
    QVariantMap manifest;
    manifest.insert(QStringLiteral("taskId"), taskId_);
    manifest.insert(QStringLiteral("taskName"), taskName_);
    manifest.insert(QStringLiteral("owner"), owner_);
    manifest.insert(QStringLiteral("state"), taskStateText_);
    manifest.insert(QStringLiteral("feedback"), feedback_);
    manifest.insert(QStringLiteral("dataSourceType"), dataSourceType_);
    manifest.insert(QStringLiteral("importPath"), importPath_);
    manifest.insert(QStringLiteral("recordCount"), recordCount());
    manifest.insert(QStringLiteral("batchCount"), batchCount());
    manifest.insert(QStringLiteral("standardRecordCount"), standardRecords_.size());
    manifest.insert(QStringLiteral("resultVersion"), summary_.value(QStringLiteral("resultVersion")).toString());
    manifest.insert(QStringLiteral("archiveState"), summary_.value(QStringLiteral("archiveState")).toString());
    manifest.insert(QStringLiteral("receiptCode"), summary_.value(QStringLiteral("receiptCode")).toString());
    manifest.insert(QStringLiteral("createdAt"), taskId_.mid(4, 14));
    manifest.insert(QStringLiteral("updatedAt"), QDateTime::currentDateTime().toString(Qt::ISODate));
    manifest.insert(QStringLiteral("storageRoot"), storageRoot());
    return manifest;
}

QVariantMap HistoryDataFusionController::recordToVariant(const TrialRecord& record) const {
    QVariantMap value;
    value.insert(QStringLiteral("recordId"), record.recordId);
    value.insert(QStringLiteral("batchNo"), record.batchNo);
    value.insert(QStringLiteral("targetType"), record.targetType);
    value.insert(QStringLiteral("location"), record.location);
    value.insert(QStringLiteral("resultLabel"), record.resultLabel);
    value.insert(QStringLiteral("modality"), record.modality);
    value.insert(QStringLiteral("sourceType"), record.sourceType);
    value.insert(QStringLiteral("weather"), record.weather);
    value.insert(QStringLiteral("eventTime"), record.eventTime.isValid() ? record.eventTime.toString(Qt::ISODate) : QString());
    value.insert(QStringLiteral("distance"), record.distance);
    value.insert(QStringLiteral("height"), record.height);
    value.insert(QStringLiteral("measurement"), record.measurement);
    value.insert(QStringLiteral("confidence"), record.confidence);
    value.insert(QStringLiteral("visibility"), record.visibility);
    value.insert(QStringLiteral("hasDistance"), record.hasDistance);
    value.insert(QStringLiteral("hasHeight"), record.hasHeight);
    value.insert(QStringLiteral("hasMeasurement"), record.hasMeasurement);
    value.insert(QStringLiteral("hasConfidence"), record.hasConfidence);
    value.insert(QStringLiteral("hasVisibility"), record.hasVisibility);
    value.insert(QStringLiteral("resultSuccess"), record.resultSuccess);
    value.insert(QStringLiteral("resultKnown"), record.resultKnown);
    value.insert(QStringLiteral("qualityScore"), record.qualityScore);
    value.insert(QStringLiteral("issues"), record.issues);
    return value;
}

HistoryDataFusionController::TrialRecord HistoryDataFusionController::recordFromStoredVariant(
    const QVariantMap& value,
    int index) const {
    TrialRecord record;
    record.recordId = firstNonEmpty(value.value(QStringLiteral("recordId")).toString(), QStringLiteral("REC-%1").arg(index, 5, 10, QLatin1Char('0')));
    record.batchNo = firstNonEmpty(value.value(QStringLiteral("batchNo")).toString(), QStringLiteral("未分批"));
    record.targetType = firstNonEmpty(value.value(QStringLiteral("targetType")).toString(), QStringLiteral("未知目标"));
    record.location = firstNonEmpty(value.value(QStringLiteral("location")).toString(), QStringLiteral("未知地点"));
    record.resultLabel = value.value(QStringLiteral("resultLabel")).toString();
    record.modality = firstNonEmpty(value.value(QStringLiteral("modality")).toString(), QStringLiteral("未知模态"));
    record.sourceType = firstNonEmpty(value.value(QStringLiteral("sourceType")).toString(), dataSourceType_);
    record.weather = firstNonEmpty(value.value(QStringLiteral("weather")).toString(), QStringLiteral("未记录"));
    record.eventTime = parseDateTimeValue(value.value(QStringLiteral("eventTime")));
    record.distance = value.value(QStringLiteral("distance")).toDouble();
    record.height = value.value(QStringLiteral("height")).toDouble();
    record.measurement = value.value(QStringLiteral("measurement")).toDouble();
    record.confidence = value.value(QStringLiteral("confidence")).toDouble();
    record.visibility = value.value(QStringLiteral("visibility")).toDouble();
    record.hasDistance = value.value(QStringLiteral("hasDistance")).toBool();
    record.hasHeight = value.value(QStringLiteral("hasHeight")).toBool();
    record.hasMeasurement = value.value(QStringLiteral("hasMeasurement")).toBool();
    record.hasConfidence = value.value(QStringLiteral("hasConfidence")).toBool();
    record.hasVisibility = value.value(QStringLiteral("hasVisibility")).toBool();
    record.resultSuccess = value.value(QStringLiteral("resultSuccess")).toBool();
    record.resultKnown = value.value(QStringLiteral("resultKnown")).toBool();
    record.qualityScore = value.value(QStringLiteral("qualityScore")).toDouble();
    const QVariantList issueValues = value.value(QStringLiteral("issues")).toList();
    for (const QVariant& issue : issueValues) {
        record.issues.push_back(issue.toString());
    }
    return record;
}

QVariantMap HistoryDataFusionController::makeIssueRow(const TrialRecord& record, const QString& issue) const {
    QVariantMap row;
    row.insert(QStringLiteral("recordId"), record.recordId);
    row.insert(QStringLiteral("batchNo"), record.batchNo);
    row.insert(QStringLiteral("targetType"), record.targetType);
    row.insert(QStringLiteral("location"), record.location);
    row.insert(QStringLiteral("issue"), issue);
    row.insert(QStringLiteral("source"), record.sourceType);
    return row;
}

QString HistoryDataFusionController::makeReportMarkdown() const {
    QString report;
    QTextStream out(&report);
    out << "# 历史数据融合分析报告\n\n";
    out << "- 任务编号: " << taskId_ << "\n";
    out << "- 任务名称: " << taskName_ << "\n";
    out << "- 责任人: " << owner_ << "\n";
    out << "- 数据来源: " << importPath_ << "\n";
    out << "- 分析状态: " << taskStateText_ << "\n";
    out << "- 质量评分: " << numberText(summary_.value(QStringLiteral("qualityScore")).toDouble(), 1)
        << " (" << summary_.value(QStringLiteral("qualityGrade")).toString() << ")\n";
    out << "- 整体成功率: " << percentText(summary_.value(QStringLiteral("overallSuccessRate")).toDouble()) << "\n";
    out << "- 关键因子: " << summary_.value(QStringLiteral("keyFactor"), QStringLiteral("暂无")).toString() << "\n";
    out << "- 推荐工况: " << summary_.value(QStringLiteral("bestCondition"), QStringLiteral("暂无")).toString() << "\n\n";

    out << "## 结论建议\n\n";
    out << summary_.value(QStringLiteral("conclusion")).toString() << "\n\n";
    out << summary_.value(QStringLiteral("suggestion")).toString() << "\n\n";
    out << "复核提示: " << summary_.value(QStringLiteral("reviewReason")).toString() << "\n\n";

    out << "## 批次摘要\n\n";
    out << "| 批次 | 样本数 | 模态 | 成功率 | 质量分 |\n";
    out << "| --- | ---: | --- | ---: | ---: |\n";
    for (const QVariant& item : batchRows_) {
        const QVariantMap row = item.toMap();
        out << "| " << row.value(QStringLiteral("batchNo")).toString()
            << " | " << row.value(QStringLiteral("sampleCount")).toInt()
            << " | " << row.value(QStringLiteral("modalities")).toString()
            << " | " << percentText(row.value(QStringLiteral("successRate")).toDouble())
            << " | " << numberText(row.value(QStringLiteral("qualityScore")).toDouble(), 1)
            << " |\n";
    }

    out << "\n## 关键因子\n\n";
    out << "| 因子 | 分层 | 样本数 | 贡献度 | 建议 |\n";
    out << "| --- | --- | ---: | ---: | --- |\n";
    for (const QVariant& item : factorRows_) {
        const QVariantMap row = item.toMap();
        out << "| " << row.value(QStringLiteral("factor")).toString()
            << " | " << row.value(QStringLiteral("level")).toString()
            << " | " << row.value(QStringLiteral("sampleCount")).toInt()
            << " | " << numberText(row.value(QStringLiteral("contribution")).toDouble(), 1) << "%"
            << " | " << row.value(QStringLiteral("suggestion")).toString()
            << " |\n";
    }

    return report;
}

QString HistoryDataFusionController::normalizeHeader(const QString& header) {
    QString value = header.trimmed().toLower();
    value.remove(QRegularExpression(QStringLiteral("[\\s_\\-（）()\\[\\]]+")));
    return value;
}

QStringList HistoryDataFusionController::splitCsvLine(const QString& line) {
    if (line.contains(QChar('\t')) && !line.contains(QChar(','))) {
        return line.split(QChar('\t'));
    }

    QStringList cells;
    QString current;
    bool inQuote = false;
    for (int i = 0; i < line.size(); ++i) {
        const QChar ch = line.at(i);
        if (ch == QChar('"')) {
            if (inQuote && i + 1 < line.size() && line.at(i + 1) == QChar('"')) {
                current.append(QChar('"'));
                ++i;
            } else {
                inQuote = !inQuote;
            }
            continue;
        }
        if (ch == QChar(',') && !inQuote) {
            cells.push_back(current);
            current.clear();
            continue;
        }
        current.append(ch);
    }
    cells.push_back(current);
    return cells;
}

double HistoryDataFusionController::parseNumber(const QVariant& value, bool* ok) {
    if (ok != nullptr) {
        *ok = false;
    }
    if (!value.isValid() || value.isNull()) {
        return 0.0;
    }

    if (value.canConvert<double>()) {
        bool converted = false;
        const double number = value.toDouble(&converted);
        if (converted) {
            if (ok != nullptr) {
                *ok = true;
            }
            return number;
        }
    }

    QString text = value.toString().trimmed();
    if (text.isEmpty()) {
        return 0.0;
    }
    const bool hasPercent = text.contains(QChar('%'));
    text.remove(QRegularExpression(QStringLiteral("[,%米mMkmKM公里千米]+")));
    text.remove(QChar('%'));
    bool converted = false;
    double number = text.toDouble(&converted);
    if (!converted) {
        return 0.0;
    }
    if (hasPercent) {
        number /= 100.0;
    }
    if (ok != nullptr) {
        *ok = true;
    }
    return number;
}

QDateTime HistoryDataFusionController::parseDateTimeValue(const QVariant& value) {
    if (!value.isValid() || value.isNull()) {
        return QDateTime();
    }
    if (value.canConvert<QDateTime>()) {
        const QDateTime dt = value.toDateTime();
        if (dt.isValid()) {
            return dt;
        }
    }
    const QString text = value.toString().trimmed();
    if (text.isEmpty()) {
        return QDateTime();
    }

    QDateTime isoDateTime = QDateTime::fromString(text, Qt::ISODate);
    if (isoDateTime.isValid()) {
        return isoDateTime;
    }

    const QStringList formats = {
        QStringLiteral("yyyy-MM-dd hh:mm:ss"),
        QStringLiteral("yyyy/M/d h:m:s"),
        QStringLiteral("yyyy/M/d h:m"),
        QStringLiteral("yyyy-MM-dd"),
        QStringLiteral("yyyyMMddhhmmss")
    };

    for (const QString& format : formats) {
        QDateTime dt;
        dt = QDateTime::fromString(text, format);
        if (dt.isValid()) {
            return dt;
        }
    }
    return QDateTime();
}

QString HistoryDataFusionController::distanceBand(double distance, bool valid) {
    if (!valid) {
        return QStringLiteral("距离缺失");
    }
    if (distance < 1000.0) {
        return QStringLiteral("近距(<1km)");
    }
    if (distance < 3000.0) {
        return QStringLiteral("中距(1-3km)");
    }
    return QStringLiteral("远距(>=3km)");
}

QString HistoryDataFusionController::heightBand(double height, bool valid) {
    if (!valid) {
        return QStringLiteral("高度缺失");
    }
    if (height < 50.0) {
        return QStringLiteral("低空(<50m)");
    }
    if (height < 200.0) {
        return QStringLiteral("中空(50-200m)");
    }
    return QStringLiteral("高空(>=200m)");
}

QString HistoryDataFusionController::qualityGrade(double score) {
    if (score >= 85.0) {
        return QStringLiteral("优");
    }
    if (score >= 70.0) {
        return QStringLiteral("良");
    }
    if (score >= 55.0) {
        return QStringLiteral("可用需复核");
    }
    return QStringLiteral("阻断复核");
}

QString HistoryDataFusionController::formatDuration(qint64 milliseconds) {
    if (milliseconds <= 0) {
        return QStringLiteral("-");
    }
    if (milliseconds < 1000) {
        return QStringLiteral("%1 ms").arg(milliseconds);
    }
    return QStringLiteral("%1 s").arg(QString::number(milliseconds / 1000.0, 'f', 2));
}
