//事件处理

#include "LOEvent1.h"
#include <stdint.h>
#include <atomic>


//==================== LOEVENTParam ===============

LOEventParamBtnRef::LOEventParamBtnRef() {
	ptr1 = ptr2 = 0;
	ref = nullptr;
}

LOEventParamBtnRef::~LOEventParamBtnRef() {
	if (ref) delete ref;
}

intptr_t LOEventParamBtnRef::GetParam(int index) {
	if (index == 0) return ptr1;
	else if (index == 1) return ptr2;
	else if (index == 2) return (intptr_t)ref;
	return 0;
}

//==================== LOEvent1 ===================
std::atomic_int LOEvent1::exitFlag{};


LOEvent1::LOEvent1(int id, int64_t pa) {
	BaseInit(id);
	param = pa;
}

LOEvent1::LOEvent1(int id, LOEventParamBase* pa) {
	BaseInit(id);
	param = (int64_t)pa;
	isParamPtr = true;
}


void LOEvent1::BaseInit(int id) {
    //cant't pass in android,so (int)
	std::atomic_init(&state, (int)STATE_NONE);
	std::atomic_init(&usecount, 0);
	eventID = id;
	param = 0;
	value = 0;
	isParamPtr = false;
	isValuePtr = false;
}

void LOEvent1::SetValue(int64_t va) {
	if (isValuePtr && value) delete (LOEventParamBase*)value;
	value = va;
	isValuePtr = false;
}

void LOEvent1::SetValuePtr(LOEventParamBase* va) {
	if (isValuePtr && value) delete (LOEventParamBase*)value;
	value = (int64_t)va;
	isValuePtr = true;
}

//因为存在着刚加入事件槽就被完成的情况，所以采用事先确定引用的次数，后期不再增加
//void LOEvent1::SetUseCount(int count) {
//	usecount.store(count);
//}

void LOEvent1::NoUseEvent(LOEvent1 *e) {
	e->usecount.fetch_sub(1);
	if (e->usecount.load() <= 0) delete e;
}

void LOEvent1::SetExitFlag(int flag) {
	exitFlag.store(flag);
}

LOEvent1::~LOEvent1() {
	if (isParamPtr) {
		LOEventParamBase *v = (LOEventParamBase*)param;
		delete v;
	}
	if (isValuePtr) {
		LOEventParamBase *v = (LOEventParamBase*)value;
		delete v;
	}
	//mark it has delete
	param = 0xcccccccc; 
	value = 0xcccccccc;
}

bool LOEvent1::isFinish() {
	return state.load() == STATE_FINISH;
};

bool LOEvent1::isInvalid() {
	return state.load() == STATE_INVALID;
}


bool LOEvent1::isActive() {
	return isState(STATE_NONE);
}

bool LOEvent1::isState(int sa) {
	return state.load() == sa;
};

bool LOEvent1::FinishMe() {
	return upState(STATE_FINISH);
}

bool LOEvent1::InvalidMe() {
	return upState(STATE_INVALID);
}

bool LOEvent1::enterEdit() {
	int ov = STATE_NONE;
	return state.compare_exchange_strong(ov, STATE_EDIT);
}

bool LOEvent1::closeEdit() {
	int ov = STATE_EDIT;
	return state.compare_exchange_strong(ov, STATE_NONE);
}

bool LOEvent1::upState(int sa) {
	int ov = state.load();
	while (ov < sa) {
		if (state.compare_exchange_strong(ov, sa)) return true;
		ov = state.load();
	}
	if (ov == sa) return true;
	else return false;
}

//
bool LOEvent1::waitEvent(int sleepT, int overT) {
	Uint32 t1 = SDL_GetTicks();

	while (!isFinish()) {
		if (sleepT > 0) SDL_Delay(sleepT);
		if ((overT > 0 && SDL_GetTicks() - t1 > overT) || exitFlag.load() || isInvalid()) return false;
	}
	return true;
}

void LOEvent1::AddUseCount() {
	usecount.fetch_add(1);
}


//================= LOEventSlot ==================
LOEventSlot *G_EventSlots[LOEvent1::EVENT_ALL_COUNT];

void G_ClearAllEventSlots() {
	for (int ii = 0; ii < LOEvent1::EVENT_ALL_COUNT; ii++) {
		if(G_EventSlots[ii]) G_EventSlots[ii]->InvalidAll();
	}
}

