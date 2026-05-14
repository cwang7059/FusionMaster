#include "protocol_55aa_device_bridge/eddy_current_55aa_device_bridge.h"
#include "protocol_55aa_device_bridge/encoder_55aa_device_bridge.h"

#include <plugin_api/idevice_capabilities.h>

#include <QDateTime>
#include <QSerialPort>
#include <QVariantList>

#include <cstring>
#include <memory>

namespace {

constexpr quint8 kHeader1 = 0x55;
constexpr quint8 kHeader2 = 0xAA;
constexpr quint16 kMaxPayloadLength = 4096;

constexpr char kEncoderSendCommand[] = "encoder55aa.send.command";
constexpr char kEncoderStopUpload[] = "encoder55aa.stop.upload";
constexpr char kEncoderStartCoarseUpload[] = "encoder55aa.start.coarse_upload";
constexpr char kEncoderStartPreciseUpload[] = "encoder55aa.start.precise_upload";
constexpr char kEncoderReadSerial[] = "encoder55aa.read.serial";
constexpr char kEncoderWriteSerial[] = "encoder55aa.write.serial";
constexpr char kEncoderReadVersion[] = "encoder55aa.read.version";
constexpr char kEncoderReadResolution[] = "encoder55aa.read.resolution";
constexpr char kEncoderWriteResolution[] = "encoder55aa.write.resolution";
constexpr char kEncoderReadProtocol[] = "encoder55aa.read.protocol";
constexpr char kEncoderWriteProtocol[] = "encoder55aa.write.protocol";
constexpr char kEncoderReadOuterIps[] = "encoder55aa.read.outer.ips";
constexpr char kEncoderWriteOuterIps[] = "encoder55aa.write.outer.ips";
constexpr char kEncoderReadInnerIps[] = "encoder55aa.read.inner.ips";
constexpr char kEncoderWriteInnerIps[] = "encoder55aa.write.inner.ips";
constexpr char kEncoderSetFeedbackPeriod[] = "encoder55aa.set.feedback_period";
constexpr char kEncoderWriteBigCycleCompensation[] = "encoder55aa.write.big_cycle_compensation";
constexpr char kEncoderWriteSmallCycleCompensation[] = "encoder55aa.write.small_cycle_compensation";
constexpr char kEncoderWriteAdCompensation[] = "encoder55aa.write.ad_compensation";
constexpr char kEncoderResetCompensation[] = "encoder55aa.reset.compensation";

constexpr char kEddySendCommand[] = "eddy55aa.send.command";
constexpr char kEddyStopUpload[] = "eddy55aa.stop.upload";
constexpr char kEddyStartAdUpload[] = "eddy55aa.start.ad_upload";
constexpr char kEddyStartDisplacementUpload[] = "eddy55aa.start.displacement_upload";
constexpr char kEddySetFeedbackPeriod[] = "eddy55aa.set.feedback_period";
constexpr char kEddyReadSerial[] = "eddy55aa.read.serial";
constexpr char kEddyWriteSerial[] = "eddy55aa.write.serial";
constexpr char kEddyReadVersion[] = "eddy55aa.read.version";
constexpr char kEddyReadFrequency[] = "eddy55aa.read.frequency";
constexpr char kEddyWriteFrequency[] = "eddy55aa.write.frequency";
constexpr char kEddyReadRange[] = "eddy55aa.read.range";
constexpr char kEddyWriteRange[] = "eddy55aa.write.range";
constexpr char kEddyReadFitParams[] = "eddy55aa.read.fit_params";
constexpr char kEddyWriteFitParams[] = "eddy55aa.write.fit_params";
constexpr char kEddyReadInterpolationParams[] = "eddy55aa.read.interpolation_params";
constexpr char kEddyWriteInterpolationParams[] = "eddy55aa.write.interpolation_params";
constexpr char kEddyReadGain[] = "eddy55aa.read.gain";
constexpr char kEddyWriteGain[] = "eddy55aa.write.gain";
constexpr char kEddyReadZero[] = "eddy55aa.read.zero";
constexpr char kEddyWriteZero[] = "eddy55aa.write.zero";
constexpr char kEddyReadProtocol[] = "eddy55aa.read.protocol";
constexpr char kEddyWriteProtocol[] = "eddy55aa.write.protocol";
constexpr char kEddyReadPhase[] = "eddy55aa.read.phase";
constexpr char kEddyWritePhase[] = "eddy55aa.write.phase";

enum class ByteOrderMode {
    LittleEndian,
    BigEndian
};

enum class FrameLengthMode {
    LittleEndian,
    BigEndian,
    Auto
};

QString byteOrderText(ByteOrderMode mode) {
    return mode == ByteOrderMode::LittleEndian ? QStringLiteral("little") : QStringLiteral("big");
}

QString frameLengthModeText(FrameLengthMode mode) {
    switch (mode) {
    case FrameLengthMode::LittleEndian:
        return QStringLiteral("little");
    case FrameLengthMode::BigEndian:
        return QStringLiteral("big");
    case FrameLengthMode::Auto:
    default:
        return QStringLiteral("auto");
    }
}

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

QString toHexString(const QByteArray& bytes) {
    QStringList parts;
    parts.reserve(bytes.size());
    for (const char byte : bytes) {
        parts.push_back(
            QStringLiteral("%1").arg(static_cast<quint8>(byte), 2, 16, QChar('0')).toUpper());
    }
    return parts.join(' ');
}

quint8 checksum8(const QByteArray& bytes) {
    quint8 checksum = 0;
    for (const char byte : bytes) {
        checksum = static_cast<quint8>(checksum + static_cast<quint8>(byte));
    }
    return checksum;
}

ByteOrderMode parseByteOrder(const QVariantMap& options, ByteOrderMode fallback) {
    const QStringList keys{
        QStringLiteral("byteOrder"),
        QStringLiteral("valueEndian"),
        QStringLiteral("endian")
    };

    for (const QString& key : keys) {
        const QString text = options.value(key).toString().trimmed().toLower();
        if (text == QStringLiteral("little") || text == QStringLiteral("le")) {
            return ByteOrderMode::LittleEndian;
        }
        if (text == QStringLiteral("big") || text == QStringLiteral("be")) {
            return ByteOrderMode::BigEndian;
        }
    }
    return fallback;
}

FrameLengthMode parseFrameLengthMode(const QVariantMap& options, FrameLengthMode fallback) {
    const QString text = options.value(QStringLiteral("frameLengthEndian")).toString().trimmed().toLower();
    if (text == QStringLiteral("little") || text == QStringLiteral("le")) {
        return FrameLengthMode::LittleEndian;
    }
    if (text == QStringLiteral("big") || text == QStringLiteral("be")) {
        return FrameLengthMode::BigEndian;
    }
    if (text == QStringLiteral("auto")) {
        return FrameLengthMode::Auto;
    }
    return fallback;
}

quint8 parseCommandCode(const QVariant& value, bool* okOut = nullptr) {
    bool ok = false;
    quint8 result = 0;

    if (value.canConvert<int>()) {
        const int number = value.toInt(&ok);
        if (ok && number >= 0 && number <= 0xFF) {
            result = static_cast<quint8>(number);
        }
    }

    if (!ok) {
        QString text = value.toString().trimmed();
        if (text.startsWith(QStringLiteral("0x"), Qt::CaseInsensitive)) {
            text = text.mid(2);
        }
        const int number = text.toInt(&ok, 16);
        if (ok && number >= 0 && number <= 0xFF) {
            result = static_cast<quint8>(number);
        }
    }

    if (okOut != nullptr) {
        *okOut = ok;
    }
    return result;
}

QString commandHex(quint8 cmd) {
    return QStringLiteral("0x%1").arg(cmd, 2, 16, QChar('0')).toUpper();
}

void appendUInt16(QByteArray& buffer, quint16 value, ByteOrderMode order) {
    if (order == ByteOrderMode::LittleEndian) {
        buffer.push_back(static_cast<char>(value & 0xFF));
        buffer.push_back(static_cast<char>((value >> 8) & 0xFF));
    } else {
        buffer.push_back(static_cast<char>((value >> 8) & 0xFF));
        buffer.push_back(static_cast<char>(value & 0xFF));
    }
}

void appendUInt32(QByteArray& buffer, quint32 value, ByteOrderMode order) {
    if (order == ByteOrderMode::LittleEndian) {
        buffer.push_back(static_cast<char>(value & 0xFF));
        buffer.push_back(static_cast<char>((value >> 8) & 0xFF));
        buffer.push_back(static_cast<char>((value >> 16) & 0xFF));
        buffer.push_back(static_cast<char>((value >> 24) & 0xFF));
    } else {
        buffer.push_back(static_cast<char>((value >> 24) & 0xFF));
        buffer.push_back(static_cast<char>((value >> 16) & 0xFF));
        buffer.push_back(static_cast<char>((value >> 8) & 0xFF));
        buffer.push_back(static_cast<char>(value & 0xFF));
    }
}

void appendFloat(QByteArray& buffer, float value, ByteOrderMode order) {
    static_assert(sizeof(float) == 4, "float size must be 4 bytes");
    quint32 raw = 0;
    std::memcpy(&raw, &value, sizeof(float));
    appendUInt32(buffer, raw, order);
}

quint16 readUInt16(const QByteArray& data, int offset, ByteOrderMode order) {
    if (offset < 0 || offset + 2 > data.size()) {
        return 0;
    }

    const quint8 b0 = static_cast<quint8>(data[offset]);
    const quint8 b1 = static_cast<quint8>(data[offset + 1]);
    if (order == ByteOrderMode::LittleEndian) {
        return static_cast<quint16>(b0) | (static_cast<quint16>(b1) << 8);
    }
    return (static_cast<quint16>(b0) << 8) | static_cast<quint16>(b1);
}

quint32 readUInt32(const QByteArray& data, int offset, ByteOrderMode order) {
    if (offset < 0 || offset + 4 > data.size()) {
        return 0;
    }

    const quint8 b0 = static_cast<quint8>(data[offset]);
    const quint8 b1 = static_cast<quint8>(data[offset + 1]);
    const quint8 b2 = static_cast<quint8>(data[offset + 2]);
    const quint8 b3 = static_cast<quint8>(data[offset + 3]);

    if (order == ByteOrderMode::LittleEndian) {
        return static_cast<quint32>(b0) | (static_cast<quint32>(b1) << 8)
            | (static_cast<quint32>(b2) << 16) | (static_cast<quint32>(b3) << 24);
    }
    return (static_cast<quint32>(b0) << 24) | (static_cast<quint32>(b1) << 16)
        | (static_cast<quint32>(b2) << 8) | static_cast<quint32>(b3);
}

float readFloat(const QByteArray& data, int offset, ByteOrderMode order) {
    const quint32 raw = readUInt32(data, offset, order);
    float value = 0.0f;
    std::memcpy(&value, &raw, sizeof(float));
    return value;
}

QString decodeTextPayload(const QByteArray& bytes) {
    QByteArray trimmed = bytes;
    const int zeroIndex = trimmed.indexOf('\0');
    if (zeroIndex >= 0) {
        trimmed.truncate(zeroIndex);
    }
    while (!trimmed.isEmpty() && (trimmed.endsWith('\0') || trimmed.endsWith(' '))) {
        trimmed.chop(1);
    }
    return QString::fromUtf8(trimmed).trimmed();
}

QStringList decodeTextPayloadItems(const QByteArray& bytes) {
    QStringList values;
    const QList<QByteArray> parts = bytes.split('\0');
    values.reserve(parts.size());
    for (const QByteArray& part : parts) {
        values.push_back(QString::fromUtf8(part).trimmed());
    }

    while (!values.isEmpty() && values.back().isEmpty()) {
        values.pop_back();
    }
    return values;
}

QVariantList parseUInt16List(const QByteArray& bytes, ByteOrderMode order) {
    QVariantList values;
    for (int offset = 0; offset + 2 <= bytes.size(); offset += 2) {
        values.push_back(static_cast<int>(readUInt16(bytes, offset, order)));
    }
    return values;
}

QVariantList parseUInt32List(const QByteArray& bytes, ByteOrderMode order) {
    QVariantList values;
    for (int offset = 0; offset + 4 <= bytes.size(); offset += 4) {
        values.push_back(static_cast<qulonglong>(readUInt32(bytes, offset, order)));
    }
    return values;
}

QVariantList parseFloatList(const QByteArray& bytes, ByteOrderMode order) {
    QVariantList values;
    for (int offset = 0; offset + 4 <= bytes.size(); offset += 4) {
        values.push_back(readFloat(bytes, offset, order));
    }
    return values;
}

double gapPercentFromRaw(quint16 rawValue) {
    return static_cast<double>(rawValue) * 100.0 / 65535.0;
}

bool variantToUInt8(const QVariant& value, quint8* out) {
    bool ok = false;
    const int number = value.toInt(&ok);
    if (!ok || number < 0 || number > 0xFF) {
        return false;
    }
    *out = static_cast<quint8>(number);
    return true;
}

bool variantToUInt16(const QVariant& value, quint16* out) {
    bool ok = false;
    const uint number = value.toUInt(&ok);
    if (!ok || number > 0xFFFF) {
        return false;
    }
    *out = static_cast<quint16>(number);
    return true;
}

bool variantToUInt32(const QVariant& value, quint32* out) {
    bool ok = false;
    const qulonglong number = value.toULongLong(&ok);
    if (!ok || number > 0xFFFFFFFFULL) {
        return false;
    }
    *out = static_cast<quint32>(number);
    return true;
}

bool variantToFloat(const QVariant& value, float* out) {
    bool ok = false;
    const double number = value.toDouble(&ok);
    if (!ok) {
        return false;
    }
    *out = static_cast<float>(number);
    return true;
}

QVariantList normalizeVariantList(const QVariant& value) {
    if (value.userType() == QMetaType::QVariantList) {
        return value.toList();
    }
    if (value.canConvert<QStringList>()) {
        QVariantList list;
        const QStringList strings = value.toStringList();
        list.reserve(strings.size());
        for (const QString& item : strings) {
            list.push_back(item);
        }
        return list;
    }
    return {};
}

bool appendUInt16Values(
    QByteArray& buffer,
    const QVariant& value,
    int expectedCount,
    ByteOrderMode order) {
    const QVariantList list = normalizeVariantList(value);
    if (list.size() != expectedCount) {
        return false;
    }

    for (const QVariant& item : list) {
        quint16 parsed = 0;
        if (!variantToUInt16(item, &parsed)) {
            return false;
        }
        appendUInt16(buffer, parsed, order);
    }
    return true;
}

bool appendUInt32Values(
    QByteArray& buffer,
    const QVariant& value,
    int expectedCount,
    ByteOrderMode order) {
    const QVariantList list = normalizeVariantList(value);
    if (list.size() != expectedCount) {
        return false;
    }

    for (const QVariant& item : list) {
        quint32 parsed = 0;
        if (!variantToUInt32(item, &parsed)) {
            return false;
        }
        appendUInt32(buffer, parsed, order);
    }
    return true;
}

bool appendFloatValues(
    QByteArray& buffer,
    const QVariant& value,
    int expectedCount,
    ByteOrderMode order) {
    const QVariantList list = normalizeVariantList(value);
    if (list.size() != expectedCount) {
        return false;
    }

    for (const QVariant& item : list) {
        float parsed = 0.0f;
        if (!variantToFloat(item, &parsed)) {
            return false;
        }
        appendFloat(buffer, parsed, order);
    }
    return true;
}

bool appendFloatValuesVariable(
    QByteArray& buffer,
    const QVariant& value,
    int minCount,
    int maxCount,
    ByteOrderMode order) {
    const QVariantList list = normalizeVariantList(value);
    if (list.size() < minCount || list.size() > maxCount) {
        return false;
    }

    for (const QVariant& item : list) {
        float parsed = 0.0f;
        if (!variantToFloat(item, &parsed)) {
            return false;
        }
        appendFloat(buffer, parsed, order);
    }
    return true;
}

bool appendFloatValuesVariableByStep(
    QByteArray& buffer,
    const QVariant& value,
    int minCount,
    int maxCount,
    int step,
    ByteOrderMode order) {
    const QVariantList list = normalizeVariantList(value);
    if (step <= 0) {
        return false;
    }
    if (list.size() < minCount || list.size() > maxCount || (list.size() % step) != 0) {
        return false;
    }

    for (const QVariant& item : list) {
        float parsed = 0.0f;
        if (!variantToFloat(item, &parsed)) {
            return false;
        }
        appendFloat(buffer, parsed, order);
    }
    return true;
}

QString encoderProtocolText(quint16 code) {
    switch (code) {
    case 0:
        return QStringLiteral("rs422");
    case 1:
        return QStringLiteral("tamagawa");
    case 2:
        return QStringLiteral("biss");
    case 3:
        return QStringLiteral("ssi");
    case 4:
        return QStringLiteral("dmc");
    default:
        return QStringLiteral("unknown");
    }
}

QString legacyEncoderProtocolText(quint8 code) {
    switch (code) {
    case 0:
        return QStringLiteral("rs422");
    case 1:
        return QStringLiteral("biss_c");
    case 2:
        return QStringLiteral("ssi");
    case 3:
        return QStringLiteral("dmc");
    default:
        return QStringLiteral("unknown");
    }
}

QString ringText(const QString& text) {
    const QString normalized = text.trimmed().toLower();
    if (normalized == QStringLiteral("outer")) {
        return QStringLiteral("outer");
    }
    if (normalized == QStringLiteral("inner")) {
        return QStringLiteral("inner");
    }
    return QString();
}

QString modeText(const QString& text) {
    const QString normalized = text.trimmed().toLower();
    if (normalized == QStringLiteral("precise")) {
        return QStringLiteral("precise");
    }
    if (normalized == QStringLiteral("coarse")) {
        return QStringLiteral("coarse");
    }
    return QString();
}

quint8 encoderCompensationCommand(
    const QString& ring,
    const QString& mode,
    int kind) {
    const QString ringValue = ringText(ring);
    const QString modeValue = modeText(mode);
    if (ringValue.isEmpty() || modeValue.isEmpty()) {
        return 0;
    }

    int base = 0;
    if (ringValue == QStringLiteral("outer") && modeValue == QStringLiteral("precise")) {
        base = 0xA0;
    } else if (ringValue == QStringLiteral("outer") && modeValue == QStringLiteral("coarse")) {
        base = 0xA4;
    } else if (ringValue == QStringLiteral("inner") && modeValue == QStringLiteral("precise")) {
        base = 0xA8;
    } else if (ringValue == QStringLiteral("inner") && modeValue == QStringLiteral("coarse")) {
        base = 0xAC;
    }

    return static_cast<quint8>(base + kind);
}

class Protocol55aaSerialBridge {
public:
    Protocol55aaSerialBridge(QString deviceId, QString displayName, QVariantList actions)
        : deviceId_(std::move(deviceId))
        , displayName_(std::move(displayName))
        , actions_(std::move(actions))
        , serialPort_(std::make_unique<QSerialPort>()) {
        QObject::connect(serialPort_.get(), &QSerialPort::readyRead, [this]() {
            const QByteArray chunk = serialPort_->readAll();
            buffer_.append(chunk);
            rxByteCount_ += static_cast<qulonglong>(chunk.size());
            consumeFrames();
        });

        QObject::connect(
            serialPort_.get(),
            &QSerialPort::errorOccurred,
            [this](QSerialPort::SerialPortError error) {
                if (error == QSerialPort::NoError) {
                    return;
                }

                lastError_ = serialPort_->errorString();
                if (error == QSerialPort::ResourceError) {
                    disconnectDevice();
                }
            });
    }

