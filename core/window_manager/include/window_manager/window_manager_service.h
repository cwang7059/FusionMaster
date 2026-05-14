#pragma once

#include <plugin_api/iwindow_manager.h>

#include <QObject>
#include <QHash>
#include <QMutex>
#include <QPointer>
#include <QString>
#include <QVariantList>

class WindowManagerService : public QObject, public IWindowManager {
    Q_OBJECT
    Q_PROPERTY(bool menuBarVisible READ menuBarVisible WRITE setMenuBarVisible NOTIFY menuBarVisibleChanged)
    Q_PROPERTY(bool toolBarVisible READ toolBarVisible WRITE setToolBarVisible NOTIFY toolBarVisibleChanged)
    Q_PROPERTY(bool statusBarVisible READ statusBarVisible WRITE setStatusBarVisible NOTIFY statusBarVisibleChanged)
    Q_PROPERTY(bool titleBarVisible READ titleBarVisible WRITE setTitleBarVisible NOTIFY titleBarVisibleChanged)
    Q_PROPERTY(QString statusText READ statusText WRITE setStatusText NOTIFY statusTextChanged)
    Q_PROPERTY(bool secondaryWindowEnabled READ secondaryWindowEnabled WRITE setSecondaryWindowEnabled NOTIFY secondaryWindowEnabledChanged)
    Q_PROPERTY(int availableScreenCount READ availableScreenCount WRITE setAvailableScreenCount NOTIFY availableScreenCountChanged)
    Q_PROPERTY(QVariantList menuActionsModel READ menuActionsModel NOTIFY menuActionsChanged)
    Q_PROPERTY(QVariantList toolBarActionsModel READ toolBarActionsModel NOTIFY toolBarActionsChanged)

public:
    explicit WindowManagerService(QObject* parent = nullptr);

    void setMainWindowObject(QObject* mainWindowObject);

    Q_INVOKABLE void setMenuBarVisible(bool visible) override;
    bool menuBarVisible() const override;

    Q_INVOKABLE void setToolBarVisible(bool visible) override;
    bool toolBarVisible() const override;

    Q_INVOKABLE void setStatusBarVisible(bool visible) override;
    bool statusBarVisible() const override;

    Q_INVOKABLE void setTitleBarVisible(bool visible) override;
    bool titleBarVisible() const override;

    Q_INVOKABLE void setRegionVisible(const QString& region, bool visible) override;
    bool regionVisible(const QString& region) const override;

    Q_INVOKABLE void setContributionVisible(const QString& contributionId, bool visible) override;
    bool contributionVisible(const QString& contributionId) const override;

    Q_INVOKABLE void setWindowButtonVisible(const QString& button, bool visible) override;
    bool windowButtonVisible(const QString& button) const override;

    void requestWindowCommand(const QString& command) override;

    void registerMenuAction(const QString& actionId, const QString& text, const QString& command) override;
    void registerToolBarAction(const QString& actionId, const QString& text, const QString& command) override;
    void unregisterAction(const QString& actionId) override;
    void triggerAction(const QString& actionId) override;

    QVariantList menuActionsModel() const override;
    QVariantList toolBarActionsModel() const override;

    Q_INVOKABLE void setStatusText(const QString& text) override;
    QString statusText() const override;

    Q_INVOKABLE void setSecondaryWindowEnabled(bool enabled) override;
    bool secondaryWindowEnabled() const override;

    void setAvailableScreenCount(int count) override;
    int availableScreenCount() const override;

    Q_INVOKABLE bool isRegionVisible(const QString& region) const;
    Q_INVOKABLE bool isContributionVisible(const QString& contributionId) const;
    Q_INVOKABLE bool isWindowButtonVisible(const QString& button) const;
    Q_INVOKABLE QVariantMap resolveAvailableGeometryAt(int x, int y) const;
    Q_INVOKABLE void executeWindowCommand(const QString& command);
    Q_INVOKABLE void triggerUiAction(const QString& actionId);

signals:
    void menuBarVisibleChanged();
    void toolBarVisibleChanged();
    void statusBarVisibleChanged();
    void titleBarVisibleChanged();
    void statusTextChanged();
    void secondaryWindowEnabledChanged();
    void availableScreenCountChanged();
    void layoutChanged();
    void menuActionsChanged();
    void toolBarActionsChanged();
    void actionTriggered(const QString& actionId, const QString& command);

private:
    struct ActionEntry {
        QString id;
        QString text;
        QString command;
    };

    static QString normalizeRegion(const QString& region);
    static QString normalizeWindowButton(const QString& button);
    static QString normalizeCommand(const QString& command);

    static QVariantList buildActionModel(const QList<ActionEntry>& actions);
    static void upsertAction(QList<ActionEntry>* actions, const ActionEntry& next);
    static int findActionIndex(const QList<ActionEntry>& actions, const QString& actionId);

    void invokeMainWindowMethod(const char* methodName);

    mutable QMutex mutex_;
    QPointer<QObject> mainWindowObject_;

    bool menuBarVisible_ = true;
    bool toolBarVisible_ = true;
    bool statusBarVisible_ = true;
    bool titleBarVisible_ = true;

    QString statusText_ = QStringLiteral("就绪");
    bool secondaryWindowEnabled_ = false;
    int availableScreenCount_ = 1;

    QHash<QString, bool> regionVisibleMap_;
    QHash<QString, bool> contributionVisibleMap_;
    QHash<QString, bool> windowButtonVisibleMap_;

    QList<ActionEntry> menuActions_;
    QList<ActionEntry> toolBarActions_;
};
