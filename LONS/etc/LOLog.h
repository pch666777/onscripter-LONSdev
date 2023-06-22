/*
//简单的日志系统
//pngchs@qq.com
//2021-2021
*/

#ifndef ONS_VERSION
#define ONS_VERSION "ver0.7.5-20230622"
#endif // !ONS_VERSION

//#define LOLog_i SDL_Log()
//#define LOLog_e SDL_LogError()

//extern void LOLog_i(const char *fmt, ...);
//extern void LOLog_e(const char *fmt, ...);
extern void FatalError(const char *fmt, ...);
extern void SimpleWarning(const char *fmt, ...);
