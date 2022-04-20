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

	//要响应的事件
	enum {
		ANSWER_TIMER = 1 ,
		ANSWER_BTNSTR = 2,
		ANSWER_SEPLAYOVER = 8,
		ANSWER_BTNCLICK = 16,
		//ANSWER_LEFTCLICK = 64,
		//ANSWER_RIGHTCLICK = 128,
		//ANSWER_LONGCLICK = 256, //0x100
		//ANSWER_MOUSEMOVE = 0x200,
	};

	////给图层发送的事件
	//enum {
	//	SEND_LEFTCLICK = 1,
	//	SEND_RIGHTCLICK = 2,
	//	SEND_LONGCLICK = 4,
	//	SEND_MOUSEMOVE = 8,
	//	//SEND_MOUSEMOVE或者SEND_MOUSECLICK传递后，如果已经被某个图层相应，
	//	//那么事件会转为SEND_UNACTIVE恢复已经激活的图层
	//	SEND_UNACTIVE = 16,
	//};

	//RunFunc的返回值
	enum {
		RUNFUNC_FINISH,

		RUNFUNC_CONTINUE,
	};

	//按钮的状态
	enum {
		//鼠标事件向图层发送的过程中，如果被符合条件的图层相应，那么设置 BTN_STATE_ACTIVED 符号
		//如果鼠标不在图层范围内，并且图层处于active状态，检测到BTN_STATE_ACTIVED，那么设置 BTN_STATE_UNACTIVED
		//鼠标事件停止传递
		BTN_STATE_ACTIVED = 1,
		BTN_STATE_UNACTIVED = 2,
	};

	enum {
		MOD_RENDER = 1,
		MOD_SCRIPTER,
		MOD_AUDIO,

		FUN_ENMPTY,
		FUN_TIMER_CHECK,
		//按钮事件已经完成的函数
		FUN_BTNFINISH,
		FUN_SPSTR,
		FUN_LAYERANSWER,
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
	//创建一个按钮被点击事件
	static LOEventHook* CreateBtnClickHook(int fullid, int btnval, int islong);
	//创建一个btnstr事件
	static LOEventHook* CreateBtnStr(int fullid, LOString *btnstr);
	//创建一个spstr运行
	static LOEventHook* CreateSpstrHook();

	//创建一个图层相应事件
	static LOEventHook* CreateLayerAnswer(int answer,void *lyr);
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
	void clear();

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

extern LOEventQue G_hookQue;


extern void G_PrecisionDelay(double t);
#endif // !__LOEVENT1_H__
