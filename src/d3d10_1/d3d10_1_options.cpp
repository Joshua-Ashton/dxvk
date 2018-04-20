#include <unordered_map>

#include "d3d10_1_options.h"

namespace dxvk {
  
  const static std::unordered_map<std::string, D3D10OptionSet> g_d3d10AppOptions = {{
    { "Dishonored2.exe", D3D10OptionSet(D3D10Option::AllowMapFlagNoWait) },
  }};
  
  
  D3D10OptionSet D3D10GetAppOptions(const std::string& AppName) {
    auto appOptions = g_d3d10AppOptions.find(AppName);
    
    return appOptions != g_d3d10AppOptions.end()
      ? appOptions->second
      : D3D10OptionSet();
  }
  
}