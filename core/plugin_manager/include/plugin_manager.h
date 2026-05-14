#pragma once

#include <plugin_api/ilogger.h>
#include <plugin_api/imessage_bus.h>
#include <plugin_api/iplugin.h>
#include <plugin_api/iplugin_manager.h>

#include <QList>
#include <QString>
#include <QStringList>

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

class PluginRuntime {
public:
    static void injectPluginManager(IPlugin& plugin, IPluginManager* manager);
    static void start(IPlugin& plugin);
    static void stop(IPlugin& plugin);
};

class PluginManager : public IPluginManager {
public:
    struct ProfileConfig {
        QString name;
        QString description;
        QStringList plugins;

        bool loadAll() const { return plugins.contains("*"); }
    };

    struct PluginDescriptor {
        QString name;
        QString version;
        QString author;
        QString description;
        QStringList dependencies;
        QString directoryPath;
        QString manifestPath;
        QString libraryPathHint;
    };

    enum class PluginState {
        Discovered,
        Loaded,
        Running,
        Stopped,
        Failed
    };

    struct PluginRuntimeInfo {
        PluginDescriptor descriptor;
        PluginState state = PluginState::Discovered;
        QString stateText;
        QString message;
    };

    using PluginFactory = std::function<std::unique_ptr<IPlugin>()>;

    PluginManager();
    ~PluginManager() override;

    void setMessageBus(IMessageBus* messageBus);
    void setLogger(ILogger* logger);

    void registerPluginFactory(const QString& pluginName, PluginFactory factory);

    IMessageBus* messageBus() override;
    ILogger* logger() override;

    // 运行时后端状态（用于判断是否已就绪 CTK 预编译产物）。
    bool isCtkAvailable() const;
    QString runtimeBackendName() const;
    QString runtimeBackendHint() const;

    bool loadProfile(const QString& profilePath, QString* errorMessage = nullptr);
    ProfileConfig profileConfig() const;
    bool shouldLoadPlugin(const QString& pluginName) const;

    QList<PluginDescriptor> discoverPlugins(const QStringList& pluginRoots, QStringList* warnings = nullptr) const;
    QList<PluginDescriptor> filterPluginsByProfile(
        const QList<PluginDescriptor>& discovered,
        QStringList* skippedPlugins = nullptr,
        QStringList* warnings = nullptr) const;

    bool startPlugins(const QList<PluginDescriptor>& selected, QStringList* warnings = nullptr);
    void stopPlugins();
    QList<PluginRuntimeInfo> runtimeStates() const;

protected:
    void registerServiceInternal(const QString& name, const char* typeId, void* service) override;
    void* getServiceInternal(const QString& name, const char* typeId) override;
    QStringList findServiceNamesInternal(const QString& prefix, const char* typeId) const override;

private:
    struct ServiceEntry {
        QString name;
        QString typeId;
        void* service = nullptr;
    };

    struct RuntimeEntry {
        PluginDescriptor descriptor;
        PluginState state = PluginState::Discovered;
        QString message;
        std::unique_ptr<IPlugin> instance;
        bool ctkManaged = false;
    };

    struct StartPlan {
        QList<PluginDescriptor> orderedPlugins;
        std::unordered_map<std::string, QString> blockedReasons;
    };

    struct CtkRuntimeHandle;

    static void appendWarning(QStringList* warnings, const QString& message);
    static QString stateToText(PluginState state);
    static std::string buildServiceKey(const QString& name, const char* typeId);
    static std::string buildPluginKey(const QString& pluginName);
    static QString normalizeDependencyName(const QString& dependencyName);
    static bool isFrameworkDependency(const QString& dependencyName);

    PluginFactory getPluginFactory(const QString& pluginName) const;
    StartPlan buildStartPlan(const QList<PluginDescriptor>& selected, QStringList* warnings) const;

    bool parsePluginManifest(
        const QString& manifestPath,
        const QString& pluginDir,
        PluginDescriptor* outDescriptor,
        QString* errorMessage) const;

    bool ensureCtkFrameworkStarted(QString* errorMessage);
    void shutdownCtkFramework();

    bool startPluginViaFactory(RuntimeEntry& entry, QStringList* warnings);
    bool startPluginViaCtk(RuntimeEntry& entry, QStringList* warnings);
    QString resolvePluginLibraryPath(const PluginDescriptor& descriptor) const;

    IMessageBus* messageBus_ = nullptr;
    ILogger* logger_ = nullptr;

    mutable std::mutex serviceMutex_;
    std::unordered_map<std::string, ServiceEntry> services_;

    mutable std::mutex factoryMutex_;
    std::unordered_map<std::string, PluginFactory> pluginFactories_;

    mutable std::mutex profileMutex_;
    ProfileConfig profileConfig_{QStringLiteral("默认配置"), QStringLiteral("加载全部插件"), QStringList{"*"}};

    mutable std::mutex runtimeMutex_;
    QList<QString> runtimeOrder_;
    std::unordered_map<std::string, RuntimeEntry> runtimeEntries_;

    std::unique_ptr<CtkRuntimeHandle> ctkRuntime_;
};
