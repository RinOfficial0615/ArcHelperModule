# 版本支持

Language: [English](version-support.md) | 简体中文

## 当前支持版本

- `6.12.11c`
- `6.13.2f`
- `6.14.0c`
- `6.16.2c`（完整 profile；自定义谱面）

## 运行时识别

1. Wrapper 等待 `libcocos2dcpp.so` 完成映射。
2. `GameVersionManager` 先读取各 profile 的 `appVersion` 字符串全局变量。
3. 如果全局变量尚未初始化，则通过 `dlsym` 动态解析并 Hook 导出的 `Java_low_moe_AppActivity_setAppVersion` 符号。
4. Hook 收到真实版本字符串后选择匹配的 profile；确认完成后才安装功能 Hook。

不支持的版本会停在"已检测"状态，不会误用错误偏移。

## 新增版本

1. 在 `src/game/GameProfile.hpp` 中新增 `GameVersionId` 枚举值。
2. 只需填入版本字符串全局变量偏移；`setAppVersion` 通过 ELF 导出符号解析，不再需要每版本偏移。
3. 填入该版本的 autoplay / network / ssl_pins / custom_charts 偏移与 capability。
4. 若对象布局发生变化，在 `src/game/GameStructs.hpp` 特化对应模板。
5. 更新本文档。

## 偏移组织

- **对象布局** — `src/game/GameStructs.hpp`（版本模板化，`offsetof` + `static_assert` 编译期校验）。
- **共享常量和字节签名** — `src/config/AutoplayConfig.h`、`src/config/NetworkBlockConfig.h` 和 `src/config/CustomChartConfig.h`。
- **函数 / RTTI / patch 偏移** — `src/game/GameProfile.hpp`（每个支持版本一条记录）。
- **文档化位点** — `docs/offsets/6.12.11c-offsets.md`、`docs/offsets/6.16.2c-offsets.md`。
