#pragma once

#include "../util/config/config.h"

namespace dxvk {

  struct DxvkOptions {
    DxvkOptions() { }
    DxvkOptions(const Config& config);

    /// Enable state cache
    bool enableStateCache;

    /// Number of compiler threads
    /// when using the state cache
    int32_t numCompilerThreads;

    /// Shader-related options
    Tristate useRawSsbo;
    Tristate useEarlyDiscard;

    /// Enable low memory checks.
    bool enableLowMemoryChecks;
    /// The amount of memory in MB to be considered a 'low memory' device
    int32_t lowMemoryThreshold;
  };

}