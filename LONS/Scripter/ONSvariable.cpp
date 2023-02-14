#include "ONSvariable.h"

extern void FatalError(const char *fmt, ...);

ONSVariableBase GNSvariable[MAXVARIABLE_COUNT];

ONSVariableBase::ONSVariableBase(){
	BaseInit();
}

ONSVariableBase::~ONSVariableBase(){
	if (arrayData) delete[] arrayData;
	if (strValue) delete strValue;
}

void ONSVariableBase::BaseInit() {
	value = 0.0;
	strValue = NULL;
	arrayData = NULL;
	isLimit = false;
	limitMax = 100;
	limitMin = 0;
}


void ONSVariableBase::DimArray(int *index) {
	if (arrayData) delete[] arrayData;
	int sum = 1;
	for (int ii = 0; ii < MAXVARIABLE_ARRAY && index[ii] >= 0; ii++) {
		sum *= (index[ii] + 1);
	}
	arrayData = new int[sum + MAXVARIABLE_ARRAY];
	memcpy(arrayData, index, sizeof(int) * MAXVARIABLE_ARRAY);
	int *arrayPtr = arrayData + MAXVARIABLE_ARRAY;
	memset(arrayPtr, 0, sizeof(int) * sum );
}

//检查数组操作参数是否正确
const char *ONSVariableBase::CheckAarryIndex(int *index) {
	if (!arrayData) return "Array undefined!";
	if (!index) return "Invalid array operands is null!";
	if (ArrayDimension(index) != ArrayDimension(arrayData)) return "Array operand dimension error!";
	if (index[0] < 0) return "Array operand dimension error!";
	for (int ii = 0; ii < MAXVARIABLE_ARRAY && index[ii] >= 0; ii++) {
		if (index[ii] > arrayData[ii] ) return "Array operands out of range!";
	}
	return NULL;
}

//数组操作数转换为绝对位置
int ONSVariableBase::ArrayIndexPosition(int *index) {
	int sumlen = 1;
	for (int ii = 0; ii < MAXVARIABLE_ARRAY && arrayData[ii] >= 0; ii++) sumlen *= (arrayData[ii] + 1);
	int pos = 0;
	for (int ii = 0; ii < MAXVARIABLE_ARRAY && index[ii] >= 0; ii++) {
		sumlen = sumlen / (arrayData[ii] + 1); //剩下的部分
		pos = pos + index[ii] * sumlen;
	}
	return pos;
}

//取得数组的维数
int ONSVariableBase::ArrayDimension(int *index) {
	if (!index) index = arrayData;
	int ii;
	for (ii = 0; ii < MAXVARIABLE_ARRAY && index[ii] >= 0; ii++);
	return ii;
}

void ONSVariableBase::SetValue(double v) {
	if (isLimit) {
		if (v > limitMax) v = limitMax;
		else if (v < limitMin) v = limitMin;
	}
	value = v;
}

//必须已经进行CheckAarryIndex检查
void ONSVariableBase::SetArrayValue(int *index, int val) {
	int position = ArrayIndexPosition(index);
	int *arrayPtr = arrayData + MAXVARIABLE_ARRAY;
	arrayPtr[position] = val;
}

//必须已经进行CheckAarryIndex检查
int ONSVariableBase::GetArrayValue(int *index) {
	int position = ArrayIndexPosition(index);
	int *arrayPtr = arrayData + MAXVARIABLE_ARRAY;
	return arrayPtr[position];
}

double ONSVariableBase::GetValue() {
	return value;
}

void ONSVariableBase::SetLimit(int min, int max) {
	isLimit = true;
	limitMin = min;
	limitMax = max;
	SetValue(GetValue());
}


void ONSVariableBase::SetString(LOString *s) {
	SetStrCore(strValue, s);
}

LOString *ONSVariableBase::GetString() {
	return strValue;
}

void ONSVariableBase::SetStrCore(LOString *&dst, LOString *s) {
	if (dst) delete dst;
	dst = NULL;
	if(s && s->length() > 0)dst = new LOString(*s);
}

void ONSVariableBase::AppendStrCore(LOString *&dst, LOString *s) {
	if (!s) return;
	if (!dst) dst = new LOString(*s);
	else dst->append(*s);
}