    virtual ~Protocol55aaSerialBridge() = default;

    QVariantMap capabilities() const {
        QVariantMap caps{
            {QStringLiteral("deviceId"), deviceId_},
            {QStringLiteral("displayName"), displayName_},
            {QStringLiteral("transports"), QVariantList{QStringLiteral("serial")}},
            {QStringLiteral("actions"), actions_},
            {QStringLiteral("protocol.header"), QStringLiteral("55 AA")},
            {QStringLiteral("protocol.byteOrder"), byteOrderText(byteOrder_)},
            {QStringLiteral("protocol.frameLengthEndian"), frameLengthModeText(frameLengthMode_)},
        };

        appendCapabilities(caps);
        return caps;
    }

    bool connectSerial(const QVariantMap& options) {
        disconnectDevice();

        const QString portName = options.value(QStringLiteral("portName")).toString().trimmed();
        if (portName.isEmpty()) {
            lastError_ = QStringLiteral("portName is required");
            return false;
        }

        byteOrder_ = parseByteOrder(options, byteOrder_);
        frameLengthMode_ = parseFrameLengthMode(options, frameLengthMode_);
        if (frameLengthMode_ == FrameLengthMode::Auto) {
            txLengthByteOrder_ = byteOrder_;
        } else {
            txLengthByteOrder_ = frameLengthMode_ == FrameLengthMode::LittleEndian
                ? ByteOrderMode::LittleEndian
                : ByteOrderMode::BigEndian;
        }

        const int baudRate = options.value(QStringLiteral("baudRate"), 115200).toInt();
        const int dataBits = options.value(QStringLiteral("dataBits"), 8).toInt();
        const int parity = options.value(QStringLiteral("parity"), 0).toInt();
        const int stopBits = options.value(QStringLiteral("stopBits"), 1).toInt();

        serialPort_->setPortName(portName);
        serialPort_->setBaudRate(baudRate);
        serialPort_->setDataBits(static_cast<QSerialPort::DataBits>(dataBits));
        serialPort_->setParity(static_cast<QSerialPort::Parity>(parity));
        serialPort_->setStopBits(static_cast<QSerialPort::StopBits>(stopBits));
        serialPort_->setFlowControl(QSerialPort::NoFlowControl);

        if (!serialPort_->open(QIODevice::ReadWrite)) {
            lastError_ = serialPort_->errorString();
            return false;
        }

        buffer_.clear();
        lastError_.clear();
        portName_ = portName;
        baudRate_ = baudRate;
        setState(QStringLiteral("status.portName"), portName_);
        setState(QStringLiteral("status.baudRate"), baudRate_);
        setState(QStringLiteral("protocol.byteOrder"), byteOrderText(byteOrder_));
        setState(QStringLiteral("protocol.frameLengthEndian"), frameLengthModeText(frameLengthMode_));
        return true;
    }

