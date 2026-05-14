#include <logging/logging_service.h>
#include <message_bus/message_bus_service.h>
#include <plugin_api/iui_contribution.h>
#include <plugin_api/iwindow_manager.h>
#include <plugin_manager.h>
#include <window_manager/window_manager_service.h>

#include <QByteArray>
#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QIcon>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QObject>
#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QScreen>
#include <QSurfaceFormat>
#include <QTimer>
#include <QStringList>
#include <QUrl>
#include <QVariantList>
#include <QVariantMap>

#include <algorithm>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace {

void setupConsoleUtf8OnWindows() {
#ifdef Q_OS_WIN
    // 仅在附着控制台时切换编码，避免影响无控制台场景。
    const HANDLE outputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (outputHandle == nullptr || outputHandle == INVALID_HANDLE_VALUE) {
        return;
    }

    DWORD mode = 0;
    if (!GetConsoleMode(outputHandle, &mode)) {
        return;
    }

    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
#endif
}

void setupRenderLoopForStability() {
#ifdef Q_OS_WIN
    // Windows 下无边框 + 高频刷新时，basic 渲染循环更稳定，减少缩放黑屏概率。
    if (qEnvironmentVariableIsEmpty("QSG_RENDER_LOOP")) {
        qputenv("QSG_RENDER_LOOP", QByteArrayLiteral("basic"));
    }
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (qEnvironmentVariableIsEmpty("QSG_RHI_BACKEND")) {
        qputenv("QSG_RHI_BACKEND", QByteArrayLiteral("opengl"));
    }
#endif
#endif
}

void setupDefaultSurfaceFormat() {
    QSurfaceFormat format = QSurfaceFormat::defaultFormat();
    if (format.swapInterval() <= 0) {
        format.setSwapInterval(1);
    }
    if (format.swapBehavior() == QSurfaceFormat::DefaultSwapBehavior) {
        format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    }
    QSurfaceFormat::setDefaultFormat(format);
}

QString detectRepoRoot() {
    QStringList candidates;
    candidates << QDir::currentPath() << QCoreApplication::applicationDirPath();

    for (const QString& startPath : candidates) {
        QDir dir(startPath);
        for (int i = 0; i < 8; ++i) {
            if (dir.exists(QStringLiteral("profiles"))
                && dir.exists(QStringLiteral("common"))
                && dir.exists(QStringLiteral("plugins_business"))) {
                return dir.absolutePath();
            }
            if (!dir.cdUp()) {
                break;
            }
        }
    }

    return QDir::currentPath();
}

QStringList collectPluginRoots(const QString& repoRoot) {
    QStringList roots;
    QSet<QString> seen;

    auto appendIfExists = [&](const QString& path) {
        const QString cleaned = QDir::cleanPath(path);
        if (cleaned.isEmpty() || seen.contains(cleaned)) {
            return;
        }
        if (!QDir(cleaned).exists()) {
            return;
        }
        seen.insert(cleaned);
        roots.push_back(cleaned);
    };

    // 优先扫描运行产物目录，避免误加载源码目录导致“旧壳/旧产物”问题。
    const QDir appDir(QCoreApplication::applicationDirPath());
    const QString appCommonRoot = QDir::cleanPath(appDir.filePath(QStringLiteral("common")));
    const QString appBusinessRoot = QDir::cleanPath(appDir.filePath(QStringLiteral("plugins_business")));
    appendIfExists(appCommonRoot);
    appendIfExists(appBusinessRoot);

    const bool hasAppRuntimePluginRoots =
        QDir(appCommonRoot).exists() && QDir(appBusinessRoot).exists();

    // 兼容旧布局（app 位于 bin/Debug 或 bin/Release，插件位于 bin/common）。
    // 当检测到当前配置独立目录时，不再回退扫描父目录，避免 Debug/Release 互相串产物。
    const QString parentCommonRoot = QDir::cleanPath(appDir.absoluteFilePath(QStringLiteral("../common")));
    const QString parentBusinessRoot = QDir::cleanPath(appDir.absoluteFilePath(QStringLiteral("../plugins_business")));
    if (!hasAppRuntimePluginRoots) {
        appendIfExists(parentCommonRoot);
        appendIfExists(parentBusinessRoot);
    }

    const bool hasParentRuntimePluginRoots =
        QDir(parentCommonRoot).exists() && QDir(parentBusinessRoot).exists();
    const bool hasRuntimePluginRoots = hasAppRuntimePluginRoots || hasParentRuntimePluginRoots;

    // 未检测到运行产物目录时才回退源码目录，避免和产物目录重复扫描。
    if (!hasRuntimePluginRoots) {
        appendIfExists(QDir(repoRoot).filePath(QStringLiteral("common")));
        appendIfExists(QDir(repoRoot).filePath(QStringLiteral("plugins_business")));
    }

    const QString extraRootsEnv = qEnvironmentVariable("OSGI_EXTRA_PLUGIN_ROOTS").trimmed();
    if (!extraRootsEnv.isEmpty()) {
        const QStringList extraRoots = extraRootsEnv.split(QDir::listSeparator(), Qt::SkipEmptyParts);
        for (const QString& rawRoot : extraRoots) {
            appendIfExists(rawRoot.trimmed());
        }
    }

    return roots;
}

