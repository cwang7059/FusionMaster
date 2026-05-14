#include <plugin_manager.h>
#include <plugin_api/ctk_bridge.h>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QHash>
#include <QMetaObject>
#include <QRegularExpression>
#include <QSet>
#include <QUrl>
#include <QVariant>
#include <QObject>
#include <QDebug>

#include <exception>

#ifndef OSGI_CTK_AVAILABLE
#define OSGI_CTK_AVAILABLE 0
#endif

#ifndef OSGI_PLATFORM_TAG
#define OSGI_PLATFORM_TAG ""
#endif

#ifndef OSGI_PLATFORM_TAG_TOOLCHAIN
#define OSGI_PLATFORM_TAG_TOOLCHAIN ""
#endif
#if OSGI_CTK_AVAILABLE
#include "plugin_manager_bridge_service.h"
#include <ctkException.h>
#include <ctkPlugin.h>
#include <ctkPluginConstants.h>
#include <ctkPluginContext.h>
#include <ctkPluginFramework.h>
#include <ctkPluginFrameworkFactory.h>

#include <QSharedPointer>
#endif

#if OSGI_CTK_AVAILABLE
namespace {

QString formatCtkException(const ctkException& ex) {
    const QString msg = ex.message().trimmed();
    if (!msg.isEmpty()) {
        return msg;
    }

    const QString what = QString::fromUtf8(ex.what()).trimmed();
    if (!what.isEmpty()) {
        return what;
    }

    return QStringLiteral("Unknown CTK exception");
}

}  // namespace
#endif

namespace {

void pushMessageBusOwnerContext(IMessageBus* messageBus, const QString& owner) {
    auto* object = dynamic_cast<QObject*>(messageBus);
    if (object == nullptr) {
        return;
    }

    QMetaObject::invokeMethod(
        object,
        "pushOwnerContext",
        Qt::DirectConnection,
        Q_ARG(QString, owner));
}

void popMessageBusOwnerContext(IMessageBus* messageBus) {
    auto* object = dynamic_cast<QObject*>(messageBus);
    if (object == nullptr) {
        return;
    }

    QMetaObject::invokeMethod(
        object,
        "popOwnerContext",
        Qt::DirectConnection);
}

class ScopedMessageBusOwnerContext {
public:
    ScopedMessageBusOwnerContext(IMessageBus* messageBus, const QString& owner)
        : messageBus_(messageBus) {
        pushMessageBusOwnerContext(messageBus_, owner);
    }

    ~ScopedMessageBusOwnerContext() {
        popMessageBusOwnerContext(messageBus_);
    }

private:
    IMessageBus* messageBus_ = nullptr;
};

}  // namespace

struct PluginManager::CtkRuntimeHandle {
#if OSGI_CTK_AVAILABLE
    std::unique_ptr<ctkPluginFrameworkFactory> factory;
    QSharedPointer<ctkPluginFramework> framework;
    ctkPluginContext* context = nullptr;
    QHash<QString, QSharedPointer<ctkPlugin>> installedPlugins;
    std::unique_ptr<QObject> pluginManagerBridgeService;
    ctkServiceRegistration pluginManagerBridgeRegistration;
#endif
};

void PluginRuntime::injectPluginManager(IPlugin& plugin, IPluginManager* manager) {
    plugin.setPluginManager(manager);
}

void PluginRuntime::start(IPlugin& plugin) {
    plugin.start();
}

void PluginRuntime::stop(IPlugin& plugin) {
    plugin.stop();
}

PluginManager::PluginManager() = default;

PluginManager::~PluginManager() {
    stopPlugins();
}

void PluginManager::setMessageBus(IMessageBus* messageBus) {
    messageBus_ = messageBus;
}

void PluginManager::setLogger(ILogger* logger) {
    logger_ = logger;
}

void PluginManager::registerPluginFactory(const QString& pluginName, PluginFactory factory) {
    if (pluginName.trimmed().isEmpty() || !factory) {
        return;
    }

    std::lock_guard<std::mutex> lock(factoryMutex_);
    pluginFactories_[buildPluginKey(pluginName)] = std::move(factory);
}

IMessageBus* PluginManager::messageBus() {
    return messageBus_;
}

ILogger* PluginManager::logger() {
    return logger_;
}

bool PluginManager::isCtkAvailable() const {
#if OSGI_CTK_AVAILABLE
    return true;
#else
    return false;
#endif
}

QString PluginManager::runtimeBackendName() const {
    if (isCtkAvailable()) {
        return QStringLiteral("ctk_framework");
    }
    return QStringLiteral("static_factory");
}

QString PluginManager::runtimeBackendHint() const {
    if (isCtkAvailable()) {
        return QStringLiteral("CTK backend enabled: use shared library first, fallback to factory");
    }
    return QStringLiteral("CTK artifacts missing: using static factory backend");
}

