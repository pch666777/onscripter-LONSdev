//事件处理

#include "LOEvent1.h"
#include <stdint.h>
#include <atomic>

LOEventQue G_hookQue;


//==================== LOEvent1 ===================
std::atomic_int LOEventHook::exitFlag{};


LOEventHook::LOEventHook() {
	catchFlag = 0;
	param1 = 0;
	param2 = 0;
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


//参数列表转移到目标尾部
void LOEventHook::paramListMoveTo(std::vector<LOVariant*> &list) {
	for (int ii = 0; ii < paramList.size(); ii++) {
		list.push_back(paramList.at(ii));
	}
	paramList.clear();
}

//删除多余的参数
void LOEventHook::paramListCut(int maxsize) {
	while (paramList.size() > maxsize) {
		delete paramList[paramList.size() - 1];
		paramList.pop_back();
	}
}


LOEventHook* LOEventHook::CreateHookBase() {
	auto *e = new LOEventHook();
	e->timeStamp = SDL_GetTicks();
	e->evType = EVENT_NONE;
	return e;
}

LOEventHook* LOEventHook::CreateTimerWaitHook(LOString *scripter, bool isclickNext) {
	auto *e = CreateHookBase();
	e->param1 = MOD_SCRIPTER;
	e->param2 = FUN_TIMER_CHECK;
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




//响应按钮事件,waittime <= 0 表示不超时，printName = nullptr表示不清除按钮定义，否则则 > 0 时清除按钮定义
//channel < 0 表示不关联语音播放，>= 0表示关联哪一个通道，最后一个参数表示按钮调用的命令
LOEventHook* LOEventHook::CreateBtnwaitHook(int waittime, int refid, const char *printName, int channel, const char *cmd) {
	auto *e = CreateHookBase();
	e->catchFlag = ANSWER_BTNCLICK;
	if (channel >= 0) e->catchFlag |= ANSWER_SEPLAYOVER;
	if (waittime > 0) e->catchFlag |= ANSWER_TIMER;
	e->param1 = MOD_SCRIPTER;
	e->param2 = FUN_BTNFINISH;
	//最大的等待时间
	e->paramList.push_back(new LOVariant(waittime));
	e->paramList.push_back(new LOVariant(refid));
	e->paramList.push_back(new LOVariant(printName, strlen(printName)));
	e->paramList.push_back(new LOVariant(channel));
	e->paramList.push_back(new LOVariant(cmd, strlen(cmd)));
	return e;
}

LOEventHook* LOEventHook::CreateBtnClickEvent(int fullid, int btnval, int islong) {
	auto *e = CreateHookBase();
	e->catchFlag = ANSWER_BTNCLICK;
	e->paramList.push_back(new LOVariant(fullid));
	e->paramList.push_back(new LOVariant(btnval));
	e->paramList.push_back(new LOVariant(islong));
	return e;
}


LOEventHook* LOEventHook::CreateClickHook(bool isleft, bool isright) {
	auto *e = CreateHookBase();
	if (isleft) e->catchFlag |= ANSWER_LEFTCLICK;
	if (isright) e->catchFlag |= ANSWER_RIGHTCLICK;
	e->param1 = MOD_RENDER;
	e->param2 = FUN_INVILIDE;
	return e;
}


LOEventHook* LOEventHook::CreateBtnStr(int fullid, LOString *btnstr) {
	auto *e = CreateHookBase();
	e->catchFlag = ANSWER_BTNSTR;
	e->paramList.push_back(new LOVariant(fullid));
	e->paramList.push_back(new LOVariant(btnstr));
	return e;
}


LOEventHook* LOEventHook::CreateSpstrHook() {
	auto *e = CreateHookBase();
	e->catchFlag = ANSWER_BTNSTR;
	e->param1 = MOD_RENDER;
	e->param2 = FUN_SPSTR;
	return e;
}


LOEventHook* LOEventHook::CreateTimerHook(int outtime, bool isleft) {
	auto *e = CreateHookBase();
	e->catchFlag = ANSWER_TIMER;
	if (isleft) e->catchFlag |= ANSWER_LEFTCLICK;
	e->param1 = MOD_RENDER;
	e->param2 = FUN_INVILIDE;
	e->paramList.push_back(new LOVariant(outtime));
	return e;
}

LOEventHook* LOEventHook::CreateLayerAnswer(int answer, void *lyr) {
	auto *e = CreateHookBase();
	e->catchFlag = answer;
	e->param1 = MOD_RENDER;
	e->param2 = FUN_LAYERANSWER;
	e->paramList.push_back(new LOVariant(lyr));
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

LOEventQue::LOEventQue() {
	
}

LOEventQue::~LOEventQue() {

}

//要保证添加时绝对不删除文件
void LOEventQue::push_back(LOShareEventHook &e, int level) {
	auto *list = GetList(level);
	_mutex.lock();
	list->push_back(e);
	_mutex.unlock();
}

LOShareEventHook LOEventQue::GetEventHook(int &index, int level, bool isenter) {
	auto *list = GetList(level);
	LOShareEventHook ret;
	_mutex.lock();
	while (index >= 0 && index < list->size()) {
		//注意，不判断有消息
		LOShareEventHook ev = list->at(index);
		if (ev) {
			if (isenter) {
				if (ev->enterEdit()) {
					ret = ev;
					break;
				}
			}
			else {
				ret = ev;
				break;
			}
		}
		index++;
	}
	_mutex.unlock();

	index++;
	return ret;
}


void LOEventQue::push_header(LOShareEventHook &e, int level) {
	auto *list = GetList(level);
	_mutex.lock();
	auto iter = list->begin();
	list->insert(iter, e);
	_mutex.unlock();
}


std::vector<LOShareEventHook>* LOEventQue::GetList(int level) {
	if (level == LEVEL_HIGH) {
		return &highList;
	}
	else {
		return &normalList;
	}
}


void LOEventQue::arrangeList() {
	_mutex.lock();
	for (int level = 0; level < 2; level++) {
		auto *list = GetList(level);
		for (auto iter = list->begin(); iter != list->end(); ) {
			if ((*iter)->isInvalid()) iter = list->erase(iter);
			else iter++;
		}
	}
	_mutex.unlock();
}


void LOEventQue::clear() {
	_mutex.lock();
	normalList.clear();
	highList.clear();
	_mutex.unlock();
}