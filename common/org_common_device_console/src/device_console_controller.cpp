#include "device_console_controller.h"

#include <plugin_api/idevice_adapter.h>
#include <plugin_api/idevice_capabilities.h>
#include <plugin_api/idevice_panel_provider.h>
#include <plugin_api/idevice_registry.h>
#include <plugin_api/idata_model.h>
#include <plugin_api/idata_model_center.h>
#include <plugin_api/iplugin_manager.h>

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QCollator>
#include <QSerialPortInfo>
#include <QTimer>

#include <algorithm>

namespace {

QString normalizeMatchKey(const QString& value) {
    return value.trimmed().toLower();
}

QVariantList toVariantList(const QVariant& value) {
    if (!value.isValid() || value.isNull()) {
        return {};
    }

    if (value.userType() == QMetaType::QVariantList) {
        return value.toList();
    }

    if (value.userType() == QMetaType::QStringList) {
        QVariantList result;
        const QStringList values = value.toStringList();
        result.reserve(values.size());
        for (const QString& item : values) {
            result.push_back(item);
        }
        return result;
    }

    return {value};
}

bool scalarVariantEquals(const QVariant& lhs, const QVariant& rhs) {
    if (!lhs.isValid() || !rhs.isValid()) {
        return false;
    }

    if ((lhs.userType() == QMetaType::Bool || rhs.userType() == QMetaType::Bool)
        && lhs.canConvert<bool>() && rhs.canConvert<bool>()) {
        return lhs.toBool() == rhs.toBool();
    }

    const bool lhsNumeric = lhs.canConvert<double>() && lhs.userType() != QMetaType::QString;
    const bool rhsNumeric = rhs.canConvert<double>() && rhs.userType() != QMetaType::QString;
    if (lhsNumeric && rhsNumeric) {
        return lhs.toDouble() == rhs.toDouble();
    }

    if (lhs.canConvert<QString>() && rhs.canConvert<QString>()) {
        return normalizeMatchKey(lhs.toString()) == normalizeMatchKey(rhs.toString());
    }

    return lhs == rhs;
}

bool capabilityValueMatches(const QVariant& actual, const QVariant& expected) {
    if (!expected.isValid() || expected.isNull()) {
        return true;
    }

    if (expected.userType() == QMetaType::QVariantMap) {
        const QVariantMap expectedMap = expected.toMap();
        const QVariantMap actualMap = actual.toMap();
        if (actualMap.isEmpty() && !actual.canConvert<QVariantMap>()) {
            return false;
        }

        for (auto it = expectedMap.cbegin(); it != expectedMap.cend(); ++it) {
            if (!capabilityValueMatches(actualMap.value(it.key()), it.value())) {
                return false;
            }
        }
        return true;
    }

    const QVariantList expectedList = toVariantList(expected);
    if (expected.userType() == QMetaType::QVariantList || expected.userType() == QMetaType::QStringList) {
        const QVariantList actualList = toVariantList(actual);
        for (const QVariant& expectedItem : expectedList) {
            bool matched = false;
            for (const QVariant& actualItem : actualList) {
                if (capabilityValueMatches(actualItem, expectedItem)) {
                    matched = true;
                    break;
                }
            }
            if (!matched) {
                return false;
            }
        }
        return true;
    }

    if (actual.userType() == QMetaType::QVariantList || actual.userType() == QMetaType::QStringList) {
        const QVariantList actualList = toVariantList(actual);
        for (const QVariant& actualItem : actualList) {
            if (capabilityValueMatches(actualItem, expected)) {
                return true;
            }
        }
        return false;
    }

    return scalarVariantEquals(actual, expected);
}

bool capabilityFiltersMatch(const QVariantMap& actualCapabilities, const QVariantMap& filters) {
    for (auto it = filters.cbegin(); it != filters.cend(); ++it) {
        if (it.key().trimmed().isEmpty()) {
            continue;
        }
        if (!capabilityValueMatches(actualCapabilities.value(it.key()), it.value())) {
            return false;
        }
    }
    return true;
}

QString resolveDeviceType(const QString& deviceId, const QVariantMap& capabilities) {
    const QString explicitType = capabilities.value(QStringLiteral("deviceType")).toString().trimmed();
    if (!explicitType.isEmpty()) {
        return explicitType;
    }
    return deviceId.trimmed();
}

int panelSpecificity(const DevicePanelContribution& panel) {
    int specificity = 0;
    if (!panel.modelId.trimmed().isEmpty()) {
        specificity += 1000;
    }
    if (!panel.deviceType.trimmed().isEmpty()) {
        specificity += 200;
    }
    if (!panel.deviceId.trimmed().isEmpty()) {
        specificity += 150;
    }
    if (!panel.capabilityFilters.isEmpty()) {
        specificity += 50 + panel.capabilityFilters.size();
    }
    return specificity;
}

void sortPanelsByPriority(QList<DevicePanelContribution>& panels) {
    std::sort(panels.begin(), panels.end(), [](const DevicePanelContribution& lhs, const DevicePanelContribution& rhs) {
        const int lhsSpecificity = panelSpecificity(lhs);
        const int rhsSpecificity = panelSpecificity(rhs);
        if (lhsSpecificity != rhsSpecificity) {
            return lhsSpecificity > rhsSpecificity;
        }
        if (lhs.order != rhs.order) {
            return lhs.order < rhs.order;
        }
        return lhs.id < rhs.id;
    });
}

bool panelMatches(
    const DevicePanelContribution& panel,
    const QString& deviceId,
    const QString& deviceType,
    const QString& modelId,
    const QVariantMap& capabilities) {
    const QString panelDeviceId = normalizeMatchKey(panel.deviceId);
    const QString panelDeviceType = normalizeMatchKey(panel.deviceType);
    const QString panelModelId = normalizeMatchKey(panel.modelId);

    if (!panelDeviceId.isEmpty() && panelDeviceId != deviceId) {
        return false;
    }
    if (!panelDeviceType.isEmpty() && panelDeviceType != deviceType) {
        return false;
    }
    if (!panelModelId.isEmpty() && panelModelId != modelId) {
        return false;
    }
    if (!capabilityFiltersMatch(capabilities, panel.capabilityFilters)) {
        return false;
    }
    return true;
}

QString variantToDisplayText(const QVariant& value) {
    if (!value.isValid() || value.isNull()) {
        return QStringLiteral("(null)");
    }

    if (value.userType() == QMetaType::Bool) {
        return value.toBool() ? QStringLiteral("true") : QStringLiteral("false");
    }

    if (value.canConvert<QString>() && value.userType() != QMetaType::QVariantList
        && value.userType() != QMetaType::QVariantMap) {
        return value.toString();
    }

    const QJsonValue jsonValue = QJsonValue::fromVariant(value);
    if (jsonValue.isArray()) {
        return QString::fromUtf8(QJsonDocument(jsonValue.toArray()).toJson(QJsonDocument::Compact));
    }
    if (jsonValue.isObject()) {
        return QString::fromUtf8(QJsonDocument(jsonValue.toObject()).toJson(QJsonDocument::Compact));
    }
    if (jsonValue.isString()) {
        return jsonValue.toString();
    }
    if (jsonValue.isDouble()) {
        return QString::number(jsonValue.toDouble());
    }

    return value.toString();
}

QVariantList sortSnapshotEntries(const QVariantMap& snapshot) {
    QStringList keys = snapshot.keys();
    std::sort(keys.begin(), keys.end(), [](const QString& lhs, const QString& rhs) {
        return lhs < rhs;
    });

    QVariantList rows;
    rows.reserve(keys.size());
    for (const QString& key : keys) {
        QVariantMap row;
        row.insert(QStringLiteral("key"), key);
        row.insert(QStringLiteral("value"), snapshot.value(key));
        row.insert(QStringLiteral("display"), variantToDisplayText(snapshot.value(key)));
        rows.push_back(row);
    }
    return rows;
}

}  // namespace

