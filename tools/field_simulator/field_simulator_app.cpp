#include "field_simulator_app.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>

#include <cmath>

namespace {

QString normalizeBaseUrl(QString url) {
    url = url.trimmed();
    while (url.endsWith('/')) {
        url.chop(1);
    }
    return url;
}

QString shortNow() {
    return QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss.zzz"));
}

}  // namespace

FieldSimulatorApp::FieldSimulatorApp(Config config, QObject* parent)
    : QObject(parent), config_(std::move(config)) {
    connect(&udpTimer_, &QTimer::timeout, this, &FieldSimulatorApp::emitUdpPackets);
    connect(&autoStopTimer_, &QTimer::timeout, this, [this]() {
        qInfo().noquote() << QStringLiteral("[%1] 到达 duration，停止模拟器").arg(shortNow());
        stopUdpEmitter();
        stopRtspWorkers();
        emit finished(0);
    });
    autoStopTimer_.setSingleShot(true);
}

FieldSimulatorApp::~FieldSimulatorApp() {
    stopUdpEmitter();
    stopRtspWorkers();
}

bool FieldSimulatorApp::validateConfig(QString* errorMessage) const {
    if (config_.streamNames.isEmpty()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("streamNames 不能为空");
        }
        return false;
    }

    if (config_.enableRtsp) {
        if (config_.ffmpegPath.trimmed().isEmpty() || !QFileInfo::exists(config_.ffmpegPath)) {
            if (errorMessage != nullptr) {
                *errorMessage = QStringLiteral("未找到 ffmpeg.exe，请通过 --ffmpeg 指定");
            }
            return false;
        }
        if (config_.mediamtxBaseUrl.trimmed().isEmpty()) {
            if (errorMessage != nullptr) {
                *errorMessage = QStringLiteral("mediamtx 基地址不能为空");
            }
            return false;
        }
    }

    if (config_.enableUdp) {
        if (config_.udpHost.trimmed().isEmpty()) {
            if (errorMessage != nullptr) {
                *errorMessage = QStringLiteral("udpHost 不能为空");
            }
            return false;
        }
        if (config_.udpPort == 0) {
            if (errorMessage != nullptr) {
                *errorMessage = QStringLiteral("udpPort 不能为 0");
            }
            return false;
        }
        if (config_.udpIntervalMs < 10) {
            if (errorMessage != nullptr) {
                *errorMessage = QStringLiteral("udpIntervalMs 不能小于 10");
            }
            return false;
        }
    }

    return true;
}

bool FieldSimulatorApp::start(QString* errorMessage) {
    if (config_.dryRun) {
        qInfo().noquote() << QStringLiteral("[%1] dry-run: 现场模拟器配置检查").arg(shortNow());
        qInfo().noquote() << QStringLiteral("RTSP 地址提示: %1").arg(buildRtspEnvHint());
        qInfo().noquote() << QStringLiteral("RTSP=%1 UDP=%2 duration=%3s")
                                 .arg(config_.enableRtsp ? QStringLiteral("ON") : QStringLiteral("OFF"))
                                 .arg(config_.enableUdp ? QStringLiteral("ON") : QStringLiteral("OFF"))
                                 .arg(config_.durationSec);
        emit finished(0);
        return true;
    }

    QString localError;
    if (!validateConfig(&localError)) {
        if (errorMessage != nullptr) {
            *errorMessage = localError;
        }
        return false;
    }

    qInfo().noquote() << QStringLiteral("[%1] 现场模拟器启动").arg(shortNow());
    qInfo().noquote() << QStringLiteral("RTSP 地址提示: %1").arg(buildRtspEnvHint());

    elapsed_.start();

    if (config_.enableRtsp) {
        if (!startRtspWorkers(&localError)) {
            if (errorMessage != nullptr) {
                *errorMessage = localError;
            }
            return false;
        }
    }

    if (config_.enableUdp) {
        startUdpEmitter();
    }

    if (config_.durationSec > 0) {
        autoStopTimer_.start(config_.durationSec * 1000);
    }

    return true;
}

bool FieldSimulatorApp::startRtspWorkers(QString* errorMessage) {
    const QStringList sourceFilters = {
        QStringLiteral("testsrc=size=1280x720:rate=%1").arg(config_.fps),
        QStringLiteral("testsrc2=size=1280x720:rate=%1").arg(config_.fps),
        QStringLiteral("smptebars=size=1280x720:rate=%1").arg(config_.fps)
    };

    for (int i = 0; i < config_.streamNames.size(); ++i) {
        const QString streamName = config_.streamNames.at(i).trimmed();
        if (streamName.isEmpty()) {
            continue;
        }

        const QString rtspUrl = buildRtspUrl(streamName);
        const QString sourceFilter = sourceFilters.at(i % sourceFilters.size());

        QStringList args;
        const QString x264Params = QStringLiteral("repeat-headers=1:aud=1:keyint=%1:min-keyint=%1:scenecut=0")
            .arg(config_.fps);
        args << QStringLiteral("-hide_banner")
             << QStringLiteral("-loglevel") << QStringLiteral("warning")
             << QStringLiteral("-re")
             << QStringLiteral("-f") << QStringLiteral("lavfi")
             << QStringLiteral("-i") << sourceFilter
             << QStringLiteral("-an")
             << QStringLiteral("-c:v") << QStringLiteral("libx264")
             << QStringLiteral("-preset") << QStringLiteral("ultrafast")
             << QStringLiteral("-tune") << QStringLiteral("zerolatency")
             << QStringLiteral("-x264-params") << x264Params
             << QStringLiteral("-pix_fmt") << QStringLiteral("yuv420p")
             << QStringLiteral("-g") << QString::number(config_.fps)
             << QStringLiteral("-f") << QStringLiteral("rtsp")
             << QStringLiteral("-rtsp_transport") << QStringLiteral("tcp")
             << rtspUrl;

        auto* process = new QProcess(this);
        process->setProgram(config_.ffmpegPath);
        process->setArguments(args);

        connect(process, &QProcess::errorOccurred, this, [streamName](QProcess::ProcessError err) {
            qWarning().noquote() << QStringLiteral("[field_simulator] ffmpeg(%1) 错误: %2").arg(streamName).arg(static_cast<int>(err));
        });
        connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            [streamName](int code, QProcess::ExitStatus status) {
                qWarning().noquote()
                    << QStringLiteral("[field_simulator] ffmpeg(%1) 退出 code=%2 status=%3")
                           .arg(streamName)
                           .arg(code)
                           .arg(static_cast<int>(status));
            });

        process->start();
        if (!process->waitForStarted(5000)) {
            const QString err = QStringLiteral("ffmpeg 启动失败: %1 -> %2")
                                    .arg(streamName, rtspUrl);
            if (errorMessage != nullptr) {
                *errorMessage = err;
            }
            delete process;
            stopRtspWorkers();
            return false;
        }

        ffmpegProcesses_.push_back(process);
        qInfo().noquote() << QStringLiteral("[field_simulator] RTSP 推流已启动: %1").arg(rtspUrl);
    }

    return true;
}

