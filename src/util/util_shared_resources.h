#pragma once

#include <string>
#include <iostream>

#include "../d3d11/d3d11_texture.h"

#include "util_string.h"

namespace dxvk {

  inline static const char* baseDirectory = "DXVK_SharedResources";

  inline std::string DetermineSharedResourceFileName(BOOL bNTHandle, HANDLE hHandle) {
    const char* prefix = bNTHandle ? "NT" : "KMT";
    return str::format(baseDirectory, "\\", prefix, "_HandleId_", hHandle);
  }

  // This will fall apart if they use NT handles (ie. D3D11.1's OpenSharedResource1/CreateSharedHandle) and they duplicate the handles...
  // I do not have a solution for this at the moment.
  inline bool OpenSharedResourceInfo(BOOL bWrite, BOOL bNTHandle, HANDLE hHandle, D3D11_COMMON_TEXTURE_DESC* pDesc) {
    DWORD mapFlags = bWrite ? GENERIC_WRITE : GENERIC_READ;

    CreateDirectoryA(baseDirectory, nullptr);

    std::string name = DetermineSharedResourceFileName(bNTHandle, hHandle);
    HANDLE file = CreateFileA(name.c_str(), mapFlags, 0, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

    if (file == INVALID_HANDLE_VALUE) {
      Logger::warn(str::format("ReadSharedResource: Failed to open file ", name));
      return false;
    }

    DWORD byteIOCount = 0;
    
    if (bWrite) {
      if (!WriteFile(file, pDesc, sizeof(D3D11_COMMON_TEXTURE_DESC), &byteIOCount, nullptr)) {
        Logger::warn(str::format("ReadSharedResource: Failed to write file ", name));
        CloseHandle(file);
        return false;
      }
    } else{
      if (!ReadFile (file, pDesc, sizeof(D3D11_COMMON_TEXTURE_DESC), &byteIOCount, nullptr)) {
        Logger::warn(str::format("ReadSharedResource: Failed to read file ",  name));
        CloseHandle(file);
        return false;
      }
    }

    CloseHandle(file);

    return true;
  }

}