bool PluginManager::loadProfile(const QString& profilePath, QString* errorMessage) {
    if (profilePath.isEmpty()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("profile path is empty");
        }
        return false;
    }

    QFile file(profilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Failed to open profile file: %1").arg(profilePath);
        }
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("profile JSON 解析失败: %1").arg(parseError.errorString());
        }
        return false;
    }
    if (!doc.isObject()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("profile JSON 根对象无效");
        }
        return false;
    }

    ProfileConfig next;
    const QJsonObject root = doc.object();
    next.name = root.value("name").toString(QStringLiteral("Unnamed Profile"));
    next.description = root.value("description").toString();

    const QJsonArray pluginArray = root.value("plugins").toArray();
    for (const QJsonValue& value : pluginArray) {
        if (!value.isString()) {
            continue;
        }
        const QString pluginName = value.toString().trimmed();
        if (!pluginName.isEmpty()) {
            next.plugins.push_back(pluginName);
        }
    }
    if (next.plugins.isEmpty()) {
        next.plugins.push_back("*");
    }

    {
        std::lock_guard<std::mutex> lock(profileMutex_);
        profileConfig_ = next;
    }

    return true;
}

PluginManager::ProfileConfig PluginManager::profileConfig() const {
    std::lock_guard<std::mutex> lock(profileMutex_);
    return profileConfig_;
}

bool PluginManager::shouldLoadPlugin(const QString& pluginName) const {
    std::lock_guard<std::mutex> lock(profileMutex_);
    return profileConfig_.loadAll() || profileConfig_.plugins.contains(pluginName);
}

QList<PluginManager::PluginDescriptor> PluginManager::discoverPlugins(
    const QStringList& pluginRoots,
    QStringList* warnings) const {
    QList<PluginDescriptor> discovered;

    for (const QString& rootPath : pluginRoots) {
        QDir rootDir(rootPath);
        if (!rootDir.exists()) {
            appendWarning(warnings, QStringLiteral("Plugin directory not found: %1").arg(rootPath));
            continue;
        }

        const QFileInfoList subDirs = rootDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
        for (const QFileInfo& dirInfo : subDirs) {
            const QString pluginDir = dirInfo.absoluteFilePath();
            const QString manifestPath = QDir(pluginDir).filePath(QStringLiteral("plugin.json"));
            if (!QFileInfo::exists(manifestPath)) {
                continue;
            }

            PluginDescriptor descriptor;
            QString parseError;
            if (!parsePluginManifest(manifestPath, pluginDir, &descriptor, &parseError)) {
                appendWarning(warnings, parseError);
                continue;
            }

            discovered.push_back(descriptor);
        }
    }

    return discovered;
}

QList<PluginManager::PluginDescriptor> PluginManager::filterPluginsByProfile(
    const QList<PluginDescriptor>& discovered,
    QStringList* skippedPlugins,
    QStringList* warnings) const {
    QList<PluginDescriptor> selected;
    if (discovered.isEmpty()) {
        return selected;
    }

    const ProfileConfig config = profileConfig();
    if (config.loadAll()) {
        return discovered;
    }

    QHash<QString, PluginDescriptor> descriptorByName;
    for (const PluginDescriptor& plugin : discovered) {
        descriptorByName.insert(plugin.name, plugin);
    }

    QSet<QString> requestedSet;
    QList<QString> pending;
    for (const QString& rawName : config.plugins) {
        const QString pluginName = rawName.trimmed();
        if (pluginName.isEmpty() || pluginName == QStringLiteral("*")) {
            continue;
        }
        if (requestedSet.contains(pluginName)) {
            continue;
        }
        requestedSet.insert(pluginName);
        pending.push_back(pluginName);
    }

    QSet<QString> selectedNames;
    QSet<QString> unknownWarned;
    QSet<QString> autoIncludedWarned;

    while (!pending.isEmpty()) {
        const QString pluginName = pending.takeFirst();
        if (selectedNames.contains(pluginName)) {
            continue;
        }

        const auto descriptorIt = descriptorByName.constFind(pluginName);
        if (descriptorIt == descriptorByName.constEnd()) {
            if (!unknownWarned.contains(pluginName)) {
                appendWarning(warnings, QStringLiteral("Plugin listed in profile not found: %1").arg(pluginName));
                unknownWarned.insert(pluginName);
            }
            continue;
        }

        selectedNames.insert(pluginName);

        const PluginDescriptor& descriptor = descriptorIt.value();
        for (const QString& dependency : descriptor.dependencies) {
            const QString dependencyName = normalizeDependencyName(dependency);
            if (dependencyName.isEmpty() || isFrameworkDependency(dependencyName)) {
                continue;
            }

            if (!descriptorByName.contains(dependencyName)) {
                appendWarning(warnings,
                              QStringLiteral("Plugin %1 missing dependency: %2")
                                  .arg(pluginName, dependencyName));
                continue;
            }

            if (selectedNames.contains(dependencyName) || pending.contains(dependencyName)) {
                continue;
            }

            pending.push_back(dependencyName);

            if (!requestedSet.contains(dependencyName)
                && !autoIncludedWarned.contains(dependencyName)) {
                appendWarning(warnings,
                              QStringLiteral("Auto-included dependency plugin: %1 (required by %2)")
                                  .arg(dependencyName, pluginName));
                autoIncludedWarned.insert(dependencyName);
            }
        }
    }

    for (const PluginDescriptor& plugin : discovered) {
        if (selectedNames.contains(plugin.name)) {
            selected.push_back(plugin);
        } else {
            appendWarning(skippedPlugins, plugin.name);
        }
    }

    return selected;
}