void ONSVariableBase::ResetAll() {
	for (int ii = 0; ii < MAXVARIABLE_COUNT; ii++) {
		GNSvariable[ii].value = 0;
		if (GNSvariable[ii].strValue) delete GNSvariable[ii].strValue;
		GNSvariable[ii].strValue = nullptr;
		if (GNSvariable[ii].arrayData) delete[] GNSvariable[ii].arrayData;
		GNSvariable[ii].arrayData = nullptr;
		GNSvariable[ii].isLimit = false;
	}
}

ONSVariableBase *ONSVariableBase::GetVariable(int id) {
	if (id < 0 || id >= MAXVARIABLE_COUNT) {
		FatalError("%s%d", "ONSVariableBase::GetVariable() out of range:", id);
		return nullptr;
	}
	return &GNSvariable[id];
}

int ONSVariableBase::StrToIntSafe(LOString *s) {
	if (!s) return 0;
	const char* buf = s->c_str();
	int sum = 0;
	while (buf[0] != 0) {
		if (buf[0] >= '0' && buf[0] <= '9') sum = sum * 10 + (buf[0] - '0');
		else return 0;
		buf++;
	}
	return sum;
}

//返回数组的字节长度，没定义返回0，不包括索引需要的字节数
int ONSVariableBase::ArrayCount() {
	if (!arrayData) return 0;
	int sum = 1;  //数组的成员总数
	for (int ii = 0; ii < MAXVARIABLE_ARRAY && arrayData[ii] >= 0; ii++) {
		sum *= (arrayData[ii] + 1);
	}
	return sum;
}


void ONSVariableBase::SaveOnsVar(BinArray *bin, int from, int count) {
	int len = bin->WriteLpksEntity("GVAR", 0, 1);
	//起点
	bin->WriteInt(from);
	//数量
	bin->WriteInt(count);
	for (int ii = from; ii < count; ii++) {
		GNSvariable[ii].Serialization(bin, ii);
	}
	bin->WriteInt(bin->Length() - len, &len);
}


bool ONSVariableBase::LoadOnsVar(BinArray *bin, int *pos) {
	int next = -1;
	if(!bin->CheckEntity("GVAR", &next, nullptr, pos)) return false;
	int from = bin->GetIntAuto(pos);
	int count = bin->GetIntAuto(pos);
	for (int ii = from; ii < count; ii++) {
		if(!GNSvariable[ii].LoadDeserialize(bin, pos)) return false;
	}

	*pos = next;
	return true;
}


//序列化
void ONSVariableBase::Serialization(BinArray *bin, int vid) {
	//需要优化存储的大小，毕竟有好几千个变量需要存储
	bin->WriteInt16(0x6176); //'va'
	//使用1个字节来标识是否有 value、string和array
	char f = 0;
	int count = ArrayCount();
	if (*(int64_t*)(&value) != 0) f |= SAVE_HAS_VALUE;
	if (strValue && strValue->length() > 0) f |= SAVE_HAS_STRING;
	if (count > 0) f |= SAVE_HAS_ARRAY;

	bin->WriteChar(f);
	if(f & SAVE_HAS_VALUE) bin->WriteDouble(value);
	if(f & SAVE_HAS_STRING) bin->WriteLOString(strValue);
	if(f & SAVE_HAS_ARRAY) {
		bin->WriteInt(count);
		count += MAXVARIABLE_ARRAY;
		bin->Append((char*)arrayData, count * 4);
	}
}


//load的反序列化是选择性的
bool ONSVariableBase::LoadDeserialize(BinArray *bin, int *pos) {
	if (bin->GetInt16Auto(pos) != 0x6176) return false;
	//首先重置变量，limit特性并不会随着load而改变
	if (strValue) delete strValue;
	strValue = nullptr;
	value = 0.0;
	//可能有define的数组大小跟存档不一致的情况
	if (ArrayCount() > 0) memset(arrayData + MAXVARIABLE_ARRAY, 0, ArrayCount() * 4);

	char f = bin->GetChar(pos);
	if (f & SAVE_HAS_VALUE) value = bin->GetDoubleAuto(pos);
	if (f & SAVE_HAS_STRING) strValue = bin->GetLOStrPtr(pos);
	if (f & SAVE_HAS_ARRAY) {
		int count = bin->GetIntAuto(pos);
		//现状数组大小不足的情况
		if (ArrayCount() < count) {
			delete[] arrayData;
			arrayData = new int[count + MAXVARIABLE_ARRAY];
		}
		bin->GetArrayAuto(arrayData, count + MAXVARIABLE_ARRAY, 4, pos);
	}
	return true;
}

