#ifndef __ONSVARIABLE_H__
#define __ONSVARIABLE_H__

//#include "BinArray.h"
#include "../etc/LOString.h"
#include "../etc/BinArray.h"

extern void LOLog_e(const char *fmt, ...);

#define MAXVARIABLE_COUNT 5120  //允许的最大变量数
#define MAXVARIABLE_ARRAY 4      //允许的最大数组维数
#define SimpleError LOLog_e

//===========================================================

class ONSVariableBase
{
public:
	ONSVariableBase();
	~ONSVariableBase();
	void SetValue(double v);
	void SetArrayValue(int *index , int val);
	void SetString(LOString *s);
	void SetLimit(int min, int max);
	double GetValue();
	int    GetArrayValue(int *index);
	int ArrayDimension(int *index);
	LOString *GetString();
	const char *CheckAarryIndex(int *index);
	void DimArray(int *index);
	void Serialization(BinArray *bin, int vid);
	bool Deserialization(BinArray *bin, int *pos);
	static void SetStrCore(LOString *&dst, LOString *s);
	static ONSVariableBase *GetVariable(int id);
	static int StrToIntSafe(LOString *s); //这个函数有重复的
	static void AppendStrCore(LOString *&dst,LOString *s);
	static void ResetAll();
	static void SaveOnsVar(BinArray *bin, int from, int count);
private:
	double value;
	LOString *strValue;
	int *arrayData;
	bool isLimit;
	int limitMax;
	int limitMin;

	void BaseInit();
	int ArrayIndexPosition(int *index);
	int ArrayCount();
};

//===========================================================

class ONSVariableRef
{
public:
	//变量类型
	enum ONSVAR_TYPE {
		TYPE_NONE = 0,
		TYPE_INT = 1,         //%xxx
		TYPE_STRING = 2,      //$xxx
		TYPE_STRING_IM = 4,   //非$得文本型
		TYPE_ARRAY = 8,       //?xx[x]
		TYPE_REAL = 16,        //实数，立即数
		TYPE_OPERATOR,     //+ - * /
		TYPE_ARRAY_FLAG,   //?数组操作标记，非常有用
		//===逻辑运算符====
		LOGIC_BIGTHAN,  // >
		LOGIC_BIGANDEQUAL, // >=
		LOGIC_LESS,  // <
		LOGIC_LESSANDEQUAL,  // <=
		LOGIC_EQUAL,
		LOGIC_UNEQUAL,
		LOGIC_AND,
		LOGIC_OR,
	};

	//执行次序
	enum {
		ORDER_NONE,    //代表最低运算次序
		ORDER_ADDSUB,  //+ - 次一级
		ORDER_MULDIRMOD,  //乘除取余第三级
		ORDER_SQU,     //平方第四级
		ORDER_GETVAR,  //% $ ?第五级
		ORDER_SQBRA,   //方括号
		ORDER_ROUNDBRA, //圆括号
	};

	ONSVariableRef();
	ONSVariableRef(ONSVAR_TYPE t, int idd);
	ONSVariableRef(ONSVAR_TYPE t);

	~ONSVariableRef();

	ONSVAR_TYPE vtype;    //变量的类型 % $ ? 算数符号
	char oper;			//运算符
	int order;			//运算顺序
	int nsvId;				//该变量的id
	
	void UpImToRef(int tp);
	void DownRefToIm(int tp);
	void SetRef(int tp, int id);
	void SetImVal(double v);
	void SetImVal(LOString *v);
	void SetValue(ONSVariableRef *ref);
	void SetValue(double v);
	void SetValue(LOString *s);
	void SetValue(LOCodePage *encode,const char *buf, int len);
	void SetAutoVal(double v);
	bool SetArryVals(int *v, int count);
	void SetAutoVal(LOString *s);
	void InitArrayIndex();
	void DimArray();
	void SetArrayIndex(int val,int index);
	int  GetArrayIndex(int index);
	int  GetTypeRefid();

	static int  GetTypeAllow(const char *param);
	static ONSVariableRef* GetRefFromTypeRefid(int refid);
	LOString *GetStr();
	double GetReal();

	int  GetOperator(const char *buf);
	bool isOperator() { return (vtype == TYPE_OPERATOR); }
	bool isStr() { return (vtype == TYPE_STRING || vtype == TYPE_STRING_IM); }
	bool isReal() { return (vtype == TYPE_INT || vtype == TYPE_ARRAY || vtype == TYPE_REAL); }
	bool isRef() { return (vtype == TYPE_INT || vtype == TYPE_ARRAY || vtype == TYPE_STRING); }
	bool isRealRef(){ return (vtype == TYPE_INT || vtype == TYPE_ARRAY); }
	bool isStrRef() { return vtype == TYPE_STRING; }
	//获取操作符需要的操作数
	int GetOpCount() {
		if (oper == '$' || oper == '%' || oper == '?') return 1;
		else return 2;
	}

	bool Calculator(ONSVariableRef *v, char op, bool isreal);
	int Compare(ONSVariableRef *v2, int comtype, bool isreal);
	void CopyFrom(ONSVariableRef *v);
private:
	ONSVariableBase *nsv;
	LOString *im_str;
	LOString *im_tmpout;
	double im_val;
	int *arrayIndex;
	void BaseInit();
	LOString *IntToStr(int v);
};
typedef std::unique_ptr<ONSVariableRef> LOUniqVariableRef;


//放在全局变量，省得在类里面传来传去
extern ONSVariableBase GNSvariable[MAXVARIABLE_COUNT];  

#endif // !__ONSVARIABLE_H__
