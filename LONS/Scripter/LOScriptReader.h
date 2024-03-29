﻿/*
//项目地址：https://github.com/pch666777/onscripter-LONSdev
//邮件联系：pngchs@qq.com
//编辑时间：2020——2023
//将scripter引擎单独分离出来，之后完全可以将脚本解析器用在其他地方，或者使用其他脚本解析器
//脚本引擎通过轮询来达到多线程脚本的目的
*/
#ifndef __LOSCRIPTREADER_H__
#define __LOSCRIPTREADER_H__

#include "LOScriptPoint.h"
#include "ONSvariable.h"
#include "FuncInterface.h"
#include "../etc/LOStack.h"
#include "../etc/LOEvent1.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <math.h>
#include <chrono>
#include <thread>



//定义你想要的log输出函数
#define LOG_PRINT printf
#define SCRIPT_VERSION 20220630
#define MAINSCRIPTER_NAME "_mainscript"

class LOScriptReader :public FunctionInterface
{
public:
	LOScriptReader();
	~LOScriptReader();

	typedef std::chrono::steady_clock  STEADY_CLOCK;

	enum {
		SECTION_DEFINE,
		SECTION_NORMAL,
	};


	//标识语句类型
	enum {
		LINE_UNKNOW,
		LINE_NOTES,    //注释
		LINE_LABEL,    //标签
		LINE_JUMP,
		LINE_CONNECT,  //连接符
		LINE_TEXT,     //文字或[开始的文字
		LINE_CAMMAND,  //字母开头，字母和数字组成的命令
		LINE_EXPRESSION,  //$1 $%1，这类表达式，需展开
		LINE_END,       //到达换行符了
		LINE_ZERO,      //遇到了一个/0
	};

	//校验算式允许的操作类型
	enum {
		ALLOW_REF = 1,  //允许表达式
		ALLOW_REAL = 2, //允许实数
	};

	enum {
		SIZE_MAXOUTCACHE = 256,   //输出char缓存的最大值
	};

	//指定的堆栈
	enum {
		STACK_NORMAL,
		STACK_TEXT
	};

	enum {
		LAST_CHECK_BTNWAIT = 1
	};

	enum class SCRIPT_TYPE {
		SCRIPT_TYPE_NORMAL,
		SCRIPT_TYPE_MAIN,
		SCRIPT_TYPE_TIMER,
		
	};

	enum {
		ALIAS_INT = 1,
		ALIAS_STR = 2,
	};

	struct SaveFileInfo{
		void reset() {
			id = -1;
			memset(this->timer, 0, 6 * 2);
			tag.clear();
		}
		int id = -1;
		int16_t timer[6];
		LOString tag;
	};


	int lastCmdCheckFlag;  //部分命令依据上一个命令确定是否追加操作的
	SCRIPT_TYPE sctype;
	LOScriptReader *nextReader;

	//是否运行到RunScript应该返回的位置
	bool isEndSub();

	int IdentifyLine(const char *&buf);  //识别出从指定位置的语句类型
	int RunCommand(const char *&buf);
	void NextWord(bool movebuf = true);

	char CurrentOP() { return currentLable->c_buf[0]; }
	void NextAdress() { currentLable->c_buf++; }
	void NextLineStart();    //到达下一行行首
	void BackLineStart();   //返回到当前行首
	LOString GetLineFromCurrent();  //从当前位置获取一行
	void GotoLine(int lineID);

	//1为else，2为endif -1表示出现错误
	int LogicJump(bool iselse); 
	//int SubDeep() { return subStack->size() - 1; }
	bool ChangePointer(int esp);
	bool ChangePointer(LOScriptPoint *label);

	//bool ChangeStackData(int totype);

	//下一个一般性的单词，由字母开头，字母和数字混合，包括$展开
	void NextNormalWord(bool movebuf = true);  

	//通常检查是否有参数
	bool NextSomething();

	//通过下一个逗号，非try模式没有,号会报错
	bool NextComma(bool isTry = false);
	bool NextStartFrom(char op);  //下一个符号是否是指定符号
	bool NextStartFrom(const char* op);  //下一个是否为指定单词，不分大小写，是则返回真
	bool LineEndIs(const char*op);  //行尾是否已制定模式结束
	bool NextStartFromStrVal();
	
	
	ONSVariableRef *ParseVariableBase(bool isstr, bool isInDialog = false);
	ONSVariableRef* TryNextNormalWord();
	int ParseIntVariable();
	LOString ParseStrVariable();
	LOString ParseLabel(bool istry = false);
	ONSVariableRef *ParseLabel2();
	LOScriptPoint  *GetParamLable(int index);

	bool ParseLogicExp();

	//当前命令是否指定名称
	bool isName(const char* name);

	//计算下一个应该是数值的值，展开% ? 和算式表达式，采用逆波兰算法
	ONSVariableRef* ParseIntExpression(const char *&buf, bool isstrAlias);

	LOString GetTextTagString();