DeviceConsoleController::DeviceConsoleController(QObject* parent)
    : QObject(parent) {}

void DeviceConsoleController::setPluginManager(IPluginManager* pluginManager) {
    pluginManager_ = pluginManager;
}

void DeviceConsoleController::start() {
    if (refreshTimer_ == nullptr) {
        refreshTimer_ = new QTimer(this);
        refreshTimer_->setInterval(500);
        connect(refreshTimer_, &QTimer::timeout, this, &DeviceConsoleController::refreshState);
    }

    refreshTimer_->start();
    refreshState();
}

void DeviceConsoleController::stop() {
    if (refreshTimer_ != nullptr) {
        refreshTimer_->stop();
    }
}

QVariantList DeviceConsoleController::devices() const {
    return devices_;
}

QVariantList DeviceConsoleController::panelDevices() const {
    QVariantList rows;
    rows.reserve(devices_.size());
    for (const QVariant& deviceRow : devices_) {
        const QVariantMap row = deviceRow.toMap();
        if (row.value(QStringLiteral("hasPanel")).toBool()) {
            rows.push_back(row);
        }
    }
    return rows;
}

QString DeviceConsoleController::selectedDeviceId() const {
    return selectedDeviceId_;
}

void DeviceConsoleController::setSelectedDeviceId(const QString& deviceId) {
    const QString normalized = deviceId.trimmed();
    if (selectedDeviceId_ == normalized) {
        return;
    }

    selectedDeviceId_ = normalized;
    selectedDevice_.clear();
    emit selectedDeviceIdChanged();
    emit selectedDeviceChanged();
    emit selectedDevicePageChanged();
    refreshState();
}

