#include "turntable_protocol_bridge/turntable_protocol_bridge.h"

#include <QByteArray>
#include <QHostAddress>
#include <QSerialPort>
#include <QUdpSocket>
#include <QVariantList>

#include <cstring>

namespace {

constexpr char kTurntableSendCommand[] = "turntable.send.command";
constexpr char kTurntableStop[] = "turntable.send.stop";
constexpr char kTurntableEnable[] = "turntable.send.enable";

constexpr quint8 kHeader1 = 0x55;
constexpr quint8 kHeader2 = 0xAA;
constexpr quint8 kTemplate1PayloadLength = 14;
constexpr quint8 kTemplate2PayloadLength = 26;
constexpr quint8 kCommandStop = 0x5A;
constexpr quint8 kCommandEnable = 0x5B;
constexpr int kTemplate1TotalLength = kTemplate1PayloadLength + 4;
constexpr int kTemplate2TotalLength = kTemplate2PayloadLength + 4;
constexpr int kStatusPayloadLength = 33;
constexpr int kStatusTotalLength = kStatusPayloadLength + 4;

QByteArray decodeHexString(const QString& text) {
    QByteArray normalized;
    normalized.reserve(text.size());
    for (const QChar ch : text) {
        if (!ch.isSpace() && ch != QChar(',') && ch != QChar('\t')) {
            normalized.push_back(ch.toLatin1());
        }
    }

    if (normalized.isEmpty() || normalized.size() % 2 != 0) {
        return {};
    }

    const QByteArray decoded = QByteArray::fromHex(normalized);
    if (decoded.isEmpty() && !normalized.isEmpty()) {
        return {};
    }
    return decoded;
}

QByteArray encodeFloatLE(float value) {
    QByteArray bytes(sizeof(float), Qt::Uninitialized);
    std::memcpy(bytes.data(), &value, sizeof(float));
    return bytes;
}

float decodeFloatLE(const QByteArray& frame, int offset) {
    if (offset < 0 || offset + static_cast<int>(sizeof(float)) > frame.size()) {
        return 0.0f;
    }

    float value = 0.0f;
    std::memcpy(&value, frame.constData() + offset, sizeof(float));
    return value;
}

quint16 decodeUInt16LE(const QByteArray& frame, int offset) {
    if (offset < 0 || offset + 2 > frame.size()) {
        return 0;
    }

    return static_cast<quint16>(static_cast<quint8>(frame[offset]))
        | (static_cast<quint16>(static_cast<quint8>(frame[offset + 1])) << 8);
}

QString toHexString(const QByteArray& bytes) {
    QStringList parts;
    parts.reserve(bytes.size());
    for (const char byte : bytes) {
        parts.push_back(QStringLiteral("%1")
                            .arg(static_cast<quint8>(byte), 2, 16, QChar('0'))
                            .toUpper());
    }
    return parts.join(' ');
}

QByteArray normalizeReservedBytes(const QVariant& value) {
    const QByteArray bytes = decodeHexString(value.toString());
    if (bytes.isEmpty()) {
        return QByteArray(4, '\0');
    }

    QByteArray reserved = bytes.left(4);
    if (reserved.size() < 4) {
        reserved.append(QByteArray(4 - reserved.size(), '\0'));
    }
    return reserved;
}

quint8 lowByteChecksum(const QByteArray& frameWithoutChecksum) {
    quint8 checksum = 0;
    for (const char byte : frameWithoutChecksum) {
        checksum = static_cast<quint8>(checksum + static_cast<quint8>(byte));
    }
    return checksum;
}

bool validateChecksum(const QByteArray& frame) {
    if (frame.size() < 5) {
        return false;
    }

    const quint8 expected = static_cast<quint8>(frame.back());
    const QByteArray fullBody = frame.left(frame.size() - 1);
    if (lowByteChecksum(fullBody) == expected) {
        return true;
    }

    const QByteArray payloadBody = fullBody.mid(2);
    return !payloadBody.isEmpty() && lowByteChecksum(payloadBody) == expected;
}

quint8 parseCommandCode(const QVariant& value, quint8 fallback) {
    if (!value.isValid() || value.isNull()) {
        return fallback;
    }

    bool ok = false;
    if (value.canConvert<int>()) {
        const int number = value.toInt(&ok);
        if (ok && number >= 0 && number <= 0xFF) {
            return static_cast<quint8>(number);
        }
    }

    QString text = value.toString().trimmed();
    if (text.startsWith(QStringLiteral("0x"), Qt::CaseInsensitive)) {
        text = text.mid(2);
    }

    const int number = text.toInt(&ok, 16);
    if (ok && number >= 0 && number <= 0xFF) {
        return static_cast<quint8>(number);
    }

    return fallback;
}

QByteArray buildTemplate1Frame(
    quint8 command,
    float azimuth,
    float elevation,
    const QByteArray& reserved,
    quint8 counter) {
    QByteArray frame;
    frame.reserve(kTemplate1TotalLength);
    frame.push_back(static_cast<char>(kHeader1));
    frame.push_back(static_cast<char>(kHeader2));
    frame.push_back(static_cast<char>(kTemplate1PayloadLength));
    frame.push_back(static_cast<char>(command));
    frame.append(encodeFloatLE(azimuth));
    frame.append(encodeFloatLE(elevation));
    frame.append(reserved.left(4));
    if (frame.size() < kTemplate1TotalLength - 2) {
        frame.append(QByteArray(kTemplate1TotalLength - 2 - frame.size(), '\0'));
    }
    frame.push_back(static_cast<char>(counter));
    frame.push_back(static_cast<char>(lowByteChecksum(frame)));
    return frame;
}

QByteArray buildTemplate2Frame(
    quint8 command,
    float azimuth,
    float elevation,
    float inertialAzimuth,
    float inertialElevation,
    float inertialRoll,
    const QByteArray& reserved,
    quint8 counter) {
    QByteArray frame;
    frame.reserve(kTemplate2TotalLength);
    frame.push_back(static_cast<char>(kHeader1));
    frame.push_back(static_cast<char>(kHeader2));
    frame.push_back(static_cast<char>(kTemplate2PayloadLength));
    frame.push_back(static_cast<char>(command));
    frame.append(encodeFloatLE(azimuth));
    frame.append(encodeFloatLE(elevation));
    frame.append(encodeFloatLE(inertialAzimuth));
    frame.append(encodeFloatLE(inertialElevation));
    frame.append(encodeFloatLE(inertialRoll));
    frame.append(reserved.left(4));
    if (frame.size() < kTemplate2TotalLength - 2) {
        frame.append(QByteArray(kTemplate2TotalLength - 2 - frame.size(), '\0'));
    }
    frame.push_back(static_cast<char>(counter));
    frame.push_back(static_cast<char>(lowByteChecksum(frame)));
    return frame;
}

QString workModeText(quint8 code) {
    switch (code) {
    case 0x11:
        return QStringLiteral("frame_angle");
    case 0x22:
        return QStringLiteral("ground_angle");
    case 0x44:
        return QStringLiteral("rate_control");
    case 0x66:
        return QStringLiteral("visible_tracking");
    case 0x88:
        return QStringLiteral("infrared_tracking");
    case 0x5A:
        return QStringLiteral("motor_disable");
    case 0x5B:
        return QStringLiteral("motor_enable");
    case 0x00:
        return QStringLiteral("self_check");
    default:
        return QStringLiteral("unknown");
    }
}

QString imageStatusText(quint8 code) {
    switch (code) {
    case 0x55:
        return QStringLiteral("stable_tracking");
    case 0x66:
        return QStringLiteral("memory_tracking");
    case 0x77:
        return QStringLiteral("target_lost");
    default:
        return QStringLiteral("unknown");
    }
}

}  // namespace

