//事件处理

#include "LOEvent1.h"
#include <stdint.h>
#include <atomic>

//LOEventManager G_hookManager;


//==================== LOEvent1 ===================
std::atomic_int LOEventHook::exitFlag{};


LOEventHook::LOEventHook() {
	callMod = MOD_SCRIPTER;
	callFun = FUN_ENMPTY;
}


/*
void LOEvent1::SetExitFlag(int flag) {
	exitFlag.store(flag);
}
*/

LOEventHook::~LOEventHook() {
	for (int ii = 0; ii < paramList.size(); ii++) delete paramList[ii];
}

bool LOEventHook::isFinish() {
	return state.load() == STATE_FINISH;
};

bool LOEventHook::isInvalid() {
	return state.load() == STATE_INVALID;
}


bool LOEventHook::isActive() {
	return isState(STATE_NONE);
}

bool LOEventHook::isState(int sa) {
	return state.load() == sa;
};

bool LOEventHook::FinishMe() {
	return upState(STATE_FINISH);
}

bool LOEventHook::InvalidMe() {
	return upState(STATE_INVALID);
}

bool LOEventHook::enterEdit() {
	int ov = STATE_NONE;
	return state.compare_exchange_strong(ov, STATE_EDIT);
}

bool LOEventHook::closeEdit() {
	int ov = STATE_EDIT;
	return state.compare_exchange_strong(ov, STATE_NONE);
}

void LOEventHook::ResetMe() {
	state.store(STATE_NONE);
}

bool LOEventHook::upState(int sa) {
	int ov = state.load();
	while (ov < sa) {
		if (state.compare_exchange_strong(ov, sa)) return true;
		ov = state.load();
	}
	if (ov == sa) return true;
	else return false;
}

//
bool LOEventHook::waitEvent(int sleepT, int overT) {
	Uint32 t1 = SDL_GetTicks();

	while (!isFinish()) {
		if (sleepT > 0) SDL_Delay(sleepT);
		if ((overT > 0 && SDL_GetTicks() - t1 > overT) || exitFlag.load() || isInvalid()) return false;
	}
	return true;
}


LOEventHook* LOEventHook::CreateHookBase() {
	auto *e = new LOEventHook();
	e->timeStamp = SDL_GetTicks();
	return e;
}

LOEventHook* LOEventHook::CreateTimerWaitHook(LOString *scripter, bool isclickNext) {
	auto *e = CreateHookBase();
	e->callMod = MOD_SCRIPTER;
	e->callFun = FUN_TIMER_CHECK;
	e->paramList.push_back(new LOVariant(scripter));
	e->paramList.push_back(new LOVariant((int)isclickNext));
	return e;
}

LOEventHook* LOEventHook::CreatePrintPreHook(LOEventHook *e, void *ef, const char *printName) {
	e->paramList.clear();
	e->timeStamp = SDL_GetTicks();
	e->paramList.push_back(new LOVariant(ef));
	e->paramList.push_back(new LOVariant(printName, strlen(printName)));
	return e;
}


//高精度延迟，阻塞线程，CPU维持在高使用率
void G_PrecisionDelay(double t) {
	Uint64 hightTimeNow, perHtickTime = SDL_GetPerformanceFrequency() / 1000;
	hightTimeNow = SDL_GetPerformanceCounter();
	double postime = 0.0;
	while (postime < t && t > 0) {
		Uint64 pos = SDL_GetPerformanceCounter() - hightTimeNow;
		postime = (double)pos / perHtickTime;

		//cpu delay litle
		int sum = rand();
		for (int ii = 0; ii < 200; ii++) {
			sum ^= ii;
			if (ii % 2) sum++;
			else sum += 2;
		}
	}
}


//===========================================//
/*
LOEventManager::LOEventManager() {
	lowStart = normalStart = highStart = 0;
}

LOEventManager::~LOEventManager() {

}

void LOEventManager::AddEvent(LOEventHook *e, int level) {
	int count;
	intptr_t zerobase;

	auto *list = GetList(level);
	int *startpos;
	if (level < 0)startpos = &lowStart;
	else if (level > 0) startpos = &highStart;
	else startpos = &normalStart;

	while (true) {
		count = 0;
		//一直到成功为止
		while (count < list->size()) {
			//轮了一圈了
			if (*startpos >= list->size()) *startpos = 0;
			zerobase = 0;
			if ((*list)[*startpos]->compare_exchange_strong(zerobase, (intptr_t)e)) return;
			count++;
			(*startpos)++;
		}
		IncList(list);
	}
}


void LOEventManager::IncList(std::vector<std::atomic_intptr_t*>* list) {
	_mutex.lock();
	for (int ii = 0; ii < 10; ii++) {
		auto ptr = new std::atomic_intptr_t();
		ptr->store(0);
		list->push_back(ptr);
	}
	_mutex.unlock();
}


LOEventHook* LOEventManager::GetNextEvent(int *listindex, int *index) {
	while (true) {
		auto *list = GetList(*listindex);
		//历遍当前的队列
		while (*index < list->size()) {
			auto e = (LOEventHook*)((*list)[*index]->load());
			(*index)++;  //指向下一个
			if (e) {
				if (e->isActive()) return e;
				else if (e->isInvalid()) {
					(*list)[*index - 1]->store(0);
					delete e;
				}
			}
		}
		//由hight切换到low
		if (*listindex < 0) return nullptr;
		else if (*listindex > 0) listindex = 0;  //hight --> normal
		else *listindex = -1;  //normal --> low
	}

	return nullptr;
}


std::vector<std::atomic_intptr_t*> *LOEventManager::GetList(int listindex) {
	if (listindex < 0) return &lowList;
	else if (listindex > 0) return &highList;
	else return &normalList;
}
*/

