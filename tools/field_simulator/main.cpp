#include "field_simulator_app.h"

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QFileInfo>
#include <QDir>
#include <QTimer>

namespace {

QStringList splitCsv(const QString& raw) {
    QStringList values;
    for (const QString& item : raw.split(',', Qt::SkipEmptyParts)) {
        const QString trimmed = item.trimmed();
        if (!trimmed.isEmpty()) {
            values.push_back(trimmed);
        }
    }
    return values;
}

QString findDefaultFfmpegPath() {
    const QString explicitPath = qEnvironmentVariable("OSGI_FFMPEG_EXE").trimmed();
    if (!explicitPath.isEmpty() && QFileInfo::exists(explicitPath)) {
        return QDir::toNativeSeparators(QFileInfo(explicitPath).absoluteFilePath());
    }

    QStringList candidates;
    const QString macroDir = QStringLiteral(OSGI_FFMPEG_BIN_DIR).trimmed();
    if (!macroDir.isEmpty()) {
        candidates.push_back(QDir::fromNativeSeparators(macroDir) + QStringLiteral("/ffmpeg.exe"));
    }
    candidates << QStringLiteral("third_party/ffmpeg/win-x64-qt5-vs2019/bin/ffmpeg.exe")
               << QStringLiteral("third_party/ffmpeg/win-x64-qt6-vs2026/bin/ffmpeg.exe");

    for (QString candidate : candidates) {
        const QFileInfo info(QDir::cleanPath(candidate));
        const QString absolutePath = info.isAbsolute()
            ? info.absoluteFilePath()
            : QDir::cleanPath(QDir::currentPath() + QStringLiteral("/") + candidate);
        if (QFileInfo::exists(absolutePath)) {
            return QDir::toNativeSeparators(absolutePath);
        }
    }

    return QString();
}

}  // namespace

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("field_simulator"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("现场模拟器：三路 RTSP + 无人机/转台 UDP 数据"));
    parser.addHelpOption();

    QCommandLineOption ffmpegOpt(
        QStringList{QStringLiteral("f"), QStringLiteral("ffmpeg")},
        QStringLiteral("ffmpeg.exe 路径"),
        QStringLiteral("path"));
    parser.addOption(ffmpegOpt);

    QCommandLineOption mediamtxOpt(
        QStringList{QStringLiteral("m"), QStringLiteral("mediamtx")},
        QStringLiteral("mediamtx 基地址，例如 rtsp://127.0.0.1:8554"),
        QStringLiteral("url"),
        QStringLiteral("rtsp://127.0.0.1:8554"));
    parser.addOption(mediamtxOpt);

    QCommandLineOption streamsOpt(
        QStringList{QStringLiteral("s"), QStringLiteral("streams")},
        QStringLiteral("流名称列表，逗号分隔"),
        QStringLiteral("list"),
        QStringLiteral("cam1,cam2,cam3"));
    parser.addOption(streamsOpt);

    QCommandLineOption udpHostOpt(
        QStringList{QStringLiteral("udp-host")},
        QStringLiteral("UDP 目标地址"),
        QStringLiteral("host"),
        QStringLiteral("127.0.0.1"));
    parser.addOption(udpHostOpt);

    QCommandLineOption udpPortOpt(
        QStringList{QStringLiteral("udp-port")},
        QStringLiteral("UDP 目标端口"),
        QStringLiteral("port"),
        QStringLiteral("50001"));
    parser.addOption(udpPortOpt);

    QCommandLineOption fpsOpt(
        QStringList{QStringLiteral("fps")},
        QStringLiteral("RTSP 推流帧率"),
        QStringLiteral("fps"),
        QStringLiteral("25"));
    parser.addOption(fpsOpt);

    QCommandLineOption udpIntervalOpt(
        QStringList{QStringLiteral("udp-interval-ms")},
        QStringLiteral("UDP 发送周期（毫秒）"),
        QStringLiteral("ms"),
        QStringLiteral("50"));
    parser.addOption(udpIntervalOpt);

    QCommandLineOption durationOpt(
        QStringList{QStringLiteral("duration-sec")},
        QStringLiteral("运行时长（秒，0=一直运行）"),
        QStringLiteral("sec"),
        QStringLiteral("0"));
    parser.addOption(durationOpt);

    QCommandLineOption noRtspOpt(
        QStringList{QStringLiteral("no-rtsp")},
        QStringLiteral("不启动 RTSP 推流，仅发送 UDP"));
    parser.addOption(noRtspOpt);

    QCommandLineOption noUdpOpt(
        QStringList{QStringLiteral("no-udp")},
        QStringLiteral("不发送 UDP，仅推 RTSP"));
    parser.addOption(noUdpOpt);

    QCommandLineOption dryRunOpt(
        QStringList{QStringLiteral("dry-run")},
        QStringLiteral("仅打印配置并退出"));
    parser.addOption(dryRunOpt);

    parser.process(app);

    bool okPort = false;
    const int udpPort = parser.value(udpPortOpt).toInt(&okPort);
    bool okFps = false;
    const int fps = parser.value(fpsOpt).toInt(&okFps);
    bool okUdpInterval = false;
    const int udpInterval = parser.value(udpIntervalOpt).toInt(&okUdpInterval);
    bool okDuration = false;
    const int duration = parser.value(durationOpt).toInt(&okDuration);

    if (!okPort || udpPort < 1 || udpPort > 65535
        || !okFps || fps < 1
        || !okUdpInterval || udpInterval < 10
        || !okDuration || duration < 0) {
        qCritical() << "参数非法，请检查 --help";
        return 2;
    }

    FieldSimulatorApp::Config config;
    config.ffmpegPath = parser.value(ffmpegOpt).trimmed();
    config.mediamtxBaseUrl = parser.value(mediamtxOpt).trimmed();
    config.streamNames = splitCsv(parser.value(streamsOpt));
    config.udpHost = parser.value(udpHostOpt).trimmed();
    config.udpPort = static_cast<quint16>(udpPort);
    config.fps = fps;
    config.udpIntervalMs = udpInterval;
    config.durationSec = duration;
    config.enableRtsp = !parser.isSet(noRtspOpt);
    config.enableUdp = !parser.isSet(noUdpOpt);
    config.dryRun = parser.isSet(dryRunOpt);

    if (config.streamNames.isEmpty()) {
        config.streamNames = QStringList{QStringLiteral("cam1"), QStringLiteral("cam2"), QStringLiteral("cam3")};
    }

    if (config.ffmpegPath.isEmpty()) {
        config.ffmpegPath = findDefaultFfmpegPath();
    }

    FieldSimulatorApp simulator(config);
    QObject::connect(&simulator, &FieldSimulatorApp::finished, &app, [&app](int code) {
        app.exit(code);
    });

    QString error;
    if (!simulator.start(&error)) {
        qCritical().noquote() << QStringLiteral("启动失败: %1").arg(error);
        return 1;
    }

    if (config.dryRun) {
        return 0;
    }

    return app.exec();
}