    void disconnectDevice() {
        if (serialPort_ && serialPort_->isOpen()) {
            serialPort_->close();
        }
        buffer_.clear();
    }

    bool isConnected() const {
        return serialPort_ != nullptr && serialPort_->isOpen();
    }

    QVariantMap snapshot() const {
        QVariantMap snapshot = state_;
        snapshot.insert(QStringLiteral("device.id"), deviceId_);
        snapshot.insert(QStringLiteral("device.name"), displayName_);
        snapshot.insert(QStringLiteral("status.connected"), isConnected());
        snapshot.insert(QStringLiteral("status.lastError"), lastError_);
        snapshot.insert(QStringLiteral("status.portName"), portName_);
        snapshot.insert(QStringLiteral("status.baudRate"), baudRate_);
        snapshot.insert(QStringLiteral("tx.frameCount"), static_cast<qulonglong>(txFrameCount_));
        snapshot.insert(QStringLiteral("tx.byteCount"), static_cast<qulonglong>(txByteCount_));
        snapshot.insert(QStringLiteral("tx.lastFrameHex"), lastTxFrameHex_);
        snapshot.insert(QStringLiteral("tx.lastCommand"), lastTxCommand_);
        snapshot.insert(QStringLiteral("rx.frameCount"), static_cast<qulonglong>(rxFrameCount_));
        snapshot.insert(QStringLiteral("rx.byteCount"), static_cast<qulonglong>(rxByteCount_));
        snapshot.insert(QStringLiteral("rx.lastFrameHex"), lastRxFrameHex_);
        snapshot.insert(QStringLiteral("rx.lastPayloadHex"), lastRxPayloadHex_);
        snapshot.insert(QStringLiteral("rx.lastCommand"), lastRxCommand_);
        snapshot.insert(QStringLiteral("rx.lastTimestamp"), lastRxTimestamp_);
        return snapshot;
    }

