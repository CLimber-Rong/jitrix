#pragma once

#define JITRIX_JIT_ENABLED 1

#define VERSION_X 0
#define VERSION_Y 0
#define VERSION_Z 1

#if defined(__clang__) || defined(__GNUC__)
  #define JITRIX_JIT_MUSTTAIL 1
#else
  #define JITRIX_JIT_MUSTTAIL 0
#endif
