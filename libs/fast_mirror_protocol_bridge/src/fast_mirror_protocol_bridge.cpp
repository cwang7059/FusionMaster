#include "fast_mirror_protocol_bridge/fast_mirror_protocol_bridge.h"

#include <plugin_api/idevice_capabilities.h>

#include <QByteArray>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QList>
#include <QSerialPort>
#include <QSet>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>
#include <QTextStream>
#include <QTimer>
#include <QUrl>
#include <QVariantList>

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstring>

namespace {

constexpr quint8 kHeader55AA1 = 0x55;
constexpr quint8 kHeader55AA2 = 0xAA;
constexpr quint8 kHeaderLegacy = 0x7E;
constexpr quint8 kHeaderIap1 = 0x7E;
constexpr quint8 kHeaderIap2 = 0xE7;
constexpr quint16 kMaxPayloadLength = 256;
constexpr quint16 kMaxIapPayloadLength = 4096;
constexpr int kDefaultBaudRate = 230400;
constexpr int kDefaultSeriesSize = 500;
constexpr quint8 kIapHostId = 0x64;
constexpr quint8 kIapDefaultTargetId = 0x30;
constexpr quint16 kIapEnterUpgrade = 0x0101;
constexpr quint16 kIapPreUpdate = 0x0102;
constexpr quint16 kIapUpgradeData = 0x0103;
constexpr quint16 kIapWriteParams = 0x0104;
constexpr quint16 kIapReadParams = 0x0105;
constexpr quint16 kIapUpgradeDone = 0x0106;
constexpr int kDefaultUpgradeChunkSize = 1024;

constexpr quint8 kCmdLegacyStop = 0xBF;
constexpr quint8 kCmdLegacySetPara = 0xBD;
constexpr quint8 kCmdLegacyReadPara = 0xBE;
constexpr quint8 kCmdLegacyStep = 0xB2;
constexpr quint8 kCmdLegacySpeed = 0xB3;
constexpr quint8 kCmdLegacyScan = 0xB4;
constexpr quint8 kCmdLegacyFixed = 0xB5;
constexpr quint8 kCmdLegacySystem = 0xB7;
constexpr quint8 kCmdLegacyOmSpeed = 0xB9;
constexpr quint8 kCmdLegacyParaReturn = 0xCC;
constexpr quint8 kCmdLegacyVersionDevice = 0x2B;
constexpr quint8 kCmdLegacyVersionSoftware = 0x2C;
constexpr quint8 kCmdLegacyFixedEam = 0xE5;
constexpr quint8 kCmdLegacySystemEam = 0xE7;

constexpr char kFastMirrorSendCommand[] = "fast_mirror.send.command";
constexpr char kFastMirrorSetIdle[] = "fast_mirror.set.idle";
constexpr char kFastMirrorSetLockZero[] = "fast_mirror.set.lock_zero";
constexpr char kFastMirrorSetPoint[] = "fast_mirror.set.point";

constexpr char kFastMirrorSetDataType[] = "fast_mirror.set.data_type";
constexpr char kFastMirrorLegacyStart[] = "fast_mirror.legacy.start_stream";
constexpr char kFastMirrorLegacyStop[] = "fast_mirror.legacy.stop_stream";
constexpr char kFastMirrorLegacyPacket[] = "fast_mirror.legacy.send_packet";
constexpr char kFastMirrorLegacyFixed[] = "fast_mirror.legacy.fixed";
constexpr char kFastMirrorLegacyStep[] = "fast_mirror.legacy.step";
constexpr char kFastMirrorLegacyStepPid[] = "fast_mirror.legacy.step_pid";
constexpr char kFastMirrorLegacyScan[] = "fast_mirror.legacy.scan";
constexpr char kFastMirrorLegacySpeed[] = "fast_mirror.legacy.speed";
constexpr char kFastMirrorLegacySystem[] = "fast_mirror.legacy.system_test";
constexpr char kFastMirrorLegacyOmSpeed[] = "fast_mirror.legacy.om_speed";
constexpr char kFastMirrorLegacyOmReparation[] = "fast_mirror.legacy.om_reparation";
constexpr char kFastMirrorLegacyReadParam[] = "fast_mirror.legacy.read_param";
constexpr char kFastMirrorLegacyWriteParam[] = "fast_mirror.legacy.write_param";
constexpr char kFastMirrorLegacyWriteA0[] = "fast_mirror.legacy.write_a0";
constexpr char kFastMirrorLegacyWriteA1[] = "fast_mirror.legacy.write_a1";
constexpr char kFastMirrorLegacyWriteA3[] = "fast_mirror.legacy.write_a3";
constexpr char kFastMirrorLegacyWriteA4[] = "fast_mirror.legacy.write_a4";
constexpr char kFastMirrorLegacyWriteA5[] = "fast_mirror.legacy.write_a5";
constexpr char kFastMirrorLegacyWriteA6[] = "fast_mirror.legacy.write_a6";
constexpr char kFastMirrorLegacyWriteA7[] = "fast_mirror.legacy.write_a7";
constexpr char kFastMirrorLegacyWriteA8[] = "fast_mirror.legacy.write_a8";
constexpr char kFastMirrorLegacyWriteA9[] = "fast_mirror.legacy.write_a9";
constexpr char kFastMirrorLegacyWriteAA[] = "fast_mirror.legacy.write_aa";
constexpr char kFastMirrorLegacyWriteAB[] = "fast_mirror.legacy.write_ab";
constexpr char kFastMirrorLegacyWriteAD[] = "fast_mirror.legacy.write_ad";
constexpr char kFastMirrorLegacyWriteB0[] = "fast_mirror.legacy.write_b0";
constexpr char kFastMirrorLegacyWriteB1[] = "fast_mirror.legacy.write_b1";
constexpr char kFastMirrorEnterTestMode[] = "fast_mirror.enter_test_mode";
constexpr char kFastMirrorReadVersionDevice[] = "fast_mirror.read.version_device";
constexpr char kFastMirrorReadVersionSoftware[] = "fast_mirror.read.version_software";
constexpr char kFastMirrorClearSeries[] = "fast_mirror.clear.series";
constexpr char kFastMirrorUpgradeLoadHex[] = "fast_mirror.upgrade.load_hex";
constexpr char kFastMirrorUpgradeEnterMode[] = "fast_mirror.upgrade.enter_mode";
constexpr char kFastMirrorUpgradePreUpdate[] = "fast_mirror.upgrade.pre_update";
constexpr char kFastMirrorUpgradeStart[] = "fast_mirror.upgrade.start";
constexpr char kFastMirrorUpgradeStop[] = "fast_mirror.upgrade.stop";
constexpr char kFastMirrorUpgradeSetInterval[] = "fast_mirror.upgrade.set_interval";
constexpr char kFastMirrorAgingStart[] = "fast_mirror.aging.start";
constexpr char kFastMirrorAgingStop[] = "fast_mirror.aging.stop";
constexpr char kFastMirrorAgingRecordEnable[] = "fast_mirror.aging.record.enable";
constexpr char kFastMirrorAgingRecordDelete[] = "fast_mirror.aging.record.delete";

int indexOf55AAHeader(const QByteArray& buffer) {
    for (int i = 0; i + 1 < buffer.size(); ++i) {
        if (static_cast<quint8>(buffer[i]) == kHeader55AA1
            && static_cast<quint8>(buffer[i + 1]) == kHeader55AA2) {
            return i;
        }
    }
    return -1;
}

QByteArray decodeHexString(const QString& text) {
    QByteArray normalized;
    normalized.reserve(text.size());
    for (const QChar ch : text) {
        if (ch.isSpace() || ch == QChar(',') || ch == QChar(';') || ch == QChar('\t')) {
            continue;
        }
        normalized.push_back(ch.toLatin1());
    }

    if (normalized.isEmpty() || normalized.size() % 2 != 0) {
        return {};
    }

    const QByteArray decoded = QByteArray::fromHex(normalized);
    if (decoded.isEmpty()) {
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

void appendUInt16LE(QByteArray& buffer, quint16 value) {
    buffer.push_back(static_cast<char>(value & 0xFF));
    buffer.push_back(static_cast<char>((value >> 8) & 0xFF));
}

quint16 checksum16(const QByteArray& bytes) {
    quint32 checksum = 0;
    for (const char byte : bytes) {
        checksum += static_cast<quint8>(byte);
    }
    return static_cast<quint16>(checksum & 0xFFFFu);
}

void appendInt16LE(QByteArray& buffer, qint16 value) {
    appendUInt16LE(buffer, static_cast<quint16>(value));
}

void appendInt32LE(QByteArray& buffer, qint32 value) {
    buffer.push_back(static_cast<char>(value & 0xFF));
    buffer.push_back(static_cast<char>((value >> 8) & 0xFF));
    buffer.push_back(static_cast<char>((value >> 16) & 0xFF));
    buffer.push_back(static_cast<char>((value >> 24) & 0xFF));
}

void appendFloatLE(QByteArray& buffer, float value) {
    static_assert(sizeof(float) == 4, "float size must be 4 bytes");
    quint32 raw = 0;
    std::memcpy(&raw, &value, sizeof(float));
    appendInt32LE(buffer, static_cast<qint32>(raw));
}

void appendUInt32LE(QByteArray& buffer, quint32 value) {
    buffer.push_back(static_cast<char>(value & 0xFFu));
    buffer.push_back(static_cast<char>((value >> 8) & 0xFFu));
    buffer.push_back(static_cast<char>((value >> 16) & 0xFFu));
    buffer.push_back(static_cast<char>((value >> 24) & 0xFFu));
}

quint16 calculateCrc16Xmodem(const QByteArray& data) {
    quint16 crc = 0x0000u;
    constexpr quint16 kPolynomial = 0x1021u;
    for (const char rawByte : data) {
        crc ^= static_cast<quint16>(static_cast<quint8>(rawByte) << 8);
        for (int bit = 0; bit < 8; ++bit) {
            if ((crc & 0x8000u) != 0u) {
                crc = static_cast<quint16>((crc << 1) ^ kPolynomial);
            } else {
                crc = static_cast<quint16>(crc << 1);
            }
        }
    }
    return crc;
}

quint32 calculateCrc32Mpeg2(const QByteArray& data) {
    quint32 crc = 0xFFFFFFFFu;
    constexpr quint32 kPolynomial = 0x04C11DB7u;
    for (const char rawByte : data) {
        crc ^= static_cast<quint32>(static_cast<quint8>(rawByte)) << 24;
        for (int bit = 0; bit < 8; ++bit) {
            if ((crc & 0x80000000u) != 0u) {
                crc = (crc << 1) ^ kPolynomial;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

QString normalizeLocalPath(const QString& pathOrUrl) {
    const QString trimmed = pathOrUrl.trimmed();
    const QUrl url(trimmed);
    if (url.isValid() && url.isLocalFile()) {
        return url.toLocalFile();
    }
    return trimmed;
}

QString normalizeHexLine(const QString& line) {
    QString normalized;
    normalized.reserve(line.size());
    for (const QChar ch : line) {
        if (ch.isSpace()) {
            continue;
        }
        normalized.push_back(ch);
    }
    return normalized.toUpper();
}

bool validateUpgradePreparedLine(const QString& line) {
    static const QString kLinePrefix = QStringLiteral("55550302A700000000");
    const QString normalized = normalizeHexLine(line);
    if (!normalized.startsWith(kLinePrefix) || normalized.size() <= kLinePrefix.size()) {
        return false;
    }

    const QByteArray payloadBytes = QByteArray::fromHex(normalized.mid(kLinePrefix.size()).toLatin1());
    if (payloadBytes.isEmpty()) {
        return false;
    }

    quint8 sum = 0;
    for (const char byte : payloadBytes) {
        sum = static_cast<quint8>(sum + static_cast<quint8>(byte));
    }
    return sum == 0u;
}

QByteArray buildUpgradeLineFrame(const QString& line) {
    QByteArray lineBytes = QByteArray::fromHex(normalizeHexLine(line).toLatin1());
    if (lineBytes.isEmpty()) {
        return {};
    }

    if (lineBytes.size() < 172) {
        lineBytes.append(QByteArray(172 - lineBytes.size(), '\0'));
    } else if (lineBytes.size() > 172) {
        lineBytes.resize(172);
    }

    const quint32 crc = calculateCrc32Mpeg2(lineBytes.mid(2));
    QByteArray frame = lineBytes;
    appendUInt32LE(frame, crc);
    if (frame.size() != 176) {
        return {};
    }
    return frame;
}

quint16 readUInt16LE(const QByteArray& data, int offset) {
    if (offset < 0 || offset + 2 > data.size()) {
        return 0;
    }

    const quint8 b0 = static_cast<quint8>(data[offset]);
    const quint8 b1 = static_cast<quint8>(data[offset + 1]);
    return static_cast<quint16>(b0) | (static_cast<quint16>(b1) << 8);
}

quint16 parseUInt16Option(const QVariantMap& args, const QString& key, quint16 fallback) {
    const QVariant value = args.value(key);
    if (!value.isValid()) {
        return fallback;
    }

    bool ok = false;
    const uint number = value.toUInt(&ok);
    if (ok && number <= 0xFFFFu) {
        return static_cast<quint16>(number);
    }

    QString text = value.toString().trimmed();
    if (text.startsWith(QStringLiteral("0x"), Qt::CaseInsensitive)) {
        text = text.mid(2);
        const uint hexNumber = text.toUInt(&ok, 16);
        if (ok && hexNumber <= 0xFFFFu) {
            return static_cast<quint16>(hexNumber);
        }
    }

    return fallback;
}

quint8 parseUInt8Option(const QVariantMap& args, const QString& key, quint8 fallback) {
    return static_cast<quint8>(
        std::min<quint16>(parseUInt16Option(args, key, fallback), static_cast<quint16>(0x00FFu)));
}

QByteArray buildIapFrame(quint8 senderId, quint8 receiverId, quint16 command, const QByteArray& payload) {
    QByteArray frame;
    frame.reserve(8 + payload.size());
    frame.push_back(static_cast<char>(kHeaderIap1));
    frame.push_back(static_cast<char>(kHeaderIap2));
    appendUInt16LE(frame, static_cast<quint16>(1 + 1 + 2 + payload.size()));
    frame.push_back(static_cast<char>(senderId));
    frame.push_back(static_cast<char>(receiverId));
    appendUInt16LE(frame, command);
    frame.append(payload);
    appendUInt16LE(frame, checksum16(frame));
    return frame;
}

bool parseIntelHexPayload(const QString& filePath, QByteArray* out, QString* error) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (error != nullptr) {
            *error = file.errorString();
        }
        return false;
    }

    QByteArray image;
    QTextStream in(&file);
    int lineNumber = 0;
    while (!in.atEnd()) {
        ++lineNumber;
        const QString line = in.readLine().trimmed();
        if (line.isEmpty()) {
            continue;
        }
        if (!line.startsWith(QLatin1Char(':'))) {
            if (error != nullptr) {
                *error = QStringLiteral("HEX line %1 missing ':'").arg(lineNumber);
            }
            return false;
        }

        const QByteArray record = QByteArray::fromHex(line.mid(1).toLatin1());
        if (record.size() < 5) {
            if (error != nullptr) {
                *error = QStringLiteral("HEX line %1 too short").arg(lineNumber);
            }
            return false;
        }

        quint8 sum = 0;
        for (const char byte : record) {
            sum = static_cast<quint8>(sum + static_cast<quint8>(byte));
        }
        if (sum != 0u) {
            if (error != nullptr) {
                *error = QStringLiteral("HEX line %1 checksum mismatch").arg(lineNumber);
            }
            return false;
        }

        const int byteCount = static_cast<quint8>(record.at(0));
        const quint8 recordType = static_cast<quint8>(record.at(3));
        if (record.size() != byteCount + 5) {
            if (error != nullptr) {
                *error = QStringLiteral("HEX line %1 length mismatch").arg(lineNumber);
            }
            return false;
        }

        if (recordType == 0x00) {
            image.append(record.mid(4, byteCount));
        } else if (recordType == 0x01) {
            break;
        }
    }

    if (image.isEmpty()) {
        if (error != nullptr) {
            *error = QStringLiteral("HEX file has no data records");
        }
        return false;
    }

    *out = image;
    return true;
}

QByteArray loadUpgradePayloadFile(const QString& filePath, QString* error) {
    const QString suffix = QFileInfo(filePath).suffix().toLower();
    if (suffix == QStringLiteral("hex")) {
        QByteArray payload;
        if (!parseIntelHexPayload(filePath, &payload, error)) {
            return {};
        }
        return payload;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error != nullptr) {
            *error = file.errorString();
        }
        return {};
    }
    const QByteArray payload = file.readAll();
    if (payload.isEmpty() && error != nullptr) {
        *error = QStringLiteral("upgrade file is empty");
    }
    return payload;
}

qint16 readInt16LE(const QByteArray& data, int offset) {
    return static_cast<qint16>(readUInt16LE(data, offset));
}

quint16 readUInt16BE(const QByteArray& data, int offset) {
    if (offset < 0 || offset + 2 > data.size()) {
        return 0;
    }

    const quint8 b0 = static_cast<quint8>(data[offset]);
    const quint8 b1 = static_cast<quint8>(data[offset + 1]);
    return (static_cast<quint16>(b0) << 8) | static_cast<quint16>(b1);
}

qint32 readInt32LE(const QByteArray& data, int offset) {
    if (offset < 0 || offset + 4 > data.size()) {
        return 0;
    }

    const quint32 raw = static_cast<quint8>(data[offset])
        | (static_cast<quint32>(static_cast<quint8>(data[offset + 1])) << 8)
        | (static_cast<quint32>(static_cast<quint8>(data[offset + 2])) << 16)
        | (static_cast<quint32>(static_cast<quint8>(data[offset + 3])) << 24);
    return static_cast<qint32>(raw);
}

float readFloatLE(const QByteArray& data, int offset) {
    if (offset < 0 || offset + 4 > data.size()) {
        return 0.0f;
    }

    quint32 raw = static_cast<quint8>(data[offset])
        | (static_cast<quint32>(static_cast<quint8>(data[offset + 1])) << 8)
        | (static_cast<quint32>(static_cast<quint8>(data[offset + 2])) << 16)
        | (static_cast<quint32>(static_cast<quint8>(data[offset + 3])) << 24);
    float value = 0.0f;
    std::memcpy(&value, &raw, sizeof(float));
    return value;
}

QString modeText(quint16 mode) {
    switch (mode) {
    case 0x00:
        return QStringLiteral("idle");
    case 0x01:
        return QStringLiteral("lock_zero");
    case 0x02:
        return QStringLiteral("point");
    case 0xB1:
        return QStringLiteral("fault");
    default:
        return QStringLiteral("unknown");
    }
}

quint16 parseModeCode(const QVariant& value, quint16 fallback) {
    if (!value.isValid() || value.isNull()) {
        return fallback;
    }

    bool ok = false;
    const uint number = value.toString().trimmed().toUInt(&ok, 0);
    if (ok && number <= 0xFFFFu) {
        return static_cast<quint16>(number);
    }

    const QString text = value.toString().trimmed().toLower();
    if (text == QStringLiteral("idle") || text == QStringLiteral("空闲")) {
        return 0x00;
    }
    if (text == QStringLiteral("lock_zero") || text == QStringLiteral("lockzero")
        || text == QStringLiteral("锁零")) {
        return 0x01;
    }
    if (text == QStringLiteral("point") || text == QStringLiteral("定点")) {
        return 0x02;
    }
    if (text == QStringLiteral("fault") || text == QStringLiteral("故障")) {
        return 0xB1;
    }

    return fallback;
}

QByteArray buildControlFrame(quint16 mode, float xGuide, float yGuide) {
    QByteArray payload;
    payload.reserve(10);
    appendUInt16LE(payload, mode);
    appendFloatLE(payload, xGuide);
    appendFloatLE(payload, yGuide);

    QByteArray frame;
    frame.reserve(4 + payload.size() + 1);
    frame.push_back(static_cast<char>(kHeader55AA1));
    frame.push_back(static_cast<char>(kHeader55AA2));
    appendUInt16LE(frame, static_cast<quint16>(payload.size()));
    frame.append(payload);
    frame.push_back(static_cast<char>(checksum8(frame)));
    return frame;
}

bool verifyChecksum8Frame(const QByteArray& frame) {
    if (frame.size() < 5) {
        return false;
    }
    return checksum8(frame.left(frame.size() - 1))
        == static_cast<quint8>(frame.at(frame.size() - 1));
}

bool verifyChecksum16CompatFrame(const QByteArray& frame) {
    if (frame.size() < 6) {
        return false;
    }
    const quint8 expected = checksum8(frame.left(frame.size() - 2));
    const quint16 actual = readUInt16LE(frame, frame.size() - 2);
    return expected == static_cast<quint8>(actual & 0x00FF);
}

bool verifyLegacyChecksum(const QByteArray& frame) {
    if (frame.size() < 3) {
        return false;
    }
    const quint8 expected = checksum8(frame.left(frame.size() - 1));
    const quint8 actual = static_cast<quint8>(frame.at(frame.size() - 1));
    return expected == actual;
}

quint16 normalizeFaultBits(quint16 bigEndianValue, quint16 littleEndianValue) {
    if ((bigEndianValue & 0xFF80u) == 0u) {
        return bigEndianValue;
    }
    if ((littleEndianValue & 0xFF80u) == 0u) {
        return littleEndianValue;
    }
    return bigEndianValue;
}

QByteArray buildLegacyKickFrame(quint8 command) {
    QByteArray frame;
    frame.reserve(7);
    frame.push_back(static_cast<char>(kHeaderLegacy));
    frame.push_back(static_cast<char>(command));
    frame.push_back(static_cast<char>(0x02));
    frame.push_back(static_cast<char>(0x00));
    frame.push_back(static_cast<char>(0x00));
    frame.push_back(static_cast<char>(0x00));
    frame.push_back(static_cast<char>(checksum8(frame)));
    return frame;
}

QByteArray buildLegacyPayloadFrame(quint8 command, const QByteArray& payload, quint16 declaredLength) {
    QByteArray frame;
    frame.reserve(4 + payload.size() + 8);
    frame.push_back(static_cast<char>(kHeaderLegacy));
    frame.push_back(static_cast<char>(command));
    appendUInt16LE(frame, declaredLength);
    frame.append(payload);

    const quint8 sum = checksum8(frame);
    frame.push_back('\0');
    frame.push_back('\0');
    frame.push_back('\0');
    frame.push_back('\0');
    frame.push_back(static_cast<char>(sum));
    frame.push_back('\0');
    frame.push_back('\0');
    frame.push_back('\0');
    return frame;
}

void pushSeriesValue(QVariantList& series, double value, int maxSize) {
    series.push_back(value);
    while (series.size() > maxSize) {
        series.pop_front();
    }
}

int toInt(const QVariantMap& args, const QString& key, int fallback) {
    const auto it = args.find(key);
    if (it == args.end() || !it->isValid() || it->isNull()) {
        return fallback;
    }
    bool ok = false;
    const int value = it->toString().trimmed().toInt(&ok, 0);
    return ok ? value : fallback;
}

float toFloat(const QVariantMap& args, const QString& key, float fallback) {
    const auto it = args.find(key);
    if (it == args.end() || !it->isValid() || it->isNull()) {
        return fallback;
    }
    bool ok = false;
    const float value = it->toString().trimmed().toFloat(&ok);
    return ok ? value : fallback;
}

int variantToInt(const QVariant& value, int fallback) {
    if (!value.isValid() || value.isNull()) {
        return fallback;
    }
    bool ok = false;
    const int parsed = value.toString().trimmed().toInt(&ok, 0);
    return ok ? parsed : fallback;
}

float variantToFloat(const QVariant& value, float fallback) {
    if (!value.isValid() || value.isNull()) {
        return fallback;
    }
    bool ok = false;
    const float parsed = value.toString().trimmed().toFloat(&ok);
    return ok ? parsed : fallback;
}

QVariantList listArg(const QVariantMap& args, const QString& key1, const QString& key2 = QString()) {
    QVariant value = args.value(key1);
    if ((!value.isValid() || value.isNull()) && !key2.isEmpty()) {
        value = args.value(key2);
    }
    return value.toList();
}

quint8 parseCommandCode(const QVariant& value, quint8 fallback) {
    if (!value.isValid() || value.isNull()) {
        return fallback;
    }
    bool ok = false;
    const uint parsed = value.toString().trimmed().toUInt(&ok, 0);
    if (ok && parsed <= 0xFFu) {
        return static_cast<quint8>(parsed);
    }
    return fallback;
}

bool isLegacyParamCommand(quint8 command) {
    switch (command) {
    case 0xA0:
    case 0xA1:
    case 0xA2:
    case 0xA3:
    case 0xA4:
    case 0xA5:
    case 0xA6:
    case 0xA7:
    case 0xA8:
    case 0xA9:
    case 0xAA:
    case 0xAB:
    case 0xAD:
    case 0xB0:
    case 0xB1:
        return true;
    default:
        return false;
    }
}

QByteArray buildLegacyParamFrameA0(const QVariantMap& args) {
    QByteArray frame;
    frame.reserve(26);

    appendUInt16LE(frame, 0xA07E);
    appendUInt16LE(frame, 26);

    const QByteArray deviceInfo =
        args.value(QStringLiteral("deviceInfo"), args.value(QStringLiteral("text"))).toString().toUtf8();
    for (int i = 0; i < 20; ++i) {
        frame.push_back(i < deviceInfo.size() ? deviceInfo.at(i) : '\0');
    }

    appendUInt16LE(frame, static_cast<quint16>(toInt(args, QStringLiteral("check"), 0)));
    return frame;
}

QByteArray buildLegacyParamFrameA1(const QVariantMap& args) {
    QByteArray frame;
    frame.reserve(44);

    appendUInt16LE(frame, 0xA17E);
    appendUInt16LE(frame, 44);

    appendInt16LE(frame, static_cast<qint16>(toInt(args, QStringLiteral("adXMin"), -32768)));
    appendInt16LE(frame, static_cast<qint16>(toInt(args, QStringLiteral("adXMax"), 32767)));
    appendInt16LE(frame, static_cast<qint16>(toInt(args, QStringLiteral("adYMin"), -32768)));
    appendInt16LE(frame, static_cast<qint16>(toInt(args, QStringLiteral("adYMax"), 32767)));

    appendInt32LE(frame, toInt(args, QStringLiteral("angleXRange"), 0));
    appendInt32LE(frame, toInt(args, QStringLiteral("angleYRange"), 0));

    appendInt16LE(frame, static_cast<qint16>(toInt(args, QStringLiteral("vinMin"), 0)));
    appendInt16LE(frame, static_cast<qint16>(toInt(args, QStringLiteral("vinMax"), 0)));
    appendInt16LE(frame, static_cast<qint16>(toInt(args, QStringLiteral("daMin"), 0)));
    appendInt16LE(frame, static_cast<qint16>(toInt(args, QStringLiteral("daMax"), 0)));

    appendInt16LE(frame, static_cast<qint16>(toInt(args, QStringLiteral("driveVoltageMin"), 0)));
    appendInt16LE(frame, static_cast<qint16>(toInt(args, QStringLiteral("driveVoltageMax"), 0)));
    appendInt16LE(frame, static_cast<qint16>(toInt(args, QStringLiteral("driveCurrentMin"), 0)));
    appendInt16LE(frame, static_cast<qint16>(toInt(args, QStringLiteral("driveCurrentMax"), 0)));

    appendInt16LE(frame, static_cast<qint16>(toInt(args, QStringLiteral("xDaZeroBias"), 0)));
    appendInt16LE(frame, static_cast<qint16>(toInt(args, QStringLiteral("yDaZeroBias"), 0)));
    appendInt16LE(frame, static_cast<qint16>(toInt(args, QStringLiteral("xZero"), 0)));
    appendInt16LE(frame, static_cast<qint16>(toInt(args, QStringLiteral("yZero"), 0)));

    return frame;
}

QByteArray buildLegacyParamFrameA3(const QVariantMap& args) {
    QByteArray frame;
    frame.reserve(384);

    appendUInt16LE(frame, 0xA37E);
    appendUInt16LE(frame, 384);

    auto appendPidBlock = [&frame](const QVariantList& values) {
        for (int i = 0; i < 12; ++i) {
            float value = 0.0f;
            if (i < values.size()) {
                bool ok = false;
                value = values.at(i).toString().toFloat(&ok);
                if (!ok) {
                    value = 0.0f;
                }
            }
            appendFloatLE(frame, value);
        }
    };

    const QStringList pidKeys = {
        QStringLiteral("pidX1"), QStringLiteral("pidY1"), QStringLiteral("pidX2"), QStringLiteral("pidY2"),
        QStringLiteral("pidX3"), QStringLiteral("pidY3"), QStringLiteral("pidX4"), QStringLiteral("pidY4")};
    for (const QString& key : pidKeys) {
        appendPidBlock(args.value(key).toList());
    }

    while (frame.size() < 384) {
        frame.push_back('\0');
    }

    if (frame.size() > 384) {
        frame.resize(384);
    }
    return frame;
}

QByteArray buildLegacyParamFrameA4(const QVariantMap& args) {
    QByteArray frame;
    frame.reserve(84);

    appendUInt16LE(frame, 0xA47E);
    appendUInt16LE(frame, 84);

    const QVariantList values = listArg(args, QStringLiteral("tempInterp"), QStringLiteral("values"));
    for (int i = 0; i < 40; ++i) {
        const int v = i < values.size() ? variantToInt(values.at(i), 0) : 0;
        appendInt16LE(frame, static_cast<qint16>(v));
    }

    return frame;
}

QByteArray buildLegacyParamFrameA5(const QVariantMap& args) {
    QByteArray frame;
    frame.reserve(254);

    appendUInt16LE(frame, 0xA57E);
    appendUInt16LE(frame, 254);
    appendInt16LE(frame, static_cast<qint16>(toInt(args, QStringLiteral("degree"), 3)));
    appendInt16LE(frame, static_cast<qint16>(toInt(args, QStringLiteral("highTemp"), 0)));
    appendInt16LE(frame, static_cast<qint16>(toInt(args, QStringLiteral("lowTemp"), 0)));
    appendInt16LE(frame, static_cast<qint16>(toInt(args, QStringLiteral("dipTemp"), 0)));

    const QVariantList coeffs = listArg(args, QStringLiteral("coeffs"), QStringLiteral("tempCoeffs"));
    for (int i = 0; i < 60; ++i) {
        const float v = i < coeffs.size() ? variantToFloat(coeffs.at(i), 0.0f) : 0.0f;
        appendFloatLE(frame, v);
    }

    while (frame.size() < 254) {
        frame.push_back('\0');
    }
    if (frame.size() > 254) {
        frame.resize(254);
    }
    return frame;
}

QByteArray buildLegacyParamFrameA6(const QVariantMap& args) {
    QByteArray frame;
    frame.reserve(548);

    appendUInt16LE(frame, 0xA67E);
    appendUInt16LE(frame, 548);

    const QVariantList coeffs = listArg(args, QStringLiteral("coeffs"), QStringLiteral("values"));
    for (int i = 0; i < 136; ++i) {
        const float v = i < coeffs.size() ? variantToFloat(coeffs.at(i), 0.0f) : 0.0f;
        appendFloatLE(frame, v);
    }

    while (frame.size() < 548) {
        frame.push_back('\0');
    }
    if (frame.size() > 548) {
        frame.resize(548);
    }
    return frame;
}

QByteArray buildLegacyParamFrameA7(const QVariantMap& args) {
    QByteArray frame;
    frame.reserve(3872);

    appendUInt16LE(frame, 0xA77E);
    appendUInt16LE(frame, 3872);
    appendInt16LE(frame, static_cast<qint16>(toInt(args, QStringLiteral("xPosAdStart"), 0)));
    appendInt16LE(frame, static_cast<qint16>(toInt(args, QStringLiteral("xPosAdStop"), 0)));
    appendInt16LE(frame, static_cast<qint16>(toInt(args, QStringLiteral("xPosAdStep"), 0)));
    appendInt16LE(frame, static_cast<qint16>(toInt(args, QStringLiteral("xCaliNu"), 0)));
    appendInt16LE(frame, static_cast<qint16>(toInt(args, QStringLiteral("xAngleMin"), 0)));
    appendInt16LE(frame, static_cast<qint16>(toInt(args, QStringLiteral("xAngleMax"), 0)));
    appendInt16LE(frame, static_cast<qint16>(toInt(args, QStringLiteral("yPosAdStart"), 0)));
    appendInt16LE(frame, static_cast<qint16>(toInt(args, QStringLiteral("yPosAdStop"), 0)));
    appendInt16LE(frame, static_cast<qint16>(toInt(args, QStringLiteral("yPosAdStep"), 0)));
    appendInt16LE(frame, static_cast<qint16>(toInt(args, QStringLiteral("yCaliNu"), 0)));
    appendInt16LE(frame, static_cast<qint16>(toInt(args, QStringLiteral("yAngleMin"), 0)));
    appendInt16LE(frame, static_cast<qint16>(toInt(args, QStringLiteral("yAngleMax"), 0)));

    const QVariantList values = listArg(args, QStringLiteral("caliArray"), QStringLiteral("values"));
    for (int i = 0; i < 1922; ++i) {
        const int v = i < values.size() ? variantToInt(values.at(i), 0) : 0;
        appendInt16LE(frame, static_cast<qint16>(v));
    }

    while (frame.size() < 3872) {
        frame.push_back('\0');
    }
    if (frame.size() > 3872) {
        frame.resize(3872);
    }
    return frame;
}

QByteArray buildLegacyParamFrameA8(const QVariantMap& args) {
    QByteArray frame;
    frame.reserve(30);

    appendUInt16LE(frame, 0xA87E);
    appendUInt16LE(frame, 30);
    appendInt16LE(frame, static_cast<qint16>(toInt(args, QStringLiteral("lockType"), 0)));
    appendInt16LE(frame, static_cast<qint16>(toInt(args, QStringLiteral("sensorType"), 0)));
    appendInt16LE(frame, static_cast<qint16>(toInt(args, QStringLiteral("driveType"), 0)));
    appendInt16LE(frame, static_cast<qint16>(toInt(args, QStringLiteral("motorLayoutAngle"), 0)));
    appendInt16LE(frame, static_cast<qint16>(toInt(args, QStringLiteral("caliMethod1"), 0)));
    appendInt16LE(frame, static_cast<qint16>(toInt(args, QStringLiteral("caliMethod2"), 0)));
    appendUInt16LE(frame, static_cast<quint16>(toInt(args, QStringLiteral("daMaxTimes"), 0)));
    appendFloatLE(frame, toFloat(args, QStringLiteral("angleCoeff"), 0.0f));
    appendFloatLE(frame, toFloat(args, QStringLiteral("daCoeff"), 0.0f));
    appendFloatLE(frame, toFloat(args, QStringLiteral("currentCoeff"), 0.0f));
    return frame;
}

QByteArray buildLegacyParamFrameA9(const QVariantMap& args) {
    QByteArray frame;
    frame.reserve(18);

    appendUInt16LE(frame, 0xA97E);
    appendUInt16LE(frame, 18);
    appendInt16LE(frame, static_cast<qint16>(toInt(args, QStringLiteral("errorState"), 0)));
    appendInt16LE(frame, static_cast<qint16>(toInt(args, QStringLiteral("errorLockNu"), 0)));
    appendInt16LE(frame, static_cast<qint16>(toInt(args, QStringLiteral("errorSensorNu"), 0)));
    appendInt16LE(frame, static_cast<qint16>(toInt(args, QStringLiteral("errorDriverNu"), 0)));
    appendInt16LE(frame, static_cast<qint16>(toInt(args, QStringLiteral("errorMaxDaNu"), 0)));
    appendInt16LE(frame, static_cast<qint16>(toInt(args, QStringLiteral("errorParaReadNu"), 0)));
    appendInt16LE(frame, static_cast<qint16>(toInt(args, QStringLiteral("errorTempNu"), 0)));
    return frame;
}

QByteArray buildLegacyParamFrameAA(const QVariantMap& args) {
    QByteArray frame;
    frame.reserve(116);

    appendUInt16LE(frame, 0xAA7E);
    appendUInt16LE(frame, 116);

    auto appendFloatList = [&frame](const QVariantList& values, int count) {
        for (int i = 0; i < count; ++i) {
            const float v = i < values.size() ? variantToFloat(values.at(i), 0.0f) : 0.0f;
            appendFloatLE(frame, v);
        }
    };

    appendFloatList(listArg(args, QStringLiteral("guideX")), 5);
    appendFloatList(listArg(args, QStringLiteral("guideY")), 5);
    appendFloatList(listArg(args, QStringLiteral("daX")), 3);
    appendFloatList(listArg(args, QStringLiteral("daY")), 3);
    appendFloatList(listArg(args, QStringLiteral("adX")), 5);
    appendFloatList(listArg(args, QStringLiteral("adY")), 5);
    appendInt16LE(frame, static_cast<qint16>(toInt(args, QStringLiteral("guideCompOn"), 0)));
    appendInt16LE(frame, static_cast<qint16>(toInt(args, QStringLiteral("daCompOn"), 0)));
    appendInt16LE(frame, static_cast<qint16>(toInt(args, QStringLiteral("adCompOn"), 0)));
    appendInt16LE(frame, static_cast<qint16>(toInt(args, QStringLiteral("speedCompOn"), 0)));

    while (frame.size() < 116) {
        frame.push_back('\0');
    }
    if (frame.size() > 116) {
        frame.resize(116);
    }
    return frame;
}

QByteArray buildLegacyParamFrameAB(const QVariantMap& args) {
    QByteArray frame;
    frame.reserve(8);

    appendUInt16LE(frame, 0xAB7E);
    appendUInt16LE(frame, 8);
    appendInt16LE(frame, static_cast<qint16>(toInt(args, QStringLiteral("xZero"), 0)));
    appendInt16LE(frame, static_cast<qint16>(toInt(args, QStringLiteral("yZero"), 0)));
    return frame;
}

QByteArray buildLegacyParamFrameAD(const QVariantMap& args) {
    QByteArray frame;
    frame.reserve(110);

    appendUInt16LE(frame, 0xAD7E);
    appendUInt16LE(frame, 110);

    const QVariantList values = listArg(args, QStringLiteral("values"), QStringLiteral("coeffs"));
    for (int i = 0; i < 20; ++i) {
        const float v = i < values.size() ? variantToFloat(values.at(i), 0.0f) : 0.0f;
        appendFloatLE(frame, v);
    }

    while (frame.size() < 110) {
        frame.push_back('\0');
    }
    if (frame.size() > 110) {
        frame.resize(110);
    }
    return frame;
}

QByteArray buildLegacyParamFrameB0(const QVariantMap& args) {
    QByteArray frame;
    frame.reserve(6004);

    appendUInt16LE(frame, 0xB07E);
    appendUInt16LE(frame, 6004);

    const QVariantList values = listArg(args, QStringLiteral("values"), QStringLiteral("reparation"));
    for (int i = 0; i < 1500; ++i) {
        const float v = i < values.size() ? variantToFloat(values.at(i), 0.0f) : 0.0f;
        appendFloatLE(frame, v);
    }

    while (frame.size() < 6004) {
        frame.push_back('\0');
    }
    if (frame.size() > 6004) {
        frame.resize(6004);
    }
    return frame;
}

QByteArray buildLegacyParamFrameB1(const QVariantMap& args) {
    QByteArray frame;
    frame.reserve(58);

    appendUInt16LE(frame, 0xB17E);
    appendUInt16LE(frame, 58);
    appendFloatLE(frame, toFloat(args, QStringLiteral("scanSpeed"), 0.0f));
    appendFloatLE(frame, toFloat(args, QStringLiteral("threshold"), 0.0f));
    appendFloatLE(frame, toFloat(args, QStringLiteral("uniformStepP"), 0.0f));
    appendFloatLE(frame, toFloat(args, QStringLiteral("maxAngle"), 0.0f));
    appendFloatLE(frame, toFloat(args, QStringLiteral("minAngle"), 0.0f));
    appendFloatLE(frame, toFloat(args, QStringLiteral("tempLimitH"), 0.0f));
    appendFloatLE(frame, toFloat(args, QStringLiteral("tempLimitL"), 0.0f));
    appendFloatLE(frame, toFloat(args, QStringLiteral("speedLimit"), 0.0f));
    appendUInt16LE(frame, static_cast<quint16>(toInt(args, QStringLiteral("scanFre"), 0)));
    appendUInt16LE(frame, static_cast<quint16>(toInt(args, QStringLiteral("upTime"), 0)));
    appendUInt16LE(frame, static_cast<quint16>(toInt(args, QStringLiteral("lineTime"), 0)));
    appendUInt16LE(frame, static_cast<quint16>(toInt(args, QStringLiteral("totalTime"), 0)));
    appendUInt16LE(frame, static_cast<quint16>(toInt(args, QStringLiteral("scanStartP"), 0)));
    appendUInt16LE(frame, static_cast<quint16>(toInt(args, QStringLiteral("adjustFlag"), 0)));
    appendInt16LE(frame, static_cast<qint16>(toInt(args, QStringLiteral("dir"), 0)));
    appendUInt16LE(frame, static_cast<quint16>(toInt(args, QStringLiteral("zeroTick"), 0)));
    appendInt16LE(frame, static_cast<qint16>(toInt(args, QStringLiteral("angleZero"), 0)));
    appendInt16LE(frame, static_cast<qint16>(toInt(args, QStringLiteral("caliMethod"), 0)));
    appendInt16LE(frame, static_cast<qint16>(toInt(args, QStringLiteral("pidType"), 0)));
    return frame;
}

QByteArray buildLegacyOmReparationFrame(const QVariantMap& args) {
    QByteArray frame;
    frame.reserve(2004);

    appendUInt16LE(frame, 0xBA7E);
    appendUInt16LE(frame, 2004);

    const QVariantList values = listArg(args, QStringLiteral("values"), QStringLiteral("reparation"));
    for (int i = 0; i < 500; ++i) {
        const float value = i < values.size() ? variantToFloat(values.at(i), 1.0f) : 1.0f;
        appendFloatLE(frame, value);
    }

    while (frame.size() < 2004) {
        frame.push_back('\0');
    }
    if (frame.size() > 2004) {
        frame.resize(2004);
    }
    return frame;
}

int legacyFrameLength(quint8 command, int dataTypeBits) {
    const bool bits32 = dataTypeBits == 32;
    switch (command) {
    case kCmdLegacyStep:
    case kCmdLegacyScan:
    case kCmdLegacyFixed:
    case kCmdLegacySystem:
    case kCmdLegacyFixedEam:
    case kCmdLegacySystemEam:
        return bits32 ? 20 : 16;
    case kCmdLegacySpeed:
        return bits32 ? 20 : 16;
    case kCmdLegacyParaReturn:
        return 7;
    case kCmdLegacyVersionDevice:
    case kCmdLegacyVersionSoftware:
        return 23;
    case kCmdLegacyOmSpeed:
        return 2038;
    default:
        return 7;
    }
}

}  // namespace

class FastMirrorProtocolBridge::Impl {
public:
    Impl()
        : serialPort_(std::make_unique<QSerialPort>())
        , upgradeSendTimer_(std::make_unique<QTimer>())
        , agingSendTimer_(std::make_unique<QTimer>())
        , agingNoDriveTimer_(std::make_unique<QTimer>()) {
        upgradeSendTimer_->setSingleShot(false);
        QObject::connect(upgradeSendTimer_.get(), &QTimer::timeout, serialPort_.get(), [this]() {
            sendNextUpgradeFrame();
        });

        agingSendTimer_->setSingleShot(false);
        QObject::connect(agingSendTimer_.get(), &QTimer::timeout, serialPort_.get(), [this]() {
            sendAgingFrame();
        });

        agingNoDriveTimer_->setSingleShot(true);
        QObject::connect(agingNoDriveTimer_.get(), &QTimer::timeout, serialPort_.get(), [this]() {
            onAgingNoDriveTimeout();
        });

        QObject::connect(serialPort_.get(), &QSerialPort::readyRead, serialPort_.get(), [this]() {
            buffer_.append(serialPort_->readAll());
            consumeBuffer();
        });
        QObject::connect(serialPort_.get(),
                         &QSerialPort::errorOccurred,
                         serialPort_.get(),
                         [this](QSerialPort::SerialPortError error) {
                             if (error == QSerialPort::NoError) {
                                 return;
                             }

                             lastError_ = serialPort_->errorString();
                             if (error == QSerialPort::ResourceError) {
                                 serialPort_->close();
                                 connected_ = false;
                                 linkType_ = QStringLiteral("none");
                             }
                         });
    }

    bool writeBytes(const QByteArray& bytes) {
        if (bytes.isEmpty()) {
            lastError_ = QStringLiteral("invalid command payload");
            return false;
        }

        lastTxFrameHex_ = toHexString(bytes);

        if (!connected_ || !serialPort_->isOpen()) {
            lastError_ = QStringLiteral("device is not connected");
            return false;
        }

        const qint64 written = serialPort_->write(bytes);
        const bool ok = written == bytes.size() && serialPort_->waitForBytesWritten(500);
        if (ok) {
            lastError_.clear();
        } else {
            lastError_ = serialPort_->errorString();
        }
        return ok;
    }

    bool sendControl(quint16 mode, float xGuide, float yGuide) {
        const QByteArray frame = buildControlFrame(mode, xGuide, yGuide);
        const bool ok = writeBytes(frame);
        if (ok) {
            lastCommandModeCode_ = mode;
            lastCommandModeText_ = modeText(mode);
            lastGuideX_ = xGuide;
            lastGuideY_ = yGuide;
        }
        return ok;
    }

    bool sendLegacyKick(quint8 command, int repeatTimes, bool updateStreamState = true) {
        const QByteArray frame = buildLegacyKickFrame(command);
        const int repeats = std::max(1, repeatTimes);
        for (int i = 0; i < repeats; ++i) {
            if (!writeBytes(frame)) {
                return false;
            }
        }
        if (updateStreamState) {
            legacyStreamCommand_ = command;
            legacyStreaming_ = command != kCmdLegacyStop;
        }
        legacyLastCommand_ = command;
        return true;
    }

    bool sendLegacyRawWithTail(const QByteArray& frameWithoutTail) {
        if (frameWithoutTail.size() < 4 || static_cast<quint8>(frameWithoutTail.at(0)) != kHeaderLegacy) {
            lastError_ = QStringLiteral("legacy raw frame invalid");
            return false;
        }

        QByteArray frame = frameWithoutTail;
        const quint8 sum = checksum8(frame);
        frame.push_back('\0');
        frame.push_back('\0');
        frame.push_back('\0');
        frame.push_back('\0');
        frame.push_back(static_cast<char>(sum));
        frame.push_back('\0');
        frame.push_back('\0');
        frame.push_back('\0');
        return writeBytes(frame);
    }

    bool sendLegacyRawWithTailSplit(const QByteArray& frameWithoutTail, int firstChunkSize) {
        if (frameWithoutTail.size() < 4 || static_cast<quint8>(frameWithoutTail.at(0)) != kHeaderLegacy) {
            lastError_ = QStringLiteral("legacy raw frame invalid");
            return false;
        }

        QByteArray frame = frameWithoutTail;
        const quint8 sum = checksum8(frame);
        frame.push_back('\0');
        frame.push_back('\0');
        frame.push_back('\0');
        frame.push_back('\0');
        frame.push_back(static_cast<char>(sum));
        frame.push_back('\0');
        frame.push_back('\0');
        frame.push_back('\0');

        if (firstChunkSize <= 0 || firstChunkSize >= frame.size()) {
            return writeBytes(frame);
        }

        if (!writeBytes(frame.left(firstChunkSize))) {
            return false;
        }
        return writeBytes(frame.mid(firstChunkSize));
    }

    bool sendLegacyPayload(quint8 command, const QByteArray& payload, quint16 declaredLength) {
        const QByteArray frame = buildLegacyPayloadFrame(command, payload, declaredLength);
        const bool ok = writeBytes(frame);
        if (ok) {
            legacyLastCommand_ = command;
            legacyLastCommandTime_ = QDateTime::currentDateTime().toString(Qt::ISODate);
        }
        return ok;
    }

    bool runLegacyParamRead(quint8 command) {
        if (!sendLegacyKick(kCmdLegacyStop, 2, false)) {
            return false;
        }
        if (!sendLegacyKick(kCmdLegacyReadPara, 2, false)) {
            return false;
        }
        return sendLegacyKick(command, 1, false);
    }

    bool sendLegacyParamFrame(const QByteArray& rawFrame, int splitFirstChunkSize = 0) {
        if (rawFrame.size() < 4 || static_cast<quint8>(rawFrame.at(0)) != kHeaderLegacy) {
            lastError_ = QStringLiteral("legacy param frame invalid");
            return false;
        }

        if (!sendLegacyKick(kCmdLegacyStop, 2, false)) {
            return false;
        }
        if (!sendLegacyKick(kCmdLegacySetPara, 2, false)) {
            return false;
        }
        if (splitFirstChunkSize > 0) {
            return sendLegacyRawWithTailSplit(rawFrame, splitFirstChunkSize);
        }
        return sendLegacyRawWithTail(rawFrame);
    }

    bool runLegacyParamWrite(const QVariantMap& args) {
        const QByteArray fullHex =
            decodeHexString(args.value(QStringLiteral("fullHex"), args.value(QStringLiteral("full"))).toString());
        if (!fullHex.isEmpty()) {
            return sendLegacyParamFrame(fullHex);
        }

        const quint8 command = parseCommandCode(args.value(QStringLiteral("command")), 0xA0);
        const QByteArray payload =
            decodeHexString(args.value(QStringLiteral("payloadHex"), args.value(QStringLiteral("payload"))).toString());
        if (payload.isEmpty()) {
            lastError_ = QStringLiteral("legacy write_param payload empty");
            return false;
        }
        quint16 declaredLength = static_cast<quint16>(toInt(args, QStringLiteral("declaredLength"), 0));
        if (declaredLength == 0) {
            declaredLength = static_cast<quint16>(payload.size() + 4);
        }

        QByteArray rawFrame;
        rawFrame.reserve(4 + payload.size());
        rawFrame.push_back(static_cast<char>(kHeaderLegacy));
        rawFrame.push_back(static_cast<char>(command));
        appendUInt16LE(rawFrame, declaredLength);
        rawFrame.append(payload);
        return sendLegacyParamFrame(rawFrame);
    }

    bool loadUpgradeHexFile(const QString& pathOrUrl) {
        const QString filePath = normalizeLocalPath(pathOrUrl);
        if (filePath.isEmpty()) {
            lastError_ = QStringLiteral("upgrade hex path is empty");
            return false;
        }

        QString loadError;
        const QByteArray payload = loadUpgradePayloadFile(filePath, &loadError);
        if (payload.isEmpty()) {
            lastError_ = loadError.isEmpty() ? QStringLiteral("upgrade file is empty") : loadError;
            return false;
        }

        QList<QByteArray> frameList;
        const int chunkSize = std::max(1, upgradeChunkSize_);
        for (int offset = 0; offset < payload.size(); offset += chunkSize) {
            frameList.push_back(payload.mid(offset, std::min(chunkSize, payload.size() - offset)));
        }

        const int previewBytes = std::min(payload.size(), 2048);
        QString previewText = toHexString(payload.left(previewBytes));
        if (payload.size() > previewBytes) {
            previewText += QStringLiteral("\n... 共 %1 字节").arg(payload.size());
        }

        stopUpgradeDownload(false);
        upgradeFilePath_ = QFileInfo(filePath).absoluteFilePath();
        upgradePreparedText_ = previewText.trimmed();
        upgradeTotalLines_ = frameList.size();
        upgradeValidLines_ = frameList.size();
        upgradeInvalidLines_ = 0;
        upgradeFrames_ = frameList;
        upgradePayloadBytes_ = payload.size();
        upgradeCrc16_ = checksum16(payload);
        upgradeCurrentLine_ = 0;
        upgradeProgress_ = 0;
        upgradeLastStatusCode_ = 0;
        upgradeLastStatus_ = QStringLiteral("升级文件加载完成");
        upgradeLastTxDataHex_.clear();
        return !frameList.isEmpty();
    }

    bool setUpgradeInterval(int intervalMs) {
        upgradeIntervalMs_ = std::max(5, intervalMs);
        if (upgradeSendTimer_->isActive()) {
            upgradeSendTimer_->start(upgradeIntervalMs_);
        }
        return true;
    }

    void applyUpgradeOptions(const QVariantMap& args) {
        upgradeHostId_ = parseUInt8Option(args, QStringLiteral("hostId"), upgradeHostId_);
        upgradeTargetId_ = parseUInt8Option(
            args,
            QStringLiteral("targetId"),
            parseUInt8Option(args, QStringLiteral("receiverId"), upgradeTargetId_));
        upgradeChunkSize_ = std::max(
            1,
            std::min<int>(
                kMaxIapPayloadLength - 4,
                toInt(args, QStringLiteral("chunkSize"), upgradeChunkSize_)));
    }

    bool sendUpgradeEnterMode(const QVariantMap& args = {}) {
        applyUpgradeOptions(args);
        const QByteArray payload = QByteArray::fromHex("BDF8C8EBD4DACFDFC9FDBCB6");
        const QByteArray frame =
            buildIapFrame(upgradeHostId_, upgradeTargetId_, kIapEnterUpgrade, payload);
        const bool ok = writeBytes(frame);
        if (ok) {
            upgradeLastStatus_ = QStringLiteral("已发送在线升级模式指令");
            upgradeLastTxDataHex_ = toHexString(frame);
        }
        return ok;
    }

    bool sendUpgradePreUpdate(const QVariantMap& args = {}) {
        applyUpgradeOptions(args);
        const QByteArray payload = QByteArray::fromHex("D4A4B8FCD0C2C6F4B6AF0101");
        const QByteArray frame = buildIapFrame(upgradeHostId_, upgradeTargetId_, kIapPreUpdate, payload);
        const bool ok = writeBytes(frame);
        if (ok) {
            upgradeLastStatus_ = QStringLiteral("已发送预更新指令");
            upgradeLastTxDataHex_ = toHexString(frame);
        }
        return ok;
    }

    bool startUpgradeDownload(const QVariantMap& args = {}) {
        applyUpgradeOptions(args);
        if (upgradeFrames_.isEmpty()) {
            lastError_ = QStringLiteral("please load valid hex file before upgrade");
            return false;
        }
        if (!connected_ || !serialPort_->isOpen()) {
            lastError_ = QStringLiteral("device is not connected");
            return false;
        }

        stopUpgradeDownload(false);
        upgradeRunning_ = true;
        upgradeCurrentLine_ = 0;
        upgradeProgress_ = 0;
        upgradeLastStatus_ = QStringLiteral("开始下载程序");
        upgradeSendTimer_->start(std::max(5, upgradeIntervalMs_));
        sendNextUpgradeFrame();
        return true;
    }

    bool stopUpgradeDownload(bool manualStop) {
        if (upgradeSendTimer_->isActive()) {
            upgradeSendTimer_->stop();
        }
        const bool wasRunning = upgradeRunning_;
        upgradeRunning_ = false;
        if (manualStop) {
            upgradeLastStatus_ = QStringLiteral("下载已停止");
        } else if (wasRunning && upgradeCurrentLine_ >= upgradeFrames_.size()) {
            upgradeLastStatus_ = QStringLiteral("程序发送完成，等待设备应答");
        }
        return true;
    }

    void sendNextUpgradeFrame() {
        if (!upgradeRunning_) {
            return;
        }
        if (upgradeCurrentLine_ >= upgradeFrames_.size()) {
            stopUpgradeDownload(false);
            upgradeProgress_ = 100;
            return;
        }

        const QByteArray frame =
            buildIapFrame(upgradeHostId_, upgradeTargetId_, kIapUpgradeData, upgradeFrames_.at(upgradeCurrentLine_));
        if (!writeBytes(frame)) {
            stopUpgradeDownload(false);
            upgradeLastStatus_ = QStringLiteral("下载中断: %1").arg(lastError_);
            return;
        }

        ++upgradeCurrentLine_;
        upgradeProgress_ =
            static_cast<int>((static_cast<double>(upgradeCurrentLine_) * 100.0) / std::max(1, upgradeFrames_.size()));
        upgradeLastStatus_ = QStringLiteral("下载中 %1/%2").arg(upgradeCurrentLine_).arg(upgradeFrames_.size());
        upgradeLastTxDataHex_ = toHexString(frame);
    }

    bool hasUpgradeModeAckPrefix() const {
        static const QByteArray kAckFrame = QByteArray::fromHex("7EAA555044415441204F4B00A1");
        if (buffer_.isEmpty() || static_cast<quint8>(buffer_.at(0)) != kHeaderLegacy) {
            return false;
        }
        const int compareLength = std::min(buffer_.size(), kAckFrame.size());
        for (int i = 0; i < compareLength; ++i) {
            if (buffer_.at(i) != kAckFrame.at(i)) {
                return false;
            }
        }
        return true;
    }

    bool tryConsumeUpgradeModeAck() {
        static const QByteArray kAckFrame = QByteArray::fromHex("7EAA555044415441204F4B00A1");
        if (!hasUpgradeModeAckPrefix() || buffer_.size() < kAckFrame.size()) {
            return false;
        }
        const QByteArray frame = buffer_.left(kAckFrame.size());
        buffer_.remove(0, kAckFrame.size());
        lastChecksumOk_ = true;
        lastError_.clear();
        lastRxFrameHex_ = toHexString(frame);
        lastRxPayloadHex_ = toHexString(frame.mid(1));
        upgradeLastRxHex_ = lastRxFrameHex_;
        upgradeLastStatus_ = QStringLiteral("设备已进入在线升级模式");
        return true;
    }

    QString upgradeStatusText(quint8 statusCode) const {
        switch (statusCode) {
        case 0x22:
            return QStringLiteral("设备进入预更新成功");
        case 0x02:
            return QStringLiteral("设备擦除FLASH完成");
        case 0xFF:
            return QStringLiteral("设备预更新失败");
        case 0x11:
            return QStringLiteral("设备数据接收完成，正在解析");
        case 0xAA:
            return QStringLiteral("设备在线升级成功");
        case 0x00:
            return QStringLiteral("设备在线升级失败");
        default:
            return QStringLiteral("在线升级状态: 0x%1")
                .arg(statusCode, 2, 16, QChar('0'))
                .toUpper();
        }
    }

    bool tryConsumeUpgradeStatusFrame() {
        if (buffer_.isEmpty() || static_cast<quint8>(buffer_.at(0)) != kHeader55AA1) {
            return false;
        }
        if (buffer_.size() >= 2 && static_cast<quint8>(buffer_.at(1)) == kHeader55AA2) {
            return false;
        }
        if (buffer_.size() < 19) {
            return false;
        }

        const QByteArray frame = buffer_.left(19);
        buffer_.remove(0, 19);
        lastChecksumOk_ = true;
        lastError_.clear();
        lastRxFrameHex_ = toHexString(frame);
        lastRxPayloadHex_ = toHexString(frame.mid(1));
        upgradeLastRxHex_ = lastRxFrameHex_;

        if (frame.size() > 5) {
            upgradeLastStatusCode_ = static_cast<quint8>(frame.at(5));
            upgradeLastStatus_ = upgradeStatusText(upgradeLastStatusCode_);
            if (upgradeLastStatusCode_ == 0xAA || upgradeLastStatusCode_ == 0x00
                || upgradeLastStatusCode_ == 0xFF) {
                stopUpgradeDownload(false);
            }
        }
        return true;
    }

    QString iapStatusText(quint16 command, quint8 status) const {
        const bool ok = status == 0x00;
        switch (command) {
        case kIapEnterUpgrade:
            return ok ? QStringLiteral("设备已进入在线升级模式")
                      : QStringLiteral("设备进入在线升级模式失败: 0x%1")
                            .arg(status, 2, 16, QChar('0')).toUpper();
        case kIapPreUpdate:
            return ok ? QStringLiteral("设备已跳转到BOOT程序")
                      : QStringLiteral("预更新启动失败: 0x%1")
                            .arg(status, 2, 16, QChar('0')).toUpper();
        case kIapUpgradeData:
            return ok ? QStringLiteral("升级数据接收成功")
                      : QStringLiteral("升级数据接收失败: 0x%1")
                            .arg(status, 2, 16, QChar('0')).toUpper();
        case kIapWriteParams:
            return ok ? QStringLiteral("升级数据拷贝成功")
                      : QStringLiteral("升级数据拷贝失败: 0x%1")
                            .arg(status, 2, 16, QChar('0')).toUpper();
        case kIapUpgradeDone:
            return ok ? QStringLiteral("设备在线升级成功")
                      : QStringLiteral("设备在线升级失败: 0x%1")
                            .arg(status, 2, 16, QChar('0')).toUpper();
        default:
            return QStringLiteral("在线升级应答 0x%1: 0x%2")
                .arg(command, 4, 16, QChar('0'))
                .arg(status, 2, 16, QChar('0'))
                .toUpper();
        }
    }

    bool tryConsumeIapFrame() {
        if (buffer_.size() < 8 || static_cast<quint8>(buffer_.at(0)) != kHeaderIap1
            || static_cast<quint8>(buffer_.at(1)) != kHeaderIap2) {
            return false;
        }

        const quint16 dataLength = readUInt16LE(buffer_, 2);
        if (dataLength < 4 || dataLength > kMaxIapPayloadLength) {
            lastChecksumOk_ = false;
            lastError_ = QStringLiteral("IAP payload length invalid");
            buffer_.remove(0, 1);
            return true;
        }

        const int frameSize = 2 + 2 + dataLength + 2;
        if (buffer_.size() < frameSize) {
            return false;
        }

        const QByteArray frame = buffer_.left(frameSize);
        const quint16 expected = checksum16(frame.left(frameSize - 2));
        const quint16 actual = readUInt16LE(frame, frameSize - 2);
        buffer_.remove(0, frameSize);
        if (expected != actual) {
            lastChecksumOk_ = false;
            lastError_ = QStringLiteral("IAP checksum mismatch");
            return true;
        }

        const quint8 sender = static_cast<quint8>(frame.at(4));
        const quint8 receiver = static_cast<quint8>(frame.at(5));
        const quint16 command = readUInt16LE(frame, 6);
        const QByteArray payload = frame.mid(8, dataLength - 4);
        lastChecksumOk_ = true;
        lastError_.clear();
        lastRxFrameHex_ = toHexString(frame);
        lastRxPayloadHex_ = toHexString(payload);
        upgradeLastRxHex_ = lastRxFrameHex_;
        upgradeLastStatusCommand_ = command;
        upgradeLastSenderId_ = sender;
        upgradeLastReceiverId_ = receiver;
        if (!payload.isEmpty()) {
            upgradeLastStatusCode_ = static_cast<quint8>(payload.at(0));
            upgradeLastStatus_ = iapStatusText(command, upgradeLastStatusCode_);
            if ((command == kIapUpgradeDone || upgradeLastStatusCode_ != 0x00) && upgradeRunning_) {
                stopUpgradeDownload(false);
            }
        } else {
            upgradeLastStatusCode_ = 0;
            upgradeLastStatus_ = QStringLiteral("收到在线升级应答 0x%1")
                .arg(command, 4, 16, QChar('0')).toUpper();
        }
        return true;
    }

    bool startAging(const QVariantMap& args) {
        if (!connected_ || !serialPort_->isOpen()) {
            lastError_ = QStringLiteral("device is not connected");
            return false;
        }

        agingAmplitude_ = std::max(0.0, args.value(QStringLiteral("amplitude"), 100).toDouble());
        agingFrequency_ = std::max(0.001, args.value(QStringLiteral("frequency"), 50).toDouble());
        agingDirection_ = std::max(0, std::min(1, toInt(args, QStringLiteral("direction"), 0)));
        agingWaveType_ = std::max(0, std::min(1, toInt(args, QStringLiteral("waveType"), 0)));
        agingPidType_ = std::max(0, std::min(2, toInt(args, QStringLiteral("pidType"), 0)));
        agingAverageNu_ = toInt(args, QStringLiteral("averageNu"), 100);
        agingSendIntervalMs_ = std::max(2, toInt(args, QStringLiteral("intervalMs"), 2));
        agingNoDriveHours_ = std::max(0, toInt(args, QStringLiteral("noDriveHours"), 1));

        stopAging(false);
        clearSeries();
        agingRunning_ = true;
        agingCurrentStep_ = 0.0;
        agingSentFrames_ = 0;
        agingNoDriveTriggered_ = false;
        agingStartedAt_ = QDateTime::currentDateTime();
        agingStopAt_ = agingNoDriveHours_ > 0
            ? agingStartedAt_.addSecs(static_cast<qint64>(agingNoDriveHours_) * 60LL * 60LL)
            : QDateTime();
        agingLastStatus_ = QStringLiteral("老化参数发送中");

        sendAgingFrame();
        if (!agingRunning_) {
            return false;
        }
        agingSendTimer_->start(agingSendIntervalMs_);
        if (agingNoDriveHours_ > 0) {
            const qint64 timeoutMs = static_cast<qint64>(agingNoDriveHours_) * 60LL * 60LL * 1000LL;
            agingNoDriveTimer_->start(static_cast<int>(std::min<qint64>(timeoutMs, INT_MAX)));
        }
        return true;
    }

    bool stopAging(bool manualStop) {
        if (agingSendTimer_->isActive()) {
            agingSendTimer_->stop();
        }
        if (agingNoDriveTimer_->isActive()) {
            agingNoDriveTimer_->stop();
        }
        const bool wasRunning = agingRunning_;
        agingRunning_ = false;
        if (manualStop) {
            agingLastStatus_ = QStringLiteral("老化发送已停止");
        } else if (wasRunning && agingNoDriveTriggered_) {
            agingLastStatus_ = QStringLiteral("老化测试已自动停止驱动");
        }
        return true;
    }

    void sendAgingFrame() {
        if (!agingRunning_) {
            return;
        }
        const double sinCount = std::max(1.0, 500.0 / agingFrequency_);
        constexpr double kPi = 3.14159265358979323846;
        const double theta = 2.0 * kPi * agingCurrentStep_ / sinCount;
        const int sinValue = static_cast<int>(std::lround(agingAmplitude_ * std::sin(theta)));
        const int cosValue = static_cast<int>(std::lround(agingAmplitude_ * std::cos(theta)));

        int guideX = 0;
        int guideY = 0;
        if (agingWaveType_ == 0) {
            if (agingDirection_ == 0) {
                guideX = sinValue;
            } else {
                guideY = sinValue;
            }
        } else {
            guideX = sinValue;
            guideY = cosValue;
        }

        const QVariantMap fixedArgs{
            {QStringLiteral("command"), static_cast<int>(kCmdLegacyFixed)},
            {QStringLiteral("guideX"), guideX},
            {QStringLiteral("guideY"), guideY},
            {QStringLiteral("pidType"), agingPidType_},
            {QStringLiteral("resOn"), 0},
            {QStringLiteral("averageNu"), agingAverageNu_},
        };
        if (!runLegacyFixed(fixedArgs)) {
            stopAging(false);
            agingLastStatus_ = QStringLiteral("老化发送失败: %1").arg(lastError_);
            return;
        }

        agingLastGuideX_ = guideX;
        agingLastGuideY_ = guideY;
        agingCurrentStep_ += 1.0;
        if (agingCurrentStep_ >= sinCount) {
            agingCurrentStep_ = 0.0;
        }
        ++agingSentFrames_;
    }

    void onAgingNoDriveTimeout() {
        if (!agingRunning_) {
            return;
        }

        QVariantMap args;
        args.insert(QStringLiteral("command"), static_cast<int>(kCmdLegacySystem));
        args.insert(QStringLiteral("da1"), 0);
        args.insert(QStringLiteral("da2"), 0);
        args.insert(QStringLiteral("ledTime"), 100);

        const bool ok = runLegacySystem(args);
        agingNoDriveTriggered_ = true;
        stopAging(false);
        agingLastStatus_ =
            ok ? QStringLiteral("到时已发送关闭驱动指令") : QStringLiteral("关闭驱动失败: %1").arg(lastError_);
    }

    bool ensureAgingRecordColumn(QSqlDatabase& db,
                                 QSet<QString>& existingColumns,
                                 const QString& columnName,
                                 const QString& columnType) {
        if (existingColumns.contains(columnName.toLower())) {
            return true;
        }

        QSqlQuery alterQuery(db);
        const QString sql = QStringLiteral("ALTER TABLE Aging_AllData ADD COLUMN %1 %2")
                                .arg(columnName, columnType);
        if (!alterQuery.exec(sql)) {
            lastError_ = alterQuery.lastError().text();
            agingRecordStatus_ = QStringLiteral("更新表结构失败: %1").arg(lastError_);
            return false;
        }

        existingColumns.insert(columnName.toLower());
        return true;
    }

    bool ensureAgingDatabase() {
        if (agingDbPath_.isEmpty()) {
            const QString baseDir =
                QDir(QCoreApplication::applicationDirPath())
                    .filePath(QStringLiteral("record/org_business_fast_mirror_device_adapter"));
            QDir dir;
            if (!dir.mkpath(baseDir)) {
                lastError_ = QStringLiteral("failed to create aging record dir: %1").arg(baseDir);
                agingRecordStatus_ = QStringLiteral("数据库初始化失败");
                return false;
            }
            agingDbPath_ = QDir::cleanPath(QDir(baseDir).filePath(QStringLiteral("fast_mirror_aging.db")));
        }

        if (agingDbConnectionName_.isEmpty()) {
            agingDbConnectionName_ = QStringLiteral("fast_mirror_aging_record_connection");
        }

        QSqlDatabase db;
        if (QSqlDatabase::contains(agingDbConnectionName_)) {
            db = QSqlDatabase::database(agingDbConnectionName_);
        } else {
            db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), agingDbConnectionName_);
        }

        if (db.databaseName() != agingDbPath_) {
            if (db.isOpen()) {
                db.close();
            }
            db.setDatabaseName(agingDbPath_);
        }
        if (!db.isOpen() && !db.open()) {
            lastError_ = db.lastError().text();
            agingRecordStatus_ = QStringLiteral("数据库打开失败: %1").arg(lastError_);
            return false;
        }

        QSqlQuery createQuery(db);
        if (!createQuery.exec(QStringLiteral(
                "CREATE TABLE IF NOT EXISTS Aging_AllData ("
                "Angle_X REAL,"
                "Angle_Y REAL,"
                "AD_X REAL,"
                "AD_Y REAL,"
                "Current REAL,"
                "Temperature REAL,"
                "Time TEXT,"
                "xSpeed REAL,"
                "ySpeed REAL)"))) {
            lastError_ = createQuery.lastError().text();
            agingRecordStatus_ = QStringLiteral("建表失败: %1").arg(lastError_);
            return false;
        }

        QSet<QString> columns;
        QSqlQuery columnsQuery(db);
        if (!columnsQuery.exec(QStringLiteral("PRAGMA table_info(Aging_AllData)"))) {
            lastError_ = columnsQuery.lastError().text();
            agingRecordStatus_ = QStringLiteral("读取表结构失败: %1").arg(lastError_);
            return false;
        }
        while (columnsQuery.next()) {
            columns.insert(columnsQuery.value(1).toString().toLower());
        }

        if (!ensureAgingRecordColumn(db, columns, QStringLiteral("Angle_X"), QStringLiteral("REAL"))
            || !ensureAgingRecordColumn(db, columns, QStringLiteral("Angle_Y"), QStringLiteral("REAL"))
            || !ensureAgingRecordColumn(db, columns, QStringLiteral("AD_X"), QStringLiteral("REAL"))
            || !ensureAgingRecordColumn(db, columns, QStringLiteral("AD_Y"), QStringLiteral("REAL"))
            || !ensureAgingRecordColumn(db, columns, QStringLiteral("Current"), QStringLiteral("REAL"))
            || !ensureAgingRecordColumn(db, columns, QStringLiteral("Temperature"), QStringLiteral("REAL"))
            || !ensureAgingRecordColumn(db, columns, QStringLiteral("Time"), QStringLiteral("TEXT"))
            || !ensureAgingRecordColumn(db, columns, QStringLiteral("xSpeed"), QStringLiteral("REAL"))
            || !ensureAgingRecordColumn(db, columns, QStringLiteral("ySpeed"), QStringLiteral("REAL"))) {
            return false;
        }

        if (agingRecordedRows_ == 0) {
            QSqlQuery countQuery(db);
            if (countQuery.exec(QStringLiteral("SELECT COUNT(1) FROM Aging_AllData")) && countQuery.next()) {
                agingRecordedRows_ = countQuery.value(0).toLongLong();
            }
        }
        return true;
    }

    bool setAgingRecordEnabled(bool enabled) {
        if (enabled) {
            if (!ensureAgingDatabase()) {
                agingRecordEnabled_ = false;
                return false;
            }
            agingRecordEnabled_ = true;
            agingRecordStatus_ = QStringLiteral("数据库记录已开启");
            return true;
        }

        agingRecordEnabled_ = false;
        agingRecordStatus_ = QStringLiteral("数据库记录已关闭");
        return true;
    }

    bool deleteAgingRecordData() {
        if (!ensureAgingDatabase()) {
            return false;
        }

        QSqlDatabase db = QSqlDatabase::database(agingDbConnectionName_);
        QSqlQuery query(db);
        if (!query.exec(QStringLiteral("DELETE FROM Aging_AllData"))) {
            lastError_ = query.lastError().text();
            agingRecordStatus_ = QStringLiteral("删除失败: %1").arg(lastError_);
            return false;
        }

        agingRecordedRows_ = 0;
        agingRecordStatus_ = QStringLiteral("数据库数据已清空");
        return true;
    }

    void appendAgingRecord(double xAngle,
                           double yAngle,
                           double xAd,
                           double yAd,
                           double temperature,
                           double currentValue,
                           double xSpeed,
                           double ySpeed) {
        if (!agingRecordEnabled_) {
            return;
        }
        if (!ensureAgingDatabase()) {
            return;
        }

        QSqlDatabase db = QSqlDatabase::database(agingDbConnectionName_);
        QSqlQuery query(db);
        query.prepare(QStringLiteral(
            "INSERT INTO Aging_AllData "
            "(Angle_X, Angle_Y, AD_X, AD_Y, Current, Temperature, Time, xSpeed, ySpeed) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)"));
        query.addBindValue(xAngle);
        query.addBindValue(yAngle);
        query.addBindValue(xAd);
        query.addBindValue(yAd);
        query.addBindValue(currentValue);
        query.addBindValue(temperature);
        query.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODateWithMs));
        query.addBindValue(xSpeed);
        query.addBindValue(ySpeed);
        if (!query.exec()) {
            lastError_ = query.lastError().text();
            agingRecordStatus_ = QStringLiteral("写入失败: %1").arg(lastError_);
            return;
        }

