# 项目结构

Language: [English](project-structure.md) | 简体中文

## 源码树

- `src/wrapper/ZygiskEntryWrapper.cpp` — Zygisk 入口，Hook `Runtime.nativeLoad`；经 `src/config/ScopeConfig.hpp` 做包名作用域校验
- `src/wrapper/JniEntryWrapper.cpp` — JNI 入口 (`JNI_OnLoad`)，用于 `libcocos2dcpp.so` 之后加载
- `src/wrapper/WrapperCommon.hpp` — 公共初始化引导（根目录发现、配置读取、功能创建）
- `src/manager/GameManager.{hpp,cpp}` — 缓存 `libcocos2dcpp.so` 基址
- `src/manager/GameVersionManager.{hpp,cpp}` — 探测游戏版本，激活对应 profile
- `src/manager/HookManager.{hpp,cpp}` — 事务式 inline hook 辅助（`RegisterInlineHook`、`CommitInlineHook`、`CALL_ORIG`）
- `src/manager/ConfigManager.{hpp,cpp}` — 无硬编码 schema 的 JSON 读取、类型化字段访问与校验、规范化与原子保存
- `src/manager/FeatureManager.{hpp,cpp}` — 显式的 Feature 创建与安装顺序
- `src/manager/NetworkManager.{hpp,cpp}` — 网络 Hook、handler 分发
- `src/manager/network/NetworkHandler.hpp` 与 `NetworkHandlerSnapshot.{hpp,cpp}` — handler 阶段、不可变顺序与 bounded view
- `src/features/Feature.hpp` — 功能名与类型化配置辅助（`AH_CFG`，小节作用域的 `AH_CFG_SECTION`/`AH_CFG_SECTION_OPT`）
- `src/features/Logging.{hpp,cpp}` — 嵌套的 logcat/文件日志配置
- src/features/Autoplay.{hpp,cpp} — 自动打歌 Hook、合成触摸、字节补丁
- src/features/CxaThrowTracer.{hpp,cpp} — 常开的 `__cxa_throw` 诊断（异常类型、调用点、短回溯）
- `src/features/NetworkLogger.{hpp,cpp}` — 高优先级请求/响应审计
- `src/features/NetworkBlock.{hpp,cpp}` — 低优先级 URL 拦截策略
- `src/features/SslPinningBypass.{hpp,cpp}` — SSL 证书绑定移除（两处字节补丁）
- `src/manager/CustomChartManager.{hpp,cpp}` — 发布并读取不可变自定义谱面快照
- `src/manager/custom_chart/CustomChartImporter.{hpp,cpp}` — `.arcpkg`/raw ZIP 解析与有界缓存抽取
- `src/manager/custom_chart/AffNormalizer.{hpp,cpp}` — 把 ArcCreate AFF 改写成官方 6.16.2c token
- src/manager/custom_chart/AffOfficialParser.{hpp,cpp} — 宿主机上的官方 TokenLexer/parseNote 校验（不链进模块）
- src/manager/custom_chart/ArcPackageFormat.{hpp,cpp} — .arcpkg index.yml/songlist.yml 有界读取器（rapidyaml）
- `src/manager/custom_chart/CustomChartAssetIndex.{hpp,cpp}` — 逻辑资源路径、APK 时代别名和目录查询
- `src/manager/custom_chart/CustomChartSnapshot.{hpp,cpp}` — 不可变歌曲模型与纯 songlist 合并
- `src/manager/custom_chart/CustomChartReportWriter.{hpp,cpp}` — manifest/report 写入和孤儿缓存提交门
- `src/manager/custom_chart/CustomChartGameplaySession.{hpp,cpp}` — 自定义谱面游玩窗口的原子状态；map `.aff` 开始，读到 `kJacketAssetName` 结束
- `src/features/CustomCharts.{hpp,cpp}` — 自定义谱面功能生命周期与 AssetVirtualizer 安装
- `src/features/AssetVirtualizer.{hpp,cpp}` — 虚拟 songlist/自定义资源及官方默认资源重定向
- `src/game/GameTypes.hpp` — `Gameplay`、`LogicArcNote`、`LogicHoldNote` 等轻量封装
- `src/utils/MemoryUtils.hpp` — 内存工具汇总头
- `src/utils/memory/*.hpp|*.cpp` — `ProcMaps`、`AddressResolver`、`RuntimeMemory`、`PatchTransaction`、`InlineHook`、`ShadowHookAdapter`、`ExecUtils`
- src/utils/Log.{h,cpp} — 带来源的 `ARC_LOGD/I/W/E`、logcat/文件 sink、截断与轮换
- src/utils/ImageRaster.{hpp,cpp} — 有界背景图解码/裁剪/缩放为官方 1920x1440 JPEG（stb）
- `third_party/json/` — 用于 JSON 解析和序列化的 nlohmann/json 子模块
- `src/utils/Sha256.{hpp,cpp}` — 包内容哈希与稳定 ID 支持
- `src/utils/ZipArchive.{hpp,cpp}` — 带路径、大小、CRC 与压缩率检查的 ZIP 读取器
- `src/game/GameStructs.hpp` — 按版本模板化的布局 struct（显式 padding，`offsetof` 校验）
- `src/game/GameProfile.hpp` — 各版本的函数/RTTI/patch 偏移
- `src/config/AutoplayConfig.h` — 自动打歌的行为常量和字节签名
- `src/config/NetworkBlockConfig.h` — 网络策略、拦截规则、字节签名
- `src/config/CustomChartConfig.h` — 资源别名、解析边界、布局保护与 Hook 签名
- src/config/ModuleConfig.h — 模块身份与目标库名
- src/config/ScopeConfig.hpp — 默认作用域包列表与包名匹配
- `module/` — Magisk/Zygisk 打包元数据、作用域和配置示例
- `third_party/libcxx/` — Android libc++ 子模块
- `third_party/lsplt/` — LSPlt 子模块
- `third_party/magic_enum/` — 编译期枚举反射子模块
- `third_party/rapidyaml/` — `.arcpkg` 索引用的 YAML 解析器
- `third_party/stb/` — stb_image / write / resize2，把自定义背景收成官方尺寸
- `third_party/shadowhook/` — ShadowHook v2.0.1 子模块；构建时应用 `patches/lsplt-live-plt.patch`
- `module/shadowhook_nothing.c` — ShadowHook linker 扫描所需的独立 helper ELF
- `third_party/zygisk.hpp` — Zygisk API 头文件

## 初始化流程

1. Wrapper 确定 ArcHelper 根目录，`ConfigManager` 读取原始 JSON。
2. `FeatureManager::CreateAll()` 先创建 `Logging`，再创建全部 Feature（包括关闭的功能），随后原子保存规范化 JSON。
3. `GameVersionManager` 读取版本全局，必要时动态 Hook 导出的 `setAppVersion` 符号，等待真实版本字符串确认。
4. `FeatureManager::InstallAll(profile)` 按固定顺序安装功能；构造函数不安装 Hook。
5. 固定网络隔离已安装且 profile 支持时，CustomCharts 才导入谱面并安装资源虚拟化。