QVariantList DeviceConsoleController::selectedDevicePanels() const {
    return selectedDevicePanels_;
}

QString DeviceConsoleController::selectedDevicePanelId() const {
    return selectedDevicePanelId_;
}

void DeviceConsoleController::setSelectedDevicePanelId(const QString& panelId) {
    QString nextPanelId = panelId.trimmed();
    if (!nextPanelId.isEmpty()) {
        const bool exists = std::any_of(selectedDevicePanels_.cbegin(), selectedDevicePanels_.cend(), [&nextPanelId](const QVariant& row) {
            return row.toMap().value(QStringLiteral("id")).toString() == nextPanelId;
        });
        if (!exists) {
            return;
        }
    } else if (!selectedDevicePanels_.isEmpty()) {
        nextPanelId = selectedDevicePanels_.constFirst().toMap().value(QStringLiteral("id")).toString();
    }

    if (selectedDevicePanelId_ == nextPanelId) {
        return;
    }

    selectedDevicePanelId_ = nextPanelId;
    emit selectedDevicePanelIdChanged();
    emit selectedDevicePageChanged();
}

QString DeviceConsoleController::selectedDevicePage() const {
    for (const QVariant& rowValue : selectedDevicePanels_) {
        const QVariantMap row = rowValue.toMap();
        if (row.value(QStringLiteral("id")).toString() == selectedDevicePanelId_) {
            return row.value(QStringLiteral("qmlSource")).toString().trimmed();
        }
    }

    if (!selectedDevicePanels_.isEmpty()) {
        return selectedDevicePanels_.constFirst().toMap().value(QStringLiteral("qmlSource")).toString().trimmed();
    }
    return {};
}

QVariantMap DeviceConsoleController::selectedDevice() const {
    return selectedDevice_;
}

QVariantMap DeviceConsoleController::selectedSnapshot() const {
    return selectedSnapshot_;
}

QVariantList DeviceConsoleController::selectedSnapshotEntries() const {
    return selectedSnapshotEntries_;
}

QString DeviceConsoleController::lastActionResult() const {
    return lastActionResult_;
}

QVariantMap DeviceConsoleController::actionFeedback() const {
    return actionFeedback_;
}

QVariant DeviceConsoleController::snapshotValue(const QString& key) const {
    const QString trimmedKey = key.trimmed();
    if (trimmedKey.isEmpty()) {
        return {};
    }

    const auto it = selectedSnapshot_.constFind(trimmedKey);
    if (it == selectedSnapshot_.constEnd() || !it->isValid() || it->isNull()) {
        return {};
    }

    return *it;
}