        ++agingRecordedRows_;
        agingRecordStatus_ = QStringLiteral("数据库记录中");
    }

    void clearSeries() {
        sampleIndex_ = 0;
        upSendNuSeries_.clear();
        xAngleSeries_.clear();
        yAngleSeries_.clear();
        xAdSeries_.clear();
        yAdSeries_.clear();
        xSpeedSeries_.clear();
        ySpeedSeries_.clear();
        currentSeries_.clear();
        temperatureSeries_.clear();
        legacyOmScanPositions_.clear();
        legacyOmValidPoints_ = 0.0f;
        legacyOmUniformVsum_ = 0.0f;
        legacyOmUniformPsum_ = 0.0f;
        legacyOmUniformAveV_ = 0.0f;
        legacyOmUniformDiffAmpV_ = 0.0f;
        legacyOmUniformDiffAveV_ = 0.0f;
        legacyOmUniformDiffAmpP_ = 0.0f;
        legacyOmUniformDiffAveP_ = 0.0f;
        legacyOmTemperature_ = 0.0f;
    }

    void appendClosedSample(double upSendNu, double xAngle, double yAngle, double xAd, double yAd, double current, double temperature) {
        legacyUpSendNu_ = upSendNu;
        currentXAngle_ = xAngle;
        currentYAngle_ = yAngle;
        currentXAd_ = xAd;
        currentYAd_ = yAd;
        currentCurrent_ = current;
        currentTemperature_ = temperature;
        currentXSpeed_ = 0.0;
        currentYSpeed_ = 0.0;

        pushSeriesValue(upSendNuSeries_, upSendNu, maxSeriesSize_);
        pushSeriesValue(xAngleSeries_, xAngle, maxSeriesSize_);
        pushSeriesValue(yAngleSeries_, yAngle, maxSeriesSize_);
        pushSeriesValue(xAdSeries_, xAd, maxSeriesSize_);
        pushSeriesValue(yAdSeries_, yAd, maxSeriesSize_);
        pushSeriesValue(currentSeries_, current, maxSeriesSize_);
        pushSeriesValue(temperatureSeries_, temperature, maxSeriesSize_);
        sampleIndex_++;
        appendAgingRecord(xAngle, yAngle, xAd, yAd, temperature, current, 0.0, 0.0);
    }

    void appendSpeedSample(double upSendNu, double xAngle, double yAngle, double xSpeed, double ySpeed, double current, double temperature) {
        legacyUpSendNu_ = upSendNu;
        currentXAngle_ = xAngle;
        currentYAngle_ = yAngle;
        currentXSpeed_ = xSpeed;
        currentYSpeed_ = ySpeed;
        currentCurrent_ = current;
        currentTemperature_ = temperature;

        pushSeriesValue(upSendNuSeries_, upSendNu, maxSeriesSize_);
        pushSeriesValue(xAngleSeries_, xAngle, maxSeriesSize_);
        pushSeriesValue(yAngleSeries_, yAngle, maxSeriesSize_);
        pushSeriesValue(xSpeedSeries_, xSpeed, maxSeriesSize_);
        pushSeriesValue(ySpeedSeries_, ySpeed, maxSeriesSize_);
        pushSeriesValue(currentSeries_, current, maxSeriesSize_);
        pushSeriesValue(temperatureSeries_, temperature, maxSeriesSize_);
        sampleIndex_++;
        appendAgingRecord(
            xAngle, yAngle, currentXAd_, currentYAd_, temperature, current, xSpeed, ySpeed);
    }

    void parseLegacyFrame(const QByteArray& frame, quint8 command) {
        lastChecksumOk_ = true;
        lastRxFrameHex_ = toHexString(frame);
        lastRxPayloadHex_ = frame.size() > 2 ? toHexString(frame.mid(2)) : QString();
        legacyLastRxCommand_ = command;

        if (command == kCmdLegacyStep || command == kCmdLegacyScan || command == kCmdLegacyFixed
            || command == kCmdLegacySystem || command == kCmdLegacyFixedEam || command == kCmdLegacySystemEam) {
            if (dataTypeBits_ == 32 && frame.size() >= 20) {
                appendClosedSample(readInt16LE(frame, 2),
                                   readInt32LE(frame, 4),
                                   readInt32LE(frame, 8),
                                   readInt16LE(frame, 12),
                                   readInt16LE(frame, 14),
                                   readInt16LE(frame, 16),
                                   static_cast<qint8>(frame[18]));
            } else if (frame.size() >= 16) {
                appendClosedSample(readInt16LE(frame, 2),
                                   readInt16LE(frame, 4),
                                   readInt16LE(frame, 6),
                                   readInt16LE(frame, 8),
                                   readInt16LE(frame, 10),
                                   readInt16LE(frame, 12),
                                   static_cast<qint8>(frame[14]));
            }
            return;
        }

        if (command == kCmdLegacySpeed) {
            if (dataTypeBits_ == 32 && frame.size() >= 20) {
                appendSpeedSample(readInt16LE(frame, 2),
                                  readInt32LE(frame, 4),
                                  readInt32LE(frame, 8),
                                  readInt16LE(frame, 12),
                                  readInt16LE(frame, 14),
                                  readInt16LE(frame, 16),
                                  static_cast<qint8>(frame[18]));
            } else if (frame.size() >= 16) {
                appendSpeedSample(readInt16LE(frame, 2),
                                  readInt16LE(frame, 4),
                                  readInt16LE(frame, 6),
                                  readInt16LE(frame, 8),
                                  readInt16LE(frame, 10),
                                  readInt16LE(frame, 12),
                                  static_cast<qint8>(frame[14]));
            }
            return;
        }

        if (isLegacyParamCommand(command) && frame.size() >= 8) {
            const int declaredLength = static_cast<int>(readUInt16LE(frame, 2));
            const int payloadLength = std::max(0, std::min(declaredLength - 4, frame.size() - 5));
            const QByteArray payload = frame.mid(4, payloadLength);
            legacyLastParamCommand_ = command;
            legacyLastParamLength_ = payloadLength;
            legacyLastParamPayloadHex_ = toHexString(payload);
            if (command == 0xA0) {
                legacyDeviceInfoText_ = QString::fromLatin1(payload.left(20)).trimmed();
            }
            return;
        }

        if (command == kCmdLegacyParaReturn && frame.size() >= 7) {
            legacyParaSetFlag_ = readInt16LE(frame, 4);
            return;
        }

        if ((command == kCmdLegacyVersionDevice || command == kCmdLegacyVersionSoftware) && frame.size() >= 23) {
            QByteArray textBytes = frame.mid(2, 20);
            const QString versionText = QString::fromLatin1(textBytes).trimmed();
            if (command == kCmdLegacyVersionDevice) {
                legacyDeviceVersion_ = versionText;
            } else {
                legacySoftwareVersion_ = versionText;
            }
            return;
        }

        if (command == kCmdLegacyOmSpeed && frame.size() >= 2038) {
            legacyOmValidPoints_ = readFloatLE(frame, 2);
            legacyOmScanPositions_.clear();
            const int pointCount =
                std::max(0, std::min(500, static_cast<int>(std::round(static_cast<double>(legacyOmValidPoints_)))));
            for (int i = 0; i < pointCount; ++i) {
                legacyOmScanPositions_.push_back(readFloatLE(frame, 6 + i * 4));
            }
            legacyOmUniformVsum_ = readFloatLE(frame, 2006);
            legacyOmUniformPsum_ = readFloatLE(frame, 2010);
            legacyOmUniformAveV_ = readFloatLE(frame, 2014);
            legacyOmUniformDiffAmpV_ = readFloatLE(frame, 2018);
            legacyOmUniformDiffAveV_ = readFloatLE(frame, 2022);
            legacyOmUniformDiffAmpP_ = readFloatLE(frame, 2026);
            legacyOmUniformDiffAveP_ = readFloatLE(frame, 2030);
            legacyOmTemperature_ = readFloatLE(frame, 2034);
            return;
        }
    }

    void consumeLegacy() {
        while (true) {
            if (buffer_.isEmpty()) {
                return;
            }
            if (static_cast<quint8>(buffer_.at(0)) != kHeaderLegacy) {
                return;
            }
            if (buffer_.size() < 2) {
                return;
            }

            const quint8 command = static_cast<quint8>(buffer_.at(1));
            int frameLen = 0;
            if (isLegacyParamCommand(command)) {
                if (buffer_.size() < 4) {
                    return;
                }
                const int declaredLength = static_cast<int>(readUInt16LE(buffer_, 2));
                frameLen = declaredLength + 5;
                if (declaredLength < 4 || frameLen > 8192) {
                    lastChecksumOk_ = false;
                    lastError_ = QStringLiteral("legacy param frame length invalid");
                    buffer_.remove(0, 1);
                    continue;
                }
            } else {
                frameLen = legacyFrameLength(command, dataTypeBits_);
            }
            if (frameLen <= 0) {
                frameLen = 7;
            }
            if (buffer_.size() < frameLen) {
                return;
            }

            const QByteArray frame = buffer_.left(frameLen);
            if (!verifyLegacyChecksum(frame)) {
                lastChecksumOk_ = false;
                lastError_ = QStringLiteral("legacy frame checksum invalid");
                buffer_.remove(0, 1);
                continue;
            }

            buffer_.remove(0, frameLen);
            parseLegacyFrame(frame, command);
        }
    }

    void consume55AA() {
        while (true) {
            const int start = indexOf55AAHeader(buffer_);
            if (start < 0) {
                return;
            }

            if (start > 0) {
                buffer_.remove(0, start);
            }

            if (buffer_.size() < 4) {
                return;
            }

            const quint16 payloadLength = readUInt16LE(buffer_, 2);
            if (payloadLength == 0 || payloadLength > kMaxPayloadLength) {
                lastChecksumOk_ = false;
                lastError_ = QStringLiteral("fast mirror payload length invalid");
                buffer_.remove(0, 1);
                continue;
            }

            const int totalWith8 = 4 + payloadLength + 1;
            const int totalWith16 = 4 + payloadLength + 2;
            if (buffer_.size() < totalWith8) {
                return;
            }

            if (verifyChecksum8Frame(buffer_.left(totalWith8))) {
                const QByteArray frame = buffer_.left(totalWith8);
                const QByteArray payload = frame.mid(4, payloadLength);
                buffer_.remove(0, totalWith8);
                parse55AAPayload(frame, payload);
                continue;
            }

            if (buffer_.size() < totalWith16) {
                return;
            }
            if (verifyChecksum16CompatFrame(buffer_.left(totalWith16))) {
                const QByteArray frame = buffer_.left(totalWith16);
                const QByteArray payload = frame.mid(4, payloadLength);
                buffer_.remove(0, totalWith16);
                parse55AAPayload(frame, payload);
                continue;
            }

            lastChecksumOk_ = false;
            lastError_ = QStringLiteral("fast mirror frame checksum invalid");
            buffer_.remove(0, 1);
        }
    }

    void parse55AAPayload(const QByteArray& frame, const QByteArray& payload) {
        lastChecksumOk_ = true;
        lastError_.clear();
        lastRxFrameHex_ = toHexString(frame);
        lastRxPayloadHex_ = toHexString(payload);

        if (payload.size() >= 10) {
            statusModeCode_ = readUInt16LE(payload, 0);
            statusModeText_ = modeText(statusModeCode_);
            currentXAngle55AA_ = readFloatLE(payload, 2);
            currentYAngle55AA_ = readFloatLE(payload, 6);
        }

        if (payload.size() >= 12) {
            const quint16 faultBitsBe = readUInt16BE(payload, 10);
            const quint16 faultBitsLe = readUInt16LE(payload, 10);
            faultBits_ = normalizeFaultBits(faultBitsBe, faultBitsLe);
        }
    }

    void consumeBuffer() {
        while (!buffer_.isEmpty()) {
            const quint8 lead = static_cast<quint8>(buffer_.at(0));

            if (lead == kHeaderIap1 && buffer_.size() >= 2
                && static_cast<quint8>(buffer_.at(1)) == kHeaderIap2) {
                if (!tryConsumeIapFrame()) {
                    return;
                }
                continue;
            }

            if (lead == kHeaderLegacy && hasUpgradeModeAckPrefix()) {
                if (buffer_.size() < 13) {
                    return;
                }
                if (tryConsumeUpgradeModeAck()) {
                    continue;
                }
            }

            if (lead == kHeader55AA1 && buffer_.size() >= 2
                && static_cast<quint8>(buffer_.at(1)) != kHeader55AA2) {
                if (buffer_.size() < 19) {
                    return;
                }
                if (tryConsumeUpgradeStatusFrame()) {
                    continue;
                }
            }

            if (lead == kHeaderLegacy) {
                consumeLegacy();
                if (!buffer_.isEmpty() && static_cast<quint8>(buffer_.at(0)) != kHeaderLegacy
                    && !(buffer_.size() >= 2
                         && static_cast<quint8>(buffer_.at(0)) == kHeader55AA1
                         && static_cast<quint8>(buffer_.at(1)) == kHeader55AA2)) {
                    buffer_.remove(0, 1);
                }
                continue;
            }

            if (lead == kHeader55AA1 && buffer_.size() < 2) {
                return;
            }

            if (buffer_.size() >= 2 && lead == kHeader55AA1
                && static_cast<quint8>(buffer_.at(1)) == kHeader55AA2) {
                consume55AA();
                if (!buffer_.isEmpty() && static_cast<quint8>(buffer_.at(0)) != kHeaderLegacy
                    && !(buffer_.size() >= 2
                         && static_cast<quint8>(buffer_.at(0)) == kHeader55AA1
                         && static_cast<quint8>(buffer_.at(1)) == kHeader55AA2)) {
                    buffer_.remove(0, 1);
                }
                continue;
            }

            buffer_.remove(0, 1);
        }
    }

    bool runLegacyFixed(const QVariantMap& args) {
        const quint8 command = parseCommandCode(args.value(QStringLiteral("command")), kCmdLegacyFixed);
        const int averageNu = toInt(args, QStringLiteral("averageNu"), 100);
        const int pidType = toInt(args, QStringLiteral("pidType"), 0);
        const int resOn = toInt(args, QStringLiteral("resOn"), 0);

        if (!sendLegacyKick(command, 2)) {
            return false;
        }

        QByteArray payload;
        quint16 declaredLength = 14;
        if (dataTypeBits_ == 32 && command == kCmdLegacyFixed) {
            appendInt32LE(payload, toInt(args, QStringLiteral("guideX"), 0));
            appendInt32LE(payload, toInt(args, QStringLiteral("guideY"), 0));
            appendInt16LE(payload, static_cast<qint16>(averageNu));
            appendInt16LE(payload, static_cast<qint16>(pidType));
            appendInt16LE(payload, static_cast<qint16>(resOn));
            declaredLength = 18;
        } else {
            appendInt16LE(payload, static_cast<qint16>(toInt(args, QStringLiteral("guideX"), 0)));
            appendInt16LE(payload, static_cast<qint16>(toInt(args, QStringLiteral("guideY"), 0)));
            appendInt16LE(payload, static_cast<qint16>(averageNu));
            appendInt16LE(payload, static_cast<qint16>(pidType));
            appendInt16LE(payload, static_cast<qint16>(resOn));
            declaredLength = 14;
        }

        return sendLegacyPayload(command, payload, declaredLength);
    }

    bool runLegacySystem(const QVariantMap& args) {
        const quint8 command = parseCommandCode(args.value(QStringLiteral("command")), kCmdLegacySystem);
        if (!sendLegacyKick(command, 2)) {
            return false;
        }

        QByteArray payload;
        appendInt16LE(payload, static_cast<qint16>(toInt(args, QStringLiteral("da1"), 0)));
        appendInt16LE(payload, static_cast<qint16>(toInt(args, QStringLiteral("da2"), 0)));
        appendInt16LE(payload, static_cast<qint16>(toInt(args, QStringLiteral("ledTime"), 100)));
        return sendLegacyPayload(command, payload, 10);
    }

    bool runLegacyStep(const QVariantMap& args) {
        if (!sendLegacyKick(kCmdLegacyStep, 2)) {
            return false;
        }

        QByteArray payload;
        quint16 declaredLength = 24;
        if (dataTypeBits_ == 32) {
            appendInt32LE(payload, toInt(args, QStringLiteral("xStartAngle"), 100));
            appendInt32LE(payload, toInt(args, QStringLiteral("xStopAngle"), -100));
            appendInt32LE(payload, toInt(args, QStringLiteral("yStartAngle"), 0));
            appendInt32LE(payload, toInt(args, QStringLiteral("yStopAngle"), 0));
            appendInt16LE(payload, static_cast<qint16>(toInt(args, QStringLiteral("xSpeed"), 0)));
            appendInt16LE(payload, static_cast<qint16>(toInt(args, QStringLiteral("ySpeed"), 0)));
            appendInt16LE(payload, static_cast<qint16>(toInt(args, QStringLiteral("startTicks"), 10000)));
            appendInt16LE(payload, static_cast<qint16>(toInt(args, QStringLiteral("upDataNu"), 500) + 1));
            appendInt16LE(payload, static_cast<qint16>(toInt(args, QStringLiteral("caliMethod"), 0)));
            appendInt16LE(payload, static_cast<qint16>(toInt(args, QStringLiteral("pidType"), 0)));
            declaredLength = 32;
        } else {
            appendInt16LE(payload, static_cast<qint16>(toInt(args, QStringLiteral("xStartAngle"), 100)));
            appendInt16LE(payload, static_cast<qint16>(toInt(args, QStringLiteral("xStopAngle"), -100)));
            appendInt16LE(payload, static_cast<qint16>(toInt(args, QStringLiteral("yStartAngle"), 0)));
            appendInt16LE(payload, static_cast<qint16>(toInt(args, QStringLiteral("yStopAngle"), 0)));
            appendInt16LE(payload, static_cast<qint16>(toInt(args, QStringLiteral("xSpeed"), 0)));
            appendInt16LE(payload, static_cast<qint16>(toInt(args, QStringLiteral("ySpeed"), 0)));
            appendInt16LE(payload, static_cast<qint16>(toInt(args, QStringLiteral("startTicks"), 10000)));
            appendInt16LE(payload, static_cast<qint16>(toInt(args, QStringLiteral("upDataNu"), 500) + 1));
            appendInt16LE(payload, static_cast<qint16>(toInt(args, QStringLiteral("caliMethod"), 0)));
            appendInt16LE(payload, static_cast<qint16>(toInt(args, QStringLiteral("pidType"), 0)));
            declaredLength = 24;
        }
        return sendLegacyPayload(kCmdLegacyStep, payload, declaredLength);
    }

    bool runLegacyStepPid(const QVariantMap& args) {
        QByteArray frame;
        frame.reserve(384);

        appendUInt16LE(frame, 0xB17E);
        appendUInt16LE(frame, 384);

        auto appendPidBlock = [&frame](const QVariantList& values) {
            for (int i = 0; i < 12; ++i) {
                float value = 0.0f;
                if (i < values.size()) {
                    bool ok = false;
                    value = values.at(i).toString().toFloat(&ok);
                    if (!ok) {
                        value = 0.0f;
                    }
                }
                appendFloatLE(frame, value);
            }
        };

        const QStringList pidKeys = {
            QStringLiteral("pidX1"),
            QStringLiteral("pidY1"),
            QStringLiteral("pidX2"),
            QStringLiteral("pidY2"),
            QStringLiteral("pidX3"),
            QStringLiteral("pidY3"),
            QStringLiteral("pidX4"),
            QStringLiteral("pidY4")
        };

        for (const QString& key : pidKeys) {
            appendPidBlock(args.value(key).toList());
        }

        while (frame.size() < 384) {
            frame.push_back('\0');
        }

        if (frame.size() > 384) {
            frame.resize(384);
        }

        return sendLegacyRawWithTail(frame);
    }

    bool runLegacyScan(const QVariantMap& args) {
        if (!sendLegacyKick(kCmdLegacyScan, 2)) {
            return false;
        }

        QByteArray payload;
        quint16 declaredLength = 24;
        appendInt16LE(payload, static_cast<qint16>(toInt(args, QStringLiteral("direction"), 0)));
        appendInt16LE(payload, static_cast<qint16>(toInt(args, QStringLiteral("guideNu"), 500)));
        appendInt16LE(payload, static_cast<qint16>(toInt(args, QStringLiteral("zeroUp"), 0)));
        appendInt16LE(payload, static_cast<qint16>(toInt(args, QStringLiteral("waveType"), 1)));
        if (dataTypeBits_ == 32) {
            appendInt32LE(payload, toInt(args, QStringLiteral("xAmp"), 100));
            appendInt32LE(payload, toInt(args, QStringLiteral("yAmp"), 0));
            declaredLength = 28;
        } else {
            appendInt16LE(payload, static_cast<qint16>(toInt(args, QStringLiteral("xAmp"), 100)));
            appendInt16LE(payload, static_cast<qint16>(toInt(args, QStringLiteral("yAmp"), 0)));
            declaredLength = 24;
        }
        appendInt16LE(payload, static_cast<qint16>(toInt(args, QStringLiteral("freq"), 50)));
        appendInt16LE(payload, static_cast<qint16>(toInt(args, QStringLiteral("caliMethod"), 0)));
        appendInt16LE(payload, static_cast<qint16>(toInt(args, QStringLiteral("pidType"), 0)));
        appendInt16LE(payload, static_cast<qint16>(toInt(args, QStringLiteral("upDataNu"), 500)));
        return sendLegacyPayload(kCmdLegacyScan, payload, declaredLength);
    }

    bool runLegacySpeed(const QVariantMap& args) {
        if (!sendLegacyKick(kCmdLegacySpeed, 2)) {
            return false;
        }

        QByteArray payload;
        appendInt16LE(payload, static_cast<qint16>(toInt(args, QStringLiteral("direction"), 0)));
        appendInt16LE(payload, static_cast<qint16>(toInt(args, QStringLiteral("xTargetSpeed"), 100)));
        appendInt16LE(payload, static_cast<qint16>(toInt(args, QStringLiteral("yTargetSpeed"), 0)));
        appendInt16LE(payload, static_cast<qint16>(toInt(args, QStringLiteral("xPos"), 100)));
        appendInt16LE(payload, static_cast<qint16>(toInt(args, QStringLiteral("yPos"), 0)));
        appendInt16LE(payload, static_cast<qint16>(toInt(args, QStringLiteral("upDataNu"), 500) + 1));
        appendInt16LE(payload, static_cast<qint16>(toInt(args, QStringLiteral("caliMethod"), 0)));
        return sendLegacyPayload(kCmdLegacySpeed, payload, 18);
    }

    bool runLegacyOmSpeed(const QVariantMap& args) {
        if (!sendLegacyKick(kCmdLegacyOmSpeed, 2)) {
            return false;
        }

        QByteArray payload;
        appendFloatLE(payload, toFloat(args, QStringLiteral("scanSpeed"), 100.0f));
        appendFloatLE(payload, toFloat(args, QStringLiteral("threshold"), 0.3f));
        appendFloatLE(payload, toFloat(args, QStringLiteral("uniformStepP"), 0.1f));
        appendFloatLE(payload, toFloat(args, QStringLiteral("maxAngle"), 6000.0f));
        appendFloatLE(payload, toFloat(args, QStringLiteral("minAngle"), -6000.0f));
        appendFloatLE(payload, toFloat(args, QStringLiteral("tempHigh"), 70.0f));
        appendFloatLE(payload, toFloat(args, QStringLiteral("tempLow"), -40.0f));
        appendFloatLE(payload, toFloat(args, QStringLiteral("speedLimit"), 1000.0f));
        appendUInt16LE(payload, static_cast<quint16>(toInt(args, QStringLiteral("scanFreq"), 5)));
        appendUInt16LE(payload, static_cast<quint16>(toInt(args, QStringLiteral("upTime"), 20)));
        appendUInt16LE(payload, static_cast<quint16>(toInt(args, QStringLiteral("lineTime"), 20)));
        appendUInt16LE(payload, static_cast<quint16>(toInt(args, QStringLiteral("totalTime"), 60)));
        appendUInt16LE(payload, static_cast<quint16>(toInt(args, QStringLiteral("scanStartP"), 0)));
        appendUInt16LE(payload, static_cast<quint16>(toInt(args, QStringLiteral("adjustFlag"), 0)));
        appendInt16LE(payload, static_cast<qint16>(toInt(args, QStringLiteral("direction"), 0)));
        appendUInt16LE(payload, static_cast<quint16>(toInt(args, QStringLiteral("zeroTick"), 0)));
        appendUInt16LE(payload, static_cast<quint16>(toInt(args, QStringLiteral("angleZero"), 0)));
        appendUInt16LE(payload, static_cast<quint16>(toInt(args, QStringLiteral("arrayMode"), 0)));
        appendInt16LE(payload, static_cast<qint16>(toInt(args, QStringLiteral("caliMethod"), 0)));
        appendInt16LE(payload, static_cast<qint16>(toInt(args, QStringLiteral("pidType"), 0)));
        appendInt32LE(payload, 0);
        return sendLegacyPayload(kCmdLegacyOmSpeed, payload, 60);
    }

    bool runLegacyOmReparation(const QVariantMap& args) {
        const QByteArray frame = buildLegacyOmReparationFrame(args);
        const bool ok = sendLegacyRawWithTail(frame);
        if (ok) {
            legacyLastCommand_ = 0xBA;
            legacyLastCommandTime_ = QDateTime::currentDateTime().toString(Qt::ISODate);
        }
        return ok;
    }

    std::unique_ptr<QSerialPort> serialPort_;
    std::unique_ptr<QTimer> upgradeSendTimer_;
    std::unique_ptr<QTimer> agingSendTimer_;
    std::unique_ptr<QTimer> agingNoDriveTimer_;
    QByteArray buffer_;

    QString linkType_ = QStringLiteral("none");
    QString lastError_;
    QString lastRxFrameHex_;
    QString lastRxPayloadHex_;
    QString lastTxFrameHex_;
    bool connected_ = false;
    bool lastChecksumOk_ = false;

    quint16 statusModeCode_ = 0x00;
    QString statusModeText_ = QStringLiteral("idle");
    quint16 faultBits_ = 0;
    float currentXAngle55AA_ = 0.0f;
    float currentYAngle55AA_ = 0.0f;
    quint16 lastCommandModeCode_ = 0x00;
    QString lastCommandModeText_ = QStringLiteral("idle");
    float lastGuideX_ = 0.0f;
    float lastGuideY_ = 0.0f;

    int dataTypeBits_ = 16;
    int maxSeriesSize_ = kDefaultSeriesSize;
    bool legacyStreaming_ = false;
    quint8 legacyStreamCommand_ = kCmdLegacyFixed;
    quint8 legacyLastCommand_ = 0;
    quint8 legacyLastRxCommand_ = 0;
    int legacyParaSetFlag_ = 0;
    quint8 legacyLastParamCommand_ = 0;
    int legacyLastParamLength_ = 0;
    QString legacyLastParamPayloadHex_;
    QString legacyDeviceInfoText_;
    QString legacyLastCommandTime_;
    QString legacyDeviceVersion_;
    QString legacySoftwareVersion_;

    double legacyUpSendNu_ = 0.0;
    double currentXAngle_ = 0.0;
    double currentYAngle_ = 0.0;
    double currentXAd_ = 0.0;
    double currentYAd_ = 0.0;
    double currentXSpeed_ = 0.0;
    double currentYSpeed_ = 0.0;
    double currentCurrent_ = 0.0;
    double currentTemperature_ = 0.0;
    int sampleIndex_ = 0;

    float legacyOmValidPoints_ = 0.0f;
    float legacyOmUniformVsum_ = 0.0f;
    float legacyOmUniformPsum_ = 0.0f;
    float legacyOmUniformAveV_ = 0.0f;
    float legacyOmUniformDiffAmpV_ = 0.0f;
    float legacyOmUniformDiffAveV_ = 0.0f;
    float legacyOmUniformDiffAmpP_ = 0.0f;
    float legacyOmUniformDiffAveP_ = 0.0f;
    float legacyOmTemperature_ = 0.0f;
    QVariantList legacyOmScanPositions_;

    QString upgradeFilePath_;
    QString upgradePreparedText_;
    QList<QByteArray> upgradeFrames_;
    int upgradeIntervalMs_ = 50;
    int upgradeChunkSize_ = kDefaultUpgradeChunkSize;
    int upgradePayloadBytes_ = 0;
    quint8 upgradeHostId_ = kIapHostId;
    quint8 upgradeTargetId_ = kIapDefaultTargetId;
    bool upgradeRunning_ = false;
    int upgradeTotalLines_ = 0;
    int upgradeValidLines_ = 0;
    int upgradeInvalidLines_ = 0;
    int upgradeCurrentLine_ = 0;
    int upgradeProgress_ = 0;
    quint16 upgradeCrc16_ = 0;
    quint16 upgradeLastStatusCommand_ = 0;
    quint8 upgradeLastSenderId_ = 0;
    quint8 upgradeLastReceiverId_ = 0;
    quint8 upgradeLastStatusCode_ = 0;
    QString upgradeLastStatus_ = QStringLiteral("未开始");
    QString upgradeLastRxHex_;
    QString upgradeLastTxDataHex_;

    bool agingRunning_ = false;
    double agingAmplitude_ = 100.0;
    double agingFrequency_ = 50.0;
    int agingDirection_ = 0;
    int agingWaveType_ = 0;
    int agingPidType_ = 0;
    int agingAverageNu_ = 100;
    int agingSendIntervalMs_ = 2;
    int agingNoDriveHours_ = 1;
    bool agingNoDriveTriggered_ = false;
    double agingCurrentStep_ = 0.0;
    int agingSentFrames_ = 0;
    int agingLastGuideX_ = 0;
    int agingLastGuideY_ = 0;
    QDateTime agingStartedAt_;
    QDateTime agingStopAt_;
    QString agingLastStatus_ = QStringLiteral("未启动");
    bool agingRecordEnabled_ = false;
    qint64 agingRecordedRows_ = 0;
    QString agingRecordStatus_ = QStringLiteral("数据库记录已关闭");
    QString agingDbPath_;
    QString agingDbConnectionName_;

    QVariantList upSendNuSeries_;
    QVariantList xAngleSeries_;
    QVariantList yAngleSeries_;
    QVariantList xAdSeries_;
    QVariantList yAdSeries_;
    QVariantList xSpeedSeries_;
    QVariantList ySpeedSeries_;
    QVariantList currentSeries_;
    QVariantList temperatureSeries_;
};