class TurntableProtocolBridge::Impl {
public:
    enum class LinkType {
        None,
        Serial,
        Udp
    };

    Impl()
        : serialPort_(std::make_unique<QSerialPort>())
        , udpSocket_(std::make_unique<QUdpSocket>()) {
        QObject::connect(serialPort_.get(), &QSerialPort::readyRead, [this]() {
            serialBuffer_.append(serialPort_->readAll());
            consumeBuffer(serialBuffer_);
        });

        QObject::connect(udpSocket_.get(), &QUdpSocket::readyRead, [this]() {
            while (udpSocket_->hasPendingDatagrams()) {
                QByteArray datagram;
                datagram.resize(static_cast<int>(udpSocket_->pendingDatagramSize()));

                QHostAddress sender;
                quint16 senderPort = 0;
                const qint64 bytes = udpSocket_->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
                if (bytes <= 0) {
                    continue;
                }

                datagram.resize(static_cast<int>(bytes));
                if (remoteAddress_.isNull()) {
                    remoteAddress_ = sender;
                    remotePort_ = senderPort;
                }

                udpBuffer_.append(datagram);
                consumeBuffer(udpBuffer_);
            }
        });
    }

    bool sendFrame(const QByteArray& frame) {
        if (frame.isEmpty()) {
            lastError_ = QStringLiteral("empty frame");
            return false;
        }

        bool ok = false;
        switch (linkType_) {
        case LinkType::Serial: {
            if (!serialPort_->isOpen()) {
                lastError_ = QStringLiteral("serial port is not open");
                return false;
            }
            const qint64 written = serialPort_->write(frame);
            ok = written == frame.size() && serialPort_->waitForBytesWritten(500);
            lastError_ = ok ? QString() : serialPort_->errorString();
            break;
        }
        case LinkType::Udp: {
            if (remoteAddress_.isNull() || remotePort_ <= 0 || !udpSocket_->isOpen()) {
                lastError_ = QStringLiteral("udp socket is not ready");
                return false;
            }
            const qint64 written = udpSocket_->writeDatagram(frame, remoteAddress_, remotePort_);
            ok = written == frame.size();
            lastError_ = ok ? QString() : udpSocket_->errorString();
            break;
        }
        case LinkType::None:
            lastError_ = QStringLiteral("device is not connected");
            return false;
        }

        if (ok) {
            lastTxHex_ = toHexString(frame);
            lastCommandCode_ = static_cast<quint8>(frame[3]);
            txCounter_ = static_cast<quint8>(frame[frame.size() - 2]);
        }
        return ok;
    }

