#ifndef UTILS_COMPAT_H
#define UTILS_COMPAT_H

#ifdef __unix
#include <unistd.h>
#endif
#ifdef _WIN32
#include <windows.h>
#endif

#ifdef __unix
    #define SLEEP(x) usleep((x) * 1000);
#endif
#ifdef _WIN32
    #define SLEEP(x) Sleep((x));
#endif

#endif
