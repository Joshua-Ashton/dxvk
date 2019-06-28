#pragma once

#include <string>
#include <iostream>
#include <filesystem>

#include "../d3d11/d3d11_texture.h"

#include "util_string.h"

NTSYSCALLAPI LONG NTAPI NtCompareObjects(HANDLE FirstObjectHandle, HANDLE SecondObjectHandle);

namespace dxvk {

  inline static const char* baseDirectory = "DXVK_SharedResources";

  inline HMODULE GetKernelBase() {
    static HMODULE kernelBase = LoadLibraryA("kernelbase.dll");
    return kernelBase;
  }

  using CompareObjectHandlesType = BOOL (STDMETHODCALLTYPE *) (HANDLE hFirstObjectHandle, HANDLE hSecondObjectHandle);

  inline BOOL DoCompareObjectHandles(HANDLE hFirstObjectHandle, HANDLE hSecondObjectHandle) {
    static CompareObjectHandlesType proc =
      (CompareObjectHandlesType)GetProcAddress(GetKernelBase(), "CompareObjectHandles");

    return proc(hFirstObjectHandle, hSecondObjectHandle);
  }

  inline std::string DetermineSharedResourceFileName(BOOL bNTHandle, DWORD hProcess, HANDLE hHandle) {
    // hProcess is null for KMT handles because we don't actually need a process for that.
    // This keeps 1 KMT handle : 1 file
    hProcess = bNTHandle ? hProcess : 0;
    return str::format(baseDirectory, "\\", hProcess, ".", (uint64_t)hHandle);
  }

  namespace fs = std::filesystem;

  inline bool OpenSharedResourceInfo(BOOL bWrite, BOOL bNTHandle, HANDLE hHandle, D3D11_COMMON_TEXTURE_DESC* pDesc) {
    static bool deleted = false;

    if (!deleted) {
      fs::remove_all(baseDirectory);
      deleted = true;
    }

    fs::create_directories(baseDirectory);

    char* pData = reinterpret_cast<char*>(pDesc);

    if (bWrite) {
      std::string name = DetermineSharedResourceFileName(bNTHandle, GetCurrentProcessId(), hHandle);

      std::ofstream stream(name, std::ios_base::trunc);

      if (stream.bad()) {
        Logger::warn(str::format("ReadSharedResource: Failed to write file ", name));
        return false;
      }

      stream.write(pData, sizeof(D3D11_COMMON_TEXTURE_DESC)).flush();
      return true;
    }
    else {
      if (bNTHandle) {
        // NT Handles are annoying as they can have duplicates.
        // And those can cross processes.

        // Lets look at all our available handles and check them...
        for (auto& p : fs::directory_iterator(baseDirectory)) {
          std::string name = p.path().filename().string();
          auto iter = name.find('.');
          if (iter == std::string::npos || name.length() <= iter)
            continue;

          // Separate parts of the file name out for us to use.
          std::string processStr    = name.substr(0, iter);
          std::string handlePtrStr  = name.substr(iter + 1);

          DWORD process        = std::strtoul(processStr.c_str(), nullptr, 0);
          bool NTHandle        = process != 0;
          uint64_t theirHandle = std::strtoull(handlePtrStr.c_str(), nullptr, 0);

          if (!NTHandle)
            continue;

          HANDLE possibleDuplicate = nullptr;
          // Duplicate this possible handle into our process space.
          HANDLE theirProcess = OpenProcess(PROCESS_ALL_ACCESS, false, process);
          HANDLE ourProcess   = OpenProcess(PROCESS_ALL_ACCESS, false, GetCurrentProcessId());

          BOOL success = DuplicateHandle(theirProcess, (HANDLE)theirHandle, ourProcess, &possibleDuplicate, 0, false, DUPLICATE_SAME_ACCESS);
          DWORD error = GetLastError();
          if (success) {
            // Did that work? Let's see if it is the same as the handle passed to us.
            // Which is going to be in our table.
            if (DoCompareObjectHandles(possibleDuplicate, hHandle)) {
              // Close the handle as we no longer need it.
              CloseHandle(possibleDuplicate);

              // Read the desc!
              std::ifstream stream(p.path());

              if (stream.bad()) {
                // Something went wrong, lets continue on.
                Logger::warn(str::format("ReadSharedResource: Failed to read file ", p.path()));
                continue;
              }

              stream.read(pData, sizeof(D3D11_COMMON_TEXTURE_DESC));
              return true;
            }

            // It's not the same, we still have to close it, let's continue on and try another one...
            CloseHandle(possibleDuplicate);
          }
        }
      }
      else {
        std::string name = DetermineSharedResourceFileName(bNTHandle, GetCurrentProcessId(), hHandle);

        // Non NT handles are so much simpler...
        std::ifstream stream(name);
        
        if (stream.bad()) {
          Logger::warn(str::format("ReadSharedResource: Failed to read file ", name));
          return false;
        }

        stream.read(pData, sizeof(D3D11_COMMON_TEXTURE_DESC));
        return true;
      }

      return false;
    }
  }

}