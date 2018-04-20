#include "d3d10_1_enums.h"

std::ostream& operator << (std::ostream& os, D3D10_FEATURE_LEVEL1 e) {
  switch (e) {
    ENUM_NAME(D3D10_FEATURE_LEVEL_9_1);
    ENUM_NAME(D3D10_FEATURE_LEVEL_9_2);
    ENUM_NAME(D3D10_FEATURE_LEVEL_9_3);
    ENUM_NAME(D3D10_FEATURE_LEVEL_10_0);
    ENUM_NAME(D3D10_FEATURE_LEVEL_10_1);
    ENUM_DEFAULT(e);
  }
}
