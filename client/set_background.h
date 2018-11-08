#ifndef SET_BACKGROUND_H
#define SET_BACKGROUND_H

// Various methods for setting background wallpaper across platforms
#ifdef _WIN32
#include "win32_background.h"
#elif defined(__APPLE__)
// TODO: OS X?
//#include "osx_background.h"
// TODO: Linux?
#endif

#endif