QString normalizeSwitchValue(const QByteArray& rawValue) {
    return QString::fromLocal8Bit(rawValue).trimmed().toLower();
}

bool shouldResetCtkStorage() {
    const QString mode = normalizeSwitchValue(qgetenv("OSGI_CTK_STORAGE_RESET"));
    if (mode.isEmpty()) {
        // 开发模式默认清理，避免 CTK 缓存旧产物导致界面不更新。
        return true;
    }

    if (mode == QStringLiteral("0")
        || mode == QStringLiteral("false")
        || mode == QStringLiteral("off")
        || mode == QStringLiteral("never")) {
        return false;
    }

    if (mode == QStringLiteral("1")
        || mode == QStringLiteral("true")
        || mode == QStringLiteral("on")
        || mode == QStringLiteral("always")) {
        return true;
    }

    // 非法值按“开启”处理，保证开发期默认行为稳定。
    return true;
}

void resetCtkStorageIfNeeded() {
    if (!shouldResetCtkStorage()) {
        qInfo() << "[APP] 已跳过 CTK 存储清理（OSGI_CTK_STORAGE_RESET=off）";
        return;
    }

    const QString storagePath = QDir(QCoreApplication::applicationDirPath())
                                    .filePath(QStringLiteral("ctk_storage"));
    QDir storageDir(storagePath);
    if (!storageDir.exists()) {
        return;
    }

    if (!storageDir.removeRecursively()) {
        qWarning().noquote() << QStringLiteral("[APP] CTK 存储清理失败: %1").arg(storagePath);
        return;
    }

    qInfo().noquote() << QStringLiteral("[APP] 已清理 CTK 存储目录: %1").arg(storagePath);
}

QString parseProfileArgument(const QStringList& args) {
    for (int i = 1; i < args.size(); ++i) {
        const QString arg = args.at(i);

        if (arg == QStringLiteral("--profile")) {
            if (i + 1 < args.size()) {
                return args.at(i + 1).trimmed();
            }
            qWarning() << "--profile 未提供参数，使用默认配置";
            return QString();
        }

        if (arg.startsWith(QStringLiteral("--profile="))) {
            return arg.mid(QStringLiteral("--profile=").size()).trimmed();
        }
    }

    return QString();
}

int parseAutoExitMsArgument(const QStringList& args) {
    for (int i = 1; i < args.size(); ++i) {
        const QString arg = args.at(i);

        QString valueText;
        if (arg == QStringLiteral("--auto-exit-ms")) {
            if (i + 1 < args.size()) {
                valueText = args.at(i + 1).trimmed();
            } else {
                qWarning() << "--auto-exit-ms 未提供参数，忽略自动退出";
                return 0;
            }
        } else if (arg.startsWith(QStringLiteral("--auto-exit-ms="))) {
            valueText = arg.mid(QStringLiteral("--auto-exit-ms=").size()).trimmed();
        } else {
            continue;
        }

        bool ok = false;
        const int value = valueText.toInt(&ok);
        if (!ok || value <= 0) {
            qWarning() << "--auto-exit-ms 参数无效，忽略自动退出" << valueText;
            return 0;
        }
        return value;
    }

    return 0;
}
QString resolveProfilePath(const QString& profileArg, const QString& repoRoot) {
    const QString defaultPath = QDir(repoRoot).filePath(QStringLiteral("profiles/default.json"));
    if (profileArg.isEmpty()) {
        return defaultPath;
    }

    QFileInfo profileInfo(profileArg);
    if (profileInfo.isAbsolute()) {
        return profileArg;
    }

    const QString currentPathCandidate = QDir::cleanPath(QDir::current().absoluteFilePath(profileArg));
    if (QFileInfo::exists(currentPathCandidate)) {
        return currentPathCandidate;
    }

    const QString repoRootCandidate = QDir::cleanPath(QDir(repoRoot).absoluteFilePath(profileArg));
    if (QFileInfo::exists(repoRootCandidate)) {
        return repoRootCandidate;
    }

    const QString profileDirCandidate = QDir::cleanPath(
        QDir(repoRoot).absoluteFilePath(QStringLiteral("profiles/%1").arg(profileArg)));
    if (QFileInfo::exists(profileDirCandidate)) {
        return profileDirCandidate;
    }

    return repoRootCandidate;
}

