/*
//简单的日志系统
//pngchs@qq.com
//2021-2021
*/

#include "LOLog.h"
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#ifdef ANDROID
#include <android/log.h>
#include <unistd.h>
#endif


#define CACHE_SIZE 1024
char myLogCache[CACHE_SIZE];

//normal infomation
void LOLog_i(const char *fmt, ...) {
	int len = 0;
	va_list argptr;
	va_start(argptr, fmt);
	memset(myLogCache, 0, CACHE_SIZE);
	len = vsnprintf(myLogCache, CACHE_SIZE - 2, fmt, argptr);
	va_end(argptr);
#ifdef ANDROID
	__android_log_print(ANDROID_LOG_INFO, "LONS", "%s", myLogCache);
#else
	while (myLogCache[len] == 0 && len > 0) len--;
	//check end is '\n'
	if (myLogCache[len] != '\n') myLogCache[len + 1] = '\n';
	printf("%s", myLogCache);
#endif // ANDROID
}


//error information
void LOLog_e(const char *fmt, ...) {
	int len = 0;
	va_list argptr;
	va_start(argptr, fmt);
	memset(myLogCache, 0, CACHE_SIZE);
	len = vsnprintf(myLogCache, CACHE_SIZE - 2, fmt, argptr);
	va_end(argptr);
#ifdef ANDROID
	__android_log_print(ANDROID_LOG_ERROR, "LONS", "%s", myLogCache);
#else
	while (myLogCache[len] == 0 && len > 0) len--;
	//check end is '\n'
	if (myLogCache[len] != '\n') myLogCache[len + 1] = '\n';
	printf("%s", myLogCache);
#endif // ANDROID
}