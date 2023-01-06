#pragma once
#include <stdio.h>

#if 1
#define LOG(fmt,...)   log_line((char *) "DEB",  __FILE__, __LINE__, (char *) fmt, ##__VA_ARGS__)
#define LOG_I(fmt,...)   log_line((char *) "INF",  __FILE__,  __LINE__, (char *) fmt, ##__VA_ARGS__)
#define LOG_W(fmt,...)   log_line((char *) "WRN",  __FILE__,  __LINE__, (char *) fmt, ##__VA_ARGS__)
#define LOG_E(fmt,...)   log_line((char *) "ERR",  __FILE__,  __LINE__, (char *) fmt, ##__VA_ARGS__)
#else
#define LOG printf
#define LOG_I printf
#define LOG_E printf
#define LOG_W printf
#endif


#if USE_APP_LOG
    #undef LV_LOG_USER
    #define LV_LOG_USER LOG
    #undef LV_LOG_ERROR
    #define LV_LOG_ERROR LOG_E
    #undef LV_LOG_WARN
    #define LV_LOG_WARN LOG_W
#endif

void log_line(char * what, const char * file, int line, char * fmt, ...);