QVariantList DeviceConsoleController::availableSerialPorts() const {
    QStringList portNames;
    const auto ports = QSerialPortInfo::availablePorts();
    portNames.reserve(ports.size());
    for (const QSerialPortInfo& port : ports) {
        const QString portName = port.portName().trimmed();
        if (portName.isEmpty()) {
            continue;
        }

        bool duplicate = false;
        for (const QString& existing : portNames) {
            if (existing.compare(portName, Qt::CaseInsensitive) == 0) {
                duplicate = true;
                break;
            }
        }
        if (!duplicate) {
            portNames.push_back(portName);
        }
    }

    QCollator collator;
    collator.setNumericMode(true);
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    std::sort(portNames.begin(), portNames.end(), [&collator](const QString& lhs, const QString& rhs) {
        return collator.compare(lhs, rhs) < 0;
    });

    QVariantList values;
    values.reserve(portNames.size());
    for (const QString& portName : portNames) {
        values.push_back(portName);
    }
    return values;
}

QVariantMap DeviceConsoleController::currentDeviceContext() const {
    QVariantMap device = selectedDevice_;
    if (!selectedDeviceId_.isEmpty()) {
        device.insert(QStringLiteral("id"), selectedDeviceId_);
    }

    if (auto* adapter = currentAdapter(); adapter != nullptr) {
        QVariantMap capabilities = device.value(QStringLiteral("capabilities")).toMap();
        if (capabilities.isEmpty()) {
            capabilities = adapter->capabilities();
            if (!capabilities.isEmpty()) {
                device.insert(QStringLiteral("capabilities"), capabilities);
            }
        }

        if (device.value(QStringLiteral("title")).toString().trimmed().isEmpty()) {
            device.insert(QStringLiteral("title"), adapter->displayName());
        }
        if (device.value(QStringLiteral("modelId")).toString().trimmed().isEmpty()) {
            device.insert(QStringLiteral("modelId"), adapter->modelId());
        }
        if (device.value(QStringLiteral("deviceType")).toString().trimmed().isEmpty()) {
            device.insert(
                QStringLiteral("deviceType"),
                resolveDeviceType(adapter->deviceId(), capabilities));
        }
    }

    return device;
}

QList<DevicePanelContribution> DeviceConsoleController::availableDevicePanels() const {
    QList<DevicePanelContribution> panels;
    if (pluginManager_ == nullptr) {
        return panels;
    }

    const QStringList serviceNames =
        pluginManager_->findServiceNames<IDevicePanelProvider>(QStringLiteral("device_panel/"));
    for (const QString& serviceName : serviceNames) {
        auto* provider = pluginManager_->getService<IDevicePanelProvider>(serviceName);
        if (provider == nullptr) {
            continue;
        }

        const QList<DevicePanelContribution> providerPanels = provider->panels();
        for (const DevicePanelContribution& panel : providerPanels) {
            if (panel.id.trimmed().isEmpty() || panel.qmlSource.trimmed().isEmpty()) {
                continue;
            }
            panels.push_back(panel);
        }
    }

    return panels;
}

QList<DevicePanelContribution> DeviceConsoleController::matchingDevicePanels(
    const QVariantMap& device,
    const QList<DevicePanelContribution>& panels) const {
    const QString deviceId = normalizeMatchKey(device.value(QStringLiteral("id")).toString());
    const QString deviceType = normalizeMatchKey(device.value(QStringLiteral("deviceType")).toString());
    const QString modelId = normalizeMatchKey(device.value(QStringLiteral("modelId")).toString());
    const QVariantMap capabilities = device.value(QStringLiteral("capabilities")).toMap();
    if (deviceId.isEmpty() && deviceType.isEmpty() && modelId.isEmpty() && capabilities.isEmpty()) {
        return {};
    }

    QList<DevicePanelContribution> candidates;
    for (const DevicePanelContribution& panel : panels) {
        if (panelMatches(panel, deviceId, deviceType, modelId, capabilities)) {
            candidates.push_back(panel);
        }
    }

    sortPanelsByPriority(candidates);
    return candidates;
}

DevicePanelContribution DeviceConsoleController::resolveDevicePanel(
    const QVariantMap& device,
    const QList<DevicePanelContribution>& panels) const {
    const QList<DevicePanelContribution> candidates = matchingDevicePanels(device, panels);
    if (candidates.isEmpty()) {
        return {};
    }

    return candidates.constFirst();
}

void DeviceConsoleController::refreshNow() {
    refreshState();
}

