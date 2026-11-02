#ifndef CLINTER_PLATFORM_COMPAT_H
#define CLINTER_PLATFORM_COMPAT_H
#include <stdio.h>
#if !defined(_WIN32)
#include <strings.h>
#define _stricmp strcasecmp
#define _snprintf snprintf
#endif
#endif
