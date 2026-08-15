# 项目结构

Language: [English](project-structure.md) | 简体中文

## 源码树

- `src/wrapper/ZygiskEntryWrapper.cpp` — Zygisk 入口，Hook `Runtime.nativeLoad`
- `src/wrapper/JniEntryWrapper.cpp` — JNI 入口 (`JNI_OnLoad`)，用于 `libcocos2dcpp.so` 之后加载
- `src/wrapper/WrapperCommon.hpp` — 公共初始化逻辑、包名校验、功能启动
- `src/manager/GameManager.{hpp,cpp}` — 缓存 `libcocos2dcpp.so` 基址
- `src/manager/GameVersionManager.{hpp,cpp}` — 探测游戏版本，激活对应 profile
- `src/manager/HookManager.{hpp,cpp}` — inline hook 辅助（`InstallInlineHook`、`CALL_ORIG`、恢复）
- `src/manager/NetworkManager.{hpp,cpp}` — 网络 Hook、handler 分发
- `src/manager/network/NetworkHandler.hpp` — handler API（`HandlerArgs`、body 视图、阶段）
- `src/features/Feature.hpp` — 功能基类，库基址缓存
- `src/features/Autoplay.{hpp,cpp}` — 自动打歌 Hook、合成触摸、字节补丁
- `src/features/NetworkLogger.{hpp,cpp}` — 高优先级请求/响应审计
- `src/features/NetworkBlock.{hpp,cpp}` — 低优先级 URL 拦截策略
- `src/features/SslPinningBypass.{hpp,cpp}` — SSL 证书绑定移除（两处字节补丁）
- `src/features/CustomChartManager.{hpp,cpp}` — 启动时导入 `.arcpkg`/raw ZIP、合并 songlist、维护缓存与报告
- `src/features/AssetVirtualizer.{hpp,cpp}` — 虚拟 songlist/自定义资源及官方默认资源重定向
- `src/features/CustomSession.{hpp,cpp}` — 自定义谱面会话状态，用于强制全网络隔离
- `src/GameTypes.hpp` — `Gameplay`、`LogicArcNote`、`LogicHoldNote` 等轻量封装
- `src/config/RuntimeConfig.{hpp,cpp}` — 启动时读取固定目录中的运行时配置
- `src/utils/MemoryUtils.hpp` — 内存工具汇总头
- `src/utils/memory/*.hpp|*.cpp` — `ProcMaps`、`AddressResolver`、`Patcher`、`InlineHook`
- `src/utils/Log.h` — `ARC_LOGI` / `ARC_LOGE` 宏
- `third_party/json/` — 用于 JSON 解析和序列化的 nlohmann/json 子模块
- `src/utils/Sha256.{hpp,cpp}` — 包内容哈希与稳定 ID 支持
- `src/utils/ZipArchive.{hpp,cpp}` — 带路径、大小、CRC 与压缩率检查的 ZIP 读取器
- `src/config/GameStructs.hpp` — 按版本模板化的布局 struct（显式 padding，`offsetof` 校验）
- `src/config/GameProfile.hpp` — 各版本的函数/RTTI/patch 偏移
- `src/config/AutoplayConfig.h` — 自动打歌的行为常量和字节签名
- `src/config/NetworkBlockConfig.h` — 网络策略、拦截规则、字节签名
- `src/config/ModuleConfig.h` — 功能开关、目标包名
- `module/` — Magisk/Zygisk 打包元数据、作用域和配置示例
- `third_party/libcxx/` — Android libc++ 子模块
- `third_party/lsplt/` — LSPlt 子模块
- `third_party/zygisk.hpp` — Zygisk API 头文件

## 初始化流程

1. Wrapper 检测到 `libcocos2dcpp.so` 就绪 → 缓存基址至 `GameManager`。
2. Zygisk 路径校验包名；JNI 路径跳过包名校验。
3. `GameVersionManager` 判断候选版本，等待真实 `appVersion` 确认。
4. 版本确认后，wrapper 回调实例化功能单例并安装网络隔离 Hook。
5. 仅当 profile 声明自定义谱面能力且网络隔离可用时，扫描固定目录、导入包并安装资源虚拟化。
6. 各功能按启动时配置和当前 profile 通过 `HookManager` 安装 Hook；配置在本次进程内不再热加载。