    void consumeBuffer(QByteArray& buffer) {
        while (true) {
            const int headerIndex = buffer.indexOf(QByteArray::fromHex("55AA"));
            if (headerIndex < 0) {
                buffer.clear();
                return;
            }

            if (headerIndex > 0) {
                buffer.remove(0, headerIndex);
            }

            if (buffer.size() < 4) {
                return;
            }

            const int payloadLength = static_cast<quint8>(buffer[2]);
            const int totalLength = payloadLength + 4;
            if (payloadLength <= 0 || totalLength < 5) {
                buffer.remove(0, 2);
                continue;
            }

            if (buffer.size() < totalLength) {
                return;
            }

            const QByteArray frame = buffer.left(totalLength);
            buffer.remove(0, totalLength);
            parseFrame(frame);
        }
    }

    void parseFrame(const QByteArray& frame) {
        lastRxHex_ = toHexString(frame);
        lastChecksumOk_ = validateChecksum(frame);
        if (!lastChecksumOk_) {
            lastError_ = QStringLiteral("invalid checksum");
            return;
        }

        lastError_.clear();
        lastStatusCode_ = static_cast<quint8>(frame[3]);
        rxCounter_ = static_cast<quint8>(frame[frame.size() - 2]);

        if (frame.size() != kStatusTotalLength || static_cast<quint8>(frame[2]) != kStatusPayloadLength) {
            return;
        }

        selfCheckCode_ = decodeUInt16LE(frame, 4);
        azimuthAngle_ = decodeFloatLE(frame, 6);
        elevationAngle_ = decodeFloatLE(frame, 10);
        azimuthRate_ = decodeFloatLE(frame, 14);
        elevationRate_ = decodeFloatLE(frame, 18);
        azimuthGuide_ = decodeFloatLE(frame, 22);
        elevationGuide_ = decodeFloatLE(frame, 26);
        rollRate_ = decodeFloatLE(frame, 30);
        imageStatusCode_ = static_cast<quint8>(frame[34]);
    }

