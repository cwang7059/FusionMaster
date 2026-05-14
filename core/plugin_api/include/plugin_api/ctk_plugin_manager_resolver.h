#pragma once

#include <plugin_api/ctk_bridge.h>
#include <plugin_api/iplugin_manager.h>

#include <ctkPluginContext.h>
#include <ctkServiceReference.h>

#include <QObject>
#include <QString>
#include <QVariant>

namespace plugin_api::ctk_bridge {

inline IPluginManager* resolvePluginManager(ctkPluginContext* context) {
    if (context == nullptr) {
        return nullptr;
    }

    const ctkServiceReference reference =
        context->getServiceReference(QString::fromUtf8(kPluginManagerBridgeServiceClass));
    if (!reference) {
        return nullptr;
    }

    QObject* serviceObject = context->getService(reference);
    IPluginManager* manager = nullptr;

    if (serviceObject != nullptr) {
        bool ok = false;
        const qulonglong rawPtr = serviceObject->property(kPluginManagerPointerProperty).toULongLong(&ok);
        if (ok && rawPtr != 0) {
            manager = reinterpret_cast<IPluginManager*>(rawPtr);
        }
    }

    context->ungetService(reference);
    return manager;
}

}  // namespace plugin_api::ctk_bridge
