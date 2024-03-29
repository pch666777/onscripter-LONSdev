﻿/*
//这是一个接口类
*/



#ifndef __FUNCINTERFACE_H__
#define __FUNCINTERFACE_H__

#include "ONSvariable.h"
#include "../etc/LOStack.h"
#include "../etc/BinArray.h"
#include "../etc/LOString.h"
#include "../etc/LOLog.h"
#include "../etc/LOEvent1.h"
#include <unordered_map>
#include <unordered_set>
#include <atomic>

class LOScriptPoint;

class FunctionInterface
{
public:
	FunctionInterface();
	virtual ~FunctionInterface();
	enum {
		RET_CONTINUE,
		RET_ERROR,      //致命错误，不应该继续运行了
		RET_WARNING,   //警告错误，跳过出错的命令
		RET_RETURN,    //如果已经到达堆栈尾部则应该返回
		RET_VIRTUAL,   //没有被实现的虚函数

		RET_PRETEXT = 21,   //从pretextgosub中返回
		RET_TEXT = 22,      //从textgosub中返回
		RET_LOAD = 23,      //从loadgosub中返回
	};

	enum {
		READER_WAIT_NONE,
		READER_WAIT_ENTER_PRINT,
		READER_WAIT_PREPARE,
	};

	enum {
		BUILT_FONT,  //内置字体
	};

	enum {
		USERGOSUB_PRETEXT,
		USERGOSUB_TEXT,
		USERGOSUB_LOAD
	};

	//用于movie command 传递参数
	enum {
		MOVIE_CMD_STOP = 1,
		MOVIE_CMD_CLICK = 2,
		MOVIE_CMD_LOOP = 4,
                MOVIE_CMD_ASYNC = 8,
                MOVIE_CMD_ALLOW_OUTSIDE = 16
	};

	enum {
		SCRIPTER_EVENT_DALAY = 1024,   //脚本阻塞延迟
		SCRIPTER_EVENT_LEFTCLICK,
		SCRIPTER_EVENT_CLICK,
		SCRIPTER_BGMLOOP_NEXT,
		//SCRIPTER_DELAY_CLICK,
		SCRIPTER_DELAY_GOSUB,
		SCRIPTER_RUN_SPSTR,     //执行spstr
		SCRIPTER_INSER_COMMAND,

		LAYER_TEXT_PREPARE,
		LAYER_TEXT_WORKING,

		AFTER_PREPARE_EFFECT_FINISH ,
		AFTER_PRINT_FINISH,
	};

	enum {
		LOGSET_FILELOG,
		LOGSET_LABELLOG
	};

        enum{
            SIMPLE_CLOSE_RUBYLINE = 1,
        };

	//用比特位表示各个模块的状态
	enum MODULE_STATE {
		//低4表示运行状态，0为没有在使用，1正在运行，4已经挂起
		MODULE_STATE_NOUSE = 0,
		MODULE_STATE_RUNNING = 1,
		//模块已经挂起，对于不同的模块有不同的意义
		//对于脚本模块，表示正在轮询其他脚本线程
		//对于渲染模块，表示已经暂停渲染
		//对于音频模块，将不在发生任何播放完成事件，
		MODULE_STATE_SUSPEND = 2,

		//其他为功能性标记位
		//状态切换标记，模块检查到此标记将会转到状态切换函数
		MODULE_FLAGE_CHANNGE = 0x400,
		//读取存档标记
		MODULE_FLAGE_LOAD = 0x800,
		//存档标记
		MODULE_FLAGE_SAVE = 0x1000,
		//重置标记
		MODULE_FLAGE_RESET = 0x2000,
		//退出标记
		MODULE_FLAGE_EXIT = 0x4000,
		//错误标记，转为一个单独的静态变量，防止覆盖
		//MODULE_FLAGE_ERROR = 0x8000,
		//move_to标记
		//MODULE_FLAGE_MOVETO = 0x10000,
	};

