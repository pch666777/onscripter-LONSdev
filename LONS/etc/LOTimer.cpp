#include "LOTimer.h"
#include <stdlib.h>

Uint32 LOTimer::startTik32;
//高精度计时器
Uint64 LOTimer::startTik64;
//高精度计时器每毫秒的计数
Uint64 LOTimer::perTik64;


LOTimer::LOTimer() {
}

LOTimer::~LOTimer() {
}


void LOTimer::Init() {
	startTik32 = SDL_GetTicks();
	startTik64 = SDL_GetPerformanceCounter();
	perTik64 = SDL_GetPerformanceFrequency() / 1000;
}


Uint64 LOTimer::GetHighTimer() {
	return SDL_GetPerformanceCounter();
}

Uint32 LOTimer::GetLowTimer() {
	return SDL_GetTicks();
}


double LOTimer::GetHighTimeDiff(Uint64 last) {
	Uint64 diff = SDL_GetPerformanceCounter() - last;
	return ((double)diff) / perTik64;
}


int LOTimer::GetLowTimeDiff(Uint32 last) {
	return (int)(SDL_GetTicks() - last);
}


void LOTimer::CpuDelay(double ms) {
	double postime = 0.0;
	Uint64 timesnap = GetHighTimer();
	while (postime < ms && ms > 0) {
		postime = GetHighTimeDiff(timesnap);

		//cpu delay litle
		int sum = rand();
		for (int ii = 0; ii < 200; ii++) {
			sum ^= ii;
			if (ii % 2) sum++;
			else sum += 2;
		}
	}
}