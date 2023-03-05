//事件处理

#include "LOEvent1.h"
#include <stdint.h>
#include <atomic>

LOEventQue G_hookQue;


//==================== LOEvent1 ===================
std::atomic_int LOEventHook::exitFlag{};
Uint32 LOEventHook::loadTimeTick = 0;


LOEventHook::LOEventHook() {
	catchFlag = 0;
	param1 = 0;
	param2 = 0;
	flags = 0;
	state.store(STATE_NONE);
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

bool LOEventHook::isAfterFinish() {
	return state.load() >= STATE_FINISH;
}

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

LOVariant* LOEventHook::GetParam(int index) {
	if (index < paramList.size()) return paramList.at(index);
	return nullptr;
}

void LOEventHook::PushParam(LOVariant *var) {
	paramList.push_back(var);
}

void LOEventHook::ClearParam() {
	for (int ii = 0; ii < paramList.size(); ii++) delete paramList.at(ii);
	paramList.clear();
}


void LOEventHook::Serialize(BinArray *bin) {
	//自身的指针作为识别ID，在反序列化时有用
	bin->WriteInt64((int64_t)this);

	int len = bin->WriteLpksEntity("even", 0, 1);
	//时间记录的是时间戳跟当前的差值
	bin->WriteInt3(catchFlag, 0, (int)(SDL_GetTicks() - timeStamp));
	bin->WriteInt16(param1);
	bin->WriteInt16(param2);
	bin->WriteInt16(flags);
	bin->WriteInt(state.load());
	//参数列表
	bin->WriteInt(paramList.size());
	for (int ii = 0; ii < paramList.size(); ii++) bin->WriteLOVariant(paramList.at(ii));

	bin->WriteInt(bin->Length() - len, &len);
}


bool LOEventHook::Deserialize(BinArray *bin, int *pos) {
	int next = -1;
	if (!bin->CheckEntity("even", &next, nullptr, pos)) return false;

	catchFlag = bin->GetIntAuto(pos);
	//evType = bin->GetIntAuto(pos);
	bin->GetIntAuto(pos);
	timeStamp = loadTimeTick + bin->GetIntAuto(pos);
	param1 = bin->GetInt16Auto(pos);
	param2 = bin->GetInt16Auto(pos);
	flags = bin->GetInt16Auto(pos);
	state.store(bin->GetIntAuto(pos));

	int count = bin->GetIntAuto(pos);
	for (int ii = 0; ii < count; ii++) {
		LOVariant *v = bin->GetLOVariant(pos);
		if (!v) return false;
		paramList.push_back(v);
	}

	*pos = next;
	return true;
}


LOEventHook* LOEventHook::CreateHookBase() {
	auto *e = new LOEventHook();
	e->timeStamp = SDL_GetTicks();
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
	e->ClearParam();
	e->timeStamp = SDL_GetTicks();
	e->param2 = FUN_PRE_EFFECT;
	e->paramList.push_back(new LOVariant(ef));
	e->paramList.push_back(new LOVariant(printName, strlen(printName)));
	return e;
}


LOEventHook* LOEventHook::CreatePrintHook(LOEventHook *e, void *ef, const char *printName) {
	e->ClearParam();
	e->timeStamp = SDL_GetTicks();
	e->catchFlag |= ANSWER_LEFTCLICK | ANSWER_PRINGJMP;
	e->param1 = MOD_RENDER;
	e->param2 = FUN_CONTINUE_EFF;
	e->paramList.push_back(new LOVariant(ef));
	e->paramList.push_back(new LOVariant(printName, strlen(printName)));
	return e;
}


LOEventHook* LOEventHook::CreateScreenShot(LOEventHook *e, int x, int y, int w, int h, int dw, int dh) {
	e->ClearParam();
	e->timeStamp = SDL_GetTicks();
	e->param2 = FUN_SCREENSHOT;
	e->paramList.push_back(new LOVariant(x));
	e->paramList.push_back(new LOVariant(y));
	e->paramList.push_back(new LOVariant(w));
	e->paramList.push_back(new LOVariant(h));
	e->paramList.push_back(new LOVariant(dw));
	e->paramList.push_back(new LOVariant(dh));
	return e;
}



//响应按钮事件,waittime <= 0 表示不超时，printName = nullptr表示不清除按钮定义，否则则 > 0 时清除按钮定义
//channel < 0 表示不关联语音播放，>= 0表示关联哪一个通道，最后一个参数表示按钮调用的命令
LOEventHook* LOEventHook::CreateBtnwaitHook(int waittime, int refid, const char *printName, int channel, const char *cmd) {
	auto *e = CreateHookBase();
	e->catchFlag = ANSWER_BTNCLICK;
	if (channel >= 0) e->catchFlag |= ANSWER_SEPLAYOVER;
	if (waittime > 0) e->catchFlag |= ANSWER_TIMER;
	e->param1 = MOD_RENDER;
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
	e->catchFlag = ANSWER_SCRIPTCALL;
	e->param1 = SCRIPT_CALL_BTNSTR;
	//e->paramList.push_back(new LOVariant(SCRIPT_CALL_BTNSTR));
	e->paramList.push_back(new LOVariant(fullid));
	e->paramList.push_back(new LOVariant(btnstr));
	return e;
}


LOEventHook* LOEventHook::CreateBtnClearEvent(int val) {
	auto *e = CreateHookBase();
	e->catchFlag = ANSWER_SCRIPTCALL;
	e->param1 = SCRIPT_CALL_BTNCLEAR;
	//e->paramList.push_back(new LOVariant(SCRIPT_CALL_BTNCLEAR));
	e->paramList.push_back(new LOVariant(val));
	return e;
}


LOEventHook* LOEventHook::CreateAudioFadeEvent(int channel, double per, double curVol) {
	auto *e = CreateHookBase();
	e->catchFlag = ANSWER_SCRIPTCALL;
	e->param1 = SCRIPT_CALL_AUDIOFADE;
	e->param2 = channel & 0xffff;
	e->paramList.push_back(new LOVariant(per));
	e->paramList.push_back(new LOVariant(curVol));
	e->flags |= FLAGS_FINISH_NOTABKE;
	e->flags |= FLAGS_NO_SAVE;
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

LOEventHook* LOEventHook::CreateTextHook(int pageEnd, int hash) {
	auto *e = CreateHookBase();
	e->catchFlag = ANSWER_NONE;
	e->param1 = MOD_RENDER;
	e->param2 = FUN_TEXT_ACTION;
	e->paramList.push_back(new LOVariant(pageEnd));
	e->paramList.push_back(new LOVariant(hash));
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


LOEventHook* LOEventHook::CreateSePalyFinishEvent(int channel) {
	auto *e = CreateHookBase();
	e->param1 = MOD_RENDER;
	e->param2 = FUN_SE_PLAYFINISH;
	e->PushParam(new LOVariant(channel));
	return e;
}

LOEventHook* LOEventHook::CreateSignal(int param1, int param2) {
	auto *e = CreateHookBase();
	e->catchFlag = ANSWER_NONE;
	e->param1 = param1;
	e->param2 = param2;
	return e;
}


LOEventHook* LOEventHook::CreateScriptCallHook() {
	auto *e = CreateHookBase();
	e->catchFlag = ANSWER_SCRIPTCALL;
	e->param1 = MOD_RENDER;
	e->param2 = FUN_SCRIPT_CALL;
	return e;
}


//===========================================//

LOEventQue::LOEventQue() {
	dLink.emplace_back(nullptr);
	nIter = dLink.begin();
}

LOEventQue::~LOEventQue() {
}


LOShareEventHook LOEventQue::GetEventHook(std::list<LOShareEventHook>::iterator &iter, bool isenter) {
	_mutex.lock();
	while (iter != dLink.end()) {
		LOShareEventHook ev = *iter;
		if (ev) {
			//清除不可用的对象
			if (ev->isInvalid()) {
				iter = dLink.erase(iter);
				continue;
			}
			//尝试进入对象
			if (isenter) {
				if (ev->enterEdit()) {
					iter++;
					_mutex.unlock();
					return ev;
				}
				iter++;
			}
			else {
				iter++;
				_mutex.unlock();
				return ev;
			}
		}
		else if (iter == nIter) iter++;   //调过低优先级标识符
		else iter = dLink.erase(iter);  //删除空对象
	}

	_mutex.unlock();
	return m_empty;
}

//返回队列中第一个符合要求的值
LOShareEventHook LOEventQue::FilterEvent(int catchFlag, int16_t param1, int16_t param2, bool isenter) {
	_mutex.lock();
	auto iter = dLink.begin();
	while (iter != dLink.end()) {
		LOShareEventHook ev = *iter;
		if (!ev) continue;

		//清除不可用的对象
		if (ev->isInvalid()) {
			iter = dLink.erase(iter);
			continue;
		}

		if ((ev->catchFlag & catchFlag) && ev->param1 == param1 && ev->param2 == param2) {
			//尝试进入对象
			if (isenter) {
				if (ev->enterEdit()) {
					_mutex.unlock();
					return ev;
				}
			}
			else {
				_mutex.unlock();
				return ev;
			}
		}
	}
	_mutex.unlock();
	return LOShareEventHook();
}


LOShareEventHook LOEventQue::TakeOutEvent() {
	_mutex.lock();
	auto iter = dLink.begin();
	if (iter == nIter) iter++;
	if (iter != dLink.end()) {
		LOShareEventHook ev = *iter;
		dLink.erase(iter);
		_mutex.unlock();
		return ev;
	}
	_mutex.unlock();
	return m_empty;
}



void LOEventQue::push_N_back(LOShareEventHook &e) {
	_mutex.lock();
	dLink.push_back(e);
	_mutex.unlock();
}

void LOEventQue::push_N_back(std::vector<LOShareEventHook> &list) {
	_mutex.lock();
	for (int ii = 0; ii < list.size(); ii++) dLink.push_back(list[ii]);
	_mutex.unlock();
}

void LOEventQue::push_N_front(LOShareEventHook &e) {
	_mutex.lock();
	auto iter = nIter;
	iter++;
	dLink.insert(iter, e);
	_mutex.unlock();
}

void LOEventQue::push_H_front(LOShareEventHook &e) {
	_mutex.lock();
	dLink.push_front(e);
	_mutex.unlock();
}

void LOEventQue::push_H_back(LOShareEventHook &e) {
	_mutex.lock();
	dLink.insert(nIter, e);
	_mutex.unlock();
}



void LOEventQue::clear() {
	_mutex.lock();
	dLink.clear();
	_mutex.unlock();
}


void LOEventQue::invalidClear() {
	_mutex.lock();
	for (auto iter = dLink.begin(); iter != dLink.end(); iter++) {
		if (*iter) (*iter)->InvalidMe();
	}
	dLink.clear();
	//重新初始化
	dLink.emplace_back(nullptr);
	nIter = dLink.begin();
	_mutex.unlock();
}


void LOEventQue::SaveHooks(BinArray *bin) {
	_mutex.lock();
	//'hque',len, version
	int len = bin->WriteLpksEntity("hque", 0, 1);

	/*
	bin->WriteInt(highList.size());
	for (int ii = 0; ii < highList.size(); ii++) highList.at(ii)->Serialize(bin);
	bin->WriteInt(normalList.size());
	for (int ii = 0; ii < normalList.size(); ii++) normalList.at(ii)->Serialize(bin);
	*/
	bin->WriteInt(bin->Length() - len, &len);
	_mutex.unlock();
}


bool LOEventQue::LoadHooks(BinArray *bin, int *pos, LOEventMap *evmap) {
	int next = -1;
	if (!bin->CheckEntity("hque", &next, nullptr, pos)) return false;

	/*
	//highList
	if (!LoadHooksList(bin, pos, evmap, &highList)) return false;
	//normalList
	if (!LoadHooksList(bin, pos, evmap, &normalList)) return false;
	*/

	*pos = next;
	return true;
}


bool LOEventQue::LoadHooksList(BinArray *bin, int *pos, LOEventMap *evmap, std::vector<LOShareEventHook> *list) {
	int count = bin->GetIntAuto(pos);
	for (int ii = 0; ii < count; ii++) {
		int64_t PID = bin->GetInt64Auto(pos);
		auto iter = evmap->find(PID);
		//已经序列化了eventhook
		if (iter != evmap->end()) {
			list->push_back(iter->second);
			int next = -1;
			if (!bin->CheckEntity("even", &next, nullptr, pos)) return false;
			*pos = next;
		}
		else {
			LOEventHook *e = new LOEventHook();
			if (!e->Deserialize(bin, pos)) return false;
			LOShareEventHook ev(e);
			list->push_back(ev);
		}
	}

	return true;
}