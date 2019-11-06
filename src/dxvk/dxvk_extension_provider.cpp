#include "dxvk_extension_provider.h"

#include "dxvk_platform_exts.h"

#ifndef DXVK_NATIVE
#include "dxvk_openvr.h"
#endif

namespace dxvk {

  DxvkExtensionProviderList DxvkExtensionProvider::s_extensionProviders = {
    &g_platformInstance,

#ifndef DXVK_NATIVE
    &g_vrInstance
#endif
  };

  const DxvkExtensionProviderList& DxvkExtensionProvider::getExtensionProviders() {
    return s_extensionProviders;
  }

}