bool PluginManager::startPlugins(const QList<PluginDescriptor>& selected, QStringList* warnings) {
    shutdownCtkFramework();

    std::lock_guard<std::mutex> lock(runtimeMutex_);
    runtimeOrder_.clear();
    runtimeEntries_.clear();

    QList<PluginDescriptor> uniqueSelected;
    QSet<QString> seen;
    for (const PluginDescriptor& plugin : selected) {
        if (plugin.name.trimmed().isEmpty()) {
            appendWarning(warnings, QStringLiteral("Found plugin with empty name, skipped"));
            continue;
        }
        if (seen.contains(plugin.name)) {
            appendWarning(warnings, QStringLiteral("Duplicate plugin name ignored: %1").arg(plugin.name));
            continue;
        }
        seen.insert(plugin.name);
        uniqueSelected.push_back(plugin);
    }

    bool allStarted = true;
    bool ctkReady = false;

    if (isCtkAvailable()) {
        QString ctkError;
        ctkReady = ensureCtkFrameworkStarted(&ctkError);
        if (!ctkReady) {
            appendWarning(warnings, QStringLiteral("CTK init failed, fallback to factory mode: %1").arg(ctkError));
        }
    }

    const StartPlan startPlan = buildStartPlan(uniqueSelected, warnings);

    for (const PluginDescriptor& plugin : uniqueSelected) {
        const std::string key = buildPluginKey(plugin.name);

        RuntimeEntry entry;
        entry.descriptor = plugin;
        entry.state = PluginState::Loaded;
        entry.message = QStringLiteral("loaded_ready_to_start");
        runtimeEntries_[key] = std::move(entry);
    }

    for (const PluginDescriptor& plugin : startPlan.orderedPlugins) {
        runtimeOrder_.push_back(plugin.name);
    }
    for (const PluginDescriptor& plugin : uniqueSelected) {
        const std::string key = buildPluginKey(plugin.name);
        if (startPlan.blockedReasons.find(key) != startPlan.blockedReasons.end()) {
            runtimeOrder_.push_back(plugin.name);
        }
    }

    for (const PluginDescriptor& plugin : startPlan.orderedPlugins) {
        const std::string key = buildPluginKey(plugin.name);
        RuntimeEntry& current = runtimeEntries_[key];

        bool started = false;
        if (ctkReady) {
            started = startPluginViaCtk(current, warnings);
        }

        if (!started) {
            started = startPluginViaFactory(current, warnings);
        }

        if (!started) {
            current.state = PluginState::Failed;
            if (current.message.isEmpty()) {
                current.message = QStringLiteral("plugin_start_failed");
            }
            allStarted = false;
        }
    }

    for (const auto& blocked : startPlan.blockedReasons) {
        const auto it = runtimeEntries_.find(blocked.first);
        if (it == runtimeEntries_.end()) {
            continue;
        }

        it->second.state = PluginState::Failed;
        it->second.message = blocked.second;
        allStarted = false;
    }

    return allStarted;
}
void PluginManager::stopPlugins() {
    std::lock_guard<std::mutex> lock(runtimeMutex_);

    for (int i = runtimeOrder_.size() - 1; i >= 0; --i) {
        const QString pluginName = runtimeOrder_.at(i);
        const std::string key = buildPluginKey(pluginName);
        auto it = runtimeEntries_.find(key);
        if (it == runtimeEntries_.end()) {
            continue;
        }

        RuntimeEntry& entry = it->second;
        if (entry.state != PluginState::Running) {
            continue;
        }

        if (entry.ctkManaged) {
#if OSGI_CTK_AVAILABLE
            QSharedPointer<ctkPlugin> plugin;
            if (ctkRuntime_) {
                plugin = ctkRuntime_->installedPlugins.take(pluginName);
            }

            if (!plugin.isNull()) {
                try {
                    plugin->stop(ctkPlugin::STOP_TRANSIENT);
                    entry.state = PluginState::Stopped;
                    entry.message = QStringLiteral("stopped_ctk");
                } catch (const ctkException& ex) {
                    entry.state = PluginState::Failed;
                    entry.message = QStringLiteral("CTK 停止异常: %1").arg(formatCtkException(ex));
                } catch (const std::exception& ex) {
                    entry.state = PluginState::Failed;
                    entry.message = QStringLiteral("CTK 停止异常: %1").arg(QString::fromUtf8(ex.what()));
                } catch (...) {
                    entry.state = PluginState::Failed;
                    entry.message = QStringLiteral("CTK stop exception: unknown");
                }
            } else {
                entry.state = PluginState::Stopped;
                entry.message = QStringLiteral("stopped_ctk");
            }
#else
            entry.state = PluginState::Stopped;
            entry.message = QStringLiteral("stopped_ctk");
#endif
            continue;
        }

        if (!entry.instance) {
            entry.state = PluginState::Stopped;
            entry.message = QStringLiteral("stopped");
            continue;
        }

        try {
            PluginRuntime::stop(*entry.instance);
            entry.state = PluginState::Stopped;
            entry.message = QStringLiteral("stopped");
        } catch (...) {
            entry.state = PluginState::Failed;
            entry.message = QStringLiteral("plugin_stop_exception");
        }

        entry.instance.reset();
    }

    shutdownCtkFramework();
}

