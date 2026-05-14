#pragma once

#include <QByteArray>
#include <QDataStream>
#include <QIODevice>
#include <QString>
#include <QtGlobal>

namespace plugin_api {

constexpr quint32 kRtspPacketMagic = 0x52545350u;  // "RTSP"
constexpr quint16 kRtspPacketVersion = 2u;
constexpr quint16 kRtspPacketLegacyVersion = 1u;

struct RtspPacketPayload {
    qint64 pts = 0;
    qint64 dts = 0;
    qint64 duration = 0;
    qint32 flags = 0;
    qint32 streamIndex = 0;
    qint32 codecId = 0;
    qint64 wallClockUs = 0;
    QByteArray extradata;
    QByteArray encoded;
};

inline QByteArray packRtspPacketPayload(const RtspPacketPayload& packet) {
    QByteArray payload;
    payload.reserve(80 + packet.extradata.size() + packet.encoded.size());

    QDataStream stream(&payload, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream.setVersion(QDataStream::Qt_5_12);

    stream << static_cast<quint32>(kRtspPacketMagic);
    stream << static_cast<quint16>(kRtspPacketVersion);
    stream << static_cast<qint64>(packet.pts);
    stream << static_cast<qint64>(packet.dts);
    stream << static_cast<qint64>(packet.duration);
    stream << static_cast<qint32>(packet.flags);
    stream << static_cast<qint32>(packet.streamIndex);
    stream << static_cast<qint32>(packet.codecId);
    stream << static_cast<qint64>(packet.wallClockUs);
    stream << static_cast<qint32>(packet.extradata.size());
    stream << static_cast<qint32>(packet.encoded.size());

    if (!packet.extradata.isEmpty()) {
        stream.writeRawData(packet.extradata.constData(), packet.extradata.size());
    }
    if (!packet.encoded.isEmpty()) {
        stream.writeRawData(packet.encoded.constData(), packet.encoded.size());
    }

    return payload;
}

inline bool parseRtspPacketPayload(const QByteArray& payload, RtspPacketPayload* out) {
    if (out == nullptr || payload.isEmpty()) {
        return false;
    }

    out->extradata.clear();
    out->encoded.clear();

    QDataStream stream(payload);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream.setVersion(QDataStream::Qt_5_12);

    quint32 magic = 0;
    quint16 version = 0;
    qint32 extradataSize = 0;
    qint32 encodedSize = 0;

    stream >> magic;
    stream >> version;
    if (stream.status() != QDataStream::Ok || magic != kRtspPacketMagic) {
        return false;
    }

    if (version == kRtspPacketLegacyVersion) {
        stream >> out->pts;
        stream >> out->dts;
        stream >> out->duration;
        stream >> out->flags;
        stream >> out->streamIndex;
        stream >> out->codecId;
        stream >> out->wallClockUs;
        stream >> encodedSize;
    } else if (version == kRtspPacketVersion) {
        stream >> out->pts;
        stream >> out->dts;
        stream >> out->duration;
        stream >> out->flags;
        stream >> out->streamIndex;
        stream >> out->codecId;
        stream >> out->wallClockUs;
        stream >> extradataSize;
        stream >> encodedSize;
    } else {
        return false;
    }

    if (stream.status() != QDataStream::Ok
        || extradataSize < 0
        || encodedSize <= 0) {
        return false;
    }

    if (extradataSize > 0) {
        out->extradata.resize(extradataSize);
        const int readExtraSize = stream.readRawData(out->extradata.data(), extradataSize);
        if (readExtraSize != extradataSize || stream.status() != QDataStream::Ok) {
            out->extradata.clear();
            out->encoded.clear();
            return false;
        }
    }

    out->encoded.resize(encodedSize);
    const int readEncodedSize = stream.readRawData(out->encoded.data(), encodedSize);
    if (readEncodedSize != encodedSize || stream.status() != QDataStream::Ok) {
        out->extradata.clear();
        out->encoded.clear();
        return false;
    }

    return true;
}

inline QString rtspStreamIdFromTopic(const QString& topic) {
    QString value = topic.trimmed();
    if (value.startsWith(QStringLiteral("rtsp/raw/"), Qt::CaseInsensitive)) {
        value = value.mid(QStringLiteral("rtsp/raw/").size());
    }
    if (value.isEmpty()) {
        return QStringLiteral("unknown");
    }
    return value;
}

}  // namespace plugin_api