QVariantList DeviceConsoleController::serialPorts() const {
    QVariantList rows;
    const QVariantList portNames = availableSerialPorts();
    const QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    rows.reserve(portNames.size());

    for (const QVariant& portNameValue : portNames) {
        const QString portName = portNameValue.toString().trimmed();
        if (portName.isEmpty()) {
            continue;
        }

        QString description;
        QString manufacturer;
        QString systemLocation;
        for (const QSerialPortInfo& portInfo : ports) {
            if (portInfo.portName().trimmed().compare(portName, Qt::CaseInsensitive) == 0) {
                description = portInfo.description().trimmed();
                manufacturer = portInfo.manufacturer().trimmed();
                systemLocation = portInfo.systemLocation().trimmed();
                break;
            }
        }

        QVariantMap row;
        row.insert(QStringLiteral("portName"), portName);
        row.insert(QStringLiteral("systemLocation"), systemLocation);
        row.insert(QStringLiteral("description"), description);
        row.insert(QStringLiteral("manufacturer"), manufacturer);
        row.insert(
            QStringLiteral("displayText"),
            description.isEmpty() ? portName : QStringLiteral("%1 (%2)").arg(portName, description));
        rows.push_back(row);
    }
    return rows;
}

bool DeviceConsoleController::connectSerial(const QString& portName, int baudRate, int dataBits, int parity, int stopBits) {
    QVariantMap args;
    args.insert(QStringLiteral("transport"), QStringLiteral("serial"));
    args.insert(QStringLiteral("portName"), portName.trimmed());
    args.insert(QStringLiteral("baudRate"), baudRate);
    args.insert(QStringLiteral("dataBits"), dataBits);
    args.insert(QStringLiteral("parity"), parity);
    args.insert(QStringLiteral("stopBits"), stopBits);
    return invokeCurrent(QString::fromLatin1(device_api::actions::kConnectSerial), args);
}

bool DeviceConsoleController::connectUdp(const QString& remoteAddress, int remotePort, const QString& localAddress, int localPort, int dataType) {
    QVariantMap args;
    args.insert(QStringLiteral("transport"), QStringLiteral("udp"));
    args.insert(QStringLiteral("remoteAddress"), remoteAddress.trimmed());
    args.insert(QStringLiteral("remotePort"), remotePort);
    args.insert(QStringLiteral("localAddress"), localAddress.trimmed());
    args.insert(QStringLiteral("localPort"), localPort);
    args.insert(QStringLiteral("dataType"), dataType);
    return invokeCurrent(QString::fromLatin1(device_api::actions::kConnectUdp), args);
}

void DeviceConsoleController::disconnectCurrent() {
    invokeCurrent(QString::fromLatin1(device_api::actions::kDisconnect));
}

bool DeviceConsoleController::sendRawHex(const QString& hexText) {
    QVariantMap args;
    args.insert(QStringLiteral("hex"), hexText.trimmed());
    const bool ok = invokeCurrent(QString::fromLatin1(device_api::actions::kSendRawHex), args);
    return ok;
}

bool DeviceConsoleController::invokeCurrent(const QString& action, const QVariantMap& args) {
    auto* adapter = currentAdapter();
    if (adapter == nullptr) {
        const QString message = QStringLiteral("未找到当前设备适配器");
        setLastActionResult(message);
        setActionFeedback(action, false, message);
        refreshState();
        return false;
    }

    bool ok = false;
    if (action == QString::fromLatin1(device_api::actions::kConnectSerial)
        || action == QString::fromLatin1(device_api::actions::kConnectUdp)) {
        ok = adapter->connectDevice(args);
    } else if (action == QString::fromLatin1(device_api::actions::kDisconnect)) {
        adapter->disconnectDevice();
        ok = true;
    } else {
        ok = adapter->invoke(action, args);
    }

    QString hex;
    QString errorMsg;
    if (auto* model = currentModel(); model != nullptr) {
        const QVariantMap snap = model->snapshot();
        hex = snap.value(QStringLiteral("transport.lastTxHex")).toString();
        if (hex.isEmpty()) hex = snap.value(QStringLiteral("transport.lastTxFrameHex")).toString();
        if (hex.isEmpty()) hex = snap.value(QStringLiteral("tx.lastFrameHex")).toString();
        if (hex.isEmpty()) hex = snap.value(QStringLiteral("eddy.last.payloadHex")).toString();
        if (hex.isEmpty()) hex = snap.value(QStringLiteral("lastTxHex")).toString();

        if (!ok) {
            errorMsg = snap.value(QStringLiteral("status.lastError")).toString();
            if (errorMsg.isEmpty()) errorMsg = snap.value(QStringLiteral("transport.lastError")).toString();
        }
    }

    QString message = QStringLiteral("%1 -> %2").arg(action, ok ? QStringLiteral("成功") : QStringLiteral("失败"));
    if (!ok && !errorMsg.isEmpty()) {
        message += QStringLiteral(" (%1)").arg(errorMsg);
    }

    setLastActionResult(message);
    setActionFeedback(action, ok, message, hex);
    refreshState();
    return ok;
}

