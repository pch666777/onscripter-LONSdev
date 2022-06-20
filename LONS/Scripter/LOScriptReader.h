/*
//项目地址：https://gitee.com/only998/onscripter-lons
//邮件联系：pngchs@qq.com
//编辑时间：2020——2021
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
#define SCRIPT_VERSION 20210626
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
	void NextLineStart();
	void BackLineStart();

	//1为else，2为endif -1表示出现错误
	int LogicJump(bool iselse); 
	int SubDeep() { return subStack->size() - 1; }
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
	
	
	ONSVariableRef *ParseVariableBase(ONSVariableRef *ref = NULL , bool str_alias = false );
	ONSVariableRef* TryNextNormalWord();
	int ParseIntVariable();
	LOString ParseStrVariable();
	LOString ParseLabel(bool istry = false);
	ONSVariableRef *ParseLabel2();
	LOScriptPoint  *GetParamLable(int index);

	bool ParseLogicExp();

	//尝试获取一个立即数，指的是别名或者实数
	const char* TryGetImmediate(int &val,const char *buf, bool isalias,bool &isok);

	//尝试获取一个单一的""字符串
	const char* TryGetImmediateStr(int &len, const char *buf, bool &isok);

	const char* TryGetStrAlias(int &ret, const char *buf, bool &isok);

	//当前命令是否指定名称
	bool isName(const char* name);

	//计算下一个应该是数值的值，展开% ? 和算式表达式，采用逆波兰算法
	ONSVariableRef* ParseIntExpression(const char *&buf, bool isalias);

	LOString GetTextTagString();

	//部分文本需展开$符号
	bool ExpandStr(LOString &s);

	//void PushCurrent(bool canfree);
	void ReadyToRun(LOScriptPoint *label, int callby = LOScriptPoint::CALL_BY_NORMAL);
	bool ReadyToRun(LOString *lname, int callby = LOScriptPoint::CALL_BY_NORMAL);
	int ReadyToBack();

	LOScriptPoint *currentLable;
	LOString *scriptbuf;
	LOString word;
	LOString curCmd;    //当前运行的命令
	LOString lastCmd;   //上一个命令
	const char* curCmdbuf;  //当前运行的命令数据
	
	LOStack<LogicPointer> *loopStack; //循环堆栈 for while用
	LogicPointer *nslogic;     //ns默认的if else模式

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
	void ResetBaseConfig();
	LOString GetReport();
	void PrintError(const char *fmt, ...);
	void UpdataGlobleVariable();
	void SaveGlobleVariable();
	void Serialize(BinArray *bin);

	int RunFunc(LOEventHook *hook, LOEventHook *e);
	int RunFuncBtnFinish(LOEventHook *hook, LOEventHook *e);
	int RunFuncBtnSetVal(LOEventHook *hook);
	int RunFuncSayFinish(LOEventHook *hook);

	//添加新的脚本到缓冲区
	static LOScripFile* AddScript(const char *buf, int length, const char* filename);
	static LOScripFile* AddScript(LOString *s, const char* filename);
	static void RemoveScript(LOScripFile *f);
	static void InitScriptLabels();
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

private:
	static LOStack<LOScripFile> filesList;   //脚本存储在这里
	static std::unordered_map<std::string, int> numAliasMap;   //整数别名
	static std::unordered_map<std::string, int> strAliasMap;   //字符串别名，存的是字符串位置
	static std::vector<LOString> strAliasList;  //字符串别名
	static std::vector<LOString> workDirs;      //脚本、打开的文件支持的搜索路径
	static std::unordered_map<std::string, LOScriptPoint*> defsubMap; //defsub定义的函数

	static int sectionState; //脚本当前是否处于定义区
	static bool st_globalon; //是否使用全局变量
	static bool st_labellog; //是否使用标签变量
	static bool st_errorsave; //是否使用错误自动保存


	//bool isAddLine;
	int  parseDeep;    //startparse进入多少个嵌套了
	LOStack<LOScriptPoint> *subStack;		//运行点堆栈，gosub sub用
	Uint64 ttimer;  //SDL计时器
	LOScriptReader *activeReader;          //当前激活的脚本
	LOString TagString;  //显示文字前的tag
	std::vector<LOString> cselList;
	int gloableMax;
	
	
	LOScriptPoint* GetScriptPoint(LOString lname);
	int ContinueRun();
	int ContinueEvent();
	void InserCommand(LOString *incmd);   //插入命令，比如 $xxx ，eval "xxx"
	bool PushParams(const char *param, const char* used);  //根据参数将变量推入paramStack中
	void ClearParams(bool isall);
	int TextPushParams(const char *&buf);   //将文本的参数推入paramStack中
	void JumpNext();    //跳过next执行

	virtual int   FileRemove(const char *name);
	
	int GetAliasRef(LOString &s, bool isstr, bool &isok);
	int ReturnEvent(int ret,const char *&buf, int &line);
	LOString GetCurrentFile();
	void NewThreadGosub(LOString *pname, LOString threadName);
	//int RunFuncBtnFinish(LOEventHook *hook, LOEventHook *e);

	const char* GetRPNstack(LOStack<ONSVariableRef> *s2,const char *buf, bool isalias = false);
	void CalculatRPNstack(LOStack<ONSVariableRef> *stack);
	//弹出堆栈s1的值到 s2，直到指定符号，如果到最后一个值则产生错误
	void PopRPNstackUtill(LOStack<ONSVariableRef> *s1, LOStack<ONSVariableRef> *s2, char op);
	const char* TryToNextCommand(const char*buf);



	//调试用
	std::vector<ONSVariableRef*> TransformStack(LOStack<ONSVariableRef> *s1);
};



#endif // !__LOScriptReader_H__
