#include "util_string.h"

#include <string>
#include <algorithm>

namespace dxvk::str {
  std::string fromws(const WCHAR *ws) {
    size_t count = wcslen(ws);

    return std::string(ws, ws + count);
  }


  std::vector<WCHAR> tows(const std::string& str) {
    auto wstr = std::wstring(str.begin(), str.end());

    auto vec = std::vector<WCHAR>(wstr.length());
    std::copy(vec.begin(), vec.end(), wstr.data());

    return vec;
  }

}
