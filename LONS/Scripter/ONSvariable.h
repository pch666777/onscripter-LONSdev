#ifndef __ONSVARIABLE_H__
#define __ONSVARIABLE_H__

//#include "BinArray.h"
#include "../etc/LOString.h"
#include "../etc/BinArray.h"

extern void LOLog_e(const char *fmt, ...);

#define MAXVARIABLE_COUNT 5120  //允许的最大变量数
#define MAXVARIABLE_ARRAY 4      //允许的最大数组维数

//===========================================================

class ONSVariableBase
{
public:
	enum {
		SAVE_HAS_VALUE = 1,
		SAVE_HAS_STRING = 2,
		SAVE_HAS_ARRAY = 4,
	};


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
	bool LoadDeserialize(BinArray *bin, int *pos);
	//bool Deserialization(BinArray *bin, int *pos);
	static void SetStrCore(LOString *&dst, LOString *s);
	static ONSVariableBase *GetVariable(int id);
	static int StrToIntSafe(LOString *s); //这个函数有重复的
	static void AppendStrCore(LOString *&dst,LOString *s);
	static void ResetAll();
	static void SaveOnsVar(BinArray *bin, int from, int count);
	static bool LoadOnsVar(BinArray *bin, int *pos);
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
		//ref标记，由IM升级为ref只需要设置此flag，反过来，降级至需要去掉此flag，并不影响nsvid
		TYPE_REF_FLAG = 1,     //
		TYPE_REAL_IM = 2,     //实数，立即数
		TYPE_STR_IM = 4,   //非$得文本型
		TYPE_ARRAY_FLAG = 8,   //?数组操作标记，非常有用，与ref标记结合表示数组引用，单标记用于RPN计算
		TYPE_INT_REF = 3,     //%
		TYPE_STR_REF = 5,     //$
		TYPE_ARRAY_REF = 9,   //?0[X]
		TYPE_OPERATOR = 16,     //+ - * /
		//===逻辑运算符====，高8位使用
		LOGIC_BIGTHAN = 0x100,  // >
		LOGIC_BIGANDEQUAL = 0x200, // >=
		LOGIC_LESS = 0x300,  // <
		LOGIC_LESSANDEQUAL = 0x400,  // <=
		LOGIC_EQUAL = 0x500,
		LOGIC_UNEQUAL = 0x600,
		LOGIC_AND = 0x700,
		LOGIC_OR = 0x800,
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

	//语法
	enum {
		YF_Int = 1,
		YF_IntRef = 2,
		YF_Str = 4,
		YF_StrRef = 8,
		YF_Array = 0x10,
		YF_Value = 0x1F,
		YF_Oper =  0x20,
		YF_Left_SQ = 0x40,  //左方括号
		YF_Right_SQ = 0x80,  //右方括号
		YF_Left_PA = 0x100,  //左圆括号
		YF_Right_PA = 0x200, //右圆括号
		YF_Alias = 0x400,    //别名，如果存在的话
		YF_Error = 0x800,
		YF_Negative = 0x1000, //负数
		YF_Break = 0x2000,
	};

	ONSVariableRef();
	ONSVariableRef(ONSVAR_TYPE t, int idd);
	ONSVariableRef(double v);
	ONSVariableRef(LOString *s);

	~ONSVariableRef();
	
	void UpImToRef(int tp);
	void DownRefToIm(int tp);

	//void SetRef(int tp, int id);   //设置引用模式
	void SetImVal(double v);
	void SetImVal(LOString *v);
	void SetValue(ONSVariableRef *ref);
	void SetValue(double v);
	void SetValue(LOString *s);
	void SetValue(LOString &s);
	void SetValue(LOCodePage *encode,const char *buf, int len);
	void SetAutoVal(double v);
	bool SetArryVals(int *v, int count);
	void SetAutoVal(LOString *s);
	void SetArrayFlag();
	int  SetOperator(const char *buf);

	void InitArrayIndex();
	void DimArray();
	void SetArrayIndex(int val,int index);
	int  GetArrayIndex(int index);
	int  GetTypeRefid();
	int  GetOrder() { return order; }
	int  GetOperator() { return oper; }
	int  GetNSid() { return nsvID; }
	ONSVAR_TYPE  GetType() { return (ONSVAR_TYPE)vtype; }

	static int  GetTypeAllow(const char *param, bool &mustRef);
	static int  GetYFtype(const char *buf, bool isfirst);
	static int  GetYFnextAllow(int cur);
	static ONSVariableRef* GetRefFromTypeRefid(int refid);
	LOString *GetStr();
	double GetReal();

	
	bool isOperator() { return (vtype == TYPE_OPERATOR); }
	bool isStr() { return (vtype & TYPE_STR_IM); }
	bool isReal() { return (vtype & TYPE_REAL_IM ); }
	bool isRef() { return (vtype & TYPE_REF_FLAG); }
	bool isIntRef(){ return vtype == TYPE_INT_REF; }
	bool isStrRef() { return vtype == TYPE_STR_REF; }
	bool isAllowType(int allow) { return vtype & allow; };
	bool isArrayFlag() { return vtype == TYPE_ARRAY_FLAG; }
	bool isArrayRef() { return vtype == TYPE_ARRAY_REF; }
	bool isNone() { return vtype == TYPE_NONE; }

	static bool isYFoperator(int yftype);
	//获取操作符需要的操作数
	int GetOpCount() {
		if (oper == '$' || oper == '%' || oper == '?') return 1;
		else return 2;
	}

	bool Calculator(ONSVariableRef *v, char op, bool isreal);
	int Compare(ONSVariableRef *v2, int comtype, bool isreal);
	void CopyFrom(ONSVariableRef *v);
private:
	union ONSVARTIAN{
		LOString *strPtr;
		int *arrayIndex;
		double real;
	};

	int16_t vtype;   //ref的类型，只会同时存在一种
	int16_t nsvID;   //ref操作的编号
	char oper;       //符号
	char order;      //顺序
	ONSVARTIAN data;
	LOString *tm_out;

	void BaseInit();
	void FreeData();
	LOString *IntToStr(int v);
};
typedef std::unique_ptr<ONSVariableRef> LOUniqVariableRef;


//放在全局变量，省得在类里面传来传去
extern ONSVariableBase GNSvariable[MAXVARIABLE_COUNT];  

#endif // !__ONSVARIABLE_H__