    std::unique_ptr<QSerialPort> serialPort_;
    std::unique_ptr<QUdpSocket> udpSocket_;
    LinkType linkType_ = LinkType::None;

    QByteArray serialBuffer_;
    QByteArray udpBuffer_;
    QHostAddress remoteAddress_;
    quint16 remotePort_ = 0;
    QHostAddress localAddress_;
    quint16 localPort_ = 0;

    QString lastError_;
    QString lastTxHex_;
    QString lastRxHex_;

    quint8 txCounter_ = 0;
    quint8 rxCounter_ = 0;
    quint8 lastCommandCode_ = 0;
    quint8 lastStatusCode_ = 0;
    quint8 imageStatusCode_ = 0;
    quint16 selfCheckCode_ = 0;

    float azimuthAngle_ = 0.0f;
    float elevationAngle_ = 0.0f;
    float azimuthRate_ = 0.0f;
    float elevationRate_ = 0.0f;
    float azimuthGuide_ = 0.0f;
    float elevationGuide_ = 0.0f;
    float rollRate_ = 0.0f;

    bool lastChecksumOk_ = false;
};

TurntableProtocolBridge::TurntableProtocolBridge()
    : impl_(std::make_unique<Impl>()) {}

TurntableProtocolBridge::~TurntableProtocolBridge() = default;

QVariantMap TurntableProtocolBridge::capabilities() const {
    QVariantList transports;
    transports.push_back(QStringLiteral("serial"));
    transports.push_back(QStringLiteral("udp"));

    QVariantList actions;
    actions.push_back(QStringLiteral("connect.serial"));
    actions.push_back(QStringLiteral("connect.udp"));
    actions.push_back(QStringLiteral("disconnect"));
    actions.push_back(QStringLiteral("send.raw.hex"));
    actions.push_back(QString::fromLatin1(kTurntableSendCommand));
    actions.push_back(QString::fromLatin1(kTurntableStop));
    actions.push_back(QString::fromLatin1(kTurntableEnable));

    QVariantMap caps;
    caps.insert(QStringLiteral("deviceId"), QStringLiteral("turntable"));
    caps.insert(QStringLiteral("displayName"), QStringLiteral("Turntable Device"));
    caps.insert(QStringLiteral("transports"), transports);
    caps.insert(QStringLiteral("actions"), actions);
    return caps;
}

bool TurntableProtocolBridge::connectSerial(const QVariantMap& options) {
    disconnectDevice();

    auto* port = impl_->serialPort_.get();
    port->setPortName(options.value(QStringLiteral("portName")).toString());
    port->setBaudRate(options.value(QStringLiteral("baudRate"), 115200).toInt());
    port->setDataBits(static_cast<QSerialPort::DataBits>(
        options.value(QStringLiteral("dataBits"), static_cast<int>(QSerialPort::Data8)).toInt()));
    port->setParity(static_cast<QSerialPort::Parity>(
        options.value(QStringLiteral("parity"), static_cast<int>(QSerialPort::NoParity)).toInt()));
    port->setStopBits(static_cast<QSerialPort::StopBits>(
        options.value(QStringLiteral("stopBits"), static_cast<int>(QSerialPort::OneStop)).toInt()));
    port->setFlowControl(static_cast<QSerialPort::FlowControl>(
        options.value(QStringLiteral("flowControl"), static_cast<int>(QSerialPort::NoFlowControl)).toInt()));

    const bool ok = port->open(QIODevice::ReadWrite);
    impl_->lastError_ = ok ? QString() : port->errorString();
    impl_->linkType_ = ok ? Impl::LinkType::Serial : Impl::LinkType::None;
    return ok;
}