void applyUiProfileConfig(const QString& profilePath, WindowManagerService& windowManager, QStringList* warnings) {
    QFile file(profilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (warnings != nullptr) {
            warnings->push_back(QStringLiteral("UI 配置读取失败: %1").arg(profilePath));
        }
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        if (warnings != nullptr) {
            warnings->push_back(QStringLiteral("UI 配置解析失败: %1").arg(profilePath));
        }
        return;
    }

    const QJsonObject root = doc.object();
    if (!root.contains(QStringLiteral("ui")) || !root.value(QStringLiteral("ui")).isObject()) {
        return;
    }

    const QJsonObject ui = root.value(QStringLiteral("ui")).toObject();

    if (ui.contains(QStringLiteral("menuBarVisible"))) {
        windowManager.setMenuBarVisible(ui.value(QStringLiteral("menuBarVisible")).toBool(true));
    }
    if (ui.contains(QStringLiteral("toolBarVisible"))) {
        windowManager.setToolBarVisible(ui.value(QStringLiteral("toolBarVisible")).toBool(true));
    }
    if (ui.contains(QStringLiteral("statusBarVisible"))) {
        windowManager.setStatusBarVisible(ui.value(QStringLiteral("statusBarVisible")).toBool(true));
    }
    if (ui.contains(QStringLiteral("titleBarVisible"))) {
        windowManager.setTitleBarVisible(ui.value(QStringLiteral("titleBarVisible")).toBool(true));
    }
    if (ui.contains(QStringLiteral("secondaryWindowEnabled"))) {
        windowManager.setSecondaryWindowEnabled(ui.value(QStringLiteral("secondaryWindowEnabled")).toBool(false));
    }
    if (ui.contains(QStringLiteral("statusText"))) {
        windowManager.setStatusText(ui.value(QStringLiteral("statusText")).toString());
    }

    if (ui.contains(QStringLiteral("regions")) && ui.value(QStringLiteral("regions")).isObject()) {
        const QJsonObject regions = ui.value(QStringLiteral("regions")).toObject();
        for (auto it = regions.begin(); it != regions.end(); ++it) {
            windowManager.setRegionVisible(it.key(), it.value().toBool(true));
        }
    }

    if (ui.contains(QStringLiteral("windowButtons")) && ui.value(QStringLiteral("windowButtons")).isObject()) {
        const QJsonObject buttons = ui.value(QStringLiteral("windowButtons")).toObject();
        for (auto it = buttons.begin(); it != buttons.end(); ++it) {
            windowManager.setWindowButtonVisible(it.key(), it.value().toBool(true));
        }
    }

    if (ui.contains(QStringLiteral("contributions")) && ui.value(QStringLiteral("contributions")).isObject()) {
        const QJsonObject contributions = ui.value(QStringLiteral("contributions")).toObject();
        for (auto it = contributions.begin(); it != contributions.end(); ++it) {
            if (it.value().isBool()) {
                windowManager.setContributionVisible(it.key(), it.value().toBool(true));
                continue;
            }
            if (it.value().isObject()) {
                const QJsonObject contributionObj = it.value().toObject();
                if (contributionObj.contains(QStringLiteral("enabled"))) {
                    windowManager.setContributionVisible(
                        it.key(),
                        contributionObj.value(QStringLiteral("enabled")).toBool(true));
                }
            }
        }
    }

    if (ui.contains(QStringLiteral("menuActions")) && ui.value(QStringLiteral("menuActions")).isArray()) {
        const QJsonArray menuActions = ui.value(QStringLiteral("menuActions")).toArray();
        for (const QJsonValue& value : menuActions) {
            if (!value.isObject()) {
                continue;
            }
            const QJsonObject actionObj = value.toObject();
            const QString id = actionObj.value(QStringLiteral("id")).toString().trimmed();
            const QString textValue = actionObj.value(QStringLiteral("text")).toString().trimmed();
            const QString command = actionObj.value(QStringLiteral("command")).toString().trimmed();
            if (id.isEmpty()) {
                continue;
            }
            windowManager.registerMenuAction(id, textValue, command);
        }
    }

    if (ui.contains(QStringLiteral("toolBarActions")) && ui.value(QStringLiteral("toolBarActions")).isArray()) {
        const QJsonArray toolBarActions = ui.value(QStringLiteral("toolBarActions")).toArray();
        for (const QJsonValue& value : toolBarActions) {
            if (!value.isObject()) {
                continue;
            }
            const QJsonObject actionObj = value.toObject();
            const QString id = actionObj.value(QStringLiteral("id")).toString().trimmed();
            const QString textValue = actionObj.value(QStringLiteral("text")).toString().trimmed();
            const QString command = actionObj.value(QStringLiteral("command")).toString().trimmed();
            if (id.isEmpty()) {
                continue;
            }
            windowManager.registerToolBarAction(id, textValue, command);
        }
    }
}
QString formatPluginList(const QList<PluginManager::PluginDescriptor>& plugins) {
    if (plugins.isEmpty()) {
        return QStringLiteral("无");
    }

    QStringList lines;
    for (const PluginManager::PluginDescriptor& plugin : plugins) {
        const QString dependencyText = plugin.dependencies.isEmpty()
            ? QStringLiteral("无")
            : plugin.dependencies.join(QStringLiteral(","));
        lines.push_back(
            QStringLiteral("- %1 | version=%2 | author=%3 | dependencies=%4")
                .arg(plugin.name, plugin.version, plugin.author, dependencyText));
    }
    return lines.join('\n');
}

