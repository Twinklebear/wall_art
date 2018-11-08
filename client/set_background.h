#pragma once

// Various methods for setting background wallpaper across platforms
#ifdef _WIN32
#include "win32_background.h"
#elif defined(__APPLE__)
// TODO: OS X
//#include "osx_background.h"
#elif defined(__linux__)
#include "linux_background.h"
#else
#error "Unrecognized platform"
#endif