	//跨越模块取变量枚举
	enum {
		MODVALUE_PAGEEND = 1,
                MODVALUE_TEXTSPEED ,
                MODVALUE_AUTOMODE,
                MODVALUE_CHANNEL_STATE,  //频道的播放状态
	};

	////本质上就是一个大型状态机，将需要设置的状态集中起来，便于管理
	//enum {
	//	ST_btnnowindowerase,  //显示按钮的时候，不消除文字窗口的显示
	//	ST_erasetextwindow,   //显示效果的时候不会消除文字窗口
	//	ST_font,           //对话框使用的字体
	//	ST_defaultfont,   //默认的字体？如果没有设置font和spfont，那么应该使用这个
	//	ST_spfont,        //默认的lsp使用的字体
	//	ST_noteraseman,   //消除窗口时，不消除立绘
	//	ST_setwindow_xy,  //对话框的位置
	//	ST_setwindow_str, //对话框
	//	ST_setwindow_size, //对话框大小
	//	ST_tateyoko,      //竖行模式
	//	ST_windowchip,    //和文字窗口一起出现和消失的精灵
	//	ST_windoweffect,  //指定文字窗口的效果
	//	ST_shadedistance, //阴影的距离
	//	ST_mousecursor,   //设定鼠标光标图像
	//	ST_setcursor,     //设定等待时的光标的图像文件
	//	ST_humanorder,    //指定立绘之间的优先顺序
	//	ST_humanpos,      //设定立绘的基准位置
	//	ST_humanz,        //对话框的位置，位置前和后的绘制顺序不一致
	//	ST_tal,           //更改立绘的透明度
	//	ST_underline,     //确定立绘的最低线
	//	ST_windowback,    //将文本窗口与立绘插入同一位置
	//	ST_effectblank,   //指定效果结束后的等待时间
	//	ST_effectcut,     //跳至下一选择支」时、瞬间显示所有的效果

	//	ST_menuselectcolor, //右键菜单中的文本文字颜色
	//	ST_menuselectvoice, //右键菜单的声音设定
	//	ST_menusetwindow,   //右键菜单的窗口设定
	//	ST_rgosub,    //设定右键菜单的时候要运行的子程序
	//	ST_rlookback, //右键单击时进入回想模式
	//	ST_lookbackbutton, //指定回想模式中箭头的图像
	//	ST_lookbackcolor,  //指定回想模式的文字颜色
	//	ST_lookbackvoice,  //指定回想模式中翻页的声音
	//	ST_maxkaisoupage,  //设定回想模式的最大记录页数
	//	ST_kidokumode,     //在跳过已读和强制跳过模式之间切换
	//	ST_kidokuskip,     //打开跳过已读功能
	//	ST_mode_wave_demo, //「跳到下一个选择支」的时候仍然播放WAVE声音
	//	ST_filelog,        //允许文件访问记录
	//	ST_globalon,       //允许全局记录
	//	ST_labellog,       //标签访问记录
	//	ST_errorsave,      //出错时自动保存至999号存档
	//	ST_setkinsoku,     //设置禁止字符，句尾禁止换行的符号？
	//	ST_nsadir,         //nsa压缩包读取目录
	//	ST_kinsoku,        //是否启用禁止符
	//	ST_noloaderror,    //忽略图像音乐读取失败的错误
	//	ST_defbgmvol,      //设置BGM的默认音量，设置MP3的默认音量
	//	ST_bgmdownmode,   //语音时降低背景音乐音量
	//	ST_bgmfadein,     //指定背景音乐渐入时间
	//	ST_bgmbgmfadeout, //指定背景音乐渐出时间
	//	ST_defsevol,      //设置音效的默认音量，各通道的音量不保存在这里
	//	ST_useescspc,     //设定使用Esc键和Spc键（截屏）
	//	ST_usewheel,      //设定使用鼠标滚轮
	//	ST_rubyon,        //ruby的大小
	//};