//反序列化
//bool ONSVariableBase::Deserialization(BinArray *bin, int *pos) {
//	if (bin->GetInt16Auto(pos) != 0x6176) return false;
//
//	if (strValue) delete strValue;
//	if (arrayData) delete[] arrayData;
//	value = 0.0;
//	strValue = nullptr;
//	arrayData = nullptr;
//
//	char f = bin->GetChar(pos);
//	if (f & SAVE_HAS_VALUE) value = bin->GetDoubleAuto(pos);
//	if (f & SAVE_HAS_STRING) strValue = bin->GetLOStrPtr(pos);
//	if (f & SAVE_HAS_ARRAY) {
//		int count = bin->GetIntAuto(pos);
//		count += MAXVARIABLE_ARRAY;
//		arrayData = new int[count];
//		bin->GetArrayAuto(arrayData, count, 4, pos);
//	}
//	return true;
//}


//====================== Ref =============

ONSVariableRef::ONSVariableRef() {
	BaseInit();
}

ONSVariableRef::ONSVariableRef(ONSVAR_TYPE t, int idd) {
	BaseInit();
	vtype = t;
	if (t == TYPE_INT || t == TYPE_ARRAY || t == TYPE_STRING) nsvID = idd;
	else data.real = (double)idd;
}

ONSVariableRef::ONSVariableRef(double v) {
	BaseInit();
	vtype = TYPE_REAL;
	data.real = v;
}

ONSVariableRef::ONSVariableRef(LOString *s) {
	BaseInit();
	vtype = TYPE_STRING_IM;
	ONSVariableBase::SetStrCore(data.strPtr, s);
}


void ONSVariableRef::BaseInit() {
	tm_out = nullptr;
	vtype = TYPE_NONE;
	oper = 0;
	order = 0;
	memset(&data, 0, sizeof(data));
}

void ONSVariableRef::FreeData() {
	if (vtype == TYPE_ARRAY) delete[] data.arrayIndex;
	if (vtype == TYPE_STRING_IM) delete data.strPtr;
	memset(&data, 0, sizeof(data));
}

ONSVariableRef::~ONSVariableRef() {
	FreeData();
	if (tm_out) delete tm_out;
}

//初始化数组
void ONSVariableRef::InitArrayIndex() {
	FreeData();
	data.arrayIndex = new int[MAXVARIABLE_ARRAY];
	for (int ii = 0; ii < MAXVARIABLE_ARRAY; ii++) data.arrayIndex[ii] = -1;
}

void ONSVariableRef::SetArrayIndex(int val, int index) {
	if (vtype != TYPE_ARRAY || !data.arrayIndex) return;
	data.arrayIndex[index] = val;
}

int  ONSVariableRef::GetArrayIndex(int index) {
	if (vtype != TYPE_ARRAY || !data.arrayIndex || index < 0 || index > MAXVARIABLE_ARRAY) return -1;
	return data.arrayIndex[index];
}

void ONSVariableRef::DimArray() {
	if(vtype != TYPE_ARRAY || !data.arrayIndex) FatalError("%s","ONSVariableRef::DimArray() null arrayIndex!");
	ONSVariableBase *nsv = ONSVariableBase::GetVariable(nsvID);
	nsv->DimArray(data.arrayIndex);
}

//将立即数升级为变量引用，参数为要改变为的引用类型 数字变量、文字变量或数组变量
void ONSVariableRef::UpImToRef(int tp) {
	if (tp != TYPE_INT && tp != TYPE_STRING && tp != TYPE_ARRAY) FatalError("%s", "ONSVariableRef::UpImToRef() error type!");
	nsvID = (int16_t)GetReal();
	FreeData();
	vtype = (ONSVAR_TYPE)tp;
}

//将nsv的值复制到本地
void ONSVariableRef::DownRefToIm(int tp) {
	if (!isStr() && !isReal()) {
		FatalError("%s", "ONSVariableRef::DownRefToIm() error type!");
		return;
	}
	if (tp == TYPE_REAL) SetImVal(GetReal());
	else if (tp == TYPE_STRING_IM) {
		//不需要设置自身 = 自身
		if (vtype != TYPE_STRING_IM) SetImVal(GetStr());
	}
	else FatalError("%s","ONSVariableRef::DownRefToIm() error type!");
}

