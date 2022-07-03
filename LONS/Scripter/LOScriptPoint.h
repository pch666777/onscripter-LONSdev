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
	LOScriptPoint();
	virtual ~LOScriptPoint();

	//标签名称
	LOString name;
	//运行点实际开始的位置
	const char* s_buf;
	//运行点开始的行
	int s_line;
	LOScripFile* file;  //位于哪个脚本中
};


//call类型，同一个LOScriptPoint可能有不同的call类型
class LOScriptPointCall :public LOScriptPoint {
public:
	LOScriptPointCall();
	LOScriptPointCall(LOScriptPoint *p);
	~LOScriptPointCall();

	void Serialize(BinArray *bin);

	int callType;
	//当前执行到的行
	int c_line;
	//当前执行到的位置
	const char* c_buf;
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
	//行跟buf的关系
	struct LineData{
		int lineID = -1;
		const char* buf = nullptr;
	};

	LOScripFile(const char *cbuf, int len, const char *filename);
	LOScripFile(LOString *s, const char *filename);
	~LOScripFile();

	LOString Name;
	LOScriptPoint *FindLable(std::string &lname);
	LOScriptPoint *FindLable(const char *lname);
	LOString *GetBuf() { return &scriptbuf; }
	void InitLables(bool lableRe = false);
	//int GetPointLineAndPosition(LOScriptPoint *p, int *position);
	void GetBufLine(const char *buf, int *lineID, const char* &lineStart);
private:
	LOString scriptbuf;
	//每100行打一个记录点
	std::vector<LineData> lineInfo;
	std::unordered_map<std::string, LOScriptPoint*> labels;

	void ClearLables();
	void CreateRootLabel();
};





#endif // __LOSCRIPTPOINT_H__
