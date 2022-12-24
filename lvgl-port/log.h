#pragma once
#include <stdio.h>

#define LOG printf
#define LOG_E printf
#define LOG_W printf

#if USE_APP_LOG
    #undef LV_LOG_USER
    #define LV_LOG_USER LOG
    #undef LV_LOG_ERROR
    #define LV_LOG_ERROR LOG_E
    #undef LV_LOG_WARN
    #define LV_LOG_WARN LOG_W
#endif