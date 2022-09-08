#ifndef __LOTIMER_H__
#define __LOTIMER_H__

#include <SDL.h>

class LOTimer{
public:
	LOTimer();
	~LOTimer();
	static void Init();
	static Uint64 GetHighTimer();
	static Uint32 GetLowTimer();
	static double GetHighTimeDiff(Uint64 last);
	static int GetLowTimeDiff(Uint32 last);
	static void CpuDelay(double ms);
	//普通计时器
	static Uint32 startTik32;
	//高精度计时器
	static Uint64 startTik64;
	//高精度计时器每毫秒的计数
	static Uint64 perTik64;
private:

};




#endif // !__LOTIMER_H__
