#pragma once

#include "d3d10_1_include.h"

namespace dxvk {
  
  enum class D3D10Option : uint64_t {
    /**
     * \brief Handle D3D10_MAP_FLAG_DO_NOT_WAIT properly
     * 
     * This can offer substantial speedups, but some games
     * (The Witcher 3, Elder Scrolls Online, possibly others)
     * seem to make incorrect assumptions about when a map
     * operation succeeds when that flag is set.
     */
    AllowMapFlagNoWait = 0,
  };
  
  using D3D10OptionSet = Flags<D3D10Option>;
  
  
  /**
   * \brief Retrieves per-app options
   * 
   * \param [in] AppName Executable name
   * \returns D3D10 options
   */
  D3D10OptionSet D3D10GetAppOptions(const std::string& AppName);
  
}