//事件处理
//事件按处理时间可分为限时事件和非限时事件，应避免历遍事件，效率很低
//事件自动关联到处理的槽上，事件完成后从槽中移除，加入到缓存区进行处理

#ifndef __LOEVENT1_H__
#define __LOEVENT1_H__

#include <stdint.h>
#include <atomic>
#include <mutex>
#include <SDL.h>
#include <vector>
#include "../Scripter/ONSvariable.h"
#include "LOVariant.h"

class LOEventParamBase {
public:
	enum PARAM_TYPE {
		TYPE_BASE,
		TYPE_TREE_PTR, //提供了三个不能del的指针存储
	};
	LOEventParamBase() { };
	virtual ~LOEventParamBase() { };
	virtual intptr_t GetParam(int index) { return 0; }
protected:
	int ptype;
};


class ONSVariableRef;
class LOEventParamBtnRef :public LOEventParamBase {
public:
	LOEventParamBtnRef();
	~LOEventParamBtnRef();
	intptr_t GetParam(int index);

	intptr_t ptr1;
	intptr_t ptr2;
	ONSVariableRef *ref;
};

//=====================================
class LOEventSlot;

class LOEvent1
{
public:
	enum {
		STATE_NONE,
		STATE_EDIT,
		STATE_FINISH,
		STATE_INVALID
	};

	enum TYPE_EVENT{  //envents
		EVENT_NONE,
		EVENT_PREPARE_EFFECT,  //收到此信号后，图像会刷新到指定纹理上，以便进行effect运算
		EVENT_WAIT_PRINT,      //标识print任务是否已经完成
		EVENT_IMGMODULE_AFTER, //图像模块延迟处理事件
//		EVENT_TIMER_WAIT,      //需要计时的事件，优先度非常高，由脚本线程维护
		                       //需要计时的事件通常都会加入不止一个事件槽中，如果是在复数的事件槽中，那么你要小心避免多次删除
		//EVENT_LEFT_CLICK,      //左键、右键都归入到catch btn事件中
		//EVENT_RIGHT_CLICK,     //
		EVENT_CATCH_BTN,       //捕获按钮
		EVENT_TEXT_ACTION,     //对话文字显示事件
		EVENT_SEZERO_FINISH,   //0通道声音播放完毕
		//EVENT_TEXT_GOSUB,
		//EVENT_TEXTACTION_FINISH,


		EVENT_ALL_COUNT
	};

	enum {
		ANSWER_TIMER = 1 ,
		ANSWER_LEFTCLICK = 2,
		ANSWER_RIGHTCLICK = 4,
		ANSWER_SEPLAYOVER = 8
	};

	LOEvent1(int id, int64_t pa);
	LOEvent1(int id, LOEventParamBase* pa);
	~LOEvent1();
	void SetValue(int64_t va);
	void SetValuePtr(LOEventParamBase* va);
	//void SetUseCount(int count);
	void AddUseCount();
	//void SubUseCount();
	//bool isUseCountZero();
	static void NoUseEvent(LOEvent1 *e);
	static void SetExitFlag(int flag);

	bool isFinish();
	bool isState(int sa);
	bool FinishMe();
	bool isInvalid();

	//是否处于可编辑状态
	bool isActive();


	bool InvalidMe();
	bool enterEdit();
	bool closeEdit();
	bool waitEvent(int sleepT, int overT);

	int eventID;
	//uint64_t function;
	int64_t param;
	int64_t value;

	//要求捕获的事件
	int64_t catchFlag;
	//时间戳
	Uint32 timeStamp;
	//参数表
	std::vector<LOVariant*> paramList;
private:
	void BaseInit(int id);
	bool upState(int sa);
	bool isParamPtr;
	bool isValuePtr;
	std::atomic_int state;
	std::atomic_int usecount;   //存在同一个事件发送到多个事件槽的情况，使用引用计数
	static std::atomic_int exitFlag;
};

//================= LOEventSlot ==================
class LOEventSlot {
public:
	struct C_Element{  //节点
		LOEvent1 *e = nullptr;
		C_Element *next = nullptr;
	};

	typedef void(*funcTS)(LOEvent1*,double);   //函数指针

	LOEventSlot();
	~LOEventSlot();

	LOEvent1 *GetFirstEvent(int *eventID, int64_t *param);
	LOEvent1 *GetHeaderEvent();
	LOEvent1 *GetHeaderEvent(LOEvent1 *es);
	void SendToSlot(LOEvent1 *e);
	void SendToSlotHeader(LOEvent1 *e);
	void Remove(LOEvent1 *e);
	void OrganizeEvent();
	void InvalidAll();
	int ForeachCall(funcTS func, double postime);
private:
	C_Element *first;
	std::atomic_int state;

	void lock();
	void unlock();
};

class LOEventManager {
public:
	LOEventManager();
	~LOEventManager();
	void AddEvent(LOEvent1 *e, int level);

	//获取下一个非空的事件，注意，这个函数只应该在主线程调用，会清除已经无效的事件
	LOEvent1* GetNextEvent(int *listindex, int *index);
private:
	//事件列表
	std::vector<std::atomic_intptr_t*> lowList;
	std::vector<std::atomic_intptr_t*> normalList;
	std::vector<std::atomic_intptr_t*> highList;

	std::vector<std::atomic_intptr_t*> *GetList(int listindex);
	void IncList(std::vector<std::atomic_intptr_t*>* list);

	//添加事件时开始的索引
	int lowStart;
	int normalStart;
	int highStart;

	//GetNextEvent的保存位置
	//int nextIndex;


	//增长队列时锁定
	std::mutex _mutex;
};

extern void G_SendEvent(LOEvent1 *e);
extern void G_SendEventMulit(LOEvent1 *e, LOEvent1::TYPE_EVENT t);
extern void G_InitSlots();
extern void G_DestroySlots();
extern LOEventSlot* GetEventSlot(int index);
extern LOEvent1 * G_GetEvent(LOEvent1::TYPE_EVENT t);
extern LOEvent1 * G_GetEvent(LOEvent1::TYPE_EVENT t, int eventID);
extern LOEvent1 * G_GetEventIsParam(LOEvent1::TYPE_EVENT t, int eventID, int64_t param);
extern LOEvent1 * G_GetSelfEventIsParam(LOEvent1::TYPE_EVENT t, int64_t param);
extern void G_TransferEvent(LOEvent1 *e, LOEvent1::TYPE_EVENT t);  //事件转移到其他事件槽上
extern void G_PrecisionDelay(double t);
extern void G_ClearAllEventSlots();
#endif // !__LOEVENT1_H__