    bool invokeCommon(const QString& action, const QVariantMap& args) {
        if (action == QString::fromLatin1(device_api::actions::kSendRawHex)) {
            const QByteArray frame = decodeHexString(args.value(QStringLiteral("hex")).toString());
            if (frame.isEmpty()) {
                lastError_ = QStringLiteral("invalid raw hex frame");
                return false;
            }
            return writeRawFrame(frame, QStringLiteral("raw.hex"));
        }
        return false;
    }

protected:
    virtual void appendCapabilities(QVariantMap& caps) const = 0;
    virtual bool handleAction(const QString& action, const QVariantMap& args) = 0;
    virtual void handlePayload(quint8 cmd, const QByteArray& content) = 0;

    bool invokeDevice(const QString& action, const QVariantMap& args) {
        if (invokeCommon(action, args)) {
            return true;
        }
        return handleAction(action, args);
    }

    bool sendCommand(quint8 cmd, const QByteArray& content = {}) {
        QByteArray payload;
        payload.reserve(1 + content.size());
        payload.push_back(static_cast<char>(cmd));
        payload.append(content);

        QByteArray frame;
        frame.reserve(payload.size() + 5);
        frame.push_back(static_cast<char>(kHeader1));
        frame.push_back(static_cast<char>(kHeader2));
        appendUInt16(frame, static_cast<quint16>(payload.size()), txLengthByteOrder_);
        frame.append(payload);
        frame.push_back(static_cast<char>(checksum8(frame)));

        return writeRawFrame(frame, commandHex(cmd));
    }

    bool sendCommandFromHex(const QVariantMap& args) {
        bool ok = false;
        const quint8 cmd = parseCommandCode(args.value(QStringLiteral("cmd")), &ok);
        if (!ok) {
            lastError_ = QStringLiteral("invalid cmd");
            return false;
        }

        const QByteArray content = decodeHexString(args.value(QStringLiteral("payloadHex")).toString());
        if (!args.value(QStringLiteral("payloadHex")).toString().trimmed().isEmpty() && content.isEmpty()) {
            lastError_ = QStringLiteral("invalid payloadHex");
            return false;
        }

        return sendCommand(cmd, content);
    }

    void setState(const QString& key, const QVariant& value) {
        state_.insert(key, value);
    }

    ByteOrderMode byteOrder() const {
        return byteOrder_;
    }

    void setLastAck(quint8 cmd, bool ok) {
        setState(QStringLiteral("status.lastAck.command"), commandHex(cmd));
        setState(QStringLiteral("status.lastAck.ok"), ok);
        setState(
            QStringLiteral("status.lastAck.timestamp"),
            QDateTime::currentDateTime().toString(Qt::ISODate));
    }

private:
    bool resolveFrameLength(const QByteArray& buffer, quint16* lengthOut) const {
        if (buffer.size() < 4) {
            return false;
        }

        const quint16 le = readUInt16(buffer, 2, ByteOrderMode::LittleEndian);
        const quint16 be = readUInt16(buffer, 2, ByteOrderMode::BigEndian);

        const auto plausible = [](quint16 length) {
            return length > 0 && length <= kMaxPayloadLength;
        };

        if (frameLengthMode_ == FrameLengthMode::LittleEndian) {
            if (!plausible(le)) {
                return false;
            }
            *lengthOut = le;
            return true;
        }

        if (frameLengthMode_ == FrameLengthMode::BigEndian) {
            if (!plausible(be)) {
                return false;
            }
            *lengthOut = be;
            return true;
        }

        const bool leOk = plausible(le);
        const bool beOk = plausible(be);
        if (!leOk && !beOk) {
            return false;
        }

        *lengthOut = leOk ? le : be;
        return true;
    }

    bool writeRawFrame(const QByteArray& frame, const QString& commandName) {
        lastTxFrameHex_ = toHexString(frame);
        lastTxCommand_ = commandName;

        if (!isConnected()) {
            lastError_ = QStringLiteral("device is not connected");
            return false;
        }

        const qint64 written = serialPort_->write(frame);
        if (written != frame.size()) {
            lastError_ = serialPort_->errorString();
            return false;
        }

        serialPort_->flush();
        lastError_.clear();
        txFrameCount_ += 1;
        txByteCount_ += static_cast<qulonglong>(frame.size());
        return true;
    }

    void consumeFrames() {
        while (true) {
            const int start = buffer_.indexOf(QByteArray("\x55\xAA", 2));
            if (start < 0) {
                buffer_.clear();
                return;
            }

            if (start > 0) {
                buffer_.remove(0, start);
            }

            if (buffer_.size() < 5) {
                return;
            }

            quint16 payloadLength = 0;
            if (!resolveFrameLength(buffer_, &payloadLength)) {
                lastError_ = QStringLiteral("invalid frame length");
                buffer_.remove(0, 1);
                continue;
            }

            const int frameSize = 2 + 2 + payloadLength + 1;
            if (buffer_.size() < frameSize) {
                return;
            }

            const QByteArray frame = buffer_.left(frameSize);
            buffer_.remove(0, frameSize);

            const quint8 actualChecksum = static_cast<quint8>(frame.back());
            const quint8 expectedChecksum = checksum8(frame.left(frameSize - 1));
            if (actualChecksum != expectedChecksum) {
                lastError_ = QStringLiteral("checksum mismatch");
                continue;
            }

            const QByteArray payload = frame.mid(4, payloadLength);
            if (payload.isEmpty()) {
                lastError_ = QStringLiteral("empty payload");
                continue;
            }

            lastError_.clear();
            lastRxFrameHex_ = toHexString(frame);
            lastRxPayloadHex_ = toHexString(payload);
            lastRxCommand_ = commandHex(static_cast<quint8>(payload.front()));
            lastRxTimestamp_ = QDateTime::currentDateTime().toString(Qt::ISODate);
            rxFrameCount_ += 1;

            handlePayload(static_cast<quint8>(payload.front()), payload.mid(1));
        }
    }

    QString deviceId_;
    QString displayName_;
    QVariantList actions_;
    std::unique_ptr<QSerialPort> serialPort_;
    QByteArray buffer_;
    QVariantMap state_;
    QString lastError_;
    QString portName_;
    int baudRate_ = 115200;
    ByteOrderMode byteOrder_ = ByteOrderMode::LittleEndian;
    FrameLengthMode frameLengthMode_ = FrameLengthMode::Auto;
    ByteOrderMode txLengthByteOrder_ = ByteOrderMode::LittleEndian;
    qulonglong txFrameCount_ = 0;
    qulonglong txByteCount_ = 0;
    qulonglong rxFrameCount_ = 0;
    qulonglong rxByteCount_ = 0;
    QString lastTxFrameHex_;
    QString lastTxCommand_;
    QString lastRxFrameHex_;
    QString lastRxPayloadHex_;
    QString lastRxCommand_;
    QString lastRxTimestamp_;
};

}  // namespace

