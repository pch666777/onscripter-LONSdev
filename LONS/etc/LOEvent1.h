//事件处理
#ifndef __LOEVENT1_H__
#define __LOEVENT1_H__

#include <stdint.h>
#include <atomic>
#include <mutex>
#include <SDL.h>
#include <list>
#include <vector>
#include <map>
#include "../Scripter/ONSvariable.h"
#include "LOVariant.h"

//如果你想修改/增加事件，那么理解LONS的线程通讯模式是十分重要的
//LONS主线程运行渲染和SDL事件，渲染包括硬件纹理的生成（因为在安卓系统上，如果在非主线程生成纹理会有几率卡死），脚本运行在子线程上
//LONS的图层分为前台数据和后台数据，前台数据就是当前正在显示的内容，后台为重新lsp以后的内容，两者在print时进行数据交换

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

	//要响应的事件
	enum {
		ANSWER_NONE = 0,
		ANSWER_TIMER = 1,
		//ANSWER_BTNSTR = 2,
		ANSWER_SEPLAYOVER = 8,
		ANSWER_BTNCLICK = 16,
		ANSWER_LEFTCLICK = 32,
		ANSWER_RIGHTCLICK = 64,
		//需要同时响应点击跳过print和文字，因此如果单击事件遇到这两个事件会
		//转换成PRINGJMP事件
		ANSWER_PRINGJMP = 128,
		//某些操作需要从脚本模块调用渲染模块
		ANSWER_RENDER_DO = 0x100,
		//响应视频播放完成
		ANSWER_VIDEOFINISH = 0x200,
		//某些操作需要由渲染模块调用脚本模块
		ANSWER_SCRIPT_DO = 0x400
	};

	//一些参数的位置
	enum {
		PINDS_REFID = 1,
		PINDS_PRINTNAME = 2,
		PINDS_SE_CHANNEL = 3,
		PINDS_CMD = 4,
		PINDS_BTNVAL = 1,
	};

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
		FUN_TEXT_ACTION,
		FUN_SCREENSHOT,
		FUN_PRE_EFFECT,
		FUN_CONTINUE_EFF,
		FUN_SE_PLAYFINISH,
		FUN_BGM_AFTER,
		//FUN_SCRIPT_CALL,
		FUN_VIDEO_FINISH,
		FUN_Video_Finish_After,
		FUN_BtnClear,
		FUN_AudioFade,
		FUN_DelLayer,
		//直接hook失效
		FUN_INVILIDE,
	};

	//enum {
	//	SCRIPT_CALL_BTNSTR = 1,
	//	SCRIPT_CALL_BTNCLEAR = 2,
	//	SCRIPT_CALL_AUDIOFADE = 3,
	//};

	enum {
		FLAGS_NO_SAVE = 1,  //只有含有此标记的事件/钩子不会存档
		FLAGS_FINISH_NOTABKE = 2,  //含有此标记的事件，在完成之前不会从事件列表中移除
		FLAGS_TEXTBTNWAIT = 4,
	};

	LOEventHook();
	~LOEventHook();

	bool isFinish();
	//事件/钩子是否已经完成或者无效
	bool isStateAfterFinish();
	bool isState(int sa);
	bool FinishMe();
	bool isInvalid();
	bool isFinishTakeOut() { return flags & FLAGS_FINISH_NOTABKE; }
	bool isTextBtnWait() { return flags & FLAGS_TEXTBTNWAIT; }

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
	//int32_t evType;

	//时间戳
	Uint32 timeStamp;
	//参数1，不同的处理函数有不同的意义
	int16_t param1;
	//参数2，不同的处理函数有不同的意义
	uint16_t param2;

	void paramListMoveTo(std::vector<LOVariant*> &list);
	void paramListCut(int maxsize);
	LOVariant* GetParam(int index);
	std::vector<LOVariant*> &GetParamList() { return paramList; }
	void PushParam(LOVariant *var);
	void ClearParam();
	int GetState() { return state; }

	void Serialize(BinArray *bin);
	bool Deserialize(BinArray *bin, int *pos);
	//创建一个等待事件
	static LOEventHook* CreateTimerWaitHook(LOString *scripter, bool isclickNext);
	//创建一个print准备
	static LOEventHook* CreatePrintPreHook(LOEventHook *e, void *ef, const char *printName);
	//创建一个print事件
	static LOEventHook* CreatePrintHook(LOEventHook *e, void *ef, const char *printName);
	//创建一个截图事件
	static LOEventHook* CreateScreenShot(LOEventHook *e, int x, int y, int w, int h, int dw, int dh);
	//创建一个btnwait事件
	static LOEventHook* CreateBtnwaitHook(int waittime, int refid, const char *printName, int channel, const char *cmd);
	//创建一个按钮被点击事件
	static LOEventHook* CreateBtnClickEvent(int fullid, int btnval, int islong);
	//创建一个鼠标左键或右键钩子
	static LOEventHook* CreateClickHook(bool isleft, bool isright);
	//创建一个spstr事件
	static LOEventHook* CreateSpStrEvent(int fullid, LOString *btnstr);

	//创建一个时间阻塞钩子，参数可设置是否响应左键
	static LOEventHook* CreateTimerHook(int outtime, bool isleft);
	//创建一个文字事件hook
	static LOEventHook* CreateTextHook(int pageEnd, int hash);
	//创建一个图层相应事件
	static LOEventHook* CreateLayerAnswer(int answer,void *lyr);
	//创建一个se播放完成事件
	static LOEventHook* CreateSePalyFinishEvent(int channel);
	//创建一个没有参数的信号
	static LOEventHook* CreateSignal(int param1, int param2);
	//创建一个脚本模块呼叫call，要求渲染线程响应
	//static LOEventHook* CreateScriptCallHook();
	//创建一个按钮清除事件
	static LOEventHook* CreateBtnClearEvent(int val);
	//创建一个音频淡入淡出事件
	static LOEventHook* CreateAudioFadeEvent(int channel, double per, double curVol);
	//创建一个视频播放hook，用来接受视频播放完成或者点击事件
	static LOEventHook* CreateVideoPlayHook(int fid, bool isclick);
	//创建一个播放完成事件
	static LOEventHook* CreateVideoFinish(int fid);
	//创建一个图层事件，类型于图层被点击等

	//创建一个图层清除事件，vdo = 1表示非newfile时删除图层，注意，只是进入列表，不需要print
	//static LOEventHook* CreateScriaptDelLayer(int fid, int vdo);

	//读取存档的时候需要一个时间搓作为参考
	static Uint32 loadTimeTick;
	//static int PushDoEventList(std::shared_ptr<LOEventHook> &ev);
	//static void PopDoEventList(int index, bool isInvalidEvent);
