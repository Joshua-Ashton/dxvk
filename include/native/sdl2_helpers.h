#include "windows_base.h"

inline int32_t monitor_cast(HMONITOR hMonitor) {
  return static_cast<int32_t>(reinterpret_cast<intptr_t>(hMonitor));
}

inline HMONITOR monitor_cast(int32_t displayId) {
  return reinterpret_cast<HMONITOR>(static_cast<intptr_t>(displayId));
}