void FieldSimulatorApp::stopRtspWorkers() {
    for (QProcess* process : ffmpegProcesses_) {
        if (process == nullptr) {
            continue;
        }
        if (process->state() != QProcess::NotRunning) {
            process->terminate();
            if (!process->waitForFinished(1500)) {
                process->kill();
                process->waitForFinished(1500);
            }
        }
        process->deleteLater();
    }
    ffmpegProcesses_.clear();
}

void FieldSimulatorApp::startUdpEmitter() {
    udpTimer_.setInterval(config_.udpIntervalMs);
    udpTimer_.start();
    qInfo().noquote() << QStringLiteral("[field_simulator] UDP 数据发送已启动: %1:%2, interval=%3ms")
                             .arg(config_.udpHost)
                             .arg(config_.udpPort)
                             .arg(config_.udpIntervalMs);
}

void FieldSimulatorApp::stopUdpEmitter() {
    if (udpTimer_.isActive()) {
        udpTimer_.stop();
    }
}

void FieldSimulatorApp::emitUdpPackets() {
    const double t = static_cast<double>(elapsed_.elapsed()) / 1000.0;
    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();

    QJsonObject drone;
    drone.insert(QStringLiteral("type"), QStringLiteral("drone"));
    drone.insert(QStringLiteral("device_id"), QStringLiteral("uav_001"));
    drone.insert(QStringLiteral("ts_ms"), nowMs);
    drone.insert(QStringLiteral("seq"), static_cast<qint64>(packetSeq_));
    drone.insert(QStringLiteral("lat"), 31.2304 + 0.001 * std::sin(t / 7.0));
    drone.insert(QStringLiteral("lon"), 121.4737 + 0.001 * std::cos(t / 7.0));
    drone.insert(QStringLiteral("alt"), 120.0 + 8.0 * std::sin(t / 2.0));
    drone.insert(QStringLiteral("yaw"), std::fmod(t * 18.0, 360.0));
    drone.insert(QStringLiteral("speed_mps"), 22.0 + 1.5 * std::sin(t));

    QJsonObject turntable;
    turntable.insert(QStringLiteral("type"), QStringLiteral("turntable"));
    turntable.insert(QStringLiteral("device_id"), QStringLiteral("turntable_001"));
    turntable.insert(QStringLiteral("ts_ms"), nowMs);
    turntable.insert(QStringLiteral("seq"), static_cast<qint64>(packetSeq_));
    turntable.insert(QStringLiteral("azimuth"), std::fmod(t * 12.0, 360.0));
    turntable.insert(QStringLiteral("elevation"), 10.0 + 4.0 * std::sin(t / 3.0));
    turntable.insert(QStringLiteral("tracking"), true);

    const QByteArray droneData = QJsonDocument(drone).toJson(QJsonDocument::Compact);
    const QByteArray turntableData = QJsonDocument(turntable).toJson(QJsonDocument::Compact);

    udpSocket_.writeDatagram(droneData, QHostAddress(config_.udpHost), config_.udpPort);
    udpSocket_.writeDatagram(turntableData, QHostAddress(config_.udpHost), config_.udpPort);

    ++packetSeq_;
    if ((packetSeq_ % 100) == 0) {
        qInfo().noquote() << QStringLiteral("[field_simulator] UDP 已发送 seq=%1").arg(packetSeq_);
    }
}

QString FieldSimulatorApp::buildRtspUrl(const QString& streamName) const {
    return QStringLiteral("%1/live/%2")
        .arg(normalizeBaseUrl(config_.mediamtxBaseUrl), streamName);
}

QString FieldSimulatorApp::buildRtspEnvHint() const {
    QStringList parts;
    for (const QString& streamName : config_.streamNames) {
        const QString trimmed = streamName.trimmed();
        if (trimmed.isEmpty()) {
            continue;
        }
        parts.push_back(QStringLiteral("%1=%2").arg(trimmed, buildRtspUrl(trimmed)));
    }
    return QStringLiteral("OSGI_RTSP_URLS=\"") + parts.join(QStringLiteral(";")) + QStringLiteral("\"");
}