void DeviceConsoleController::refreshState() {
    updateDevices();
    updateSelectedDevicePanels();
    updateSelectedSnapshot();
}

IDeviceRegistry* DeviceConsoleController::deviceRegistry() const {
    return pluginManager_ == nullptr
        ? nullptr
        : pluginManager_->getService<IDeviceRegistry>(QStringLiteral("device_registry"));
}

IDataModelCenter* DeviceConsoleController::dataModelCenter() const {
    return pluginManager_ == nullptr
        ? nullptr
        : pluginManager_->getService<IDataModelCenter>(QStringLiteral("data_model_center"));
}

IDeviceAdapter* DeviceConsoleController::currentAdapter() const {
    auto* registry = deviceRegistry();
    if (registry == nullptr || selectedDeviceId_.isEmpty()) {
        return nullptr;
    }
    return registry->adapter(selectedDeviceId_);
}

IDataModel* DeviceConsoleController::currentModel() const {
    auto* adapter = currentAdapter();
    auto* center = dataModelCenter();
    if (adapter == nullptr || center == nullptr) {
        return nullptr;
    }
    return center->model(adapter->modelId());
}

void DeviceConsoleController::updateDevices() {
    QVariantList rows;
    const QList<DevicePanelContribution> panels = availableDevicePanels();
    auto* registry = deviceRegistry();
    if (registry != nullptr) {
        const QStringList ids = registry->deviceIds();
        for (const QString& id : ids) {
            auto* adapter = registry->adapter(id);
            if (adapter == nullptr) {
                continue;
            }

            const QVariantMap caps = adapter->capabilities();
            QVariantMap row;
            row.insert(QStringLiteral("id"), adapter->deviceId());
            row.insert(QStringLiteral("title"), adapter->displayName());
            row.insert(QStringLiteral("connected"), adapter->isConnected());
            row.insert(QStringLiteral("deviceType"), resolveDeviceType(adapter->deviceId(), caps));
            row.insert(QStringLiteral("modelId"), adapter->modelId());
            row.insert(QStringLiteral("transports"), caps.value(QStringLiteral("transports")).toList());
            row.insert(QStringLiteral("actions"), caps.value(QStringLiteral("actions")).toList());
            row.insert(QStringLiteral("capabilities"), caps);
            const DevicePanelContribution panel = resolveDevicePanel(row, panels);
            row.insert(QStringLiteral("hasPanel"), !panel.qmlSource.trimmed().isEmpty());
            row.insert(QStringLiteral("panelId"), panel.id);
            row.insert(QStringLiteral("panelTitle"), panel.title);
            row.insert(QStringLiteral("panelQmlSource"), panel.qmlSource);
            rows.push_back(row);
        }
    }

    std::sort(rows.begin(), rows.end(), [](const QVariant& lhs, const QVariant& rhs) {
        return lhs.toMap().value(QStringLiteral("id")).toString()
            < rhs.toMap().value(QStringLiteral("id")).toString();
    });

    if (devices_ != rows) {
        devices_ = rows;
        emit devicesChanged();
        emit selectedDevicePageChanged();
    }

    QString nextSelected = selectedDeviceId_;
    if (!devices_.isEmpty()) {
        const bool exists = std::any_of(devices_.cbegin(), devices_.cend(), [this](const QVariant& row) {
            return row.toMap().value(QStringLiteral("id")).toString() == selectedDeviceId_;
        });
        if (!exists) {
            nextSelected = devices_.constFirst().toMap().value(QStringLiteral("id")).toString();
        }
    } else {
        nextSelected.clear();
    }

    if (selectedDeviceId_ != nextSelected) {
        selectedDeviceId_ = nextSelected;
        emit selectedDeviceIdChanged();
        emit selectedDevicePageChanged();
    }

    QVariantMap selected;
    for (const QVariant& row : devices_) {
        const QVariantMap map = row.toMap();
        if (map.value(QStringLiteral("id")).toString() == selectedDeviceId_) {
            selected = map;
            break;
        }
    }

    if (selectedDevice_ != selected) {
        selectedDevice_ = selected;
        emit selectedDeviceChanged();
        emit selectedDevicePageChanged();
    }
}