void G_InitSlots() {
	for (int ii = 0; ii < LOEvent1::EVENT_ALL_COUNT; ii++) G_EventSlots[ii] = nullptr;
}

LOEventSlot* GetEventSlot(int index) {
	if (!G_EventSlots[index]) G_EventSlots[index] = new LOEventSlot;
	return G_EventSlots[index];
}

void G_SendEvent(LOEvent1 *e) {
	LOEventSlot *slot = GetEventSlot(e->eventID);
	slot->SendToSlot(e);
}

//信号同时发送到复数的信号槽中，注意只会由与事件slot参数相同的事件槽删除
//小心发送的顺序！
void G_SendEventMulit(LOEvent1 *e, LOEvent1::TYPE_EVENT t) {
	LOEventSlot *slot = GetEventSlot(t);
	slot->SendToSlot(e);
}

LOEvent1 * G_GetEvent(LOEvent1::TYPE_EVENT t) {
	LOEventSlot *slot = G_EventSlots[t];
	if (!slot) return nullptr;
	else return slot->GetFirstEvent(nullptr, nullptr);
}

//获取指定ID的事件
LOEvent1 * G_GetEvent(LOEvent1::TYPE_EVENT t, int eventID) {
	LOEventSlot *slot = G_EventSlots[t];
	if (!slot) return nullptr;
	else return slot->GetFirstEvent(&eventID, nullptr);
}

LOEvent1 * G_GetEventIsParam(LOEvent1::TYPE_EVENT t, int eventID, int64_t param) {
	LOEventSlot *slot = G_EventSlots[t];
	if (!slot) return nullptr;
	else return slot->GetFirstEvent(&eventID, &param);
}

LOEvent1 * G_GetSelfEventIsParam(LOEvent1::TYPE_EVENT t, int64_t param) {
	int t1 = t;
	LOEventSlot *slot = G_EventSlots[t];
	if (!slot) return nullptr;
	else return slot->GetFirstEvent(&t1, &param);
}

void G_DestroySlots() {
	for (int ii = 0; ii < LOEvent1::EVENT_ALL_COUNT; ii++) {
		if (G_EventSlots[ii]) delete G_EventSlots[ii];
		G_EventSlots[ii] = nullptr;
	}
}

