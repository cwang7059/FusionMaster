#pragma once

namespace device_api::actions {

inline constexpr const char* kConnectSerial = "connect.serial";
inline constexpr const char* kConnectUdp = "connect.udp";
inline constexpr const char* kDisconnect = "disconnect";
inline constexpr const char* kSendRawHex = "send.raw.hex";
inline constexpr const char* kSetReceiveType = "set.receive.type";
inline constexpr const char* kEncoderReadVersion = "encoder.read.version";
inline constexpr const char* kEncoderReadDiv = "encoder.read.div";
inline constexpr const char* kEncoderReadPact = "encoder.read.pact";
inline constexpr const char* kEncoderReadOuterResas = "encoder.read.outer_resas";
inline constexpr const char* kEncoderReadInnerResas = "encoder.read.inner_resas";
inline constexpr const char* kEncoderWriteState = "encoder.write.state";
inline constexpr const char* kEncoderWriteOuterResas = "encoder.write.outer_resas";
inline constexpr const char* kEncoderWriteInnerResas = "encoder.write.inner_resas";

}  // namespace device_api::actions
