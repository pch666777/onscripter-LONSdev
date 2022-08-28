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

	//检查行是否正确
	void CheckCurrentLine();
	void Serialize(BinArray *bin);
	bool DeSerialize(BinArray *bin, int *pos);

	int callType;
	//当前执行到的行
	int c_line;
	//当前执行到的位置
	const char* c_buf;
	//行开始的位置
	const char* c_buf_start;
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
	////设置记录点，会对行数据做一个校验
	void SetPoint(LOScriptPointCall *p);
	////返回记录点
	void BackToPoint(LOScriptPointCall *p);
	void Serialize(BinArray *bin);
	bool LoadSetPoint(LOScriptPoint *p, int r_line, int r_buf);

	//for循环step递增的数量
	int step;
	//for %1 = 1 to %2 step n中的%1
	ONSVariableRef *forVar;
	//for 中的 %2，可以被动态改变
	ONSVariableRef *dstVar;
private:
	int flags;
	//相对于标签s_line的行数
	int relativeLine;
	//相对于行首的前进量
	int relativeByte;
	//行首buf
	const char *lineStart;
	//循环在哪个标签中
	LOScriptPoint *label;
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
	//获取指定行的信息，需要提供行号或者buf
	//AMD zen1 1400  100次查找release耗时0.5ms -->行记录设定为80行
	LineData GetLineInfo(const char *buf, int lineID, bool isLine);
private:
	LOString scriptbuf;
	//每100行打一个记录点
	std::vector<LineData> lineInfo;
	std::unordered_map<std::string, LOScriptPoint*> labels;

	void ClearLables();
	void CreateRootLabel();

	//二分法定位基行，返回的是lineInfo的索引
	LineData* MidFindBaseIndex(const char *buf, int dstLine, bool isLine);
	//从指定的起始位置查找到目标，startBuf和startLine会被修改为找到的目标，应该在MidFindBaseIndex做了有效性检查
	void NextFindDest(const char *dst, int dstLine, const char *&startBuf, int &startLine, bool isLine);
};





#endif // __LOSCRIPTPOINT_H__