FastMirrorProtocolBridge::FastMirrorProtocolBridge()
    : impl_(std::make_unique<Impl>()) {}

FastMirrorProtocolBridge::~FastMirrorProtocolBridge() = default;

QVariantMap FastMirrorProtocolBridge::capabilities() const {
    return QVariantMap{
        {QStringLiteral("deviceId"), QStringLiteral("fast_mirror")},
        {QStringLiteral("deviceType"), QStringLiteral("fast_mirror")},
        {QStringLiteral("displayName"), QStringLiteral("Fast Mirror Device")},
        {QStringLiteral("protocol.type"), QStringLiteral("rs422.fast_mirror")},
        {QStringLiteral("transports"), QVariantList{QStringLiteral("serial")}},
        {QStringLiteral("serial"),
         QVariantMap{
             {QStringLiteral("defaultBaudRate"), kDefaultBaudRate},
             {QStringLiteral("dataBits"), static_cast<int>(QSerialPort::Data8)},
             {QStringLiteral("parity"), static_cast<int>(QSerialPort::NoParity)},
             {QStringLiteral("stopBits"), static_cast<int>(QSerialPort::OneStop)},
         }},
        {QStringLiteral("actions"),
         QVariantList{
             QString::fromLatin1(device_api::actions::kConnectSerial),
             QString::fromLatin1(device_api::actions::kDisconnect),
             QString::fromLatin1(device_api::actions::kSendRawHex),
             QString::fromLatin1(kFastMirrorSendCommand),
             QString::fromLatin1(kFastMirrorSetIdle),
             QString::fromLatin1(kFastMirrorSetLockZero),
             QString::fromLatin1(kFastMirrorSetPoint),
             QString::fromLatin1(kFastMirrorSetDataType),
             QString::fromLatin1(kFastMirrorLegacyStart),
             QString::fromLatin1(kFastMirrorLegacyStop),
             QString::fromLatin1(kFastMirrorLegacyPacket),
             QString::fromLatin1(kFastMirrorLegacyFixed),
             QString::fromLatin1(kFastMirrorLegacyStep),
             QString::fromLatin1(kFastMirrorLegacyStepPid),
             QString::fromLatin1(kFastMirrorLegacyScan),
             QString::fromLatin1(kFastMirrorLegacySpeed),
             QString::fromLatin1(kFastMirrorLegacySystem),
             QString::fromLatin1(kFastMirrorLegacyOmSpeed),
             QString::fromLatin1(kFastMirrorLegacyOmReparation),
             QString::fromLatin1(kFastMirrorLegacyReadParam),
             QString::fromLatin1(kFastMirrorLegacyWriteParam),
             QString::fromLatin1(kFastMirrorLegacyWriteA0),
             QString::fromLatin1(kFastMirrorLegacyWriteA1),
             QString::fromLatin1(kFastMirrorLegacyWriteA3),
             QString::fromLatin1(kFastMirrorLegacyWriteA4),
             QString::fromLatin1(kFastMirrorLegacyWriteA5),
             QString::fromLatin1(kFastMirrorLegacyWriteA6),
             QString::fromLatin1(kFastMirrorLegacyWriteA7),
             QString::fromLatin1(kFastMirrorLegacyWriteA8),
             QString::fromLatin1(kFastMirrorLegacyWriteA9),
             QString::fromLatin1(kFastMirrorLegacyWriteAA),
             QString::fromLatin1(kFastMirrorLegacyWriteAB),
             QString::fromLatin1(kFastMirrorLegacyWriteAD),
             QString::fromLatin1(kFastMirrorLegacyWriteB0),
             QString::fromLatin1(kFastMirrorLegacyWriteB1),
             QString::fromLatin1(kFastMirrorEnterTestMode),
             QString::fromLatin1(kFastMirrorReadVersionDevice),
             QString::fromLatin1(kFastMirrorReadVersionSoftware),
             QString::fromLatin1(kFastMirrorClearSeries),
             QString::fromLatin1(kFastMirrorUpgradeLoadHex),
             QString::fromLatin1(kFastMirrorUpgradeEnterMode),
             QString::fromLatin1(kFastMirrorUpgradePreUpdate),
             QString::fromLatin1(kFastMirrorUpgradeStart),
             QString::fromLatin1(kFastMirrorUpgradeStop),
             QString::fromLatin1(kFastMirrorUpgradeSetInterval),
             QString::fromLatin1(kFastMirrorAgingStart),
             QString::fromLatin1(kFastMirrorAgingStop),
             QString::fromLatin1(kFastMirrorAgingRecordEnable),
             QString::fromLatin1(kFastMirrorAgingRecordDelete),
         }}}; 
}