QList<PluginManager::PluginRuntimeInfo> PluginManager::runtimeStates() const {
    std::lock_guard<std::mutex> lock(runtimeMutex_);
    QList<PluginRuntimeInfo> states;

    for (const QString& pluginName : runtimeOrder_) {
        const auto it = runtimeEntries_.find(buildPluginKey(pluginName));
        if (it == runtimeEntries_.end()) {
            continue;
        }

        const RuntimeEntry& entry = it->second;
        PluginRuntimeInfo info;
        info.descriptor = entry.descriptor;
        info.state = entry.state;
        info.stateText = stateToText(entry.state);
        info.message = entry.message;
        states.push_back(info);
    }

    return states;
}

void PluginManager::registerServiceInternal(const QString& name, const char* typeId, void* service) {
    if (name.isEmpty() || typeId == nullptr || *typeId == '\0' || service == nullptr) {
        return;
    }

    std::lock_guard<std::mutex> lock(serviceMutex_);
    services_[buildServiceKey(name, typeId)] = ServiceEntry{name, QString::fromUtf8(typeId), service};
}

void* PluginManager::getServiceInternal(const QString& name, const char* typeId) {
    if (name.isEmpty() || typeId == nullptr || *typeId == '\0') {
        return nullptr;
    }

    std::lock_guard<std::mutex> lock(serviceMutex_);
    const auto it = services_.find(buildServiceKey(name, typeId));
    if (it == services_.end()) {
        return nullptr;
    }
    return it->second.service;
}

QStringList PluginManager::findServiceNamesInternal(const QString& prefix, const char* typeId) const {
    if (typeId == nullptr || *typeId == '\0') {
        return {};
    }

    const QString normalizedPrefix = prefix.trimmed();
    const QString expectedTypeId = QString::fromUtf8(typeId);
    QStringList names;

    std::lock_guard<std::mutex> lock(serviceMutex_);
    for (const auto& entryPair : services_) {
        const ServiceEntry& entry = entryPair.second;
        if (entry.typeId != expectedTypeId) {
            continue;
        }
        if (!normalizedPrefix.isEmpty() && !entry.name.startsWith(normalizedPrefix)) {
            continue;
        }
        names.push_back(entry.name);
    }

    names.removeDuplicates();
    std::sort(names.begin(), names.end());
    return names;
}

void PluginManager::appendWarning(QStringList* warnings, const QString& message) {
    if (warnings != nullptr && !message.isEmpty()) {
        warnings->push_back(message);
    }
}

QString PluginManager::stateToText(PluginState state) {
    switch (state) {
    case PluginState::Discovered:
        return QStringLiteral("discovered");
    case PluginState::Loaded:
        return QStringLiteral("loaded");
    case PluginState::Running:
        return QStringLiteral("running");
    case PluginState::Stopped:
        return QStringLiteral("stopped");
    case PluginState::Failed:
        return QStringLiteral("failed");
    }
    return QStringLiteral("unknown");
}

std::string PluginManager::buildServiceKey(const QString& name, const char* typeId) {
    const QString key = name + QStringLiteral("::") + QString::fromUtf8(typeId);
    return key.toUtf8().toStdString();
}

std::string PluginManager::buildPluginKey(const QString& pluginName) {
    return pluginName.toUtf8().toStdString();
}

QString PluginManager::normalizeDependencyName(const QString& dependencyName) {
    return dependencyName.trimmed();
}

bool PluginManager::isFrameworkDependency(const QString& dependencyName) {
    static const QSet<QString> frameworkDeps = {
        QStringLiteral("plugin_api")
    };
    return frameworkDeps.contains(dependencyName.trimmed());
}

