﻿/*
//简单的日志系统
//pngchs@qq.com
//2021-2023
*/

#ifndef ONS_VERSION
#define ONS_VERSION "ver0.7.8-20231111"
#endif // !ONS_VERSION

//#define LOLog_i SDL_Log()
//#define LOLog_e SDL_LogError()

//extern void LOLog_i(const char *fmt, ...);
//extern void LOLog_e(const char *fmt, ...);
extern void FatalError(const char *fmt, ...);
extern void SimpleWarning(const char *fmt, ...);
