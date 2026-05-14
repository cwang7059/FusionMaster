#pragma once

#include <plugin_api/idevice_panel_provider.h>

#include <QObject>
#include <QVariantList>
#include <QVariantMap>

class IDeviceAdapter;
class IDeviceRegistry;
class IDataModel;
class IDataModelCenter;
class IPluginManager;
class QTimer;

class DeviceConsoleController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList devices READ devices NOTIFY devicesChanged)
    Q_PROPERTY(QVariantList panelDevices READ panelDevices NOTIFY devicesChanged)
    Q_PROPERTY(QString selectedDeviceId READ selectedDeviceId WRITE setSelectedDeviceId NOTIFY selectedDeviceIdChanged)
    Q_PROPERTY(QVariantList selectedDevicePanels READ selectedDevicePanels NOTIFY selectedDevicePanelsChanged)
    Q_PROPERTY(QString selectedDevicePanelId READ selectedDevicePanelId WRITE setSelectedDevicePanelId NOTIFY selectedDevicePanelIdChanged)
    Q_PROPERTY(QString selectedDevicePage READ selectedDevicePage NOTIFY selectedDevicePageChanged)
    Q_PROPERTY(QVariantMap selectedDevice READ selectedDevice NOTIFY selectedDeviceChanged)
    Q_PROPERTY(QVariantMap selectedSnapshot READ selectedSnapshot NOTIFY selectedSnapshotChanged)
    Q_PROPERTY(QVariantList selectedSnapshotEntries READ selectedSnapshotEntries NOTIFY selectedSnapshotChanged)
    Q_PROPERTY(QString lastActionResult READ lastActionResult NOTIFY lastActionResultChanged)
    Q_PROPERTY(QVariantMap actionFeedback READ actionFeedback NOTIFY actionFeedbackChanged)

public:
    explicit DeviceConsoleController(QObject* parent = nullptr);

    void setPluginManager(IPluginManager* pluginManager);
    void start();
    void stop();

    QVariantList devices() const;
    QVariantList panelDevices() const;
    QString selectedDeviceId() const;
    void setSelectedDeviceId(const QString& deviceId);
    QVariantList selectedDevicePanels() const;
    QString selectedDevicePanelId() const;
    void setSelectedDevicePanelId(const QString& panelId);
    QString selectedDevicePage() const;
    QVariantMap selectedDevice() const;
    QVariantMap selectedSnapshot() const;
    QVariantList selectedSnapshotEntries() const;
    QString lastActionResult() const;
    QVariantMap actionFeedback() const;

    Q_INVOKABLE void refreshNow();
    Q_INVOKABLE QVariant snapshotValue(const QString& key) const;
    Q_INVOKABLE QVariantList availableSerialPorts() const;
    Q_INVOKABLE QVariantList serialPorts() const;
    Q_INVOKABLE bool connectSerial(const QString& portName, int baudRate = 115200, int dataBits = 8, int parity = 0, int stopBits = 1);
    Q_INVOKABLE bool connectUdp(const QString& remoteAddress, int remotePort, const QString& localAddress = QString(), int localPort = 0, int dataType = 1);
    Q_INVOKABLE void disconnectCurrent();
    Q_INVOKABLE bool sendRawHex(const QString& hexText);
    Q_INVOKABLE bool invokeCurrent(const QString& action, const QVariantMap& args = QVariantMap());

signals:
    void devicesChanged();
    void selectedDeviceIdChanged();
    void selectedDevicePanelsChanged();
    void selectedDevicePanelIdChanged();
    void selectedDevicePageChanged();
    void selectedDeviceChanged();
    void selectedSnapshotChanged();
    void lastActionResultChanged();
    void actionFeedbackChanged();

private:
    QVariantMap currentDeviceContext() const;
    QList<DevicePanelContribution> availableDevicePanels() const;
    QList<DevicePanelContribution> matchingDevicePanels(
        const QVariantMap& device,
        const QList<DevicePanelContribution>& panels) const;
    DevicePanelContribution resolveDevicePanel(
        const QVariantMap& device,
        const QList<DevicePanelContribution>& panels) const;
    void refreshState();
    IDeviceRegistry* deviceRegistry() const;
    IDataModelCenter* dataModelCenter() const;
    IDeviceAdapter* currentAdapter() const;
    IDataModel* currentModel() const;
    void updateDevices();
    void updateSelectedDevicePanels();
    void updateSelectedSnapshot();
    void setLastActionResult(QString text);
    void setActionFeedback(const QString& action, bool ok, const QString& message, const QString& hex = QString());

private:
    IPluginManager* pluginManager_ = nullptr;
    QTimer* refreshTimer_ = nullptr;
    QVariantList devices_;
    QString selectedDeviceId_;
    QVariantList selectedDevicePanels_;
    QString selectedDevicePanelId_;
    QVariantMap selectedDevice_;
    QVariantMap selectedSnapshot_;
    QVariantList selectedSnapshotEntries_;
    QString lastActionResult_;
    QVariantMap actionFeedback_;
};
