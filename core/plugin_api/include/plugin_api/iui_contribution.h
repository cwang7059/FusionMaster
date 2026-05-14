#pragma once

#include <QList>
#include <QString>
#include <QStringList>

struct UiContribution {
    QString id;
    QString title;
    QString surface = QStringLiteral("panel");
    QString region;
    QString qmlSource;
    int order = 0;
    int screenIndex = 0;  // 0=主屏，1=副屏
    QString navText;
    QString navIcon;
    int navOrder = -1;
    QStringList requiredServices;
};

class IUiContributionProvider {
public:
    virtual ~IUiContributionProvider() = default;

    // 返回插件提供的界面组件清单。
    // surface 推荐值: main_tab / panel / dialog
    // region 推荐值: top / left / center / right / bottom
    // screenIndex: 0=主屏，1=副屏（多屏时可继续扩展）
    virtual QList<UiContribution> contributions() const = 0;
};