QString formatRuntimeList(const QList<PluginManager::PluginRuntimeInfo>& states) {
    if (states.isEmpty()) {
        return QStringLiteral("无");
    }

    QStringList lines;
    for (const PluginManager::PluginRuntimeInfo& state : states) {
        lines.push_back(
            QStringLiteral("- %1 | state=%2 | message=%3")
                .arg(state.descriptor.name, state.stateText, state.message));
    }
    return lines.join('\n');
}

QString formatStringList(const QStringList& values) {
    if (values.isEmpty()) {
        return QStringLiteral("无");
    }

    QStringList lines;
    for (const QString& value : values) {
        lines.push_back(QStringLiteral("- %1").arg(value));
    }
    return lines.join('\n');
}

int normalizeUiScreenIndex(int screenIndex) {
    return screenIndex < 0 ? 0 : screenIndex;
}

QString normalizeUiRegion(const QString& region) {
    const QString value = region.trimmed().toLower();
    if (value == QStringLiteral("top")
        || value == QStringLiteral("left")
        || value == QStringLiteral("center")
        || value == QStringLiteral("right")
        || value == QStringLiteral("bottom")) {
        return value;
    }
    return QStringLiteral("center");
}

QString resolveUiQmlSource(const PluginManager::PluginDescriptor& descriptor, const QString& qmlSource) {
    const QString trimmed = qmlSource.trimmed();
    if (trimmed.isEmpty()) {
        return QString();
    }

    if (trimmed.startsWith(QStringLiteral("qrc:/")) || trimmed.startsWith(QStringLiteral("file:/"))) {
        return trimmed;
    }

    QFileInfo qmlInfo(trimmed);
    if (qmlInfo.isAbsolute()) {
        return QUrl::fromLocalFile(qmlInfo.absoluteFilePath()).toString();
    }

    const QString absolutePath = QDir(descriptor.directoryPath).filePath(trimmed);
    return QUrl::fromLocalFile(QDir::cleanPath(absolutePath)).toString();
}

