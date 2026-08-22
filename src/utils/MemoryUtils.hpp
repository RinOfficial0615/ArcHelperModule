#pragma once

// Compatibility umbrella for memory utilities.
//
// New code can include specific headers under `utils/memory/*` directly.

#include "utils/memory/AddressResolver.hpp"
#include "utils/memory/ExecUtils.hpp"
#include "utils/memory/InlineHook.hpp"
#include "utils/memory/MemoryError.hpp"
#include "utils/memory/MemoryPrimitives.hpp"
#include "utils/memory/PatchTransaction.hpp"
#include "utils/memory/ProcMaps.hpp"
#include "utils/memory/RuntimeMemory.hpp"
#include "utils/memory/ShadowHookAdapter.hpp"
