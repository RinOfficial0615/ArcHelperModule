#pragma once

#include <concepts>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "manager/ConfigManager.hpp"

namespace arc_helper {

class Feature {
public:
    virtual ~Feature() = default;

    std::string_view Name() const { return name_; }

protected:
    explicit Feature(std::string_view name) : name_(name) {}

    template <config_detail::ConfigScalar T>
    T ReadConfig(std::string_view key, T default_value) {
        return ConfigManager::Instance().Read(name_, key, std::move(default_value));
    }

    template <typename T, typename Validator>
        requires config_detail::ConfigScalar<T> &&
                 config_detail::ConfigValidator<Validator, T>
    T ReadConfig(std::string_view key, T default_value, Validator validator) {
        return ConfigManager::Instance().Read(
            name_, key, std::move(default_value), std::move(validator));
    }

    template <config_detail::ConfigScalar T>
    T ReadConfig(std::string_view key, T default_value, T minimum, T maximum) {
        return ConfigManager::Instance().Read(
            name_, key, std::move(default_value), std::move(minimum), std::move(maximum));
    }

    template <config_detail::ConfigScalar T>
    T ReadConfigSection(std::string_view subsection, std::string_view key,
                        T default_value) {
        return ConfigManager::Instance().Read(
            name_, subsection, key, std::move(default_value));
    }

    template <typename T, typename Validator>
        requires config_detail::ConfigScalar<T> &&
                 config_detail::ConfigValidator<Validator, T>
    T ReadConfigSection(std::string_view subsection, std::string_view key,
                        T default_value, Validator validator) {
        return ConfigManager::Instance().Read(
            name_, subsection, key, std::move(default_value), std::move(validator));
    }

    template <config_detail::ConfigScalar T>
    T ReadConfigSection(std::string_view subsection, std::string_view key,
                        T default_value, T minimum, T maximum) {
        return ConfigManager::Instance().Read(
            name_, subsection, key, std::move(default_value),
            std::move(minimum), std::move(maximum));
    }

    template <config_detail::ConfigScalar T>
    std::optional<T> ReadConfigOptional(std::string_view subsection,
                                        std::string_view key,
                                        T minimum, T maximum) {
        return ConfigManager::Instance().TryRead<T>(
            name_, subsection, key, std::move(minimum), std::move(maximum));
    }

    template <config_detail::ConfigScalar T, typename Validator>
        requires config_detail::ConfigValidator<Validator, T>
    std::optional<T> ReadConfigOptional(std::string_view subsection,
                                        std::string_view key,
                                        Validator validator) {
        return ConfigManager::Instance().TryRead<T>(
            name_, subsection, key, std::move(validator));
    }

private:
    std::string_view name_;
};

} // namespace arc_helper

#define AH_DETAIL_CFG_SELECT(_1, _2, _3, _4, NAME, ...) NAME
#define AH_DETAIL_CFG_2(name, default_value) \
    arc_helper::config_detail::StorageTypeT<decltype(default_value)> name##_ = \
        ReadConfig(#name, arc_helper::config_detail::NormalizeDefault(default_value))
#define AH_DETAIL_CFG_3(name, default_value, validator) \
    arc_helper::config_detail::StorageTypeT<decltype(default_value)> name##_ = \
        ReadConfig(#name, arc_helper::config_detail::NormalizeDefault(default_value), validator)
#define AH_DETAIL_CFG_4(name, default_value, minimum, maximum) \
    arc_helper::config_detail::StorageTypeT<decltype(default_value)> name##_ = \
        ReadConfig(#name, arc_helper::config_detail::NormalizeDefault(default_value), \
                   arc_helper::config_detail::NormalizeDefault(minimum), \
                   arc_helper::config_detail::NormalizeDefault(maximum))
#define AH_CFG(...) \
    AH_DETAIL_CFG_SELECT(__VA_ARGS__, AH_DETAIL_CFG_4, AH_DETAIL_CFG_3, AH_DETAIL_CFG_2)(__VA_ARGS__)

#define AH_DETAIL_CFG_SECTION_SELECT(_1, _2, _3, _4, _5, NAME, ...) NAME
#define AH_DETAIL_CFG_SECTION_3(subsection, name, default_value) \
    arc_helper::config_detail::StorageTypeT<decltype(default_value)> \
        subsection##_##name##_ = \
            ReadConfigSection(#subsection, #name, \
                              arc_helper::config_detail::NormalizeDefault(default_value))
#define AH_DETAIL_CFG_SECTION_4(subsection, name, default_value, validator) \
    arc_helper::config_detail::StorageTypeT<decltype(default_value)> \
        subsection##_##name##_ = \
            ReadConfigSection(#subsection, #name, \
                              arc_helper::config_detail::NormalizeDefault(default_value), \
                              validator)
#define AH_DETAIL_CFG_SECTION_5(subsection, name, default_value, minimum, maximum) \
    arc_helper::config_detail::StorageTypeT<decltype(default_value)> \
        subsection##_##name##_ = \
            ReadConfigSection(#subsection, #name, \
                              arc_helper::config_detail::NormalizeDefault(default_value), \
                              arc_helper::config_detail::NormalizeDefault(minimum), \
                              arc_helper::config_detail::NormalizeDefault(maximum))
#define AH_CFG_SECTION(subsection, ...) \
    AH_DETAIL_CFG_SECTION_SELECT(subsection, __VA_ARGS__, AH_DETAIL_CFG_SECTION_5, \
                                 AH_DETAIL_CFG_SECTION_4, \
                                 AH_DETAIL_CFG_SECTION_3)(subsection, __VA_ARGS__)

#define AH_DETAIL_CFG_OPT_SELECT(_1, _2, _3, _4, _5, NAME, ...) NAME
#define AH_DETAIL_CFG_SECTION_OPT_4(subsection, name, type, validator) \
    std::optional<type> subsection##_##name##_ = \
        ReadConfigOptional<type>(#subsection, #name, validator)
#define AH_DETAIL_CFG_SECTION_OPT_5(subsection, name, type, minimum, maximum) \
    std::optional<type> subsection##_##name##_ = \
        ReadConfigOptional<type>(#subsection, #name, minimum, maximum)
#define AH_CFG_SECTION_OPT(subsection, ...) \
    AH_DETAIL_CFG_OPT_SELECT(subsection, __VA_ARGS__, AH_DETAIL_CFG_SECTION_OPT_5, \
                             AH_DETAIL_CFG_SECTION_OPT_4)(subsection, __VA_ARGS__)
