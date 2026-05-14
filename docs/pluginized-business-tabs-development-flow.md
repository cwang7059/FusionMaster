# FusionMaster 六业务 Tab 插件化开发流程

本文档用于指导后续把 FusionMaster 的六个业务 Tab 逐步改造成独立业务插件，同时保留 `org_common_data_center` 作为共享数据服务，并把数据库访问与公共 UI 能力拆到可复用模块中。

## 目标架构

插件化后的职责边界如下：

- `app`：主程序壳、窗口布局、插件发现、插件 UI 装载、应用级主题入口。
- `core/plugin_api`：只放稳定接口，例如 UI 贡献接口、数据模型中心接口、数据库仓储接口。
- `common/org_common_data_center`：共享运行期数据模型中心，负责模型注册、查询、订阅和变更通知。
- `common/org_common_data_repository`：建议新增，负责 MySQL 连接、事务、查询、导入导出、持久化和数据迁移。
- `common/org_common_ui`：建议新增，负责公共 QML 控件、主题 token、按钮、表格、图表、卡片等。
- `plugins_business/*`：六个业务 Tab 插件，每个插件拥有自己的业务控制器、页面 QML 和必要的数据模型。

`org_common_data_center` 不建议承担数据库 CRUD。它应该保持为运行期数据模型中心；数据库读写应由独立仓储插件提供服务。

## 六个业务插件

六个原型 Tab 建议拆成以下业务插件：

- `org_business_system_overview`：首页总览。
- `org_business_data_management`：数据调取与管理。
- `org_business_fusion_analysis`：多批次融合分析。
- `org_business_algorithm_optimization`：算法优选与建模。
- `org_business_statistics`：统计与可视化。
- `org_business_system_settings`：系统管理。

每个业务插件应至少包含：

- `plugin.json`：插件清单、依赖项、动态库名。
- `CMakeLists.txt`：插件构建规则。
- `src/*_plugin.*`：插件入口，注册服务和 UI contribution。
- `src/*_controller.*`：页面控制器，可按复杂度拆分。
- `qml/*View.qml`：该 Tab 的主页面。
- `resources/*.qrc`：QML 和静态资源。

## 分阶段开发流程

### 阶段 1：沉淀数据库仓储服务

目标：把数据库操作从业务插件中抽出来，形成所有业务插件可复用的公共服务。

建议步骤：

1. 在 `core/plugin_api/include/plugin_api` 新增数据库服务接口，例如 `idata_repository.h`。
2. 在接口中定义通用能力：初始化、连接状态、查询、事务、数据集导入、数据集导出、配置读取。
3. 在 `common` 下新增 `org_common_data_repository` 插件。
4. `org_common_data_repository` 默认使用 Qt SQL 的 `QMYSQL` 驱动连接 MySQL。
5. MySQL 连接参数从 `config/database.json` 或环境变量读取，避免把账号密码写死在业务插件中。
6. `org_common_data_repository` 启动时注册服务名，例如 `data_repository`。
7. 业务插件通过 `IPluginManager::getService<IDataRepository>("data_repository")` 获取服务。
8. `plugin.json` 中声明依赖：`plugin_api`、`org_common_data_center`、`org_common_data_repository`。

验收标准：

- 数据库连接和初始化只出现在仓储插件中。
- 业务插件不直接依赖 `QSqlDatabase` 或具体 MySQL 连接参数。
- 仓储插件启动失败时，业务插件可以给出明确错误状态。

MySQL 配置示例：

```json
{
  "database": {
    "driver": "QMYSQL",
    "hostName": "127.0.0.1",
    "port": 3306,
    "databaseName": "fusionmaster",
    "userName": "root",
    "password": "",
    "connectOptions": "MYSQL_OPT_RECONNECT=1"
  }
}
```

阶段 1 落地后的业务侧调用边界：

- 查询：`query(sql, namedValues, positionalValues, connectionName)`。
- 执行：`execute(sql, namedValues, positionalValues, connectionName)`。
- 事务：`beginTransaction(...)`、`commitTransaction(...)`、`rollbackTransaction(...)`。
- 数据集导入：`importRows(tableName, rows, connectionName)`，由仓储服务统一处理批量插入事务。
- 数据集导出：`exportRows(tableName, columns, whereClause, namedValues, positionalValues, connectionName)`。

### 阶段 2：扩展 UI Contribution 元数据

目标：让插件可以声明“我是主界面的一个业务 Tab”，主程序根据插件贡献动态生成顶部导航和页面内容。

当前 `UiContribution` 已有 `id`、`title`、`region`、`qmlSource`、`order`、`screenIndex`。建议增加或约定以下字段：

- `surface`：界面承载区域，例如 `main_tab`、`panel`、`dialog`。
- `navText`：顶部导航显示名称。
- `navIcon`：顶部导航图标。
- `navOrder`：顶部导航排序。
- `requiredServices`：可选，用于声明页面运行需要的服务。

阶段 2 落地后，旧插件不填写新字段时默认按 `panel` 处理；只有声明 `surface = "main_tab"` 的贡献才会在阶段 3 被主界面动态 Tab 容器识别。

建议步骤：

1. 扩展 `core/plugin_api/include/plugin_api/iui_contribution.h`。
2. 更新 `app/main.cpp` 中 `collectUiContributions(...)`，把新增字段透传给 QML。
3. 保持旧字段兼容，旧插件仍可使用 `region` 和 `qmlSource`。
4. 新业务 Tab 插件将 `surface` 设置为 `main_tab`。

验收标准：

- 插件贡献的 Tab 可以在 QML 中被筛选出来。
- 没有贡献 `main_tab` 的普通面板插件不出现在顶部六 Tab 导航里。
- 旧的中心区、左右区插件仍可正常加载。