QVariantList collectUiContributions(
    const QList<PluginManager::PluginDescriptor>& selected,
    PluginManager& pluginManager,
    QStringList* warnings) {
    QVariantList rows;

    for (const PluginManager::PluginDescriptor& descriptor : selected) {
        const QString serviceName = QStringLiteral("ui/%1").arg(descriptor.name);
        auto* provider = pluginManager.getService<IUiContributionProvider>(serviceName);
        if (provider == nullptr) {
            continue;
        }

        const QList<UiContribution> contributions = provider->contributions();
        for (const UiContribution& contribution : contributions) {
            const QString id = contribution.id.trimmed();
            const QString source = resolveUiQmlSource(descriptor, contribution.qmlSource);
            if (id.isEmpty() || source.isEmpty()) {
                if (warnings != nullptr) {
                    warnings->push_back(QStringLiteral("插件 %1 的 UI 组件声明无效（id/source 为空）").arg(descriptor.name));
                }
                continue;
            }

            QVariantMap row;
            row.insert(QStringLiteral("pluginName"), descriptor.name);
            row.insert(QStringLiteral("id"), id);
            row.insert(
                QStringLiteral("title"),
                contribution.title.trimmed().isEmpty() ? descriptor.name : contribution.title.trimmed());
            row.insert(QStringLiteral("region"), normalizeUiRegion(contribution.region));
            row.insert(QStringLiteral("screenIndex"), normalizeUiScreenIndex(contribution.screenIndex));
            row.insert(QStringLiteral("order"), contribution.order);
            row.insert(QStringLiteral("qmlSource"), source);
            rows.push_back(row);
        }
    }

    std::sort(rows.begin(), rows.end(), [](const QVariant& lhs, const QVariant& rhs) {
        const QVariantMap l = lhs.toMap();
        const QVariantMap r = rhs.toMap();

        const int lScreenIndex = l.value(QStringLiteral("screenIndex")).toInt();
        const int rScreenIndex = r.value(QStringLiteral("screenIndex")).toInt();
        if (lScreenIndex != rScreenIndex) {
            return lScreenIndex < rScreenIndex;
        }

        const QString lRegion = l.value(QStringLiteral("region")).toString();
        const QString rRegion = r.value(QStringLiteral("region")).toString();
        if (lRegion != rRegion) {
            return lRegion < rRegion;
        }

        const int lOrder = l.value(QStringLiteral("order")).toInt();
        const int rOrder = r.value(QStringLiteral("order")).toInt();
        if (lOrder != rOrder) {
            return lOrder < rOrder;
        }

        const QString lPlugin = l.value(QStringLiteral("pluginName")).toString();
        const QString rPlugin = r.value(QStringLiteral("pluginName")).toString();
        if (lPlugin != rPlugin) {
            return lPlugin < rPlugin;
        }

        return l.value(QStringLiteral("id")).toString() < r.value(QStringLiteral("id")).toString();
    });

    return rows;
}

void logWarningList(const QString& title, const QStringList& values) {
    if (values.isEmpty()) {
        qInfo().noquote() << QStringLiteral("[APP] %1: 无").arg(title);
        return;
    }

    qWarning().noquote() << QStringLiteral("[APP] %1:").arg(title);
    for (const QString& value : values) {
        qWarning().noquote() << QStringLiteral("  - %1").arg(value);
    }
}

void logRuntimeStates(const QList<PluginManager::PluginRuntimeInfo>& states) {
    if (states.isEmpty()) {
        qInfo() << "[APP] 运行时插件状态为空";
        return;
    }

    qInfo().noquote() << QStringLiteral("[APP] 运行时插件状态共 %1 项").arg(states.size());
    for (const PluginManager::PluginRuntimeInfo& state : states) {
        qInfo().noquote() << QStringLiteral("  - %1 | %2 | %3")
            .arg(state.descriptor.name, state.stateText, state.message);
    }
}

}  // namespace

