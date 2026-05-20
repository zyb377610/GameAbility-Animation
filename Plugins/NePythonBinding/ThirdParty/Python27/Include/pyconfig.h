#ifndef _PYCONFIG_H
#define _PYCONFIG_H

#ifdef _WIN32
#include "pyconfig_win.h"
#endif

#if defined(__APPLE__) || defined(__darwin__)
#include "TargetConditionals.h"
#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
#include "pyconfig_ios.h"
#else
#include "pyconfig_mac.h"
#endif
#elif defined(ANDROID)
#include "pyconfig_android.h"
#elif defined(__unix)
#include "pyconfig_debian.h"
#endif

#endif
