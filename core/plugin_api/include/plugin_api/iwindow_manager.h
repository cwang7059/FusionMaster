#pragma once

#include <QString>
#include <QVariantList>

class IWindowManager {
public:
    virtual ~IWindowManager() = default;

    // 菜单栏/工具栏/状态栏/标题栏可见性控制。
    virtual void setMenuBarVisible(bool visible) = 0;
    virtual bool menuBarVisible() const = 0;

    virtual void setToolBarVisible(bool visible) = 0;
    virtual bool toolBarVisible() const = 0;

    virtual void setStatusBarVisible(bool visible) = 0;
    virtual bool statusBarVisible() const = 0;

    virtual void setTitleBarVisible(bool visible) = 0;
    virtual bool titleBarVisible() const = 0;

    // 区域与组件显示控制。
    virtual void setRegionVisible(const QString& region, bool visible) = 0;
    virtual bool regionVisible(const QString& region) const = 0;

    virtual void setContributionVisible(const QString& contributionId, bool visible) = 0;
    virtual bool contributionVisible(const QString& contributionId) const = 0;

    // 窗口按钮显示控制，button: minimize/maximize/restore/fullscreen/close
    virtual void setWindowButtonVisible(const QString& button, bool visible) = 0;
    virtual bool windowButtonVisible(const QString& button) const = 0;

    // 窗口命令，command: minimize/maximize/restore/fullscreen/close
    virtual void requestWindowCommand(const QString& command) = 0;

    // 菜单栏/工具栏动作注册。
    // command 为空时只触发 actionTriggered 信号，不执行内置窗口命令。
    virtual void registerMenuAction(const QString& actionId, const QString& text, const QString& command) = 0;
    virtual void registerToolBarAction(const QString& actionId, const QString& text, const QString& command) = 0;
    virtual void unregisterAction(const QString& actionId) = 0;
    virtual void triggerAction(const QString& actionId) = 0;

    virtual QVariantList menuActionsModel() const = 0;
    virtual QVariantList toolBarActionsModel() const = 0;

    // 状态栏文本。
    virtual void setStatusText(const QString& text) = 0;
    virtual QString statusText() const = 0;

    // 双屏能力。
    virtual void setSecondaryWindowEnabled(bool enabled) = 0;
    virtual bool secondaryWindowEnabled() const = 0;

    virtual void setAvailableScreenCount(int count) = 0;
    virtual int availableScreenCount() const = 0;
};