PluginManager::StartPlan PluginManager::buildStartPlan(
    const QList<PluginDescriptor>& selected,
    QStringList* warnings) const {
    StartPlan plan;
    if (selected.isEmpty()) {
        return plan;
    }

    QHash<QString, PluginDescriptor> descriptorByName;
    for (const PluginDescriptor& plugin : selected) {
        descriptorByName.insert(plugin.name, plugin);
    }

    for (const PluginDescriptor& plugin : selected) {
        QStringList missingDependencies;

        for (const QString& dependency : plugin.dependencies) {
            const QString dependencyName = normalizeDependencyName(dependency);
            if (dependencyName.isEmpty() || isFrameworkDependency(dependencyName)) {
                continue;
            }
            if (!descriptorByName.contains(dependencyName)) {
                missingDependencies.push_back(dependencyName);
            }
        }

        if (!missingDependencies.isEmpty()) {
            plan.blockedReasons[buildPluginKey(plugin.name)] =
                QStringLiteral("missing dependency: %1").arg(missingDependencies.join(QStringLiteral(", ")));
            appendWarning(
                warnings,
                QStringLiteral("Plugin %1 missing dependency: %2")
                    .arg(plugin.name, missingDependencies.join(QStringLiteral(", "))));
        }
    }

    QHash<QString, int> indegree;
    QHash<QString, QStringList> dependentsByDependency;
    for (const PluginDescriptor& plugin : selected) {
        if (plan.blockedReasons.find(buildPluginKey(plugin.name)) != plan.blockedReasons.end()) {
            continue;
        }
        indegree.insert(plugin.name, 0);
    }

    for (const PluginDescriptor& plugin : selected) {
        const std::string pluginKey = buildPluginKey(plugin.name);
        if (plan.blockedReasons.find(pluginKey) != plan.blockedReasons.end()) {
            continue;
        }

        QSet<QString> uniqueDependencies;
        for (const QString& dependency : plugin.dependencies) {
            const QString dependencyName = normalizeDependencyName(dependency);
            if (dependencyName.isEmpty() || isFrameworkDependency(dependencyName) || !indegree.contains(dependencyName)) {
                continue;
            }
            if (uniqueDependencies.contains(dependencyName)) {
                continue;
            }
            uniqueDependencies.insert(dependencyName);
            indegree[plugin.name] += 1;
            dependentsByDependency[dependencyName].push_back(plugin.name);
        }
    }

    QList<QString> ready;
    for (const PluginDescriptor& plugin : selected) {
        if (!indegree.contains(plugin.name)) {
            continue;
        }
        if (indegree.value(plugin.name) == 0) {
            ready.push_back(plugin.name);
        }
    }

    while (!ready.isEmpty()) {
        const QString pluginName = ready.takeFirst();
        plan.orderedPlugins.push_back(descriptorByName.value(pluginName));

        const QStringList dependents = dependentsByDependency.value(pluginName);
        for (const QString& dependent : dependents) {
            if (!indegree.contains(dependent)) {
                continue;
            }
            const int next = indegree.value(dependent) - 1;
            indegree[dependent] = next;
            if (next == 0) {
                ready.push_back(dependent);
            }
        }
    }

    if (plan.orderedPlugins.size() == indegree.size()) {
        return plan;
    }

    QStringList cyclePlugins;
    for (const PluginDescriptor& plugin : selected) {
        if (!indegree.contains(plugin.name)) {
            continue;
        }
        if (indegree.value(plugin.name) <= 0) {
            continue;
        }

        cyclePlugins.push_back(plugin.name);
        const std::string key = buildPluginKey(plugin.name);
        if (plan.blockedReasons.find(key) == plan.blockedReasons.end()) {
            plan.blockedReasons[key] = QStringLiteral("dependency_cycle");
        }
    }

    if (!cyclePlugins.isEmpty()) {
        appendWarning(
            warnings,
            QStringLiteral("Dependency cycle detected: %1")
                .arg(cyclePlugins.join(QStringLiteral(", "))));
    }

    return plan;
}
PluginManager::PluginFactory PluginManager::getPluginFactory(const QString& pluginName) const {
    std::lock_guard<std::mutex> lock(factoryMutex_);
    const auto it = pluginFactories_.find(buildPluginKey(pluginName));
    if (it == pluginFactories_.end()) {
        return PluginFactory();
    }
    return it->second;
}