void ONSVariableRef::SetRef(int tp, int id) {
	if(tp != TYPE_INT && tp != TYPE_STRING && tp != TYPE_ARRAY) FatalError("%s","ONSVariableRef::SetRef() error type!");
	FreeData();
	vtype = tp;
	nsvID = id;
}

void ONSVariableRef::SetImVal(double v) {
	FreeData();
	vtype = TYPE_REAL;
	data.real = v;
}

void ONSVariableRef::SetImVal(LOString *v) {
	FreeData();
	vtype = TYPE_STRING_IM;
	ONSVariableBase::SetStrCore(data.strPtr, v);
}

void ONSVariableRef::SetValue(double v) {
	const char *errinfo = NULL;
	ONSVariableBase *nsv = nullptr;
	switch (vtype){
	case TYPE_INT:
		nsv = ONSVariableBase::GetVariable(nsvID);
		nsv->SetValue(v);
		break;
	case TYPE_ARRAY:
		nsv = ONSVariableBase::GetVariable(nsvID);
		errinfo = nsv->CheckAarryIndex(data.arrayIndex);
		if (errinfo) {
			FatalError("%s%s", "ONSVariableRef::SetValue(double v) array faild:" , errinfo);
			return;
		}
		nsv->SetArrayValue(data.arrayIndex, (int)v);
		break;
	case TYPE_REAL:
		data.real = v;
		break;
	default:
		FatalError("%s", "ONSVariableRef::SetValue(double v) unsupported assignment type!");
		return;
		break;
	}
}

void ONSVariableRef::SetValue(LOString *s) {
	const char *errinfo = NULL;
	ONSVariableBase *nsv = nullptr;
	switch (vtype){
	case TYPE_STRING:
		nsv = ONSVariableBase::GetVariable(nsvID);
		nsv->SetString(s);
		break;
	case TYPE_STRING_IM:
		ONSVariableBase::SetStrCore(data.strPtr, s);
		break;
	default:
		FatalError("%s","ONSVariableRef::SetValue(LOString *s) unsupported assignment type!");
		break;
	}
}

void ONSVariableRef::SetValue(LOString &s) {
	SetValue(&s);
}

void ONSVariableRef::SetValue(LOCodePage *encode, const char *buf, int len) {
	LOString s1(buf, len);
	s1.SetEncoder(encode);
	SetValue(&s1);
}

void ONSVariableRef::SetValue(ONSVariableRef *ref) {
	if (!ref) return;
	switch (vtype)
	{
	case TYPE_INT: case TYPE_ARRAY: case TYPE_REAL:
		SetValue(ref->GetReal());
		break;
	case TYPE_STRING: case TYPE_STRING_IM:
		SetValue(ref->GetStr());
		break;
	default:
		FatalError("%s","ONSVariableRef::SetValue(ONSVariableRef *ref) unsupported assignment type!");
		break;
	}
}

//如果自身为文字类则自动转换为文字
void ONSVariableRef::SetAutoVal(double v) {
	if (isReal()) SetValue(v);
	else if (isStr()) {
		int ret = (int)v;
		LOString s = std::to_string(ret);
		SetValue(&s);
	}
	else FatalError("%s","ONSVariableRef::SetAutoReal(double v) unsupported type!");
}

//如果自身为整数则自动转换为整数
void ONSVariableRef::SetAutoVal(LOString *s) {
	if (isStr()) SetValue(s);
	else if (isReal()) {
		if (s) {
			int ret = ONSVariableBase::StrToIntSafe(s);
			SetValue((double)ret);
		}
		else SetValue(0.0);
	}
	else FatalError("%s","ONSVariableRef::SetAutoReal(LOString *s) unsupported type!");
}


void ONSVariableRef::SetArrayFlag() {
	FreeData();
	vtype = TYPE_ARRAY_FLAG;
}

double ONSVariableRef::GetReal() {
	const char *errinfo = NULL;
	ONSVariableBase *nsv = nullptr;
	switch (vtype) {
	case TYPE_INT:
		nsv = ONSVariableBase::GetVariable(nsvID);
		return nsv->GetValue();
	case TYPE_STRING:
		nsv = ONSVariableBase::GetVariable(nsvID);
		return ONSVariableBase::StrToIntSafe(nsv->GetString());
	case TYPE_STRING_IM:
		return ONSVariableBase::StrToIntSafe(data.strPtr);
	case TYPE_REAL:
		return data.real;
	case TYPE_ARRAY:
		nsv = ONSVariableBase::GetVariable(nsvID);
		errinfo = nsv->CheckAarryIndex(data.arrayIndex);
		if (errinfo) {
			FatalError("%s%s", "ONSVariableRef::GetReal() array faild:", errinfo);
			return -1.0;
		}
		return nsv->GetArrayValue(data.arrayIndex);
	default:
		FatalError("ONSVariableRef::GetReal() Unsupported assignment type!");
		break;
	}
	return -1.0;
}