	typedef int (FunctionInterface::*FuncBase)(FunctionInterface*);
	struct FuncLUT {
		const char *cmd;
		const char *param;
		const char *used;
		FuncBase method;
	};

	struct ST_SET{
		int stype;
		int16_t sint1;
		int16_t sint2;
		LOString* str;
	};

	LOStack<ONSVariableRef> paramStack;
	int moduleState;
	LOEventQue waitEventQue;

	FuncLUT* GetFunction(LOString &func);
	ONSVariableRef *GetParamRef(int index);
	virtual LOScriptPoint  *GetParamLable(int index);
	LOString GetParamStr(int index);
	int GetParamColor(int index);
	int GetParamInt(int index);
	double GetParamDoublePer(int index);
	int GetParamCount();
	bool CheckTimer(LOEventHook *e, int waittmer);

	void ChangeModuleState(int s);
	void ChangeModuleFlags(int f);
	bool isStateChange() { return moduleState & MODULE_FLAGE_CHANNGE; }
	bool isModuleExit() { return (moduleState & MODULE_FLAGE_EXIT) || errorFlag; }
	bool isModuleReset() { return moduleState & MODULE_FLAGE_RESET; }
	bool isModuleSaving() { return moduleState & MODULE_FLAGE_SAVE; }
	bool isModuleLoading() { return moduleState & MODULE_FLAGE_LOAD; }
	bool isModuleNoUse() { return (moduleState&0xf) == MODULE_STATE_NOUSE; }
	bool isModuleError() { return errorFlag; }
	bool isModuleFlagSNone() { return (moduleState >> 4) == 0; }
	bool isModuleSuspend() { return moduleState & MODULE_STATE_SUSPEND; }
	bool isModuleRunning() { return moduleState & MODULE_STATE_RUNNING; }

	static LOString StringFormat(int max, const char *format, ...);

	static FunctionInterface *imgeModule;    //图像模块
	static FunctionInterface *audioModule;   //视频模块
	static FunctionInterface *scriptModule;  //脚本模块
	static FunctionInterface *fileModule;    //文件系统

	int RunCommand(FuncBase it);
	virtual BinArray *ReadFile(LOString *fileName, bool err = true);
	virtual BinArray *GetBuiltMem(int type) { return NULL; }

	virtual bool NotUseTextrue(LOString *fname, uint64_t flag) { return false; };
	virtual int  AddBtn(int fullid, intptr_t ptr) { return 0; };
	virtual intptr_t GetSDLRender() { return 0; }
	virtual void ResetMe() { return; };
	virtual void LoadFinish() { return; };
    //简单的事件，比如想要在脚本线程更改渲染线程模块的变量，避免头文件相互依赖
    virtual void SimpleEvent(int e, void *data){return ;}
	virtual int RunFunc(LOEventHook *hook, LOEventHook *e) { return 0; }
	//virtual int DoMustEvent(int val) { return 0; }
	int RunFuncBase(LOEventHook *hook, LOEventHook *e);
	void SetExitFlag(int flag);
	void ReadLog(int logt);
	void WriteLog(int logt);
	virtual void ClearBtndef() { return; };
	virtual void Serialize(BinArray *bin) { return; };
	virtual bool DeSerialize(BinArray *bin, int *pos, LOEventMap *evmap) { return false; };
	//跨越模块获取变量
	virtual void GetModValue(int vtype, void *val) {};
        virtual void SetModValue(int vtype, intptr_t val){};

	//===== scripter virtual  ========
	virtual void GetGameInit(int &w, int &h) { return; };
	virtual intptr_t GetEncoder() { return 0; }
	virtual bool isName(const char* name) { return false; }
	virtual bool ExpandStr(LOString &s) { return false; }
	virtual const char* GetPrintName() { return "_main"; }
	virtual const char* GetCmdChar() { return "\0"; }
	virtual void AddNextEvent(intptr_t e) { return; }
	virtual int GetCurrentLine() { return -1; }

