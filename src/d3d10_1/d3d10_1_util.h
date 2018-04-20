#pragma once

#include "../dxvk/dxvk_device.h"

#include "../dxbc/dxbc_util.h"

#include "d3d10_1_include.h"

namespace dxvk {
  
  HRESULT DecodeSampleCount(
          UINT                      Count,
          VkSampleCountFlagBits*    pCount);
    
  VkSamplerAddressMode DecodeAddressMode(
          D3D10_TEXTURE_ADDRESS_MODE  mode);
  
  VkBorderColor DecodeBorderColor(
    const FLOAT                     BorderColor[4]);
  
  VkCompareOp DecodeCompareOp(
          D3D10_COMPARISON_FUNC     Mode);
  
  VkMemoryPropertyFlags GetMemoryFlagsForUsage(
          D3D10_USAGE               Usage);
  
}