class Encoder55aaDeviceBridge::Impl final : public Protocol55aaSerialBridge {
public:
    Impl()
        : Protocol55aaSerialBridge(
              QStringLiteral("encoder"),
              QStringLiteral("Encoder 55AA Device"),
              QVariantList{
                  QString::fromLatin1(device_api::actions::kConnectSerial),
                  QString::fromLatin1(device_api::actions::kDisconnect),
                  QString::fromLatin1(device_api::actions::kSendRawHex),
                  QString::fromLatin1(kEncoderSendCommand),
                  QString::fromLatin1(kEncoderStopUpload),
                  QString::fromLatin1(kEncoderStartCoarseUpload),
                  QString::fromLatin1(kEncoderStartPreciseUpload),
                  QString::fromLatin1(kEncoderReadSerial),
                  QString::fromLatin1(kEncoderWriteSerial),
                  QString::fromLatin1(kEncoderReadVersion),
                  QString::fromLatin1(kEncoderReadResolution),
                  QString::fromLatin1(kEncoderWriteResolution),
                  QString::fromLatin1(kEncoderReadProtocol),
                  QString::fromLatin1(kEncoderWriteProtocol),
                  QString::fromLatin1(kEncoderReadOuterIps),
                  QString::fromLatin1(kEncoderWriteOuterIps),
                  QString::fromLatin1(kEncoderReadInnerIps),
                  QString::fromLatin1(kEncoderWriteInnerIps),
                  QString::fromLatin1(kEncoderSetFeedbackPeriod),
                  QString::fromLatin1(kEncoderWriteBigCycleCompensation),
                  QString::fromLatin1(kEncoderWriteSmallCycleCompensation),
                  QString::fromLatin1(kEncoderWriteAdCompensation),
                  QString::fromLatin1(kEncoderResetCompensation),
              }) {}

    bool invoke(const QString& action, const QVariantMap& args) {
        return invokeDevice(action, args);
    }

protected:
    void appendCapabilities(QVariantMap& caps) const override {
        caps.insert(QStringLiteral("protocol.type"), QStringLiteral("55aa.encoder"));
    }

    bool handleAction(const QString& action, const QVariantMap& args) override {
        if (action == QString::fromLatin1(kEncoderSendCommand)) {
            return sendCommandFromHex(args);
        }
        if (action == QString::fromLatin1(kEncoderStopUpload)) {
            return sendCommand(0xF0);
        }
        if (action == QString::fromLatin1(kEncoderStartCoarseUpload)) {
            return sendCommand(0xF1);
        }
        if (action == QString::fromLatin1(kEncoderStartPreciseUpload)) {
            return sendCommand(0xF2);
        }
        if (action == QString::fromLatin1(kEncoderReadSerial)) {
            return sendCommand(0xF3);
        }
        if (action == QString::fromLatin1(kEncoderWriteSerial)) {
            quint32 serialNumber = 0;
            if (!variantToUInt32(args.value(QStringLiteral("serialNumber")), &serialNumber)) {
                return false;
            }
            QByteArray payload;
            appendUInt32(payload, serialNumber, byteOrder());
            return sendCommand(0xF4, payload);
        }
        if (action == QString::fromLatin1(kEncoderReadVersion)) {
            return sendCommand(0xF5);
        }
        if (action == QString::fromLatin1(kEncoderReadResolution)) {
            return sendCommand(0xF6);
        }
        if (action == QString::fromLatin1(kEncoderWriteResolution)) {
            QByteArray payload;
            quint8 resolutionInner = 0;
            quint8 resolutionOuter = 0;
            const bool hasInner = variantToUInt8(args.value(QStringLiteral("resolutionInner")), &resolutionInner);
            const bool hasOuter = variantToUInt8(args.value(QStringLiteral("resolutionOuter")), &resolutionOuter);
            if (hasInner || hasOuter) {
                if (!hasInner) {
                    resolutionInner = resolutionOuter;
                }
                if (!hasOuter) {
                    resolutionOuter = resolutionInner;
                }
                payload.push_back(static_cast<char>(resolutionOuter));
                payload.push_back(static_cast<char>(resolutionInner));
                return sendCommand(0xF7, payload);
            }

            quint16 resolution = 0;
            if (!variantToUInt16(args.value(QStringLiteral("resolution")), &resolution)) {
                return false;
            }

            const QString encodingHint = args.value(QStringLiteral("resolutionEncoding")).toString().trimmed().toLower();
            const bool useUInt16Payload = args.value(QStringLiteral("resolutionAsUInt16")).toBool()
                || encodingHint == QStringLiteral("u16")
                || encodingHint == QStringLiteral("single_u16")
                || resolution > 0xFF;

            if (useUInt16Payload) {
                appendUInt16(payload, resolution, byteOrder());
            } else {
                const quint8 resolutionValue = static_cast<quint8>(resolution & 0xFF);
                payload.push_back(static_cast<char>(resolutionValue));
                payload.push_back(static_cast<char>(resolutionValue));
            }
            return sendCommand(0xF7, payload);
        }
        if (action == QString::fromLatin1(kEncoderReadProtocol)) {
            return sendCommand(0xF8);
        }
        if (action == QString::fromLatin1(kEncoderWriteProtocol)) {
            quint16 protocolType = 0;
            if (!variantToUInt16(args.value(QStringLiteral("protocolType")), &protocolType)) {
                return false;
            }
            QByteArray payload;
            const QString encodingHint = args.value(QStringLiteral("protocolEncoding")).toString().trimmed().toLower();
            const bool useUInt16Payload = args.value(QStringLiteral("protocolAsUInt16")).toBool()
                || encodingHint == QStringLiteral("u16")
                || encodingHint == QStringLiteral("single_u16")
                || protocolType > 0xFF;
            if (useUInt16Payload) {
                appendUInt16(payload, protocolType, byteOrder());
            } else {
                payload.push_back(static_cast<char>(protocolType & 0xFF));
            }
            return sendCommand(0xF9, payload);
        }
        if (action == QString::fromLatin1(kEncoderReadOuterIps)) {
            return sendCommand(0xFA);
        }
        if (action == QString::fromLatin1(kEncoderWriteOuterIps)) {
            QByteArray payload;
            if (!appendUInt16Values(payload, args.value(QStringLiteral("values")), 8, byteOrder())) {
                return false;
            }
            return sendCommand(0xFB, payload);
        }
        if (action == QString::fromLatin1(kEncoderReadInnerIps)) {
            return sendCommand(0xFC);
        }
        if (action == QString::fromLatin1(kEncoderWriteInnerIps)) {
            QByteArray payload;
            if (!appendUInt16Values(payload, args.value(QStringLiteral("values")), 8, byteOrder())) {
                return false;
            }
            return sendCommand(0xFD, payload);
        }
        if (action == QString::fromLatin1(kEncoderSetFeedbackPeriod)) {
            quint32 seconds = 0;
            if (!variantToUInt32(args.value(QStringLiteral("seconds")), &seconds)) {
                return false;
            }
            QByteArray payload;
            appendUInt32(payload, seconds, byteOrder());
            return sendCommand(0xFE, payload);
        }
        if (action == QString::fromLatin1(kEncoderWriteBigCycleCompensation)
            || action == QString::fromLatin1(kEncoderWriteSmallCycleCompensation)) {
            const quint8 command = encoderCompensationCommand(
                args.value(QStringLiteral("ring")).toString(),
                args.value(QStringLiteral("mode")).toString(),
                action == QString::fromLatin1(kEncoderWriteBigCycleCompensation) ? 1 : 2);
            if (command == 0) {
                return false;
            }

            quint8 order = 0;
            quint8 times = 0;
            if (!variantToUInt8(args.value(QStringLiteral("order")), &order)
                || !variantToUInt8(args.value(QStringLiteral("times")), &times)) {
                return false;
            }

            QByteArray payload;
            payload.push_back(static_cast<char>(order));
            payload.push_back(static_cast<char>(times));
            if (!appendFloatValuesVariableByStep(
                    payload,
                    args.value(QStringLiteral("coefficients")),
                    3,
                    24,
                    3,
                    byteOrder())) {
                return false;
            }
            return sendCommand(command, payload);
        }
        if (action == QString::fromLatin1(kEncoderWriteAdCompensation)) {
            const quint8 command = encoderCompensationCommand(
                args.value(QStringLiteral("ring")).toString(),
                args.value(QStringLiteral("mode")).toString(),
                3);
            if (command == 0) {
                return false;
            }

            float sinOffset = 0.0f;
            float cosOffset = 0.0f;
            float sinAmplitude = 0.0f;
            float cosAmplitude = 0.0f;
            if (!variantToFloat(args.value(QStringLiteral("sinOffset")), &sinOffset)
                || !variantToFloat(args.value(QStringLiteral("cosOffset")), &cosOffset)
                || !variantToFloat(args.value(QStringLiteral("sinAmplitude")), &sinAmplitude)
                || !variantToFloat(args.value(QStringLiteral("cosAmplitude")), &cosAmplitude)) {
                return false;
            }

            QByteArray payload;
            appendFloat(payload, sinOffset, byteOrder());
            appendFloat(payload, cosOffset, byteOrder());
            appendFloat(payload, sinAmplitude, byteOrder());
            appendFloat(payload, cosAmplitude, byteOrder());
            return sendCommand(command, payload);
        }
        if (action == QString::fromLatin1(kEncoderResetCompensation)) {
            const quint8 command = encoderCompensationCommand(
                args.value(QStringLiteral("ring")).toString(),
                args.value(QStringLiteral("mode")).toString(),
                4);
            return command != 0 && sendCommand(command);
        }
        return false;
    }

