
#ifndef __UTILS_H__
#define __UTILS_H__

#ifdef WIN32
#define NOMINMAX
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_WARNINGS
#include <SDKDDKVer.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <string>
#include <iostream>
#include <algorithm>
#endif

#include "windefsws.h"

#endif //__UTILS_H__

