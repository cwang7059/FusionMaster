#pragma once

#include <QByteArray>
#include <QVariantMap>
#include <QString>

class IDeviceTransport {
public:
    virtual ~IDeviceTransport() = default;

    virtual QString transportType() const = 0;
    virtual bool open(const QVariantMap& options) = 0;
    virtual void close() = 0;
    virtual bool isOpen() const = 0;
    virtual bool send(const QByteArray& payload) = 0;
};
