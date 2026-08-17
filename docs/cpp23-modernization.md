# C++23 Modernization Notes

审查基线：当前工作树使用 Android NDK `30.0.14904198`，其 host `clang++` 报告为 Clang 21.0.0；`tests/run_host_tests.py` 已改为自动选择最新 NDK `clang++`，不再把 g++ 作为兼容目标。Android release 仍由同一 NDK 的 arm64 Clang 构建。

## 已采用

| 特性 | 当前落点 | 结论 |
| --- | --- | --- |
| concepts / `requires` | `ConfigManager.hpp`、`Feature.hpp` | 用 `ConfigScalar` 和 `ConfigValidator` 把错误从函数体前移到接口约束；`ReadJsonValue`、`Read`、`ReadConfig` 应继续保持同一套约束。 |
| `std::remove_cvref_t` / `std::same_as` / `std::predicate` | 配置类型推导与 validator | 比 `is_same_v` 与函数体 `static_assert` 更直接，保留。 |
| `std::in_range` | 整数 JSON 读取 | 先验证再窄化，避免未定义行为，保留。 |
| `std::ranges::find_if` | `GameProfile.hpp` | 适合小型只读 profile 表，保留，不为此引入额外 registry。 |
| `std::string_view` / `std::span` | Feature 名称、Hook 注册批次、配置键 | 只读借用值使用 vocabulary type，生命周期由调用者持有。 |
| `std::to_underlying` | 适合枚举日志/索引转换 | 后续替换手写 `static_cast` 时优先使用；必须先保证枚举值已经经过验证。 |

## 推荐采用

### `ConfigManager` 的约束写法

```cpp
template <class T>
concept ConfigScalar =
    std::same_as<std::remove_cvref_t<T>, bool> ||
    std::integral<std::remove_cvref_t<T>> ||
    std::floating_point<std::remove_cvref_t<T>> ||
    std::same_as<std::remove_cvref_t<T>, std::string>;

template <class Validator, class T>
concept ConfigValidator =
    ConfigScalar<T> && std::predicate<Validator, const T &>;

template <ConfigScalar T, class Validator>
    requires ConfigValidator<Validator, T>
T ReadValidated(std::string_view section, std::string_view key,
                T default_value, Validator validator);
```

实际声明可继续写成 `requires config_detail::ConfigValidator<Validator, T>`，因为当前 concept 参数顺序是 `(Validator, T)`。不要为了缩短语法引入字段 registry、反射模拟或 `auto` 非静态成员；C++23 仍不允许从初始化器推导普通非静态数据成员类型，当前 `AH_CFG` 根据默认值 trait 推导存储类型已经是较小的接口。

### `std::expected`

配置加载、导入和动态符号解析都有“成功值 / 失败原因”形状。若错误需要被调用者区分，优先考虑 `std::expected<T, Error>` 或一个等价的项目状态类型；不要用异常，因为 Android release 明确关闭 exceptions。迁移时保持现有 bool 调用点不变，先在内部 seam 使用，再统一错误日志。

### `std::source_location`

它可以替代显式 `__FILE__` / `__LINE__` 参数，但本项目的日志要求是编译期只保留 basename，并且宏已经把 `BaseName(__FILE__)` 变成了无路径字符串。除非日志接口改为接收 `source_location` 并在调用点验证二进制开销，否则保留当前宏；这不是为了兼容旧标准，而是为了满足现有日志格式与零运行时路径裁剪。

## 有条件采用

| 特性 | 适用判断 |
| --- | --- |
| `std::format` / `std::print` | 当前 Android/host libc++ 组合和模块需要严格控制单条日志大小；现有 `vsnprintf` 经过 bounded sink 处理。只有确认 NDK libc++、二进制体积和截断行为后才迁移。 |
| `std::mdspan` | 游戏布局是少量固定偏移字段，不是矩阵算法；引入会增加接口知识，当前不值得。 |
| deducing `this` / explicit object parameter | 目前没有需要把 const/non-const 成员模板合并的深模块；不要为了展示语法改写 Feature。 |
| `std::ranges` 管道 | 导入器的解析步骤有错误状态和容量上限，显式循环更容易审查。只在不隐藏边界检查时使用。 |
| 模块、协程、`std::execution` | NDK build、Zygisk 装载和无 RTTI/no exceptions 约束下没有真实收益，本轮不采用。 |

## 编码规则

1. 先确认当前 NDK Clang++ 支持，再使用最新 C++23 写法；用 feature-test macro 或最小编译探针验证，不凭 host g++ 行为推断。
2. 优先 vocabulary types、concepts、`constexpr`/`consteval` 和标准算法；如果写法让边界、错误路径或运行时布局更难读，保留清晰的显式代码。
3. 不为单一调用点建立 registry、DI、反射或层层 wrapper。通过删除测试：删除候选模块后复杂度若只是平移到多个 caller，就说明它提供了真实 leverage；否则保持内联。
4. 运行时地址、文件、ZIP、JSON 和网络响应都必须先做范围/容量验证，再转换或调用；C++23 语法不能替代验证。

## 参考

- [cppreference: C++ language features](https://en.cppreference.com/w/cpp/23)
- [cppreference: C++ library features](https://en.cppreference.com/w/cpp/23#Library_features)
- [Android NDK documentation](https://developer.android.com/ndk)