	//部分文本需展开$符号
	bool ExpandStr(LOString &s);

	//void PushCurrent(bool canfree);
	void ReadyToRun(LOScriptPoint *label, int callby = LOScriptPoint::CALL_BY_NORMAL);
	bool ReadyToRun(LOString *lname, int callby = LOScriptPoint::CALL_BY_NORMAL);
	bool ReadyToRunEval(LOString *eval);
	bool ReadyToBackEval();
	int ReadyToBack();

	LOScriptPointCall *currentLable;
	LOString *scriptbuf;
	LOString word;
	LOString curCmd;    //当前运行的命令
	LOString lastCmd;   //上一个命令
	const char* curCmdbuf;  //当前运行的命令数据
	
	//循环堆栈 for while用
	LOStack<LogicPointer> loopStack;
	//ns默认的if else模式
	LogicPointer normalLogic;
	

	LOString Name;      //脚本线程的名称
	LOString printName;  //图像信息加载到的队列名称

	//主脚本启动，只有主线程才能执行
	int MainTreadRunning();

	int DefaultStep();  //搜索0.txt 00.txt开始作为默认启动
	void GetGameInit(int &w, int &h);
	const char* GetPrintName();
	const char* GetCmdChar() { return curCmdbuf; };
	intptr_t GetEncoder();
	int GetCurrentLine() { return currentLable->c_line; }
	void ResetMe();
	void LoadReset();
	void ResetBaseConfig();
	LOString GetReport();
	void UpdataGlobleVariable();
	void ReadGlobleVarFile();
	void ReadGlobleVariable(BinArray *bin, int *pos);
	void SaveGlobleVariable();
	void Serialize(BinArray *bin);
	bool DeSerialize(BinArray *bin, int *pos, LOEventMap *evmap);
	BinArray *ReadSaveFile(int id, int readLen);
	void      ReadSaveInfo(int id);
        void GetModValue(int vtype, void *val);

	//需要用到脚本的函数，因此运行点的反序列化放在这里，point为null
	bool ScCallDeSerialize(BinArray *bin, int *pos);
	bool LogicCallDeSerialize(BinArray *bin, int *pos);

	int RunFunc(LOEventHook *hook, LOEventHook *e);
	int RunFuncBtnSetVal(LOEventHook *hook);
	int RunFuncSayFinish(LOEventHook *hook);
	int RunEventAfterFinish(LOEventHook *e);

	//添加新的脚本到缓冲区
	//static LOScripFile* AddScript(const char *buf, int length, const char* filename);
	//static LOScripFile* AddScript(LOString *s, const char* filename);
	//static void RemoveScript(LOScripFile *f);
	//static void InitScriptLabels();
	const char* debugCharPtr(int cur);

	//启动一个新的脚本
	static LOScriptReader* EnterScriptReader(LOString name);
	static LOScriptReader* GetScriptReader(LOString &name);
	static void AddWorkDir(LOString dir);

	//脚本支持完成后不会从列表中删除，而是进入休眠
	static LOScriptReader* LeaveScriptReader(LOScriptReader* reader);
	int gosubCore(LOScriptPoint *p, bool isgoto);