bool PluginManager::parsePluginManifest(
    const QString& manifestPath,
    const QString& pluginDir,
    PluginDescriptor* outDescriptor,
    QString* errorMessage) const {
    if (outDescriptor == nullptr) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("internal error: output descriptor is null");
        }
        return false;
    }

    QFile file(manifestPath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Failed to open plugin manifest: %1").arg(manifestPath);
        }
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("插件清单 JSON 解析失败: %1").arg(manifestPath);
        }
        return false;
    }

    const QJsonObject root = doc.object();
    const QString pluginName = root.value("name").toString().trimmed();
    if (pluginName.isEmpty()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("插件清单缺少 name 字段: %1").arg(manifestPath);
        }
        return false;
    }

    static const QRegularExpression kPluginNamePattern(
        QStringLiteral("^[a-z][a-z0-9]*_[a-z][a-z0-9]*_[a-z][a-z0-9_]*$"));
    if (!kPluginNamePattern.match(pluginName).hasMatch()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("插件名不符合三段式规范: %1，示例: org_common_debug_assistant")
                                .arg(pluginName);
        }
        return false;
    }

    const QString dirName = QFileInfo(pluginDir).fileName().trimmed();
    if (!dirName.isEmpty() && dirName != pluginName) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("插件目录名与 plugin.json name 不一致: 目录=%1, name=%2")
                                .arg(dirName, pluginName);
        }
        return false;
    }

    PluginDescriptor descriptor;
    descriptor.name = pluginName;
    descriptor.version = root.value("version").toString(QStringLiteral("0.0.0"));
    descriptor.author = root.value("author").toString(QStringLiteral("unknown"));
    descriptor.description = root.value("description").toString();
    const QJsonArray dependencies = root.value("dependencies").toArray();
    for (const QJsonValue& dependency : dependencies) {
        if (!dependency.isString()) {
            continue;
        }
        const QString dependencyName = dependency.toString().trimmed();
        if (!dependencyName.isEmpty()) {
            descriptor.dependencies.push_back(dependencyName);
        }
    }
    descriptor.directoryPath = pluginDir;
    descriptor.manifestPath = manifestPath;
    descriptor.libraryPathHint = root.value("library").toString().trimmed();

    *outDescriptor = descriptor;
    return true;
}

bool PluginManager::ensureCtkFrameworkStarted(QString* errorMessage) {
#if OSGI_CTK_AVAILABLE
    if (ctkRuntime_ && !ctkRuntime_->framework.isNull() && ctkRuntime_->context != nullptr) {
        return true;
    }

    try {
        ctkProperties initProps;
        const QString storagePath = QDir::cleanPath(
            QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("ctk_storage")));

        QDir storageDir(storagePath);
        if (storageDir.exists() && !storageDir.removeRecursively()) {
            if (errorMessage != nullptr) {
                *errorMessage = QStringLiteral("Failed to clean CTK storage directory: %1").arg(storagePath);
            }
            return false;
        }

        if (!QDir().mkpath(storagePath)) {
            if (errorMessage != nullptr) {
                *errorMessage = QStringLiteral("Failed to create CTK storage directory: %1").arg(storagePath);
            }
            return false;
        }

        initProps.insert(ctkPluginConstants::FRAMEWORK_STORAGE, storagePath);
        initProps.insert(
            ctkPluginConstants::FRAMEWORK_STORAGE_CLEAN,
            ctkPluginConstants::FRAMEWORK_STORAGE_CLEAN_ONFIRSTINIT);

        auto runtime = std::make_unique<CtkRuntimeHandle>();
        runtime->factory = std::make_unique<ctkPluginFrameworkFactory>(initProps);
        QSharedPointer<ctkPluginFramework> framework = runtime->factory->getFramework();
        if (framework.isNull()) {
            if (errorMessage != nullptr) {
                *errorMessage = QStringLiteral("CTK Framework 初始化失败");
            }
            return false;
        }

        framework->init();
        framework->start();

        ctkPluginContext* context = framework->getPluginContext();
        if (context == nullptr) {
            if (errorMessage != nullptr) {
                *errorMessage = QStringLiteral("CTK PluginContext is null");
            }
            return false;
        }

        runtime->framework = framework;
        runtime->context = context;

        runtime->pluginManagerBridgeService = std::make_unique<PluginManagerBridgeService>();
        runtime->pluginManagerBridgeService->setProperty(
            plugin_api::ctk_bridge::kPluginManagerPointerProperty,
            QVariant::fromValue<qulonglong>(reinterpret_cast<qulonglong>(this)));

        runtime->pluginManagerBridgeRegistration = context->registerService(
            plugin_api::ctk_bridge::kPluginManagerBridgeServiceClass,
            runtime->pluginManagerBridgeService.get());
        if (!runtime->pluginManagerBridgeRegistration) {
            if (errorMessage != nullptr) {
                *errorMessage = QStringLiteral("CTK PluginManager bridge service registration failed");
            }
            return false;
        }

        qInfo() << "[PLUGIN] CTK bridge service registered";
        ctkRuntime_ = std::move(runtime);
        return true;
    } catch (const ctkException& ex) {
        if (errorMessage != nullptr) {
            *errorMessage = formatCtkException(ex);
        }
        ctkRuntime_.reset();
        return false;
    } catch (const std::exception& ex) {
        if (errorMessage != nullptr) {
            *errorMessage = QString::fromUtf8(ex.what());
        }
        ctkRuntime_.reset();
        return false;
    } catch (...) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("unknown exception");
        }
        ctkRuntime_.reset();
        return false;
    }
#else
    if (errorMessage != nullptr) {
        *errorMessage = QStringLiteral("CTK not enabled in this build");
    }
    return false;
#endif
}