bool FastMirrorProtocolBridge::connectSerial(const QVariantMap& options) {
    if (impl_->serialPort_->isOpen()) {
        impl_->serialPort_->close();
    }

    impl_->stopUpgradeDownload(false);
    impl_->stopAging(false);
    impl_->buffer_.clear();
    impl_->lastChecksumOk_ = false;
    impl_->lastRxFrameHex_.clear();
    impl_->lastRxPayloadHex_.clear();
    impl_->lastTxFrameHex_.clear();
    impl_->lastError_.clear();
    impl_->clearSeries();

    impl_->serialPort_->setPortName(options.value(QStringLiteral("portName")).toString().trimmed());
    impl_->serialPort_->setBaudRate(options.value(QStringLiteral("baudRate"), kDefaultBaudRate).toInt());
    impl_->serialPort_->setDataBits(static_cast<QSerialPort::DataBits>(
        options.value(QStringLiteral("dataBits"), static_cast<int>(QSerialPort::Data8)).toInt()));
    impl_->serialPort_->setParity(static_cast<QSerialPort::Parity>(
        options.value(QStringLiteral("parity"), static_cast<int>(QSerialPort::NoParity)).toInt()));
    impl_->serialPort_->setStopBits(static_cast<QSerialPort::StopBits>(
        options.value(QStringLiteral("stopBits"), static_cast<int>(QSerialPort::OneStop)).toInt()));
    impl_->serialPort_->setFlowControl(static_cast<QSerialPort::FlowControl>(
        options.value(QStringLiteral("flowControl"), static_cast<int>(QSerialPort::NoFlowControl)).toInt()));

    const bool ok = impl_->serialPort_->open(QIODevice::ReadWrite);
    impl_->connected_ = ok;
    impl_->linkType_ = ok ? QStringLiteral("serial") : QStringLiteral("none");
    impl_->lastError_ = ok ? QString() : impl_->serialPort_->errorString();
    return ok;
}