	//===== audio virtual  ========
	virtual void PlayAfter() { return; };
	virtual void SePlay(int channel,LOString s, int loopcount) { return; }

	//======================  command =======================//
	virtual int movCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int operationCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int dimCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int textCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int addCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int intlimitCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int itoaCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int lenCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int midCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int movlCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int debuglogCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int numaliasCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int straliasCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int gotoCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int jumpCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int returnCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int skipCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int tablegotoCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int forCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int nextCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int breakCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int ifCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int elseCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int endifCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int resettimerCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int gettimerCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int delayCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int gameCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int endCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int defsubCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int getparamCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int labelexistCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int fileexistCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int fileremoveCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int readfileCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int splitCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int getversionCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int getenvCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int dateCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int usergosubCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int atoiCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int getmouseposCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int chkValueCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int cselCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int getcselstrCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int delaygosubCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int savefileexistCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int savepointCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int savegameCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int savedirCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int resetCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int defineresetCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int labellogCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int globalonCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int testcmdsCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	//virtual int _movto_Command(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int loadgameCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int setintvarCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int saveonCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int savetimeCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int getsavestrCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int linepageCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int systemcallCommand(FunctionInterface *reader) { return RET_VIRTUAL; }

	virtual int movieCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int aviCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int lspCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int printCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int bgCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int cspCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int mspCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int cellCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int humanzCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int strspCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int transmodeCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int bgcopyCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int spfontCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int effectskipCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int getspmodeCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int getspsizeCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int getspposCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int vspCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int allspCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int windowbackCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int lsp2Command(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int loadscriptCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int effectCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int windoweffectCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int btnwaitCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int spbtnCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int texecCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int texthideCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int textonCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int textbtnsetCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int setwindowCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int setwindow2Command(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int gettagCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int gettextCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int clickCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int erasetextwindowCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int btndefCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int btntimeCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int getpixcolorCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int getspposexCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int getspalphaCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int chkcolorCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int mouseclickCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int getscreenshotCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int savescreenshotCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int rubyCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int getcursorposCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int ispageCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int exbtn_dCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int btnCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int spstrCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int filelogCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int speventCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int quakeCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int ldCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int clCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int humanorderCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int captionCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int textspeedCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
    virtual int actionCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
    virtual int actionloopCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int monocroCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int negaCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
    virtual int textcolorCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int automode_timeCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int isskipCommand(FunctionInterface *reader) { return RET_VIRTUAL; }

	virtual int bgmCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int bgmonceCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int bgmstopCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int loopbgmCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int bgmfadeCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int waveCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int bgmdownmodeCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int voicevolCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int chvolCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int dwaveCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int dwaveloadCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int dwaveplayCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int dwavestopCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int getvoicevolCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int loopbgmstopCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int stopCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int getvoicestateCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int getrealvolCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int getvoicefileCommand(FunctionInterface *reader) { return RET_VIRTUAL; }

	virtual int nsadirCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int nsaCommand(FunctionInterface *reader) { return RET_VIRTUAL; }

	//static std::vector<LOString> workDirs;        //搜索目录，因为接口类到处都在使用，干脆把io部分挪到这里来
	//static std::atomic_int flagPrepareEffect;        //print时要求截取当前画面
	//static std::atomic_int flagRenderNew;            //每次完成画面刷新后，flagRenderNew都会置为1
	static LOShareEventHook effcetRunHook;    //要求抓取图像
	static LOShareEventHook printHook;       //要求等待print完成
	static bool errorFlag;
	static bool breakFlag;

	static LOString userGoSubName[3];
	//存储流
	static BinArray *GloVariableFS;
	static BinArray *GloSaveFS;
private:
	static std::unordered_map<std::string, FuncLUT*> baseFuncMap;
	static std::unordered_set<std::string> fileLogSet;
	static std::unordered_set<std::string> labelLogSet;
	static void RegisterBaseFunc();
	BinArray *logMem;
};

#endif // !__FUNCINTERFACE_H__