    void handlePayload(quint8 cmd, const QByteArray& content) override {
        setState(QStringLiteral("encoder.last.command"), commandHex(cmd));
        setState(QStringLiteral("encoder.last.payloadHex"), toHexString(content));

        if (content.size() == 1 && (content.front() == '\0' || content.front() == '\1')) {
            setLastAck(cmd, static_cast<quint8>(content.front()) == 1);
            return;
        }

        if ((cmd == 0xF1 || cmd == 0xF2) && content.size() >= 12) {
            setState(
                QStringLiteral("encoder.streaming.mode"),
                cmd == 0xF1 ? QStringLiteral("coarse") : QStringLiteral("precise"));
            setState(QStringLiteral("encoder.streaming.payloadLength"), content.size());

            setState(QStringLiteral("encoder.outer.sinAd"), static_cast<int>(readUInt16(content, 0, byteOrder())));
            setState(QStringLiteral("encoder.outer.cosAd"), static_cast<int>(readUInt16(content, 2, byteOrder())));
            setState(QStringLiteral("encoder.outer.angle"), readFloat(content, 4, byteOrder()));
            setState(QStringLiteral("encoder.outer.angleComp"), readFloat(content, 8, byteOrder()));

            if (content.size() >= 24) {
                setState(QStringLiteral("encoder.channelType"), QStringLiteral("double"));
                setState(QStringLiteral("encoder.inner.sinAd"), static_cast<int>(readUInt16(content, 4, byteOrder())));
                setState(QStringLiteral("encoder.inner.cosAd"), static_cast<int>(readUInt16(content, 6, byteOrder())));
                setState(QStringLiteral("encoder.outer.angle"), readFloat(content, 8, byteOrder()));
                setState(QStringLiteral("encoder.outer.angleComp"), readFloat(content, 12, byteOrder()));
                setState(QStringLiteral("encoder.inner.angle"), readFloat(content, 16, byteOrder()));
                setState(QStringLiteral("encoder.inner.angleComp"), readFloat(content, 20, byteOrder()));
            } else {
                setState(QStringLiteral("encoder.channelType"), QStringLiteral("single"));
                setState(QStringLiteral("encoder.inner.sinAd"), 0);
                setState(QStringLiteral("encoder.inner.cosAd"), 0);
                setState(QStringLiteral("encoder.inner.angle"), 0.0);
                setState(QStringLiteral("encoder.inner.angleComp"), 0.0);
            }

            const bool hasOuterGap = content.size() >= 26;
            const bool hasInnerGap = content.size() >= 28;
            if (hasOuterGap) {
                const quint16 outerGapRaw = readUInt16(content, 24, byteOrder());
                setState(QStringLiteral("encoder.outer.gapRaw"), static_cast<int>(outerGapRaw));
                setState(QStringLiteral("encoder.outer.gapPercent"), gapPercentFromRaw(outerGapRaw));
            } else {
                setState(QStringLiteral("encoder.outer.gapRaw"), -1);
                setState(QStringLiteral("encoder.outer.gapPercent"), -1.0);
            }

            if (hasInnerGap) {
                const quint16 innerGapRaw = readUInt16(content, 26, byteOrder());
                setState(QStringLiteral("encoder.inner.gapRaw"), static_cast<int>(innerGapRaw));
                setState(QStringLiteral("encoder.inner.gapPercent"), gapPercentFromRaw(innerGapRaw));
            } else {
                setState(QStringLiteral("encoder.inner.gapRaw"), -1);
                setState(QStringLiteral("encoder.inner.gapPercent"), -1.0);
            }

            setState(
                QStringLiteral("encoder.gap.source"),
                (hasOuterGap || hasInnerGap) ? QStringLiteral("frame_ext") : QStringLiteral("none"));
            return;
        }

        if (cmd == 0xF3 && content.size() >= 4) {
            setState(
                QStringLiteral("encoder.serialNumber"),
                static_cast<qulonglong>(readUInt32(content, 0, byteOrder())));
            return;
        }

        if (cmd == 0xF5) {
            const QString fallbackText = decodeTextPayload(content);
            const QStringList versionItems = decodeTextPayloadItems(content);
            QString versionId;
            QString versionSoftware;
            QString versionHardware;

            if (versionItems.size() >= 3) {
                versionId = versionItems[0];
                versionSoftware = versionItems[1];
                versionHardware = versionItems[2];
            } else {
                if (versionItems.size() >= 1) {
                    versionId = versionItems[0];
                }
                if (versionItems.size() >= 2) {
                    versionSoftware = versionItems[1];
                }
                if (versionItems.size() >= 3) {
                    versionHardware = versionItems[2];
                }
            }

            if (versionHardware.isEmpty()) {
                versionHardware = !fallbackText.isEmpty() ? fallbackText : versionSoftware;
            }
            if (versionSoftware.isEmpty()) {
                versionSoftware = !fallbackText.isEmpty() ? fallbackText : versionHardware;
            }
            if (versionId.isEmpty()) {
                versionId = fallbackText;
            }

            setState(QStringLiteral("encoder.versionText"), versionHardware);
            setState(QStringLiteral("encoder.version.id"), versionId);
            setState(QStringLiteral("encoder.version.software"), versionSoftware);
            setState(QStringLiteral("encoder.version.hardware"), versionHardware);
            setState(QStringLiteral("encoder.versionId"), versionId);
            setState(QStringLiteral("encoder.versionSoftware"), versionSoftware);
            setState(QStringLiteral("encoder.versionHardware"), versionHardware);
            setState(QStringLiteral("encoder.versionItemCount"), versionItems.size());
            setState(QStringLiteral("encoder.versionRawHex"), toHexString(content));
            return;
        }

        if (cmd == 0xF6 && content.size() >= 2) {
            const quint16 rawResolution = readUInt16(content, 0, byteOrder());
            int resolutionInner = static_cast<int>(rawResolution);
            int resolutionOuter = static_cast<int>(rawResolution);
            QString resolutionSource = QStringLiteral("u16");

            if (content.size() >= 4) {
                resolutionOuter = static_cast<int>(readUInt16(content, 0, byteOrder()));
                resolutionInner = static_cast<int>(readUInt16(content, 2, byteOrder()));
                resolutionSource = QStringLiteral("u16_pair");
            } else {
                const int rawOuter = static_cast<int>(static_cast<quint8>(content[0]));
                const int rawInner = static_cast<int>(static_cast<quint8>(content[1]));
                const bool looksLegacyPair = rawOuter >= 8 && rawOuter <= 32 && rawInner >= 8 && rawInner <= 32;
                if (looksLegacyPair) {
                    resolutionOuter = rawOuter;
                    resolutionInner = rawInner;
                    resolutionSource = QStringLiteral("u8_pair");
                }
            }

            setState(QStringLiteral("encoder.resolution"), resolutionInner);
            setState(QStringLiteral("encoder.resolutionInner"), resolutionInner);
            setState(QStringLiteral("encoder.resolutionOuter"), resolutionOuter);
            setState(
                QStringLiteral("encoder.resolutionText"),
                QStringLiteral("inner:%1 outer:%2").arg(resolutionInner).arg(resolutionOuter));
            setState(QStringLiteral("encoder.resolutionSource"), resolutionSource);
            return;
        }

        if (cmd == 0xF8 && content.size() >= 1) {
            quint16 protocol = 0;
            QString protocolText;
            QString protocolSource;
            if (content.size() == 1) {
                protocol = static_cast<quint8>(content[0]);
                protocolText = legacyEncoderProtocolText(static_cast<quint8>(protocol));
                protocolSource = QStringLiteral("u8");
            } else {
                protocol = readUInt16(content, 0, byteOrder());
                protocolText = encoderProtocolText(protocol);
                protocolSource = QStringLiteral("u16");
            }
            setState(QStringLiteral("encoder.protocolType"), static_cast<int>(protocol));
            setState(QStringLiteral("encoder.protocolText"), protocolText);
            setState(QStringLiteral("encoder.protocolSource"), protocolSource);
            setState(QStringLiteral("encoder.protocolPayloadLength"), content.size());
            return;
        }

        if ((cmd == 0xFA || cmd == 0xFC) && content.size() >= 16) {
            const QVariantList values = parseUInt16List(content, byteOrder());
            const QString prefix = cmd == 0xFA ? QStringLiteral("encoder.outerIps") : QStringLiteral("encoder.innerIps");
            const QStringList keys{
                QStringLiteral("sysCfg1"),
                QStringLiteral("sysCfg2"),
                QStringLiteral("r1r2Gain"),
                QStringLiteral("r1Gain"),
                QStringLiteral("r1Offset"),
                QStringLiteral("r2Gain"),
                QStringLiteral("r2Offset"),
                QStringLiteral("txCurrent"),
            };
            setState(prefix, values);
            for (int i = 0; i < keys.size() && i < values.size(); ++i) {
                setState(prefix + QLatin1Char('.') + keys[i], values[i]);
            }
            return;
        }

        if (cmd == 0xFF && content.size() >= 4) {
            setState(
                QStringLiteral("encoder.exceptionStatus"),
                static_cast<qulonglong>(readUInt32(content, 0, byteOrder())));
        }
    }
};

