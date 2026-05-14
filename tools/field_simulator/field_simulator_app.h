#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QProcess>
#include <QTimer>
#include <QUdpSocket>

class FieldSimulatorApp final : public QObject {
    Q_OBJECT

public:
    struct Config {
        QString ffmpegPath;
        QString mediamtxBaseUrl;
        QStringList streamNames;
        QString udpHost;
        quint16 udpPort = 50001;
        int fps = 25;
        int udpIntervalMs = 50;
        int durationSec = 0;
        bool enableRtsp = true;
        bool enableUdp = true;
        bool dryRun = false;
    };

    explicit FieldSimulatorApp(Config config, QObject* parent = nullptr);
    ~FieldSimulatorApp() override;

    bool start(QString* errorMessage);

signals:
    void finished(int exitCode);

private:
    bool validateConfig(QString* errorMessage) const;
    bool startRtspWorkers(QString* errorMessage);
    void stopRtspWorkers();
    void startUdpEmitter();
    void stopUdpEmitter();
    void emitUdpPackets();

    QString buildRtspUrl(const QString& streamName) const;
    QString buildRtspEnvHint() const;

private:
    Config config_;
    QList<QProcess*> ffmpegProcesses_;
    QUdpSocket udpSocket_;
    QTimer udpTimer_;
    QTimer autoStopTimer_;
    QElapsedTimer elapsed_;
    quint64 packetSeq_ = 0;
};
