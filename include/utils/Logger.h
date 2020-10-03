#pragma once

#include <cstring>
#include <iostream>

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define L_NONE 0
#define L_ERROR 1
#define L_INFO 2
#define L_DEBUG 3
#define L_TRACE 4

#ifndef L_LEVEL
#define L_LEVEL L_INFO
#endif

#define LOG_MSG(level, level_str, log_msg)                                                                             \
    {                                                                                                                  \
        if (L_LEVEL >= level)                                                                                          \
            std::cout << "[" << level_str << "] [" << __FILENAME__ << ":" << __LINE__ << "] " << log_msg << std::endl; \
    }
#define LOG_ERROR(x) LOG_MSG(L_ERROR, "Error", x)
#define LOG_INFO(x) LOG_MSG(L_INFO, "Info ", x)
#define LOG_DEBUG(x) LOG_MSG(L_DEBUG, "Debug", x)
#define LOG_TRACE(x) LOG_MSG(L_TRACE, "Trace", x)
