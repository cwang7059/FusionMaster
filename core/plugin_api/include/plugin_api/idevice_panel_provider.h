#pragma once

#include <QList>
#include <QString>
#include <QVariantMap>

struct DevicePanelContribution {
    QString id;
    QString title;
    QString deviceId;
    QString deviceType;
    QString modelId;
    QString qmlSource;
    QVariantMap capabilityFilters;
    int order = 0;
};

class IDevicePanelProvider {
public:
    virtual ~IDevicePanelProvider() = default;

    virtual QList<DevicePanelContribution> panels() const = 0;
};
