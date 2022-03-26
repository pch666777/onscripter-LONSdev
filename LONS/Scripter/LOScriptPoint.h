#ifndef __LOSCRIPTPOINT_H__
#define __LOSCRIPTPOINT_H__

#include "../etc/LOString.h"
#include "ONSvariable.h"

#include <string>
#include <unordered_map>

class LOScripFile;
//脚本运行点，只能由LOScripFile清理
class LOScriptPoint {
public:
	enum {
		//TYPE_LABEL,
		//TYPE_FOR,
		//TYPE_SUB,
		//TYPE_GOSUB,
		CALL_BY_NORMAL,
		CALL_BY_PRETEXT_GOSUB,
		CALL_BY_TEXT_GOSUB,
		CALL_BY_LOAD_GOSUB,
		CALL_BY_EVAL,
	};
	LOScriptPoint(int t, bool canf);
	~LOScriptPoint();

	//LOScriptPoint *CopyMe();

	LOString name;  //标签名称
	const char* s_buf;    //运行点实际开始的位置
	const char* c_buf;    //运行点当前运行的行
	int s_line;     //运行点开始的行
	int c_line;     //运行点当前运行的位置
	LOScripFile* file;  //位于哪个脚本中
};


//逻辑运行点
class LogicPointer
{
public:
	LogicPointer(int t);
	~LogicPointer();
	enum {
		TYPE_FOR,
		TYPE_IF,
		TYPE_IFTHEN,
		TYPE_ELSE,
		TYPE_WHILE
	};
	//char *adress; //运行点的位置，判断逻辑之后
	int relAdress; //相对于标签的地址
	int startLine;  //adress起始的行
	int step;   //for循环step递增的数量
	int type;   //类型
	bool ifret; //if then 的判断结果
	ONSVariableRef *var;  //for循环的递增变量
	ONSVariableRef *dstvar;  //for的目标增量，可以被动态改变
	LOString *lableName;
private:
};



class LOScripFile {
public:
	LOScripFile(const char *cbuf, int len, const char *filename);
	LOScripFile(LOString *s, const char *filename);
	~LOScripFile();

	LOString Name;
	LOScriptPoint *FindLable(std::string &lname);
	LOScriptPoint *FindLable(const char *lname);
	LOString *GetBuf() { return &scriptbuf; }
	void InitLables(bool lableRe = false);
	int GetPointLineAndPosition(LOScriptPoint *p, int *position);
private:
	LOString scriptbuf;
	std::unordered_map<std::string, LOScriptPoint*> labels;

	void ClearLables();
	void CreateRootLabel();
};





#endif // __LOSCRIPTPOINT_H__
