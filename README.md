# osgi_business_dev

业务开发仓库，目录结构与 `osgi_core` 保持一致。

约定：

- `app` 源码可见，便于本地联调与 F5 调试。
- `core/common/tools` 以制品形态下发（头文件/库/可执行文件）。
- 业务开发者仅在 `plugins_business` 目录进行业务插件开发。
- `third_party` 已下发可离线构建依赖（含 `protoc.exe`）。

建议阅读：

- `docs/插件开发指南.md`
- `docs/协议与插件职责规范.md`

调试约定：

- 生成 Visual Studio 解决方案后，默认启动项目为 `app`。
- 默认调试参数为 `--profile <仓库>/profiles/default.json`，可直接 F5。