void DeviceConsoleController::updateSelectedDevicePanels() {
    QVariantList rows;
    const QList<DevicePanelContribution> panels = matchingDevicePanels(selectedDevice_, availableDevicePanels());
    rows.reserve(panels.size());
    for (const DevicePanelContribution& panel : panels) {
        QVariantMap row;
        row.insert(QStringLiteral("id"), panel.id);
        row.insert(QStringLiteral("title"), panel.title);
        row.insert(QStringLiteral("deviceId"), panel.deviceId);
        row.insert(QStringLiteral("deviceType"), panel.deviceType);
        row.insert(QStringLiteral("modelId"), panel.modelId);
        row.insert(QStringLiteral("qmlSource"), panel.qmlSource);
        row.insert(QStringLiteral("order"), panel.order);
        rows.push_back(row);
    }

    if (selectedDevicePanels_ != rows) {
        selectedDevicePanels_ = rows;
        emit selectedDevicePanelsChanged();
        emit selectedDevicePageChanged();
    }

    QString nextPanelId = selectedDevicePanelId_;
    const bool exists = std::any_of(selectedDevicePanels_.cbegin(), selectedDevicePanels_.cend(), [&nextPanelId](const QVariant& rowValue) {
        return rowValue.toMap().value(QStringLiteral("id")).toString() == nextPanelId;
    });
    if (!exists) {
        nextPanelId = selectedDevicePanels_.isEmpty()
            ? QString()
            : selectedDevicePanels_.constFirst().toMap().value(QStringLiteral("id")).toString();
    }

    if (selectedDevicePanelId_ != nextPanelId) {
        selectedDevicePanelId_ = nextPanelId;
        emit selectedDevicePanelIdChanged();
        emit selectedDevicePageChanged();
    }
}

void DeviceConsoleController::updateSelectedSnapshot() {
    QVariantMap snapshot;
    if (auto* model = currentModel(); model != nullptr) {
        snapshot = model->snapshot();
    }

    if (selectedSnapshot_ != snapshot) {
        selectedSnapshot_ = snapshot;
        emit selectedSnapshotChanged();
    }

    const QVariantList entries = sortSnapshotEntries(snapshot);
    if (selectedSnapshotEntries_ != entries) {
        selectedSnapshotEntries_ = entries;
        emit selectedSnapshotChanged();
    }
}

void DeviceConsoleController::setLastActionResult(QString text) {
    if (lastActionResult_ == text) {
        return;
    }

    lastActionResult_ = std::move(text);
    emit lastActionResultChanged();
}

void DeviceConsoleController::setActionFeedback(const QString& action, bool ok, const QString& message, const QString& hex) {
    QVariantMap feedback;
    feedback.insert(QStringLiteral("action"), action);
    feedback.insert(QStringLiteral("ok"), ok);
    feedback.insert(QStringLiteral("message"), message);
    feedback.insert(QStringLiteral("hex"), hex);
    feedback.insert(QStringLiteral("deviceId"), selectedDeviceId_);
    feedback.insert(
        QStringLiteral("timestamp"),
        QDateTime::currentDateTime().toString(Qt::ISODate));

    if (actionFeedback_ == feedback) {
        return;
    }

    actionFeedback_ = feedback;
    emit actionFeedbackChanged();
}