bool TurntableProtocolBridge::connectUdp(const QVariantMap& options) {
    disconnectDevice();

    const QString remoteAddressText = options.value(QStringLiteral("remoteAddress")).toString().trimmed();
    const quint16 remotePort = static_cast<quint16>(options.value(QStringLiteral("remotePort")).toUInt());
    if (remoteAddressText.isEmpty() || remotePort == 0) {
        impl_->lastError_ = QStringLiteral("remote udp endpoint is invalid");
        return false;
    }

    const QString localAddressText = options.value(QStringLiteral("localAddress")).toString().trimmed();
    const quint16 localPort = static_cast<quint16>(options.value(QStringLiteral("localPort")).toUInt());
    const QHostAddress bindAddress = localAddressText.isEmpty()
        ? QHostAddress(QHostAddress::AnyIPv4)
        : QHostAddress(localAddressText);

    const bool ok = impl_->udpSocket_->bind(
        bindAddress,
        localPort,
        QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);
    impl_->lastError_ = ok ? QString() : impl_->udpSocket_->errorString();
    if (!ok) {
        return false;
    }

    impl_->remoteAddress_ = QHostAddress(remoteAddressText);
    impl_->remotePort_ = remotePort;
    impl_->localAddress_ = bindAddress;
    impl_->localPort_ = localPort;
    impl_->linkType_ = Impl::LinkType::Udp;
    return true;
}

void TurntableProtocolBridge::disconnectDevice() {
    if (impl_->serialPort_->isOpen()) {
        impl_->serialPort_->close();
    }
    if (impl_->udpSocket_->isOpen()) {
        impl_->udpSocket_->close();
    }

    impl_->linkType_ = Impl::LinkType::None;
    impl_->remoteAddress_ = QHostAddress();
    impl_->remotePort_ = 0;
    impl_->localAddress_ = QHostAddress();
    impl_->localPort_ = 0;
}

bool TurntableProtocolBridge::isConnected() const {
    switch (impl_->linkType_) {
    case Impl::LinkType::Serial:
        return impl_->serialPort_->isOpen();
    case Impl::LinkType::Udp:
        return impl_->udpSocket_->isOpen();
    case Impl::LinkType::None:
        return false;
    }
    return false;
}

bool TurntableProtocolBridge::invoke(const QString& action, const QVariantMap& args) {
    if (action == QStringLiteral("connect.serial")) {
        return connectSerial(args);
    }
    if (action == QStringLiteral("connect.udp")) {
        return connectUdp(args);
    }
    if (action == QStringLiteral("disconnect")) {
        disconnectDevice();
        return true;
    }
    if (action == QStringLiteral("send.raw.hex")) {
        const QByteArray bytes = decodeHexString(
            args.value(QStringLiteral("payload"), args.value(QStringLiteral("hex")).toString()).toString());
        return impl_->sendFrame(bytes);
    }

    QByteArray frame;
    if (action == QString::fromLatin1(kTurntableStop)) {
        frame = buildTemplate1Frame(
            kCommandStop, 0.0f, 0.0f, QByteArray(4, '\0'), static_cast<quint8>(impl_->txCounter_ + 1));
    } else if (action == QString::fromLatin1(kTurntableEnable)) {
        frame = buildTemplate1Frame(
            kCommandEnable, 0.0f, 0.0f, QByteArray(4, '\0'), static_cast<quint8>(impl_->txCounter_ + 1));
    } else if (action == QString::fromLatin1(kTurntableSendCommand)) {
        const int templateVersion = args.value(QStringLiteral("templateVersion"), 1).toInt();
        const quint8 commandCode = parseCommandCode(args.value(QStringLiteral("command")), 0x11);
        const float azimuth = args.value(QStringLiteral("azimuth")).toFloat();
        const float elevation = args.value(QStringLiteral("elevation")).toFloat();
        const QByteArray reserved = normalizeReservedBytes(args.value(QStringLiteral("reserved")));
        const quint8 counter = static_cast<quint8>(impl_->txCounter_ + 1);

        if (templateVersion == 2) {
            frame = buildTemplate2Frame(
                commandCode,
                azimuth,
                elevation,
                args.value(QStringLiteral("inertialAzimuth")).toFloat(),
                args.value(QStringLiteral("inertialElevation")).toFloat(),
                args.value(QStringLiteral("inertialRoll")).toFloat(),
                reserved,
                counter);
        } else {
            frame = buildTemplate1Frame(commandCode, azimuth, elevation, reserved, counter);
        }
    }

    if (frame.isEmpty()) {
        impl_->lastError_ = QStringLiteral("invalid turntable command");
        return false;
    }

    return impl_->sendFrame(frame);
}

