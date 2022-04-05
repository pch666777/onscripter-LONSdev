//事件处理
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

class LOEventHook
{
public:
	enum {
		STATE_NONE,
		STATE_EDIT,
		STATE_FINISH,
		STATE_INVALID
	};

	enum TYPE_EVENT{  //envents
		//没有必要了解类型的
		EVENT_NONE,
		//定义的按钮
		EVENT_BTNDEF,
		EVENT_BTNWAIT,
	};

	enum {
		ANSWER_TIMER = 1 ,
		ANSWER_LEFTCLICK = 2,
		ANSWER_RIGHTCLICK = 4,
		ANSWER_SEPLAYOVER = 8
	};

	//发送的事件
	enum {
		SEND_MOUSEMOVE,
		SEND_MOUSECLICK,
		//SEND_MOUSEMOVE或者SEND_MOUSECLICK传递后，如果已经被某个图层相应，
		//那么事件会转为SEND_UNACTIVE恢复已经激活的图层
		SEND_UNACTIVE,
	};


	enum {
		MOD_RENDER = 1,
		MOD_SCRIPTER,
		MOD_AUDIO,

		FUN_ENMPTY,
		FUN_TIMER_CHECK,

		//按钮处理函数
		FUN_BTNRUN,
		//按钮事件已经完成的函数
		FUN_BTNFINISH,
	};

	LOEventHook();
	~LOEventHook();

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
	int32_t catchFlag;
	//事件的类型
	int32_t evType;

	//时间戳
	Uint32 timeStamp;
	//参数1，不同的处理函数有不同的意义
	int16_t param1;
	//参数2，不同的处理函数有不同的意义
	uint16_t param2;
	//参数表
	std::vector<LOVariant*> paramList;

	//创建一个等待事件
	static LOEventHook* CreateTimerWaitHook(LOString *scripter, bool isclickNext);
	//创建一个print准备
	static LOEventHook* CreatePrintPreHook(LOEventHook *e, void *ef, const char *printName);
	//创建一个btnwait事件
	static LOEventHook* CreateBtnwaitHook(int onsType, int onsID, int waittime, int waitse);

private:
	bool upState(int sa);
	std::atomic_int state;
	static std::atomic_int exitFlag;
	//===================================
	static LOEventHook* CreateHookBase();
};

typedef std::shared_ptr<LOEventHook> LOShareEventHook;
typedef std::unique_ptr<LOEventHook> LOUniqEventHook;

//=============================

class LOEventQue {
public:
	LOEventQue();
	~LOEventQue();

	enum {
		LEVEL_NORMAL,
		LEVEL_HIGH,
	};

	void push_back(LOShareEventHook &e, int level);
	void push_header(LOShareEventHook &e, int level);
	LOShareEventHook GetEventHook(int &index, int level, bool isenter);

	//整理队列，注意调用的时机
	void arrangeList();

	//获取下一个非空的事件，注意，这个函数只应该在主线程调用，会清除已经无效的事件
	//LOEventHook* GetNextEvent(int *listindex, int *index);
	
private:
	//普通事件列表
	std::vector<LOShareEventHook> normalList;
	//优先事件列表
	std::vector<LOShareEventHook> highList;

	//添加、删除时锁定
	std::mutex _mutex;

	//空的
	//LOShareEventHook emptyHook;

	std::vector<LOShareEventHook>* GetList(int level);
};

//extern LOEventQue G_hookQue;


extern void G_PrecisionDelay(double t);
#endif // !__LOEVENT1_H__
