/*
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

	//用比特位表示各个模块的状态
	enum MODULE_STATE {
		MODULE_STATE_NOUSE = 0,
		MODULE_STATE_RUNNING = 1,
		MODULE_STATE_SUSPEND = 2,     //挂起，多表示在脚本模块中，表示正则轮询其他脚本线程，脚本本身还是在工作的
		MODULE_STATE_LOAD = 0x08000000, 
		MODULE_STATE_RESET = 0x10000000,
		MODULE_STATE_EXIT = 0x20000000,
		MODULE_STATE_ERROR = 0x40000000
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
	virtual void ResetMeFinish() { return; };
	virtual void PrintError(LOString *err) { return; };
	virtual int RunFunc(LOEventHook *hook, LOEventHook *e) { return 0; }
	int RunFuncBase(LOEventHook *hook, LOEventHook *e);
	virtual void ClearBtndef(const char *printName) { return; }
	void PrintErrorStatic(LOString *err);
	void SetExitFlag(int flag);
	void ReadLog(int logt);
	void WriteLog(int logt);
	

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

	virtual int nsadirCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int nsaCommand(FunctionInterface *reader) { return RET_VIRTUAL; }

	//static std::vector<LOString> workDirs;        //搜索目录，因为接口类到处都在使用，干脆把io部分挪到这里来
	//static std::atomic_int flagPrepareEffect;        //print时要求截取当前画面
	//static std::atomic_int flagRenderNew;            //每次完成画面刷新后，flagRenderNew都会置为1
	static LOShareEventHook printPreHook;    //要求抓取图像
	static LOShareEventHook printHook;       //要求等待print完成

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