LOString *ONSVariableRef::IntToStr(int v) {
	if (tm_out) delete tm_out;
	tm_out = new LOString(std::to_string(v));
	return tm_out;
}

//非文字变量会尝试转换为文字
LOString *ONSVariableRef::GetStr() {
	const char *errinfo = NULL;
	ONSVariableBase *nsv = nullptr;
	switch (vtype) {
	case TYPE_INT:
		nsv = ONSVariableBase::GetVariable(nsvID);
		return IntToStr((int)nsv->GetValue());
	case TYPE_STRING:
		nsv = ONSVariableBase::GetVariable(nsvID);
		return nsv->GetString();
	case TYPE_STRING_IM:
		return data.strPtr;
	case TYPE_REAL:
		return IntToStr((int)data.real);
	case TYPE_ARRAY:
		nsv = ONSVariableBase::GetVariable(nsvID);
		errinfo = nsv->CheckAarryIndex(data.arrayIndex);
		if (errinfo) {
			LOString errs = "ONSVariableRef::GetStr() array faild:";
			errs.append(errinfo);
			FatalError(errs.c_str());
			return nullptr;
		}
		return IntToStr(nsv->GetArrayValue(data.arrayIndex));
	default:
		FatalError("ONSVariableRef::GetStr() Unsupported assignment type!");
		break;
	}
	return NULL;
}

//返回的是获取的符号长度
int ONSVariableRef::SetOperator(const char *buf) {
	int ret = 0;
	FreeData();
	oper = buf[0];
	vtype = TYPE_OPERATOR;

	switch (buf[0])
	{
	case '+':case '-':
		order = ORDER_ADDSUB;
		break;
	case '*':case '/':
		order = ORDER_MULDIRMOD;
		break;
	case '?':
		order = ORDER_GETVAR;
		break;
	case '$':case '%':
		order = ORDER_GETVAR;
		break;
	case '[':case ']':
		order = ORDER_SQBRA;
		break;
	case '(':case ')':
		order = ORDER_ROUNDBRA;
		break;
	case '^':
		order = ORDER_SQU;
		break;
	case 'm': case 'M':
		if (tolower(buf[1]) == 'o' && tolower(buf[2]) == 'd' && 
			(tolower(buf[3]) < 'a' || tolower(buf[3]) > 'z'))   {
			oper = '~';
			order = ORDER_MULDIRMOD;
		}
		else vtype = TYPE_NONE;
		break;
	default:
		vtype = TYPE_NONE;
		break;
	}

	if (vtype == TYPE_NONE) return 0;
	else if (oper == '~') return 3; //mod
	return 1;
}

//进过计算，ref都会编程im_，除非遇到 ? % $ 这种变量操作代表
bool ONSVariableRef::Calculator(ONSVariableRef *v, char op, bool isreal) {
	if (!v) return false;
	if (op == '+' && isStr()) {
		//must be str_im，注意im的时候getstr()会造成错误
		if (vtype != TYPE_STRING_IM) SetImVal(GetStr());
		ONSVariableBase::AppendStrCore(data.strPtr, v->GetStr());
		return true;
	}
	else {
		if (!isReal()) return false;
		double dst = GetReal();      //注意必须先获取值再改类型，不然会出现错误的结果
		double src = v->GetReal();
		int itmp;
		//转为整数计算
		if (isreal) {
			itmp = dst; dst = (double)itmp;
			itmp = src; src = (double)itmp;
		}

		switch (op)
		{
		case '+':
			SetImVal(dst + src);
			break;
		case '-':
			SetImVal(dst - src);
			break;
		case '*':
			SetImVal(dst * src);
			break;
		case '/':
			SetImVal(dst / src);
			break;
		case '~':
			SetImVal((int)dst % (int)src);
			break;
		case '^':
			SetImVal(pow(dst, src));
			break;
		default:
			return false;
		}
	}

	return true;
}