bool FastMirrorProtocolBridge::connectUdp(const QVariantMap& options) {
    Q_UNUSED(options)
    impl_->lastError_ = QStringLiteral("fast mirror only supports serial transport");
    return false;
}

void FastMirrorProtocolBridge::disconnectDevice() {
    if (impl_->serialPort_->isOpen()) {
        impl_->serialPort_->close();
    }

    impl_->stopUpgradeDownload(false);
    impl_->stopAging(false);
    impl_->connected_ = false;
    impl_->linkType_ = QStringLiteral("none");
    impl_->legacyStreaming_ = false;
    impl_->buffer_.clear();
}

bool FastMirrorProtocolBridge::isConnected() const {
    return impl_->connected_;
}

bool FastMirrorProtocolBridge::invoke(const QString& action, const QVariantMap& args) {
    if (action == QString::fromLatin1(device_api::actions::kDisconnect)) {
        disconnectDevice();
        return true;
    }

    if (action == QString::fromLatin1(device_api::actions::kSendRawHex)) {
        const QString source =
            args.value(QStringLiteral("payload"), args.value(QStringLiteral("hex")).toString()).toString();
        return impl_->writeBytes(decodeHexString(source));
    }

    if (action == QString::fromLatin1(kFastMirrorSendCommand)) {
        const quint16 mode = parseModeCode(args.value(QStringLiteral("mode")), 0x02);
        const float xGuide = args.value(QStringLiteral("xGuide"), args.value(QStringLiteral("x"), 0.0)).toFloat();
        const float yGuide = args.value(QStringLiteral("yGuide"), args.value(QStringLiteral("y"), 0.0)).toFloat();
        return impl_->sendControl(mode, xGuide, yGuide);
    }

    if (action == QString::fromLatin1(kFastMirrorSetIdle)) {
        return impl_->sendControl(0x00, 0.0f, 0.0f);
    }

    if (action == QString::fromLatin1(kFastMirrorSetLockZero)) {
        return impl_->sendControl(0x01, 0.0f, 0.0f);
    }

    if (action == QString::fromLatin1(kFastMirrorSetPoint)) {
        const float xGuide = args.value(QStringLiteral("xGuide"), args.value(QStringLiteral("x"), 0.0)).toFloat();
        const float yGuide = args.value(QStringLiteral("yGuide"), args.value(QStringLiteral("y"), 0.0)).toFloat();
        return impl_->sendControl(0x02, xGuide, yGuide);
    }

    if (action == QString::fromLatin1(kFastMirrorSetDataType)) {
        const int bits = toInt(args, QStringLiteral("bits"), toInt(args, QStringLiteral("dataType"), 16));
        impl_->dataTypeBits_ = (bits == 32) ? 32 : 16;
        return true;
    }

    if (action == QString::fromLatin1(kFastMirrorLegacyStart)) {
        const quint8 command =
            parseCommandCode(args.value(QStringLiteral("command")), impl_->legacyStreamCommand_);
        const int repeat = toInt(args, QStringLiteral("repeat"), 2);
        return impl_->sendLegacyKick(command, repeat);
    }

    if (action == QString::fromLatin1(kFastMirrorLegacyStop)) {
        return impl_->sendLegacyKick(kCmdLegacyStop, 2);
    }

    if (action == QString::fromLatin1(kFastMirrorLegacyPacket)) {
        const quint8 command = parseCommandCode(args.value(QStringLiteral("command")), kCmdLegacyFixed);
        const QByteArray payload =
            decodeHexString(args.value(QStringLiteral("payloadHex"), args.value(QStringLiteral("payload"))).toString());
        if (payload.isEmpty()) {
            return impl_->sendLegacyKick(command, std::max(1, toInt(args, QStringLiteral("repeat"), 1)));
        }
        const quint16 declaredLength =
            static_cast<quint16>(toInt(args, QStringLiteral("declaredLength"), 4 + payload.size()));
        return impl_->sendLegacyPayload(command, payload, declaredLength);
    }

    if (action == QString::fromLatin1(kFastMirrorLegacyFixed)) {
        return impl_->runLegacyFixed(args);
    }

    if (action == QString::fromLatin1(kFastMirrorLegacyStep)) {
        return impl_->runLegacyStep(args);
    }

    if (action == QString::fromLatin1(kFastMirrorLegacyStepPid)) {
        return impl_->runLegacyStepPid(args);
    }

    if (action == QString::fromLatin1(kFastMirrorLegacyScan)) {
        return impl_->runLegacyScan(args);
    }

    if (action == QString::fromLatin1(kFastMirrorLegacySpeed)) {
        return impl_->runLegacySpeed(args);
    }

    if (action == QString::fromLatin1(kFastMirrorLegacySystem)) {
        return impl_->runLegacySystem(args);
    }

    if (action == QString::fromLatin1(kFastMirrorLegacyOmSpeed)) {
        return impl_->runLegacyOmSpeed(args);
    }

    if (action == QString::fromLatin1(kFastMirrorLegacyOmReparation)) {
        return impl_->runLegacyOmReparation(args);
    }

    if (action == QString::fromLatin1(kFastMirrorLegacyReadParam)) {
        const quint8 command = parseCommandCode(args.value(QStringLiteral("command")), 0xA0);
        return impl_->runLegacyParamRead(command);
    }

    if (action == QString::fromLatin1(kFastMirrorLegacyWriteParam)) {
        return impl_->runLegacyParamWrite(args);
    }

    if (action == QString::fromLatin1(kFastMirrorLegacyWriteA0)) {
        return impl_->sendLegacyParamFrame(buildLegacyParamFrameA0(args));
    }

    if (action == QString::fromLatin1(kFastMirrorLegacyWriteA1)) {
        return impl_->sendLegacyParamFrame(buildLegacyParamFrameA1(args));
    }

    if (action == QString::fromLatin1(kFastMirrorLegacyWriteA3)) {
        return impl_->sendLegacyParamFrame(buildLegacyParamFrameA3(args));
    }

    if (action == QString::fromLatin1(kFastMirrorLegacyWriteA4)) {
        return impl_->sendLegacyParamFrame(buildLegacyParamFrameA4(args));
    }

    if (action == QString::fromLatin1(kFastMirrorLegacyWriteA5)) {
        return impl_->sendLegacyParamFrame(buildLegacyParamFrameA5(args));
    }

    if (action == QString::fromLatin1(kFastMirrorLegacyWriteA6)) {
        return impl_->sendLegacyParamFrame(buildLegacyParamFrameA6(args));
    }

    if (action == QString::fromLatin1(kFastMirrorLegacyWriteA7)) {
        return impl_->sendLegacyParamFrame(buildLegacyParamFrameA7(args));
    }

    if (action == QString::fromLatin1(kFastMirrorLegacyWriteA8)) {
        return impl_->sendLegacyParamFrame(buildLegacyParamFrameA8(args));
    }

    if (action == QString::fromLatin1(kFastMirrorLegacyWriteA9)) {
        return impl_->sendLegacyParamFrame(buildLegacyParamFrameA9(args));
    }

    if (action == QString::fromLatin1(kFastMirrorLegacyWriteAA)) {
        return impl_->sendLegacyParamFrame(buildLegacyParamFrameAA(args));
    }

    if (action == QString::fromLatin1(kFastMirrorLegacyWriteAB)) {
        return impl_->sendLegacyParamFrame(buildLegacyParamFrameAB(args));
    }

    if (action == QString::fromLatin1(kFastMirrorLegacyWriteAD)) {
        return impl_->sendLegacyParamFrame(buildLegacyParamFrameAD(args));
    }

    if (action == QString::fromLatin1(kFastMirrorLegacyWriteB0)) {
        return impl_->sendLegacyParamFrame(buildLegacyParamFrameB0(args), 3000);
    }

    if (action == QString::fromLatin1(kFastMirrorLegacyWriteB1)) {
        return impl_->sendLegacyParamFrame(buildLegacyParamFrameB1(args));
    }

    if (action == QString::fromLatin1(kFastMirrorEnterTestMode)) {
        QByteArray frame;
        frame.reserve(14);
        frame.push_back(static_cast<char>(0x7E));
        frame.push_back(static_cast<char>(0x55));
        frame.push_back(static_cast<char>(0xAA));
        frame.push_back(static_cast<char>(0x55));
        frame.push_back(static_cast<char>(0xAA));
        frame.push_back(static_cast<char>(0x01));
        frame.push_back(static_cast<char>(0x02));
        frame.push_back(static_cast<char>(0x03));
        frame.push_back(static_cast<char>(0x04));
        frame.push_back(static_cast<char>(0x05));
        frame.push_back(static_cast<char>(0x06));
        frame.push_back(static_cast<char>(0x07));
        frame.push_back(static_cast<char>(0xE7));
        frame.push_back(static_cast<char>(0x7F));
        return impl_->writeBytes(frame);
    }

    if (action == QString::fromLatin1(kFastMirrorReadVersionDevice)) {
        return impl_->sendLegacyKick(kCmdLegacyVersionDevice, 1, false);
    }

    if (action == QString::fromLatin1(kFastMirrorReadVersionSoftware)) {
        return impl_->sendLegacyKick(kCmdLegacyVersionSoftware, 1, false);
    }

    if (action == QString::fromLatin1(kFastMirrorClearSeries)) {
        impl_->clearSeries();
        return true;
    }

    if (action == QString::fromLatin1(kFastMirrorUpgradeLoadHex)) {
        const QString path = args.value(QStringLiteral("path"), args.value(QStringLiteral("filePath"))).toString();
        impl_->applyUpgradeOptions(args);
        return impl_->loadUpgradeHexFile(path);
    }

    if (action == QString::fromLatin1(kFastMirrorUpgradeSetInterval)) {
        const int intervalMs = toInt(args, QStringLiteral("intervalMs"), toInt(args, QStringLiteral("interval"), 50));
        return impl_->setUpgradeInterval(intervalMs);
    }

    if (action == QString::fromLatin1(kFastMirrorUpgradeEnterMode)) {
        return impl_->sendUpgradeEnterMode(args);
    }

    if (action == QString::fromLatin1(kFastMirrorUpgradePreUpdate)) {
        return impl_->sendUpgradePreUpdate(args);
    }

    if (action == QString::fromLatin1(kFastMirrorUpgradeStart)) {
        return impl_->startUpgradeDownload(args);
    }

    if (action == QString::fromLatin1(kFastMirrorUpgradeStop)) {
        return impl_->stopUpgradeDownload(true);
    }

    if (action == QString::fromLatin1(kFastMirrorAgingStart)) {
        return impl_->startAging(args);
    }

    if (action == QString::fromLatin1(kFastMirrorAgingStop)) {
        return impl_->stopAging(true);
    }

    if (action == QString::fromLatin1(kFastMirrorAgingRecordEnable)) {
        const bool enabled =
            args.value(QStringLiteral("enabled"), args.value(QStringLiteral("on"), true)).toBool();
        return impl_->setAgingRecordEnabled(enabled);
    }

    if (action == QString::fromLatin1(kFastMirrorAgingRecordDelete)) {
        return impl_->deleteAgingRecordData();
    }

    return false;
}