void G_TransferEvent(LOEvent1 *e, LOEvent1::TYPE_EVENT t) {
	LOEventSlot *slotsrc = GetEventSlot(e->eventID);
	LOEventSlot *slotdst = GetEventSlot(t);
	slotsrc->Remove(e);
	slotdst->SendToSlot(e);
	e->closeEdit();     //转发的时候应关闭编辑状态，这样在转发后的队列才能够获取
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

//=========================

LOEventSlot::LOEventSlot() {
	first = new C_Element ;  //有一个空的首元素更方便
	std::atomic_init(&state, 1);
}

LOEventSlot::~LOEventSlot() {
	//lock();
	C_Element *ptr = first;

	while (ptr) {
		C_Element *next = ptr->next;
		if (ptr->e) ptr->e->FinishMe(); //正常来说只有退出程序或者程序崩溃才会删除事件槽
		delete ptr;                     //因此事件链表删不删除影响不大
		ptr = next; 
	}

	//unlock();
}


//不涉及到耗时的处理，直接使用自旋锁
void LOEventSlot::lock() {
	int old = 0;
	while (!state.compare_exchange_strong(old, 1));
}


void LOEventSlot::unlock() {
	int old1 = 1;
	int old2 = 0;
	while (!state.compare_exchange_strong(old1, 0)) { //锁定->不锁定，成功解锁
		if (state.compare_exchange_strong(old2, 0)) break;  //不锁定->不锁定，成功解锁
	}
}

LOEvent1 *LOEventSlot::GetFirstEvent(int *eventID, int64_t *param) {
	lock();
	C_Element *last, *ptr;
	last = first;
	//ptr = last->next;

	while (last && last->next) {
		ptr = last->next;
		if (ptr->e->isInvalid()) { //事件无效了，上一个元素指向ptr的下一个元素
			last->next = last->next->next;
			LOEvent1::NoUseEvent(ptr->e);
			delete ptr;
		}
		else {
			bool isok = true;
			if (eventID && ptr->e->eventID != *eventID) isok = false;
			if (param && ptr->e->param != *param) isok = false;
			if (isok && ptr->e->enterEdit()) {
				LOEvent1 *e = ptr->e;
				unlock();
				return e;
			}
		}
		last = last->next;
	}

	unlock();
	return nullptr;
}

//获取队列头的事件，事件无效会被移除
LOEvent1 *LOEventSlot::GetHeaderEvent() {
	C_Element *last, *ptr;
	LOEvent1 *e = nullptr;

	lock();
	last = first;
	while (last && last->next) {
		ptr = last->next;
		if (ptr->e->isInvalid()) { //事件无效了，上一个元素指向ptr的下一个元素
			last->next = last->next->next;
			LOEvent1::NoUseEvent(ptr->e);
			delete ptr;
		}
		else {
			e = ptr->e;
			break;
		}
		last = last->next;
	}
	unlock();

	return e;
}


//获取指定事件的下一个有效事件
LOEvent1 *LOEventSlot::GetHeaderEvent(LOEvent1 *es) {
	C_Element *last, *ptr;
	LOEvent1 *e = nullptr;

	lock();
	last = first;
	while (last && last->next) {
		ptr = last->next;
		if (ptr->e->isInvalid()) { //事件无效了，上一个元素指向ptr的下一个元素
			last->next = last->next->next;
			LOEvent1::NoUseEvent(ptr->e);
			delete ptr;
		}
		else if(ptr->e == es){
			if (ptr->next) e = ptr->next->e;
			break;
		}
		last = last->next;
	}
	unlock();

	return e;
}


void LOEventSlot::SendToSlot(LOEvent1 *e) {
	if (!e) return;

	e->AddUseCount();
	//if (this == G_EventSlots[LOEvent1::EVENT_CATCH_BTN]) {
	//	int debugbreak = 1;
	//}

	lock();

	C_Element *last, *ptr;
	last = first;
	while (last->next) {
		ptr = last->next;
		if (ptr->e->isInvalid()) { //事件无效了，上一个元素指向ptr的下一个元素
			last->next = last->next->next;
			LOEvent1::NoUseEvent(ptr->e);
			delete ptr;
		}
		if (!last->next) break;
		last = last->next;
	}

	ptr = new C_Element;
	ptr->e = e;
	last->next = ptr;

	unlock();
}


//优先处理的事件
void LOEventSlot::SendToSlotHeader(LOEvent1 *e) {
	if (!e) return;
	e->AddUseCount();
	C_Element *ptr = new C_Element;
	ptr->e = e;

	lock();
	ptr->next = first->next;
	first->next = ptr;
	unlock();
}

//并不会删除事件，慎用
void LOEventSlot::Remove(LOEvent1 *e) {
	if (!e) return;
	lock();

	C_Element *last, *ptr;
	last = first;
	while (last && last->next) {
		ptr = last->next;
		if (ptr->e == e) { //事件无效了，上一个元素指向ptr的下一个元素
			last->next = last->next->next;
			delete ptr;
		}
		last = last->next;
	}

	unlock();
}

//整理队列，移除已经失效的事件
void LOEventSlot::OrganizeEvent() {
	lock();
	C_Element *last, *ptr;
	last = first;
	while (last && last->next) {
		ptr = last->next;
		if (ptr->e->isInvalid()) { //事件无效了，上一个元素指向ptr的下一个元素
			last->next = last->next->next;
			LOEvent1::NoUseEvent(ptr->e);
			delete ptr;
		}
		last = last->next;
	}

	unlock();
}

//历遍队列消息，依次call指定函数，返回被调用的次数
int LOEventSlot::ForeachCall(funcTS func, double postime) {
	int callcount = 0;

	lock();
	C_Element *ptr = first->next;
	while (ptr) {
		if(ptr->e)  (*func)(ptr->e, postime);
		callcount++;
		ptr = ptr->next;
	}
	unlock();
	return callcount;
}


//移除所有消息
void LOEventSlot::InvalidAll() {
	lock();
	while (first->next) {
		C_Element *ptr = first->next;
		first->next = ptr->next;
		ptr->e->InvalidMe();
		LOEvent1::NoUseEvent(ptr->e);
		delete ptr;
	}
	unlock();
}


LOEventManager::LOEventManager() {
	lowStart = normalStart = highStart = 0;
}

LOEventManager::~LOEventManager() {

}

void LOEventManager::AddEvent(LOEvent1 *e, int level) {
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


LOEvent1* LOEventManager::GetNextEvent(int *listindex, int *index) {
	while (true) {
		auto *list = GetList(*listindex);
		//历遍当前的队列
		while (*index < list->size()) {
			auto e = (LOEvent1*)((*list)[*index]->load());
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