class EddyCurrent55aaDeviceBridge::Impl final : public Protocol55aaSerialBridge {
public:
    Impl()
        : Protocol55aaSerialBridge(
              QStringLiteral("eddy_current"),
              QStringLiteral("Eddy Current Device"),
              QVariantList{
                  QString::fromLatin1(device_api::actions::kConnectSerial),
                  QString::fromLatin1(device_api::actions::kDisconnect),
                  QString::fromLatin1(device_api::actions::kSendRawHex),
                  QString::fromLatin1(kEddySendCommand),
                  QString::fromLatin1(kEddyStopUpload),
                  QString::fromLatin1(kEddyStartAdUpload),
                  QString::fromLatin1(kEddyStartDisplacementUpload),
                  QString::fromLatin1(kEddySetFeedbackPeriod),
                  QString::fromLatin1(kEddyReadSerial),
                  QString::fromLatin1(kEddyWriteSerial),
                  QString::fromLatin1(kEddyReadVersion),
                  QString::fromLatin1(kEddyReadFrequency),
                  QString::fromLatin1(kEddyWriteFrequency),
                  QString::fromLatin1(kEddyReadRange),
                  QString::fromLatin1(kEddyWriteRange),
                  QString::fromLatin1(kEddyReadFitParams),
                  QString::fromLatin1(kEddyWriteFitParams),
                  QString::fromLatin1(kEddyReadInterpolationParams),
                  QString::fromLatin1(kEddyWriteInterpolationParams),
                  QString::fromLatin1(kEddyReadGain),
                  QString::fromLatin1(kEddyWriteGain),
                  QString::fromLatin1(kEddyReadZero),
                  QString::fromLatin1(kEddyWriteZero),
                  QString::fromLatin1(kEddyReadProtocol),
                  QString::fromLatin1(kEddyWriteProtocol),
                  QString::fromLatin1(kEddyReadPhase),
                  QString::fromLatin1(kEddyWritePhase),
              }) {}

    bool invoke(const QString& action, const QVariantMap& args) {
        return invokeDevice(action, args);
    }

protected:
    void appendCapabilities(QVariantMap& caps) const override {
        caps.insert(QStringLiteral("protocol.type"), QStringLiteral("55aa.eddy_current"));
    }

    bool handleAction(const QString& action, const QVariantMap& args) override {
        if (action == QString::fromLatin1(kEddySendCommand)) {
            return sendCommandFromHex(args);
        }
        if (action == QString::fromLatin1(kEddyStopUpload)) {
            return sendCommand(0xB0);
        }
        if (action == QString::fromLatin1(kEddyStartAdUpload)) {
            return sendCommand(0xB1);
        }
        if (action == QString::fromLatin1(kEddyStartDisplacementUpload)) {
            return sendCommand(0xB2);
        }
        if (action == QString::fromLatin1(kEddySetFeedbackPeriod)) {
            quint32 seconds = 0;
            if (!variantToUInt32(args.value(QStringLiteral("seconds")), &seconds)) {
                return false;
            }
            QByteArray payload;
            appendUInt32(payload, seconds, byteOrder());
            return sendCommand(0xB3, payload);
        }
        if (action == QString::fromLatin1(kEddyReadSerial)) {
            return sendCommand(0xE0);
        }
        if (action == QString::fromLatin1(kEddyWriteSerial)) {
            quint32 serialNumber = 0;
            if (!variantToUInt32(args.value(QStringLiteral("serialNumber")), &serialNumber)) {
                return false;
            }
            QByteArray payload;
            appendUInt32(payload, serialNumber, byteOrder());
            return sendCommand(0xE1, payload);
        }
        if (action == QString::fromLatin1(kEddyReadVersion)) {
            return sendCommand(0xE2);
        }
        if (action == QString::fromLatin1(kEddyReadFrequency)) {
            return sendCommand(0xE3);
        }
        if (action == QString::fromLatin1(kEddyWriteFrequency)) {
            quint32 frequency = 0;
            if (!variantToUInt32(args.value(QStringLiteral("frequency")), &frequency)) {
                return false;
            }
            QByteArray payload;
            appendUInt32(payload, frequency, byteOrder());
            return sendCommand(0xE4, payload);
        }
        if (action == QString::fromLatin1(kEddyReadRange)) {
            return sendCommand(0xE5);
        }
        if (action == QString::fromLatin1(kEddyWriteRange)) {
            QByteArray payload;
            if (!appendFloatValues(payload, args.value(QStringLiteral("values")), 2, byteOrder())) {
                return false;
            }
            return sendCommand(0xE6, payload);
        }
        if (action == QString::fromLatin1(kEddyReadFitParams)) {
            return sendCommand(0xE7);
        }
        if (action == QString::fromLatin1(kEddyWriteFitParams)) {
            QByteArray payload;
            if (!appendFloatValuesVariable(payload, args.value(QStringLiteral("values")), 2, 24, byteOrder())) {
                return false;
            }
            return sendCommand(0xE8, payload);
        }
        if (action == QString::fromLatin1(kEddyReadInterpolationParams)) {
            return sendCommand(0xE9);
        }
        if (action == QString::fromLatin1(kEddyWriteInterpolationParams)) {
            QByteArray payload;
            if (!appendFloatValuesVariable(payload, args.value(QStringLiteral("values")), 2, 24, byteOrder())) {
                return false;
            }
            return sendCommand(0xEA, payload);
        }
        if (action == QString::fromLatin1(kEddyReadGain)) {
            return sendCommand(0xE7);
        }
        if (action == QString::fromLatin1(kEddyWriteGain)) {
            QByteArray payload;
            if (!appendFloatValuesVariable(payload, args.value(QStringLiteral("values")), 2, 24, byteOrder())) {
                return false;
            }
            return sendCommand(0xE8, payload);
        }
        if (action == QString::fromLatin1(kEddyReadZero)) {
            return sendCommand(0xE9);
        }
        if (action == QString::fromLatin1(kEddyWriteZero)) {
            QByteArray payload;
            if (!appendFloatValuesVariable(payload, args.value(QStringLiteral("values")), 2, 24, byteOrder())) {
                return false;
            }
            return sendCommand(0xEA, payload);
        }
        if (action == QString::fromLatin1(kEddyReadProtocol)) {
            return sendCommand(0xEB);
        }
        if (action == QString::fromLatin1(kEddyWriteProtocol)) {
            quint32 protocolType = 0;
            if (!variantToUInt32(args.value(QStringLiteral("protocolType")), &protocolType)) {
                return false;
            }
            QByteArray payload;
            appendUInt32(payload, protocolType, byteOrder());
            return sendCommand(0xEC, payload);
        }
        if (action == QString::fromLatin1(kEddyReadPhase)) {
            return sendCommand(0xED);
        }
        if (action == QString::fromLatin1(kEddyWritePhase)) {
            QByteArray payload;
            if (!appendUInt32Values(payload, args.value(QStringLiteral("values")), 2, byteOrder())) {
                return false;
            }
            return sendCommand(0xEE, payload);
        }
        return false;
    }