int ONSVariableRef::Compare(ONSVariableRef *v2, int comtype, bool isreal) {
	//类型不同，直接不相等
	if ((isReal() && !v2->isReal()) || (isStr() && !v2->isStr())) return 0;
	if (isStr()) {
		LOString *s1 = GetStr();
		LOString *s2 = v2->GetStr();
		bool equalfalg = false;
		if (!s1 || s1->length() == 0) equalfalg = (!s2 || s2->length() == 0);
		else if (!s2 || s2->length() == 0) equalfalg = false;
		else equalfalg = (s1->compare(*s2) == 0);
		//result
		if (comtype == LOGIC_EQUAL) return equalfalg;
		else if (comtype == LOGIC_UNEQUAL) return !equalfalg;
		else return 0;
	}
	else if (!isreal) {  //ons只支持整数比较
		int r1 = GetReal();
		int r2 = v2->GetReal();
		if (comtype == LOGIC_EQUAL) {  //浮点数都有精度问题，这里取6位小数
			if (r1 == r2) return 1;
			else return 0;
		}
		else if (comtype == LOGIC_UNEQUAL) {
			if (r1 != r2) return 1;
			else return 0;
		}
		else if (comtype == LOGIC_BIGANDEQUAL) { //大于等于
			if (r1 >= r2)return 1;
			else return 0;
		}
		else if (comtype == LOGIC_BIGTHAN) { //大于，注意考虑相等的误差
			if (r1 > r2) return 1;
			else return 0;
		}
		else if (comtype == LOGIC_LESSANDEQUAL) { //小于等于
			if (r1 <= r2)return 1;
			return 0;
		}
		else if (comtype == LOGIC_LESS) { //小于，注意考虑相等的误差
			if (r1 < r2) return 1;
			else return 0;
		}
	}
	else { //实数比较
		double r1 = GetReal();
		double r2 = v2->GetReal();
		if (comtype == LOGIC_EQUAL) {  //浮点数都有精度问题，这里取6位小数
			if (fabs(r1 - r2) < 0.000001) return 1;
			else return 0;
		}
		else if (comtype == LOGIC_UNEQUAL) {
			if (fabs(r1 - r2) > 0.000001) return 1;
			else return 0;
		}
		else if (comtype == LOGIC_BIGANDEQUAL) { //大于等于
			if (r1 > r2 || fabs(r1 - r2) < 0.000001)return 1;
			else return 0;
		}
		else if (comtype == LOGIC_BIGTHAN) { //大于，注意考虑相等的误差
			if (r1 > r2 && fabs(r1 - r2) > 0.000001) return 1;
			else return 0;
		}
		else if (comtype == LOGIC_LESSANDEQUAL) { //小于等于
			if (r1 < r2 || fabs(r1 - r2) < 0.000001)return 1;
			return 0;
		}
		else if (comtype == LOGIC_LESS) { //小于，注意考虑相等的误差
			if (r1 < r2 && fabs(r1 - r2) > 0.000001) return 1;
			else return 0;
		}
	}
	return 0;  //不能识别的都返回假
}


void ONSVariableRef::CopyFrom(ONSVariableRef *v) {
	FreeData();
	vtype = v->vtype;
	nsvID = v->nsvID;
	if(vtype == TYPE_STRING_IM) ONSVariableBase::SetStrCore(data.strPtr, v->data.strPtr);
	else if (vtype == TYPE_ARRAY) {
		InitArrayIndex();
		for (int ii = 0; ii < MAXVARIABLE_ARRAY; ii++) data.arrayIndex[ii] = v->data.arrayIndex[ii];
	}
}

bool ONSVariableRef::SetArryVals(int *v, int count) {
	ONSVariableBase *nsv = ONSVariableBase::GetVariable(nsvID);
	int dime = nsv->ArrayDimension(data.arrayIndex);
	if (!data.arrayIndex || (nsv->ArrayDimension(nullptr) != dime + 1) || dime > MAXVARIABLE_ARRAY - 1) {
		FatalError("SetArryVals() dimension error!");
		return false;
	}

	for (int ii = 0; ii < count; ii++) {
		data.arrayIndex[dime] = ii;
		nsv->SetArrayValue(data.arrayIndex, *(v + ii));
	}
	data.arrayIndex[dime] = -1;
	return true;
}


//获取type和操作ID的组合数
int  ONSVariableRef::GetTypeRefid() {
	int v = vtype;
	v <<= 16;
	v |= nsvID;
	return v;
}