private:
	bool upState(int sa);
	//参数表
	std::vector<LOVariant*> paramList;
	std::atomic_int state;
	int16_t flags;
	static std::atomic_int exitFlag;
	//static std::vector<std::shared_ptr<LOEventHook>> doEventList;
	//===================================
	static LOEventHook* CreateHookBase();
};

typedef std::shared_ptr<LOEventHook> LOShareEventHook;
typedef std::unique_ptr<LOEventHook> LOUniqEventHook;
typedef std::map<int64_t, LOShareEventHook> LOEventMap;
//=============================


//通常来说，只有一端读取，多端写入
class LOEventQue {
public:
	LOEventQue();
	~LOEventQue();

	void push_N_front(LOShareEventHook &e);
	void push_N_back(LOShareEventHook &e);
	void push_N_back(std::vector<LOShareEventHook> &list);
	void push_H_front(LOShareEventHook &e);
	void push_H_back(LOShareEventHook &e);

	LOShareEventHook GetEventHook(std::list<LOShareEventHook>::iterator &iter, bool isenter);
	LOShareEventHook FilterEvent(int catchFlag, int16_t param1, int16_t param2, bool isenter);
	std::list<LOShareEventHook>::iterator begin() { return dLink.begin(); }

	//从头部开始取出
	LOShareEventHook TakeOutEvent();

	void clear();
	void invalidClear();
	void SaveHooks(BinArray *bin);
	bool LoadHooks(BinArray *bin, int *pos, LOEventMap *evmap);
	bool LoadHooksList(BinArray *bin, int *pos, LOEventMap *evmap, std::vector<LOShareEventHook> *list);

private:

	std::list<LOShareEventHook> dLink;
	std::list<LOShareEventHook>::iterator nIter;

	//添加、删除时锁定
	std::mutex _mutex;
	LOShareEventHook m_empty;
};

extern LOEventQue G_hookQue;

#endif // !__LOEVENT1_H__