void PluginManager::shutdownCtkFramework() {
#if OSGI_CTK_AVAILABLE
    if (!ctkRuntime_) {
        return;
    }

    if (ctkRuntime_->pluginManagerBridgeRegistration) {
        try {
            ctkRuntime_->pluginManagerBridgeRegistration.unregister();
        } catch (...) {
            // 閫€鍑洪樁娈靛拷鐣ュ紓甯搞€?
        }
    }
    ctkRuntime_->pluginManagerBridgeRegistration = ctkServiceRegistration();
    ctkRuntime_->pluginManagerBridgeService.reset();

    ctkRuntime_->installedPlugins.clear();

    if (!ctkRuntime_->framework.isNull()) {
        try {
            ctkRuntime_->framework->stop();
            ctkRuntime_->framework->waitForStop(3000);
        } catch (...) {
            // 閫€鍑洪樁娈靛拷鐣ュ紓甯搞€?
        }
    }
#endif

    ctkRuntime_.reset();
}

bool PluginManager::startPluginViaFactory(RuntimeEntry& entry, QStringList* warnings) {
    const PluginFactory factory = getPluginFactory(entry.descriptor.name);
    if (!factory) {
        appendWarning(warnings, QStringLiteral("Plugin factory not registered: %1").arg(entry.descriptor.name));
        entry.message = QStringLiteral("factory_missing");
        return false;
    }

    std::unique_ptr<IPlugin> instance = factory();
    if (!instance) {
        appendWarning(warnings, QStringLiteral("Plugin factory returned null instance: %1").arg(entry.descriptor.name));
        entry.message = QStringLiteral("factory_null_instance");
        return false;
    }

    PluginRuntime::injectPluginManager(*instance, this);

    ScopedMessageBusOwnerContext ownerContext(messageBus_, entry.descriptor.name);
    try {
        PluginRuntime::start(*instance);
    } catch (...) {
        appendWarning(warnings, QStringLiteral("Plugin start exception: %1").arg(entry.descriptor.name));
        entry.message = QStringLiteral("start_exception");
        return false;
    }

    entry.instance = std::move(instance);
    entry.ctkManaged = false;
    entry.state = PluginState::Running;
    entry.message = QStringLiteral("running_factory");

    qInfo().noquote() << QStringLiteral("[PLUGIN] start via factory: %1").arg(entry.descriptor.name);
    return true;
}