QVariantMap FastMirrorProtocolBridge::snapshot() const {
    const quint16 faultBits = impl_->faultBits_;
    const QDateTime now = QDateTime::currentDateTime();
    const qint64 agingElapsedSeconds =
        impl_->agingStartedAt_.isValid() ? std::max<qint64>(0, impl_->agingStartedAt_.secsTo(now)) : 0;
    const qint64 agingRemainingSeconds =
        (impl_->agingRunning_ && impl_->agingStopAt_.isValid())
            ? std::max<qint64>(0, now.secsTo(impl_->agingStopAt_))
            : 0;
    return QVariantMap{
        {QStringLiteral("status.connected"), impl_->connected_},
        {QStringLiteral("status.linkType"), impl_->linkType_},
        {QStringLiteral("status.modeCode"), impl_->statusModeCode_},
        {QStringLiteral("status.modeText"), impl_->statusModeText_},
        {QStringLiteral("status.faultBits"), faultBits},
        {QStringLiteral("status.faultBitsHex"),
         QStringLiteral("0x%1").arg(faultBits, 4, 16, QChar('0')).toUpper()},
        {QStringLiteral("status.lastChecksumOk"), impl_->lastChecksumOk_},
        {QStringLiteral("status.selfCheckFault"), (faultBits & 0x0001u) != 0u},
        {QStringLiteral("status.inputVoltageFault"), (faultBits & 0x0002u) != 0u},
        {QStringLiteral("status.driveOutputFault"), (faultBits & 0x0004u) != 0u},
        {QStringLiteral("status.commTimeout"), (faultBits & 0x0008u) != 0u},
        {QStringLiteral("status.checksumFault"), (faultBits & 0x0010u) != 0u},
        {QStringLiteral("status.motorDisabled"), (faultBits & 0x0020u) != 0u},
        {QStringLiteral("status.locked"), (faultBits & 0x0040u) != 0u},
        {QStringLiteral("runtime.currentXAngle"), impl_->currentXAngle_},
        {QStringLiteral("runtime.currentYAngle"), impl_->currentYAngle_},
        {QStringLiteral("runtime.currentXAd"), impl_->currentXAd_},
        {QStringLiteral("runtime.currentYAd"), impl_->currentYAd_},
        {QStringLiteral("runtime.currentXSpeed"), impl_->currentXSpeed_},
        {QStringLiteral("runtime.currentYSpeed"), impl_->currentYSpeed_},
        {QStringLiteral("runtime.currentCurrent"), impl_->currentCurrent_},
        {QStringLiteral("runtime.currentTemperature"), impl_->currentTemperature_},
        {QStringLiteral("runtime.currentXAngle55AA"), impl_->currentXAngle55AA_},
        {QStringLiteral("runtime.currentYAngle55AA"), impl_->currentYAngle55AA_},
        {QStringLiteral("runtime.legacyUpSendNu"), impl_->legacyUpSendNu_},
        {QStringLiteral("runtime.sampleIndex"), impl_->sampleIndex_},
        {QStringLiteral("runtime.series.upSendNu"), impl_->upSendNuSeries_},
        {QStringLiteral("runtime.series.xAngle"), impl_->xAngleSeries_},
        {QStringLiteral("runtime.series.yAngle"), impl_->yAngleSeries_},
        {QStringLiteral("runtime.series.xAd"), impl_->xAdSeries_},
        {QStringLiteral("runtime.series.yAd"), impl_->yAdSeries_},
        {QStringLiteral("runtime.series.xSpeed"), impl_->xSpeedSeries_},
        {QStringLiteral("runtime.series.ySpeed"), impl_->ySpeedSeries_},
        {QStringLiteral("runtime.series.current"), impl_->currentSeries_},
        {QStringLiteral("runtime.series.temperature"), impl_->temperatureSeries_},
        {QStringLiteral("legacy.dataTypeBits"), impl_->dataTypeBits_},
        {QStringLiteral("legacy.streaming"), impl_->legacyStreaming_},
        {QStringLiteral("legacy.streamCommand"),
         QStringLiteral("0x%1").arg(impl_->legacyStreamCommand_, 2, 16, QChar('0')).toUpper()},
        {QStringLiteral("legacy.lastCommand"),
         QStringLiteral("0x%1").arg(impl_->legacyLastCommand_, 2, 16, QChar('0')).toUpper()},
        {QStringLiteral("legacy.lastRxCommand"),
         QStringLiteral("0x%1").arg(impl_->legacyLastRxCommand_, 2, 16, QChar('0')).toUpper()},
        {QStringLiteral("legacy.lastCommandTime"), impl_->legacyLastCommandTime_},
        {QStringLiteral("legacy.paraSetFlag"), impl_->legacyParaSetFlag_},
        {QStringLiteral("legacy.lastParamCommand"),
         QStringLiteral("0x%1").arg(impl_->legacyLastParamCommand_, 2, 16, QChar('0')).toUpper()},
        {QStringLiteral("legacy.lastParamLength"), impl_->legacyLastParamLength_},
        {QStringLiteral("legacy.lastParamPayloadHex"), impl_->legacyLastParamPayloadHex_},
        {QStringLiteral("legacy.deviceInfoText"), impl_->legacyDeviceInfoText_},
        {QStringLiteral("legacy.deviceVersion"), impl_->legacyDeviceVersion_},
        {QStringLiteral("legacy.softwareVersion"), impl_->legacySoftwareVersion_},
        {QStringLiteral("legacy.om.validPoints"), impl_->legacyOmValidPoints_},
        {QStringLiteral("legacy.om.uniformVsum"), impl_->legacyOmUniformVsum_},
        {QStringLiteral("legacy.om.uniformPsum"), impl_->legacyOmUniformPsum_},
        {QStringLiteral("legacy.om.uniformAveV"), impl_->legacyOmUniformAveV_},
        {QStringLiteral("legacy.om.uniformDiffAmpV"), impl_->legacyOmUniformDiffAmpV_},
        {QStringLiteral("legacy.om.uniformDiffAveV"), impl_->legacyOmUniformDiffAveV_},
        {QStringLiteral("legacy.om.uniformDiffAmpP"), impl_->legacyOmUniformDiffAmpP_},
        {QStringLiteral("legacy.om.uniformDiffAveP"), impl_->legacyOmUniformDiffAveP_},
        {QStringLiteral("legacy.om.temperature"), impl_->legacyOmTemperature_},
        {QStringLiteral("legacy.om.scanPositions"), impl_->legacyOmScanPositions_},
        {QStringLiteral("upgrade.filePath"), impl_->upgradeFilePath_},
        {QStringLiteral("upgrade.preparedText"), impl_->upgradePreparedText_},
        {QStringLiteral("upgrade.totalLines"), impl_->upgradeTotalLines_},
        {QStringLiteral("upgrade.validLines"), impl_->upgradeValidLines_},
        {QStringLiteral("upgrade.invalidLines"), impl_->upgradeInvalidLines_},
        {QStringLiteral("upgrade.payloadBytes"), impl_->upgradePayloadBytes_},
        {QStringLiteral("upgrade.chunkSize"), impl_->upgradeChunkSize_},
        {QStringLiteral("upgrade.hostId"),
         QStringLiteral("0x%1").arg(impl_->upgradeHostId_, 2, 16, QChar('0')).toUpper()},
        {QStringLiteral("upgrade.targetId"),
         QStringLiteral("0x%1").arg(impl_->upgradeTargetId_, 2, 16, QChar('0')).toUpper()},
        {QStringLiteral("upgrade.currentLine"), impl_->upgradeCurrentLine_},
        {QStringLiteral("upgrade.progress"), impl_->upgradeProgress_},
        {QStringLiteral("upgrade.crc16"),
         QStringLiteral("0x%1").arg(impl_->upgradeCrc16_, 4, 16, QChar('0')).toUpper()},
        {QStringLiteral("upgrade.running"), impl_->upgradeRunning_},
        {QStringLiteral("upgrade.lastStatusCode"),
         QStringLiteral("0x%1").arg(impl_->upgradeLastStatusCode_, 2, 16, QChar('0')).toUpper()},
        {QStringLiteral("upgrade.lastStatusCommand"),
         QStringLiteral("0x%1").arg(impl_->upgradeLastStatusCommand_, 4, 16, QChar('0')).toUpper()},
        {QStringLiteral("upgrade.lastSenderId"),
         QStringLiteral("0x%1").arg(impl_->upgradeLastSenderId_, 2, 16, QChar('0')).toUpper()},
        {QStringLiteral("upgrade.lastReceiverId"),
         QStringLiteral("0x%1").arg(impl_->upgradeLastReceiverId_, 2, 16, QChar('0')).toUpper()},
        {QStringLiteral("upgrade.lastStatus"), impl_->upgradeLastStatus_},
        {QStringLiteral("upgrade.lastRxHex"), impl_->upgradeLastRxHex_},
        {QStringLiteral("upgrade.lastTxHex"), impl_->upgradeLastTxDataHex_},
        {QStringLiteral("upgrade.intervalMs"), impl_->upgradeIntervalMs_},
        {QStringLiteral("aging.running"), impl_->agingRunning_},
        {QStringLiteral("aging.amplitude"), impl_->agingAmplitude_},
        {QStringLiteral("aging.frequency"), impl_->agingFrequency_},
        {QStringLiteral("aging.direction"), impl_->agingDirection_},
        {QStringLiteral("aging.waveType"), impl_->agingWaveType_},
        {QStringLiteral("aging.pidType"), impl_->agingPidType_},
        {QStringLiteral("aging.averageNu"), impl_->agingAverageNu_},
        {QStringLiteral("aging.intervalMs"), impl_->agingSendIntervalMs_},
        {QStringLiteral("aging.noDriveHours"), impl_->agingNoDriveHours_},
        {QStringLiteral("aging.noDriveTriggered"), impl_->agingNoDriveTriggered_},
        {QStringLiteral("aging.startedAt"),
         impl_->agingStartedAt_.isValid() ? impl_->agingStartedAt_.toString(Qt::ISODateWithMs) : QString()},
        {QStringLiteral("aging.stopAt"),
         impl_->agingStopAt_.isValid() ? impl_->agingStopAt_.toString(Qt::ISODateWithMs) : QString()},
        {QStringLiteral("aging.elapsedSeconds"), agingElapsedSeconds},
        {QStringLiteral("aging.remainingSeconds"), agingRemainingSeconds},
        {QStringLiteral("aging.sentFrames"), impl_->agingSentFrames_},
        {QStringLiteral("aging.lastGuideX"), impl_->agingLastGuideX_},
        {QStringLiteral("aging.lastGuideY"), impl_->agingLastGuideY_},
        {QStringLiteral("aging.lastStatus"), impl_->agingLastStatus_},
        {QStringLiteral("aging.recordEnabled"), impl_->agingRecordEnabled_},
        {QStringLiteral("aging.recordRows"), impl_->agingRecordedRows_},
        {QStringLiteral("aging.recordStatus"), impl_->agingRecordStatus_},
        {QStringLiteral("aging.recordDbPath"), impl_->agingDbPath_},
        {QStringLiteral("command.lastModeCode"), impl_->lastCommandModeCode_},
        {QStringLiteral("command.lastModeText"), impl_->lastCommandModeText_},
        {QStringLiteral("command.guideX"), impl_->lastGuideX_},
        {QStringLiteral("command.guideY"), impl_->lastGuideY_},
        {QStringLiteral("transport.portName"), impl_->serialPort_->portName()},
        {QStringLiteral("transport.baudRate"), impl_->serialPort_->baudRate()},
        {QStringLiteral("transport.bufferBytes"), impl_->buffer_.size()},
        {QStringLiteral("transport.lastRxFrameHex"), impl_->lastRxFrameHex_},
        {QStringLiteral("transport.lastRxPayloadHex"), impl_->lastRxPayloadHex_},
        {QStringLiteral("transport.lastTxFrameHex"), impl_->lastTxFrameHex_},
        {QStringLiteral("transport.lastError"), impl_->lastError_},
    };
}