QVariantMap TurntableProtocolBridge::snapshot() const {
    QString linkType = QStringLiteral("none");
    switch (impl_->linkType_) {
    case Impl::LinkType::Serial:
        linkType = QStringLiteral("serial");
        break;
    case Impl::LinkType::Udp:
        linkType = QStringLiteral("udp");
        break;
    case Impl::LinkType::None:
        break;
    }

    QVariantMap snapshot;
    snapshot.insert(QStringLiteral("status.connected"), isConnected());
    snapshot.insert(QStringLiteral("status.linkType"), linkType);
    snapshot.insert(QStringLiteral("status.modeCode"), impl_->lastStatusCode_);
    snapshot.insert(QStringLiteral("status.modeText"), workModeText(impl_->lastStatusCode_));
    snapshot.insert(QStringLiteral("status.imageCode"), impl_->imageStatusCode_);
    snapshot.insert(QStringLiteral("status.imageText"), imageStatusText(impl_->imageStatusCode_));
    snapshot.insert(QStringLiteral("status.selfCheckCode"), impl_->selfCheckCode_);
    snapshot.insert(QStringLiteral("status.lastChecksumOk"), impl_->lastChecksumOk_);
    snapshot.insert(QStringLiteral("transport.portName"), impl_->serialPort_->portName());
    snapshot.insert(QStringLiteral("transport.baudRate"), impl_->serialPort_->baudRate());
    snapshot.insert(QStringLiteral("transport.localAddress"), impl_->localAddress_.toString());
    snapshot.insert(QStringLiteral("transport.localPort"), impl_->localPort_);
    snapshot.insert(QStringLiteral("transport.remoteAddress"), impl_->remoteAddress_.toString());
    snapshot.insert(QStringLiteral("transport.remotePort"), impl_->remotePort_);
    snapshot.insert(QStringLiteral("transport.lastError"), impl_->lastError_);
    snapshot.insert(QStringLiteral("transport.lastTxHex"), impl_->lastTxHex_);
    snapshot.insert(QStringLiteral("transport.lastRxHex"), impl_->lastRxHex_);
    snapshot.insert(QStringLiteral("transport.txCounter"), impl_->txCounter_);
    snapshot.insert(QStringLiteral("transport.rxCounter"), impl_->rxCounter_);
    snapshot.insert(QStringLiteral("transport.lastCommandCode"), impl_->lastCommandCode_);
    snapshot.insert(QStringLiteral("runtime.azimuthAngle"), impl_->azimuthAngle_);
    snapshot.insert(QStringLiteral("runtime.elevationAngle"), impl_->elevationAngle_);
    snapshot.insert(QStringLiteral("runtime.azimuthRate"), impl_->azimuthRate_);
    snapshot.insert(QStringLiteral("runtime.elevationRate"), impl_->elevationRate_);
    snapshot.insert(QStringLiteral("runtime.azimuthGuide"), impl_->azimuthGuide_);
    snapshot.insert(QStringLiteral("runtime.elevationGuide"), impl_->elevationGuide_);
    snapshot.insert(QStringLiteral("runtime.rollRate"), impl_->rollRate_);
    return snapshot;
}