int main(int argc, char** argv) {
    setupConsoleUtf8OnWindows();
    setupRenderLoopForStability();
    setupDefaultSurfaceFormat();
    qputenv("QT_QUICK_CONTROLS_STYLE", QByteArrayLiteral("Basic"));
    QApplication app(argc, argv);
    const QIcon appIcon(QStringLiteral(":/app/assets/app_icon.png"));
    if (!appIcon.isNull()) {
        app.setWindowIcon(appIcon);
    }

    const QString repoRoot = detectRepoRoot();
    const QString logRootDir = QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("logs"));

    LoggingService loggingService;
    loggingService.initialize(logRootDir);

    resetCtkStorageIfNeeded();

    MessageBusService messageBus;

    WindowManagerService windowManager;
    windowManager.setAvailableScreenCount(QGuiApplication::screens().size());

    QObject::connect(
        &windowManager,
        &WindowManagerService::actionTriggered,
        [&messageBus](const QString& actionId, const QString& command) {
            QJsonObject payload;
            payload.insert(QStringLiteral("actionId"), actionId);
            payload.insert(QStringLiteral("command"), command);
            payload.insert(QStringLiteral("timestampMs"), QDateTime::currentMSecsSinceEpoch());
            messageBus.publish(
                QStringLiteral("ui/action"),
                QJsonDocument(payload).toJson(QJsonDocument::Compact));
        });

    QObject::connect(&app, &QGuiApplication::screenAdded, [&windowManager](QScreen*) {
        windowManager.setAvailableScreenCount(QGuiApplication::screens().size());
    });
    QObject::connect(&app, &QGuiApplication::screenRemoved, [&windowManager](QScreen*) {
        windowManager.setAvailableScreenCount(QGuiApplication::screens().size());
    });

    PluginManager pluginManager;
    pluginManager.setMessageBus(&messageBus);
    pluginManager.setLogger(&loggingService);
    pluginManager.registerService<LoggingService>(QStringLiteral("service/logging"), &loggingService);
    pluginManager.registerService<IWindowManager>(QStringLiteral("window_manager"), &windowManager);

    const QString profileArg = parseProfileArgument(app.arguments());
    QString resolvedProfilePath = resolveProfilePath(profileArg, repoRoot);
    const int autoExitMs = parseAutoExitMsArgument(app.arguments());

    QString startupStatus;
    QString profileError;
    bool profileLoaded = pluginManager.loadProfile(resolvedProfilePath, &profileError);

    if (!profileLoaded) {
        const QString defaultProfilePath = QDir(repoRoot).filePath(QStringLiteral("profiles/default.json"));
        if (resolvedProfilePath != defaultProfilePath) {
            QString fallbackError;
            if (pluginManager.loadProfile(defaultProfilePath, &fallbackError)) {
                startupStatus = QStringLiteral("指定 profile 加载失败，已回退默认配置。失败原因: %1")
                                    .arg(profileError);
                resolvedProfilePath = defaultProfilePath;
                profileLoaded = true;
            } else {
                startupStatus = QStringLiteral("profile 加载失败: %1；默认 profile 也失败: %2")
                                    .arg(profileError, fallbackError);
            }
        } else {
            startupStatus = QStringLiteral("profile 加载失败: %1").arg(profileError);
        }
    }

    QStringList uiConfigWarnings;
    applyUiProfileConfig(resolvedProfilePath, windowManager, &uiConfigWarnings);

    QStringList scanWarnings;
    QStringList skippedPlugins;
    QStringList lifecycleWarnings;
    QList<PluginManager::PluginDescriptor> discovered;
    QList<PluginManager::PluginDescriptor> selected;
    bool startOk = false;
    QVariantList uiContributions;
    QObject* busMonitorController = nullptr;
    QObject* logViewerController = nullptr;
    QObject* recordReplayController = nullptr;

    if (profileLoaded) {
        const QStringList pluginRoots = collectPluginRoots(repoRoot);
        qInfo().noquote() << QStringLiteral("[APP] 插件扫描目录: %1").arg(pluginRoots.join(QStringLiteral(" | ")));

        discovered = pluginManager.discoverPlugins(pluginRoots, &scanWarnings);
        selected = pluginManager.filterPluginsByProfile(discovered, &skippedPlugins, &lifecycleWarnings);
        startOk = pluginManager.startPlugins(selected, &lifecycleWarnings);

        qInfo().noquote() << QStringLiteral("[APP] 插件扫描完成: 发现=%1, 选中=%2, 启动结果=%3")
            .arg(discovered.size())
            .arg(selected.size())
            .arg(startOk ? QStringLiteral("成功") : QStringLiteral("存在失败"));
        logWarningList(QStringLiteral("扫描告警"), scanWarnings);
        logWarningList(QStringLiteral("被过滤插件"), skippedPlugins);
        logWarningList(QStringLiteral("生命周期告警"), lifecycleWarnings);
        logRuntimeStates(pluginManager.runtimeStates());

        uiContributions = collectUiContributions(selected, pluginManager, &lifecycleWarnings);
        busMonitorController = pluginManager.getService<QObject>(QStringLiteral("controller/org_common_bus_monitor"));
        logViewerController = pluginManager.getService<QObject>(QStringLiteral("controller/org_common_log_viewer"));
        recordReplayController = pluginManager.getService<QObject>(QStringLiteral("controller/org_common_record_replay"));

        if (startupStatus.isEmpty()) {
            startupStatus = startOk ? QStringLiteral("插件启动完成") : QStringLiteral("插件启动完成，但存在失败项");
        }
    }

    QStringList allScanWarnings = scanWarnings;
    allScanWarnings.append(uiConfigWarnings);

    const PluginManager::ProfileConfig profileConfig = pluginManager.profileConfig();
    const QString profileMode = profileConfig.loadAll() ? QStringLiteral("全量加载") : QStringLiteral("白名单加载");

    QQmlApplicationEngine engine;
    engine.addImportPath(QCoreApplication::applicationDirPath());

    engine.rootContext()->setContextProperty(QStringLiteral("startupStatus"), startupStatus);
    engine.rootContext()->setContextProperty(
        QStringLiteral("runtimeBackendName"),
        pluginManager.runtimeBackendName());
    engine.rootContext()->setContextProperty(
        QStringLiteral("runtimeBackendHint"),
        pluginManager.runtimeBackendHint());
    engine.rootContext()->setContextProperty(QStringLiteral("repoRoot"), repoRoot);
    engine.rootContext()->setContextProperty(QStringLiteral("profilePath"), resolvedProfilePath);
    engine.rootContext()->setContextProperty(QStringLiteral("profileName"), profileConfig.name);
    engine.rootContext()->setContextProperty(QStringLiteral("profileMode"), profileMode);
    engine.rootContext()->setContextProperty(
        QStringLiteral("discoveredPluginsText"),
        formatPluginList(discovered));
    engine.rootContext()->setContextProperty(
        QStringLiteral("selectedPluginsText"),
        formatPluginList(selected));
    engine.rootContext()->setContextProperty(
        QStringLiteral("runtimeStatesText"),
        formatRuntimeList(pluginManager.runtimeStates()));
    engine.rootContext()->setContextProperty(
        QStringLiteral("scanWarningsText"),
        formatStringList(allScanWarnings));
    engine.rootContext()->setContextProperty(
        QStringLiteral("skippedPluginsText"),
        formatStringList(skippedPlugins));
    engine.rootContext()->setContextProperty(
        QStringLiteral("lifecycleWarningsText"),
        formatStringList(lifecycleWarnings));
    engine.rootContext()->setContextProperty(
        QStringLiteral("uiContributionsModel"),
        uiContributions);
    engine.rootContext()->setContextProperty(
        QStringLiteral("availableScreenCount"),
        QGuiApplication::screens().size());
    engine.rootContext()->setContextProperty(QStringLiteral("windowManager"), &windowManager);
    engine.rootContext()->setContextProperty(QStringLiteral("messageBusService"), &messageBus);
    engine.rootContext()->setContextProperty(QStringLiteral("busMonitorController"), busMonitorController);
    engine.rootContext()->setContextProperty(QStringLiteral("logViewerController"), logViewerController);
    engine.rootContext()->setContextProperty(QStringLiteral("recordReplayController"), recordReplayController);

    QObject::connect(
        &app,
        &QCoreApplication::aboutToQuit,
        [&pluginManager, &loggingService]() {
            pluginManager.stopPlugins();
            loggingService.shutdown();
        });

    engine.load(QUrl(QStringLiteral("qrc:/app/qml/qml/Main.qml")));
    if (engine.rootObjects().isEmpty()) {
        pluginManager.stopPlugins();
        loggingService.shutdown();
        return 1;
    }

    if (!appIcon.isNull()) {
        for (QObject* object : engine.rootObjects()) {
            if (object == nullptr) {
                continue;
            }
            object->setProperty("icon", appIcon);
        }
    }

    windowManager.setMainWindowObject(engine.rootObjects().constFirst());

    if (autoExitMs > 0) {
        qInfo().noquote() << QStringLiteral("[APP] 自动退出已启用: %1 ms").arg(autoExitMs);
        QTimer::singleShot(autoExitMs, &app, &QCoreApplication::quit);
    }

    const int exitCode = app.exec();
    if (!profileLoaded || !startOk) {
        return exitCode == 0 ? 2 : exitCode;
    }
    return exitCode;
}