    void handlePayload(quint8 cmd, const QByteArray& content) override {
        setState(QStringLiteral("eddy.last.command"), commandHex(cmd));
        setState(QStringLiteral("eddy.last.payloadHex"), toHexString(content));

        if (content.size() == 1 && (content.front() == '\0' || content.front() == '\1')) {
            setLastAck(cmd, static_cast<quint8>(content.front()) == 1);
            return;
        }

        if (cmd == 0xB1 && content.size() >= 4) {
            const QVariantList values = parseUInt32List(content, byteOrder());
            setState(QStringLiteral("eddy.uploadMode"), QStringLiteral("ad"));
            setState(QStringLiteral("eddy.adValues"), values);
            setState(QStringLiteral("eddy.adCount"), values.size());
            return;
        }

        if (cmd == 0xB2 && content.size() >= 4) {
            const QVariantList values = parseFloatList(content, byteOrder());
            setState(QStringLiteral("eddy.uploadMode"), QStringLiteral("displacement"));
            setState(QStringLiteral("eddy.displacementValues"), values);
            setState(QStringLiteral("eddy.displacementCount"), values.size());
            return;
        }

        if (cmd == 0xB4 && content.size() >= 6) {
            setState(QStringLiteral("eddy.status.overRange"), readUInt16(content, 0, byteOrder()) != 0);
            setState(QStringLiteral("eddy.status.power"), static_cast<int>(readUInt16(content, 2, byteOrder())));
            setState(QStringLiteral("eddy.status.temperature"), static_cast<int>(readUInt16(content, 4, byteOrder())));
            if (content.size() > 6) {
                setState(QStringLiteral("eddy.status.rawHex"), toHexString(content.mid(6)));
            }
            return;
        }

        if (cmd == 0xE0 && content.size() >= 4) {
            setState(
                QStringLiteral("eddy.serialNumber"),
                static_cast<qulonglong>(readUInt32(content, 0, byteOrder())));
            return;
        }

        if (cmd == 0xE2) {
            setState(QStringLiteral("eddy.versionText"), decodeTextPayload(content));
            setState(QStringLiteral("eddy.versionRawHex"), toHexString(content));
            return;
        }

        if (cmd == 0xE3 && content.size() >= 4) {
            setState(
                QStringLiteral("eddy.frequency"),
                static_cast<qulonglong>(readUInt32(content, 0, byteOrder())));
            return;
        }

        if (cmd == 0xE5 && content.size() >= 8) {
            setState(QStringLiteral("eddy.range"), parseFloatList(content.left(8), byteOrder()));
            return;
        }

        if (cmd == 0xE7 && content.size() >= 4) {
            const QVariantList values = parseFloatList(content, byteOrder());
            setState(QStringLiteral("eddy.fitParams"), values);
            setState(QStringLiteral("eddy.gain"), values);
            setState(QStringLiteral("eddy.fitParamCount"), values.size());
            return;
        }

        if (cmd == 0xE9 && content.size() >= 4) {
            const QVariantList values = parseFloatList(content, byteOrder());
            setState(QStringLiteral("eddy.interpolationParams"), values);
            setState(QStringLiteral("eddy.zero"), values);
            setState(QStringLiteral("eddy.interpolationParamCount"), values.size());
            return;
        }

        if (cmd == 0xEB && content.size() >= 4) {
            const qulonglong protocolType = static_cast<qulonglong>(readUInt32(content, 0, byteOrder()));
            setState(
                QStringLiteral("eddy.protocolType"),
                protocolType);
            setState(
                QStringLiteral("eddy.protocolText"),
                protocolType == 0 ? QStringLiteral("standard")
                                  : (protocolType == 1 ? QStringLiteral("rkx")
                                                       : QStringLiteral("unknown")));
            return;
        }

        if (cmd == 0xED && content.size() >= 8) {
            const QVariantList values = parseUInt32List(content.left(8), byteOrder());
            setState(QStringLiteral("eddy.phase"), values);
            setState(QStringLiteral("eddy.phaseValues"), values);
        }
    }
};

Encoder55aaDeviceBridge::Encoder55aaDeviceBridge()
    : impl_(std::make_unique<Impl>()) {}

Encoder55aaDeviceBridge::~Encoder55aaDeviceBridge() = default;

QVariantMap Encoder55aaDeviceBridge::capabilities() const {
    return impl_->capabilities();
}

bool Encoder55aaDeviceBridge::connectSerial(const QVariantMap& options) {
    return impl_->connectSerial(options);
}

void Encoder55aaDeviceBridge::disconnectDevice() {
    impl_->disconnectDevice();
}

bool Encoder55aaDeviceBridge::isConnected() const {
    return impl_->isConnected();
}

bool Encoder55aaDeviceBridge::invoke(const QString& action, const QVariantMap& args) {
    return impl_->invoke(action, args);
}

QVariantMap Encoder55aaDeviceBridge::snapshot() const {
    return impl_->snapshot();
}

EddyCurrent55aaDeviceBridge::EddyCurrent55aaDeviceBridge()
    : impl_(std::make_unique<Impl>()) {}

EddyCurrent55aaDeviceBridge::~EddyCurrent55aaDeviceBridge() = default;

QVariantMap EddyCurrent55aaDeviceBridge::capabilities() const {
    return impl_->capabilities();
}

bool EddyCurrent55aaDeviceBridge::connectSerial(const QVariantMap& options) {
    return impl_->connectSerial(options);
}

void EddyCurrent55aaDeviceBridge::disconnectDevice() {
    impl_->disconnectDevice();
}

bool EddyCurrent55aaDeviceBridge::isConnected() const {
    return impl_->isConnected();
}

bool EddyCurrent55aaDeviceBridge::invoke(const QString& action, const QVariantMap& args) {
    return impl_->invoke(action, args);
}

QVariantMap EddyCurrent55aaDeviceBridge::snapshot() const {
    return impl_->snapshot();
}