bool PluginManager::startPluginViaCtk(RuntimeEntry& entry, QStringList* warnings) {
#if OSGI_CTK_AVAILABLE
    if (!ctkRuntime_ || ctkRuntime_->context == nullptr) {
        entry.message = QStringLiteral("ctk_runtime_not_ready");
        return false;
    }

    const QString libraryPath = resolvePluginLibraryPath(entry.descriptor);
    if (libraryPath.isEmpty()) {
        entry.ctkManaged = false;
        entry.instance.reset();
        entry.state = PluginState::Stopped;
        entry.message = QStringLiteral("skipped_not_built");
        appendWarning(
            warnings,
            QStringLiteral("Plugin shared library not found, skipped: %1").arg(entry.descriptor.name));
        qWarning().noquote() << QStringLiteral("[PLUGIN] no shared library for %1, skip this plugin")
                                    .arg(entry.descriptor.name);
        return true;
    }

    try {
        QSharedPointer<ctkPlugin> plugin = ctkRuntime_->context->installPlugin(QUrl::fromLocalFile(libraryPath));
        if (plugin.isNull()) {
            entry.message = QStringLiteral("ctk_install_null");
            return false;
        }

        ScopedMessageBusOwnerContext ownerContext(messageBus_, entry.descriptor.name);
        plugin->start(ctkPlugin::START_TRANSIENT | ctkPlugin::START_ACTIVATION_POLICY);
        ctkRuntime_->installedPlugins.insert(entry.descriptor.name, plugin);

        entry.ctkManaged = true;
        entry.state = PluginState::Running;
        entry.message = QStringLiteral("running_ctk:%1").arg(QFileInfo(libraryPath).fileName());

        qInfo().noquote() << QStringLiteral("[PLUGIN] start via ctk: %1 (%2)")
                                 .arg(entry.descriptor.name, QFileInfo(libraryPath).fileName());
        return true;
    } catch (const ctkException& ex) {
        const QString reason = formatCtkException(ex);
        appendWarning(
            warnings,
            QStringLiteral("CTK start failed: %1 (%2)").arg(entry.descriptor.name, reason));
        entry.message = QStringLiteral("ctk_start_failed:%1").arg(reason);
        return false;
    } catch (const std::exception& ex) {
        const QString reason = QString::fromUtf8(ex.what());
        appendWarning(
            warnings,
            QStringLiteral("CTK start failed: %1 (%2)").arg(entry.descriptor.name, reason));
        entry.message = QStringLiteral("ctk_start_failed:%1").arg(reason);
        return false;
    } catch (...) {
        appendWarning(warnings, QStringLiteral("CTK start failed: %1 (unknown)").arg(entry.descriptor.name));
        entry.message = QStringLiteral("ctk_start_failed:unknown");
        return false;
    }
#else
    Q_UNUSED(entry)
    Q_UNUSED(warnings)
    return false;
#endif
}
QString PluginManager::resolvePluginLibraryPath(const PluginDescriptor& descriptor) const {
    if (descriptor.directoryPath.trimmed().isEmpty()) {
        return QString();
    }

#ifdef Q_OS_WIN
    const QString extension = QStringLiteral("dll");
#else
    const QString extension = QStringLiteral("so");
#endif

#if defined(QT_NO_DEBUG)
    const bool preferDebugLibrary = false;
#else
    const bool preferDebugLibrary = true;
#endif

    QDir pluginDir(descriptor.directoryPath);
    QStringList candidates;
    QSet<QString> visited;

    auto appendCandidate = [&](const QString& path) {
        if (path.trimmed().isEmpty()) {
            return;
        }

        const QString cleaned = QDir::cleanPath(path);
        if (visited.contains(cleaned)) {
            return;
        }

        visited.insert(cleaned);
        candidates.push_back(cleaned);
    };

    QStringList baseNames;
    // Prefer canonical plugin basename first. For CTK plugins, resource manifest
    // prefix is usually bound to symbolic name without debug suffix.
    baseNames << descriptor.name;
    if (preferDebugLibrary) {
        baseNames << (descriptor.name + QStringLiteral("d"));
    } else {
        baseNames << (descriptor.name + QStringLiteral("d"));
    }

    QStringList platformDirs;
    const QString configuredToolchainTag = QString::fromLatin1(OSGI_PLATFORM_TAG_TOOLCHAIN).trimmed();
    const QString configuredBaseTag = QString::fromLatin1(OSGI_PLATFORM_TAG).trimmed();

    if (!configuredToolchainTag.isEmpty()) {
        platformDirs.push_back(configuredToolchainTag);
    }
    if (!configuredBaseTag.isEmpty() && !platformDirs.contains(configuredBaseTag)) {
        platformDirs.push_back(configuredBaseTag);
    }

#ifdef Q_OS_WIN
    if (!platformDirs.contains(QStringLiteral("win-x64"))) {
        platformDirs.push_back(QStringLiteral("win-x64"));
    }
#else
    if (!platformDirs.contains(QStringLiteral("linux-x64"))) {
        platformDirs.push_back(QStringLiteral("linux-x64"));
    }
#endif

    const QDir appDir(QCoreApplication::applicationDirPath());
    QDir appParentDir = appDir;
    const bool hasAppParent = appParentDir.cdUp();

    for (const QString& baseName : baseNames) {
        const QString fileName = baseName + QStringLiteral(".") + extension;
        appendCandidate(pluginDir.filePath(fileName));
        appendCandidate(pluginDir.filePath(QStringLiteral("bin/") + fileName));
        appendCandidate(pluginDir.filePath(QStringLiteral("bin/Release/") + fileName));
        appendCandidate(pluginDir.filePath(QStringLiteral("bin/Debug/") + fileName));

        for (const QString& platformDir : platformDirs) {
            appendCandidate(pluginDir.filePath(platformDir + QStringLiteral("/bin/") + fileName));
            appendCandidate(pluginDir.filePath(platformDir + QStringLiteral("/bin/Release/") + fileName));
            appendCandidate(pluginDir.filePath(platformDir + QStringLiteral("/bin/Debug/") + fileName));
        }

        appendCandidate(appDir.filePath(fileName));
        appendCandidate(appDir.filePath(QStringLiteral("plugins/") + fileName));
        appendCandidate(appDir.filePath(QStringLiteral("common/%1/%2").arg(descriptor.name, fileName)));
        appendCandidate(appDir.filePath(QStringLiteral("plugins_business/%1/%2").arg(descriptor.name, fileName)));

        if (hasAppParent) {
            appendCandidate(appParentDir.filePath(fileName));
            appendCandidate(appParentDir.filePath(QStringLiteral("plugins/") + fileName));
            appendCandidate(appParentDir.filePath(QStringLiteral("common/%1/%2").arg(descriptor.name, fileName)));
            appendCandidate(appParentDir.filePath(
                QStringLiteral("plugins_business/%1/%2").arg(descriptor.name, fileName)));
        }
    }

    if (!descriptor.libraryPathHint.trimmed().isEmpty()) {
        QFileInfo libraryHintInfo(descriptor.libraryPathHint);
        if (libraryHintInfo.isAbsolute()) {
            appendCandidate(libraryHintInfo.absoluteFilePath());
        } else {
            appendCandidate(pluginDir.filePath(descriptor.libraryPathHint));
        }
    }

    for (const QString& candidate : candidates) {
        const QFileInfo fileInfo(candidate);
        if (fileInfo.exists() && fileInfo.isFile()) {
            return fileInfo.absoluteFilePath();
        }
    }

    const QFileInfoList anyLibraries = pluginDir.entryInfoList(
        QStringList() << QStringLiteral("*.") + extension,
        QDir::Files | QDir::NoSymLinks,
        QDir::Name);

    if (!anyLibraries.isEmpty()) {
        return anyLibraries.first().absoluteFilePath();
    }

    return QString();
}















