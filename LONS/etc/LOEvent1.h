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

//=====================================

class LOEventHook_t
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

	enum {
		MOD_RENDER = 1,
		MOD_SCRIPTER,
		MOD_AUDIO,

		FUN_ENMPTY,
		FUN_TIMER_CHECK,
	};

	LOEventHook_t();
	~LOEventHook_t();

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
	void ResetMe();

	//要求捕获的事件
	int64_t catchFlag;
	//时间戳
	Uint32 timeStamp;
	//模块
	int16_t callMod;
	//函数
	uint16_t callFun;
	//参数表
	std::vector<LOVariant*> paramList;

	//创建一个等待事件
	static LOEventHook_t* CreateTimerWaitHook(LOString *scripter, bool isclickNext);
	//创建一个print准备
	static LOEventHook_t* CreatePrintPreHook(LOEventHook_t *e, void *ef, const char *printName);
private:
	bool upState(int sa);
	std::atomic_int state;
	static std::atomic_int exitFlag;
	//===================================
	static LOEventHook_t* CreateHookBase();
};

typedef std::shared_ptr<LOEventHook_t> LOEventHook;

//=============================
/*
class LOEventManager {
public:
	LOEventManager();
	~LOEventManager();
	void AddEvent(LOEventHook *e, int level);

	//获取下一个非空的事件，注意，这个函数只应该在主线程调用，会清除已经无效的事件
	LOEventHook* GetNextEvent(int *listindex, int *index);
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
extern LOEventManager G_hookManager;
*/

extern void G_PrecisionDelay(double t);
extern void G_ClearAllEventSlots();
#endif // !__LOEVENT1_H__