	int movCommand(FunctionInterface *reader);
	int operationCommand(FunctionInterface *reader);
	int dimCommand(FunctionInterface *reader);
	int addCommand(FunctionInterface *reader);
	int intlimitCommand(FunctionInterface *reader);
	int itoaCommand(FunctionInterface *reader);
	int lenCommand(FunctionInterface *reader);
	int midCommand(FunctionInterface *reader);
	int movlCommand(FunctionInterface *reader);
	int debuglogCommand(FunctionInterface *reader);
	int numaliasCommand(FunctionInterface *reader);
	int straliasCommand(FunctionInterface *reader);
	int gotoCommand(FunctionInterface *reader);
	int jumpCommand(FunctionInterface *reader);
	int returnCommand(FunctionInterface *reader);
	int skipCommand(FunctionInterface *reader);
	int tablegotoCommand(FunctionInterface *reader);
	int forCommand(FunctionInterface *reader);
	int nextCommand(FunctionInterface *reader);
	int breakCommand(FunctionInterface *reader);
	int ifCommand(FunctionInterface *reader);
	int elseCommand(FunctionInterface *reader);
	int endifCommand(FunctionInterface *reader);
	int resettimerCommand(FunctionInterface *reader);
	int gettimerCommand(FunctionInterface *reader);
	int delayCommand(FunctionInterface *reader);
	int gameCommand(FunctionInterface *reader);
	int endCommand(FunctionInterface *reader);
	int defsubCommand(FunctionInterface *reader);
	int getparamCommand(FunctionInterface *reader);
	int labelexistCommand(FunctionInterface *reader);
	//int fileexistCommand(FunctionInterface *reader);
	int fileremoveCommand(FunctionInterface *reader);
	//int readfileCommand(FunctionInterface *reader);
	int splitCommand(FunctionInterface *reader);
	int getversionCommand(FunctionInterface *reader);
	int getenvCommand(FunctionInterface *reader);
	int dateCommand(FunctionInterface *reader);
	int usergosubCommand(FunctionInterface *reader);
	int loadscriptCommand(FunctionInterface *reader);
	int gettagCommand(FunctionInterface *reader);
	int atoiCommand(FunctionInterface *reader);
	int chkValueCommand(FunctionInterface *reader);
	int cselCommand(FunctionInterface *reader);
	int getcselstrCommand(FunctionInterface *reader);
	int delaygosubCommand(FunctionInterface *reader);
	int savefileexistCommand(FunctionInterface *reader);
	int savepointCommand(FunctionInterface *reader);
	int savegameCommand(FunctionInterface *reader);
	int resetCommand(FunctionInterface *reader);
	int defineresetCommand(FunctionInterface *reader);
	int labellogCommand(FunctionInterface *reader);
	int globalonCommand(FunctionInterface *reader);
	int testcmdsCommand(FunctionInterface *reader);
	//转移脚本，等同于reset，然后从指定脚本开始执行
	//int _movto_Command(FunctionInterface *reader);
	int loadgameCommand(FunctionInterface *reader);
	int setintvarCommand(FunctionInterface *reader);
	int saveonCommand(FunctionInterface *reader);
	int savetimeCommand(FunctionInterface *reader);
	int getsavestrCommand(FunctionInterface *reader);
	int linepageCommand(FunctionInterface *reader);
	int movieCommand(FunctionInterface *reader);
	int systemcallCommand(FunctionInterface *reader);

private:
	static std::unordered_map<std::string, int> numAliasMap;   //整数别名
	static std::unordered_map<std::string, int> strAliasMap;   //字符串别名，存的是字符串位置
	static std::vector<LOString> strAliasList;  //字符串别名
	static std::vector<LOString> workDirs;      //脚本、打开的文件支持的搜索路径
	static std::unordered_map<std::string, LOScriptPoint*> defsubMap; //defsub定义的函数

	static int sectionState; //脚本当前是否处于定义区
	static bool st_globalon; //是否使用全局变量
	static bool st_labellog; //是否使用标签变量
	static bool st_errorsave; //是否使用错误自动保存
	static bool st_saveonflag; //是否处于saveon模式
	static bool st_autosaveoff_flag;   //是否处于saveon/saveoff关闭模式
	static int gloableBorder;
	static SaveFileInfo s_saveinfo;

	//bool isAddLine;
	int  parseDeep;    //startparse进入多少个嵌套了
	//LOStack<LOScriptPoint> *subStack;		//运行点堆栈，gosub sub用
	//运行点堆栈，gosub sub用，每次增长时都会重新取currentlabel的指针，因此用vector时安全的
	std::vector<LOScriptPointCall> subStack;
	//标识有eval的运行状态
	//std::vector<int8_t> evalStack;
	
	Uint64 ttimer;  //SDL计时器
	LOScriptReader *activeReader;          //当前激活的脚本
	LOString TagString;  //显示文字前的tag
	std::vector<LOString> cselList;

        int autoModeTime;  //auto模式下应该等待的时间
	
	//LOScriptPoint* GetScriptPoint(LOString lname);
	int GetCurrentLableIndex();
	int ContinueRun();
	int ContinueEvent();
	void InserCommand(LOString *incmd);   //插入命令，比如 $xxx ，eval "xxx"
	bool InserCommand();   //从当前脚本的位置获取文字变量值，然后插入命令并运行
	bool PushParams(const char *param, const char* used);  //根据参数将变量推入paramStack中
	void ClearParams(bool isall);
	void TextPushParams();   //将文本的参数推入paramStack中
	void JumpNext();    //跳过next执行
	bool LoadCore(int id);

	virtual int   FileRemove(const char *name);
	
	int GetAliasRef(LOString &s, bool isstr, int &out);
	int GetAliasRef(const char *&buf, bool isstr, int &out);
	int ReturnEvent(int ret,const char *&buf, int &line);
	LOString GetCurrentFile();
	void NewThreadGosub(LOString *pname, LOString threadName);
	//int RunFuncBtnFinish(LOEventHook *hook, LOEventHook *e);

	//const char* GetRPNstack(LOStack<ONSVariableRef> *s2,const char *buf, bool isalias = false);
	const char* GetRPNstack2(std::vector<LOUniqVariableRef> *s2, const char *buf, bool isstrAlia);
	void CalculatRPNstack(std::vector<LOUniqVariableRef> *stack);
	//弹出堆栈s1的值到 s2，直到指定符号，如果到最后一个值则产生错误
	void PopRPNstackUtill(std::vector<LOUniqVariableRef> *s1, std::vector<LOUniqVariableRef> *s2, char op);
	const char* TryToNextCommand(const char*buf);



	//调试用
	std::vector<ONSVariableRef*> TransformStack(LOStack<ONSVariableRef> *s1);
};



#endif // !__LOScriptReader_H__
