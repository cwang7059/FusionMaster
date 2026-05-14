# FusionMaster

FusionMaster 是一个基于 Qt/QML 与插件化框架构建的试验数据融合分析平台。当前版本已按照 `figma_ui` 原型完成主界面集成，并保留历史数据融合分析业务插件作为核心业务入口。

## 已集成界面

主程序顶部导航已集成 6 个原型界面：

- 首页总览
- 数据调取与管理
- 多批次融合分析
- 算法优选与建模
- 统计与可视化
- 系统管理

底部状态栏、顶部导航、窗口按钮、业务内容区域均已接入现有 QML 框架。

## 业务插件

当前 `plugins_business` 仅保留：

- `org_business_history_data_fusion_analysis`

其他业务插件源码已移除，默认配置只加载数据中心、数据仓储和历史数据融合分析插件。

## 开发文档

- [六业务 Tab 插件化开发流程](docs/pluginized-business-tabs-development-flow.md)：整理数据库仓储插件、六个业务 Tab 插件、共享 UI 模块的分阶段改造步骤。

## MySQL 配置

数据仓储插件 `org_common_data_repository` 默认使用 Qt SQL 的 `QMYSQL` 驱动。连接参数可通过 `config/database.json` 或环境变量配置，示例见 [database.example.json](config/database.example.json)。

支持的环境变量：

- `FUSIONMASTER_DB_HOST`
- `FUSIONMASTER_DB_PORT`
- `FUSIONMASTER_DB_NAME`
- `FUSIONMASTER_DB_USER`
- `FUSIONMASTER_DB_PASSWORD`
- `FUSIONMASTER_DB_CONNECT_OPTIONS`

## 环境要求

- Windows
- Visual Studio 2019 x64 工具链
- Qt 5.14.2，路径示例：`D:/Qt5.14.2`
- CMake
- Git LFS，用于管理 `.dll`、`.lib`、`.exe` 等二进制依赖

## 构建

生成工程：

```powershell
cmake -S . -B build_codex_qt5 -G "Visual Studio 16 2019" -A x64 -DOSGI_QT_MAJOR=5 -DOSGI_QT_ROOT=D:/Qt5.14.2
```

编译 Debug 版本：

```powershell
cmake --build build_codex_qt5 --config Debug --target app
```

## 运行

```powershell
.\build_codex_qt5\bin\Debug\app.exe --profile .\profiles\default.json
```

默认 profile：

- 加载 `org_common_data_center`
- 加载 `org_common_data_repository`
- 加载 `org_business_history_data_fusion_analysis`
- 启用原型顶部导航和底部状态栏

## 目录结构

- `app`：主程序、QML 主界面和资源
- `common`：通用插件
- `core`：插件 API、插件管理、窗口管理、主题等核心模块
- `plugins_business`：业务插件，目前仅保留历史数据融合分析插件
- `profiles`：运行配置
- `third_party`：离线构建依赖
- `tools`：辅助工具

## 验证记录

已完成以下验证：

- QML lint 检查通过
- Debug 版本构建通过
- 默认配置启动成功
- 6 个顶部导航界面可切换
- 运行时启动 `org_common_data_center`、`org_common_data_repository` 与 `org_business_history_data_fusion_analysis`
