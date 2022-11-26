/*
//简单的日志系统
//pngchs@qq.com
//2021-2021
*/

#ifndef ONS_VERSION
#define ONS_VERSION "ver0.5-20221120"
#endif // !ONS_VERSION


extern void LOLog_i(const char *fmt, ...);
extern void LOLog_e(const char *fmt, ...);
extern void FatalError(const char *fmt, ...);