ONSVariableRef* ONSVariableRef::GetRefFromTypeRefid(int refid) {
	int t = (refid >> 16) & 0xffff;
	int id = refid & 0xffff;
	return new ONSVariableRef( (ONSVariableRef::ONSVAR_TYPE)t, id);
}

//R-ref, I-realref, S-strref, N-normal word, L-label, A-arrayref, C-color
//r-any, i-real, s-string, *-repeat, #-repeat last
int  ONSVariableRef::GetTypeAllow(const char *param) {
	int ret = 0;
	for (int ii = 0; param[ii] != ',' && param[ii] != 0; ii++) {
		switch (param[ii]) {
		case 'R':
			ret |= (TYPE_INT | TYPE_ARRAY | TYPE_STRING); break;
		case 'I':
			ret |= (TYPE_INT | TYPE_ARRAY); break;
		case 'S':
			ret |= TYPE_STRING; break;
		case 'A':
			ret |= TYPE_ARRAY; break;
		case 'i':
			ret |= (TYPE_INT | TYPE_REAL | TYPE_ARRAY); break;
		case 's':
			ret |= (TYPE_STRING | TYPE_STRING_IM); break;
		case 'r':
			ret |= (TYPE_INT | TYPE_ARRAY | TYPE_STRING | TYPE_REAL | TYPE_STRING_IM); break;
		default:
			break;
		}
	}
	return ret;
}


//获取语法类型
int ONSVariableRef::GetYFtype(const char *buf, bool isfirst) {
	int ctype = LOString::GetCharacter(buf);
	if (ctype == LOCodePage::CHARACTER_NUMBER) return YF_Int;
	else if (ctype == LOCodePage::CHARACTER_SYMBOL) {
		if (isfirst && buf[0] == '-') { //检测负数
			if (LOString::GetCharacter(buf + 1) == LOCodePage::CHARACTER_NUMBER) return YF_Negative;
		}
		if (buf[0] == '+' || buf[0] == '-' || buf[0] == '*' || buf[0] == '/' || buf[0] == '^') return YF_Oper;
		if (buf[0] == '"') return YF_Str;
		if (buf[0] == '%') return YF_IntRef;
		if (buf[0] == '$') return YF_StrRef;
		if (buf[0] == '(') return YF_Left_PA;
		if (buf[0] == ')') return YF_Right_PA;
		if (buf[0] == '[') return YF_Left_SQ;
		if (buf[0] == ']') return YF_Right_SQ;
		if (buf[0] == '?') return YF_Array;
		if ((buf[0] == 'm' || buf[0] == 'M') && (buf[0] == 'o' || buf[0] == 'O') && (buf[0] == 'd' || buf[0] == 'D')) return YF_Oper;
		if (buf[0] == '\n' || buf[0] == ',' || buf[0] == ':') return YF_Break;
		return YF_Error;
	}

	return YF_Alias;
}


bool ONSVariableRef::isYFoperator(int yftype) {
	int flag = YF_Oper | YF_IntRef | YF_StrRef;
	return flag;
}


//获取下一个语法允许的类型
int  ONSVariableRef::GetYFnextAllow(int cur) {
	switch (cur){
	case 0:
		return YF_Value | YF_Negative;
	case YF_Int:  //{+-*/^%} ] ) 0[
		return YF_Oper | YF_Right_PA | YF_Right_SQ | YF_Left_SQ;
	case YF_IntRef: // % -> %%, %(, %?0[], %num
		return YF_IntRef | YF_Left_PA | YF_Array| YF_Int;
	case YF_Str: // "" -> "" + 
		return YF_Oper;
	case YF_StrRef: //$ -> $(, $%, $num
		return YF_Left_PA | YF_IntRef | YF_Int;
	case YF_Array: // ? -> ?num, ?%, ?(
		return YF_Int | YF_IntRef | YF_Left_PA;
	case YF_Oper:  // + -> +val, +(
		return YF_Value | YF_Left_PA;
	case YF_Left_SQ: // [%num], [?num]
		return YF_Int | YF_IntRef | YF_Array;
	case YF_Right_SQ: //] -> ][, ]+
		return YF_Left_SQ | YF_Oper;
	case YF_Left_PA: //( -> (first  //左括号相当于重新开始
		return YF_Value | YF_Negative;
	case YF_Right_PA: //) --> )+
		return YF_Oper;
	default:
		break;
	}
	return YF_Error;
}

