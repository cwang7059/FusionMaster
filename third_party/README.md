# third_party 目录说明

按平台存放预编译第三方依赖：
- `ctk`
- `zeromq`
- `protobuf`
- `spdlog`
- `cppzmq`
- `ffmpeg`（RTSP 记录/重演可选能力）
- `mediamtx`（RTSP 流媒体服务，用于现场模拟联调）

要求：预编译头文件和二进制产物统一放在本目录，确保离线可编译。

目录约定（以 ffmpeg 为例）：
- `third_party/ffmpeg/include`
- `third_party/ffmpeg/win-x64-qt5-vs2019/{lib,bin}`
- `third_party/ffmpeg/win-x64-qt6-vs2026/{lib,bin}`
- `third_party/ffmpeg/linux-x64/{lib,bin}`

目录约定（mediamtx）：
- `third_party/mediamtx/win-x64/mediamtx.exe`
- `third_party/mediamtx/win-x64/mediamtx.yml`
- `third_party/mediamtx/win-x64/LICENSE`
