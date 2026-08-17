#pragma once

#include "features/Feature.hpp"
#include "utils/Log.h"

namespace arc_helper {

class Logging final : public Feature {
public:
    static Logging &Instance();

private:
    Logging();

    LogSinkConfig logcat_{};
    LogSinkConfig file_{};
};

} // namespace arc_helper
