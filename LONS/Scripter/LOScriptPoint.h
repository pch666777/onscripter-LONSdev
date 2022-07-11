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
		TYPE_FOR = 1,
		TYPE_IF = 2,
		TYPE_IFTHEN = 4,
		TYPE_ELSE = 8,
		TYPE_WHILE = 16,
		//if的判断结果，有这个符号为真，没有这个符号为假
		TYPE_RESULT_TRUE = 32
	};

	void SetFlags(int f) { flags |= f; }
	void UnSetFlags(int f) { flags &= (~f); }
	bool isFor() { return flags & TYPE_FOR; }
	bool isIF() { return flags & TYPE_IF; }
	bool isIFthen(){ return flags & TYPE_IFTHEN; }
	bool isElse() { return flags & TYPE_ELSE; }
	bool isWhile() { return flags & TYPE_WHILE; }
	bool isRetTrue() { return flags & TYPE_RESULT_TRUE; }
	void reset();
	void SetRet(bool it);
	////设置记录点
	//void SetPoint(LOScriptPointCall *p);
	////返回记录点
	//void BackToPoint(LOScriptPointCall *p);
	void Serialize(BinArray *bin);

	//循环的起点，存档的时候转换为行位置
	const char *point;
	int pointLine;
	//for循环step递增的数量
	int step;
	//for %1 = 1 to %2 step n中的%1
	ONSVariableRef *forVar;
	//for 中的 %2，可以被动态改变
	ONSVariableRef *dstVar;
	//循环在哪个标签中
	LOScriptPoint *label;
private:
	int flags;
};



class LOScripFile {
public:
	//行跟buf的关系
	struct LineData{
		LineData(int id, const char* bin) {
			lineID = id;
			buf = bin;
		}
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

	//获取某一行的开始位置
	void GetLineStart(int lineID);
private:
	LOString scriptbuf;
	//每100行打一个记录点
	std::vector<LineData> lineInfo;
	std::unordered_map<std::string, LOScriptPoint*> labels;

	void ClearLables();
	void CreateRootLabel();

	//二分法查找到指定行的基行位置，会修改lintID为基行，失败返回null
	const char* MidFindLinePoint(int &lineID);
};





#endif // __LOSCRIPTPOINT_H__
