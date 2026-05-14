#pragma once

#include <QList>
#include <QString>

struct UiContribution {
    QString id;
    QString title;
    QString region;
    QString qmlSource;
    int order = 0;
    int screenIndex = 0;  // 0=主屏，1=副屏
};

class IUiContributionProvider {
public:
    virtual ~IUiContributionProvider() = default;

    // 返回插件提供的界面组件清单。
    // region 推荐值: top / left / center / right / bottom
    // screenIndex: 0=主屏，1=副屏（多屏时可继续扩展）
    virtual QList<UiContribution> contributions() const = 0;
};