### 阶段 3：主界面改为动态 Tab 容器

目标：移除主界面对六个业务页面的硬编码。

当前需要关注的文件：

- `app/qml/Main.qml`：当前顶部导航模型仍写死六个 Tab。
- `app/qml/AppTopBar.qml`：当前存在六个 Tab 的兜底文案。
- `app/qml/AppMainContent.qml`：当前 `StackLayout` 直接写死六个页面组件。

建议步骤：

1. 在 `Main.qml` 中从 `uiContributionsModel` 筛选 `surface === "main_tab"` 的贡献。
2. 按 `navOrder` 或 `order` 排序后生成顶部导航模型。
3. `AppTopBar.qml` 改为只渲染传入的导航模型。
4. `AppMainContent.qml` 改为 `Repeater + Loader`，根据当前 Tab 的 `qmlSource` 加载页面。
5. 保留一个空态页面：没有业务 Tab 插件时显示“未加载业务页面”。

验收标准：

- 新增、删除、禁用某个业务插件后，顶部 Tab 自动变化。
- `currentTabIndex` 始终落在有效范围内。
- 插件 QML 加载失败时有明确提示，不导致主程序崩溃。

### 阶段 4：迁移六个原型页面到业务插件

目标：把当前 `app/qml` 下的六个业务 View 迁移到对应业务插件中。

建议迁移顺序：

1. `SystemOverviewView.qml` -> `org_business_system_overview/qml/SystemOverviewView.qml`
2. `DataManagementView.qml` -> `org_business_data_management/qml/DataManagementView.qml`
3. `FusionAnalysisView.qml` -> `org_business_fusion_analysis/qml/FusionAnalysisView.qml`
4. `AlgorithmOptimizationView.qml` -> `org_business_algorithm_optimization/qml/AlgorithmOptimizationView.qml`
5. `StatisticsView.qml` -> `org_business_statistics/qml/StatisticsView.qml`
6. `SystemSettingsView.qml` -> `org_business_system_settings/qml/SystemSettingsView.qml`

每迁移一个页面都要完成：

1. 新建业务插件目录和 `plugin.json`。
2. 新建插件入口类，实现 `IUiContributionProvider`。
3. 把页面 QML 放入插件 `qml` 目录，并加入 qrc。
4. 将 contribution 的 `qmlSource` 指向插件资源路径或相对路径。
5. 按需注册控制器到 QML context 或通过服务暴露数据。
6. 在 profile 中启用该插件。
7. 构建并启动验证。

验收标准：

- 六个业务页面均由插件贡献，不再由 `AppMainContent.qml` 直接引用。
- 禁用任意一个业务插件时，其他 Tab 仍可正常工作。
- 业务页面依赖的公共组件来自共享 UI 模块或主程序公共模块，不复制粘贴。

### 阶段 5：抽取共享 UI 模块

目标：避免六个业务插件重复维护按钮、表格、图表、卡片和主题样式。

短期可以继续复用 `app/qml` 中的公共组件；长期建议迁移到 `common/org_common_ui` 或 `core/ui_theme/qml`。

建议公共组件范围：

- 按钮：主按钮、次按钮、图标按钮、工具栏按钮。
- 表格：数据表、分页、筛选、空态、加载态。
- 图表：折线图、柱状图、饼图、指标卡。
- 布局：页面标题栏、分区标题、工具栏、侧栏、状态条。
- 弹窗：确认框、导入导出进度、错误详情。
- 主题：颜色、字号、间距、边框、阴影、状态色。

验收标准：

- 业务插件 QML 中不重复定义大块通用按钮/表格/图表样式。
- 修改主题或公共组件后，六个业务 Tab 能统一生效。
- 公共 UI 模块不依赖具体业务插件。

## 开发检查清单

新增或改造一个业务 Tab 插件时，按以下清单检查：

- 插件名符合三段式命名，例如 `org_business_xxx`。
- `plugin.json` 的 `name` 与目录名一致。
- `plugin.json` 正确声明 `plugin_api`、`org_common_data_center`、`org_common_data_repository` 等依赖。
- CMake 中插件目标被加入运行时插件目标集合。
- 插件启动时注册 `IUiContributionProvider`。
- 插件 contribution 包含唯一 `id`、标题、排序、`surface` 和 `qmlSource`。
- 页面 QML 不直接访问其他业务插件内部对象。
- 数据共享走 `IDataModelCenter`，持久化读写走 `IDataRepository`。
- 公共 UI 优先复用共享模块。
- 禁用该插件后，主程序仍可启动。
- QML lint 通过。
- Debug 构建通过。
- 默认 profile 启动成功。

## 推荐提交粒度

每个阶段建议单独提交，便于回滚和排查：

1. `新增数据库仓储服务接口`
2. `新增公共数据库仓储插件`
3. `扩展 UI 贡献元数据`
4. `主界面支持动态业务 Tab`
5. `迁移首页总览为业务插件`
6. `迁移数据管理为业务插件`
7. `迁移融合分析为业务插件`
8. `迁移算法优化为业务插件`
9. `迁移统计可视化为业务插件`
10. `迁移系统管理为业务插件`
11. `抽取公共 UI 组件模块`

## 关键原则

- 主程序只负责装载和编排，不承载具体业务逻辑。
- 数据中心负责运行期共享数据，不负责数据库细节。
- 数据库仓储负责持久化，不直接控制界面。
- 业务插件负责业务页面和业务交互，不重复建设基础设施。
- 公共 UI 模块负责视觉和组件一致性，不绑定具体业务。
- 插件之间不直接互相依赖实现类，通过稳定接口和共享服务通信。
