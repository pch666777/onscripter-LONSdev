#include "ONSvariable.h"

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
		SimpleError("%s%d", "ONSVariableBase::GetVariable() out of range:", id);
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


//序列化
void ONSVariableBase::Serialization(BinArray *bin) {
	bin->WriteDouble(value);         //value
	bin->WriteLOString(strValue);      //string
	int count = ArrayCount();
	bin->WriteInt(count);  //array count
	if (count > 0) {       //array data
		count += MAXVARIABLE_ARRAY;
		bin->Append((char*)arrayData, count * 4);
	}
}

//反序列化
bool ONSVariableBase::Deserialization(BinArray *bin, int *pos) {
	value = bin->GetDouble(pos);
	if (strValue) delete strValue;
	strValue = bin->GetLOString(pos);

	if (arrayData) delete[] arrayData;
	int count = bin->GetInt(pos);
	if (count > 0) {
		count += MAXVARIABLE_ARRAY;
		arrayData = new int[count];
		memcpy(arrayData, bin->bin + (*pos), count * 4);
		pos += count * 4;
	}
	else arrayData = nullptr;
	return true;
}


//====================== Ref =============

ONSVariableRef::ONSVariableRef() {
	BaseInit();
}

ONSVariableRef::ONSVariableRef(ONSVAR_TYPE t, int idd) {
	BaseInit();
	vtype = t;
	if (t == TYPE_INT || t == TYPE_ARRAY || t == TYPE_STRING) nsvId = idd;
	else im_val = idd;
}

ONSVariableRef::ONSVariableRef(ONSVAR_TYPE t) {
	BaseInit();
	vtype = t;
}

void ONSVariableRef::BaseInit() {
	vtype = TYPE_NONE;
	order = ORDER_NONE;
	oper = '\0';
	nsvId = -1;
	nsv = NULL;
	im_tmpout = NULL;
	im_str = NULL;
	arrayIndex = NULL ;
}

ONSVariableRef::~ONSVariableRef() {
	if (arrayIndex) delete[] arrayIndex;
	if (im_str) delete im_str;
	if (im_tmpout) delete im_tmpout;
	arrayIndex = NULL;
	im_str = NULL;
	im_tmpout = NULL;
}

void ONSVariableRef::InitArrayIndex() {
	if (arrayIndex) delete[] arrayIndex;
	arrayIndex = new int[MAXVARIABLE_ARRAY];
	for (int ii = 0; ii < MAXVARIABLE_ARRAY; ii++) arrayIndex[ii] = -1;
}

void ONSVariableRef::SetArrayIndex(int val, int index) {
	arrayIndex[index] = val;
}

int  ONSVariableRef::GetArrayIndex(int index) {
	if (!arrayIndex || index < 0 || index > MAXVARIABLE_ARRAY) return -1;
	return arrayIndex[index];
}

void ONSVariableRef::DimArray() {
	if(!arrayIndex) SimpleError("%s","ONSVariableRef::DimArray() null arrayIndex!");
	nsv = ONSVariableBase::GetVariable(nsvId);
	nsv->DimArray(arrayIndex);
}

//将立即数升级为变量引用，参数为要改变为的引用类型 数字变量、文字变量或数组变量
void ONSVariableRef::UpImToRef(int tp) {
	if (tp != TYPE_INT && tp != TYPE_STRING && tp != TYPE_ARRAY) SimpleError("%s", "ONSVariableRef::UpImToRef() error type!");
	nsvId = (int)GetReal();
	vtype = (ONSVAR_TYPE)tp;
	nsv = ONSVariableBase::GetVariable(nsvId);
}

//将nsv的值复制到本地
void ONSVariableRef::DownRefToIm(int tp) {
	if(!isStr() && !isReal()) SimpleError("%s","ONSVariableRef::DownRefToIm() error type!");
	if (tp == TYPE_REAL) {
		im_val = GetReal();
		vtype = TYPE_REAL;
	}
	else if (tp == TYPE_STRING_IM) {
		if (vtype != TYPE_STRING_IM) ONSVariableBase::SetStrCore(im_str, GetStr());
		vtype = TYPE_STRING_IM;
	}
	else SimpleError("%s","ONSVariableRef::DownRefToIm() error type!");
}

void ONSVariableRef::SetRef(int tp, int id) {
	if(tp != TYPE_INT && tp != TYPE_STRING && tp != TYPE_ARRAY) SimpleError("%s","ONSVariableRef::SetRef() error type!");
	vtype = (ONSVAR_TYPE)tp;
	nsvId = id;
}

void ONSVariableRef::SetImVal(double v) {
	vtype = TYPE_REAL;
	im_val = v;
}

void ONSVariableRef::SetImVal(LOString *v) {
	vtype = TYPE_STRING_IM;
	ONSVariableBase::SetStrCore(im_str, v);
}

void ONSVariableRef::SetValue(double v) {
	const char *errinfo = NULL;
	switch (vtype)
	{
	case TYPE_INT:
		nsv = ONSVariableBase::GetVariable(nsvId);
		nsv->SetValue(v);
		break;
	case TYPE_ARRAY:
		nsv = ONSVariableBase::GetVariable(nsvId);
		errinfo = nsv->CheckAarryIndex(arrayIndex);
		if (errinfo) {
			SimpleError("%s%s", "ONSVariableRef::SetValue(double v) array faild:" , errinfo);
		}
		nsv->SetArrayValue(arrayIndex, (int)v);
		break;
	case TYPE_REAL:
		im_val = v;
		break;
	default:
		SimpleError("%s", "ONSVariableRef::SetValue(double v) unsupported assignment type!");
		break;
	}
}

void ONSVariableRef::SetValue(LOString *s) {
	const char *errinfo = NULL;
	switch (vtype)
	{
	case TYPE_STRING:
		nsv = ONSVariableBase::GetVariable(nsvId);
		nsv->SetString(s);
		break;
	case TYPE_STRING_IM:
		ONSVariableBase::SetStrCore(im_str, s);
		break;
	default:
		SimpleError("%s","ONSVariableRef::SetValue(LOString *s) unsupported assignment type!");
		break;
	}
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
		SimpleError("%s","ONSVariableRef::SetValue(ONSVariableRef *ref) unsupported assignment type!");
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
	else SimpleError("%s","ONSVariableRef::SetAutoReal(double v) unsupported type!");
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
	else SimpleError("%s","ONSVariableRef::SetAutoReal(LOString *s) unsupported type!");
}

double ONSVariableRef::GetReal() {
	const char *errinfo = NULL;
	switch (vtype) {
	case TYPE_INT:
		nsv = ONSVariableBase::GetVariable(nsvId);
		return nsv->GetValue();
	case TYPE_STRING:
		nsv = ONSVariableBase::GetVariable(nsvId);
		return ONSVariableBase::StrToIntSafe(nsv->GetString());
	case TYPE_STRING_IM:
		return ONSVariableBase::StrToIntSafe(im_str);
	case TYPE_REAL:
		return im_val;
	case TYPE_ARRAY:
		nsv = ONSVariableBase::GetVariable(nsvId);
		errinfo = nsv->CheckAarryIndex(arrayIndex);
		if (errinfo) {
			SimpleError("%s%s", "ONSVariableRef::GetReal() array faild:", errinfo);
		}
		return nsv->GetArrayValue(arrayIndex);
	default:
		SimpleError("ONSVariableRef::GetReal() Unsupported assignment type!");
		break;
	}
	return -1;
}


LOString *ONSVariableRef::IntToStr(int v) {
	if (im_tmpout) delete im_tmpout;
	im_tmpout = new LOString(std::to_string(v));
	return im_tmpout;
}

//非文字变量会尝试转换为文字
LOString *ONSVariableRef::GetStr() {
	const char *errinfo = NULL;
	switch (vtype) {
	case TYPE_INT:
		nsv = ONSVariableBase::GetVariable(nsvId);
		return IntToStr((int)nsv->GetValue());
	case TYPE_STRING:
		nsv = ONSVariableBase::GetVariable(nsvId);
		return nsv->GetString();
	case TYPE_STRING_IM:
		return im_str;
	case TYPE_REAL:
		return IntToStr((int)im_val);
	case TYPE_ARRAY:
		nsv = ONSVariableBase::GetVariable(nsvId);
		errinfo = nsv->CheckAarryIndex(arrayIndex);
		if (errinfo) {
			LOString errs = "ONSVariableRef::GetStr() array faild:";
			errs.append(errinfo);
			SimpleError(errs.c_str());
		}
		return IntToStr(nsv->GetArrayValue(arrayIndex));
	default:
		SimpleError("ONSVariableRef::GetStr() Unsupported assignment type!");
		break;
	}
	return NULL;
}

//返回的是获取的符号长度
int ONSVariableRef::GetOperator(const char *buf) {
	int ret = 0;
	vtype = TYPE_NONE;
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
		else return ret;
		break;
	default:
		return ret;
		break;
	}
	if (oper == 0) {
		oper = buf[0];
		ret = 1;
	}
	else ret = 3;  //mod
	vtype = TYPE_OPERATOR;
	return ret;
}

//进过计算，ref都会编程im_，除非遇到 ? % $ 这种变量操作代表
bool ONSVariableRef::Calculator(ONSVariableRef *v, char op, bool isreal) {
	if (!v) return false;
	if (op == '+' && isStr()) {
		if (vtype != TYPE_STRING_IM) {  //must be str_im
			LOString *s = GetStr();
			ONSVariableBase::SetStrCore(im_str, s);
		}
		vtype = TYPE_STRING_IM;
		ONSVariableBase::AppendStrCore(im_str, v->GetStr());
		return true;
	}
	else {
		if (!isReal()) return false;
		double dst = GetReal();      //注意必须先获取值再改类型，不然会出现错误的结果
		double src = v->GetReal();
		int dsti = dst;
		int srci = src;

		vtype = TYPE_REAL;
		switch (op)
		{
		case '+':
			if(isreal) im_val = dst + src;
			else im_val = dsti + srci;
			break;
		case '-':
			if(isreal)im_val = dst - src;
			else im_val = dsti - srci;
			break;
		case '*':
			if(isreal) im_val = dst * src; 
			else im_val = dsti * srci;
			break;
		case '/':
			if(isreal)im_val = dst / src; 
			else im_val = dsti / srci;
			break;
		case '~':
			im_val = dsti % srci;
			break;
		case '^':
			if (isreal) im_val = pow(dst, srci);
			else im_val = pow(dsti, srci);
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
	vtype = v->vtype;
	oper = v->oper;
	order = v->order;
	nsvId = v->nsvId;
	ONSVariableBase::SetStrCore(im_str, v->im_str);
	im_val = v->im_val;
	if (arrayIndex) delete[] arrayIndex;
	arrayIndex = NULL;
	if (v->arrayIndex) {

		InitArrayIndex();
		for (int ii = 0; ii < MAXVARIABLE_ARRAY; ii++) arrayIndex[ii] = v->arrayIndex[ii];
	}
}

bool ONSVariableRef::SetArryVals(int *v, int count) {
	nsv = ONSVariableBase::GetVariable(nsvId);
	int dime = nsv->ArrayDimension(arrayIndex);
	if (!arrayIndex || (nsv->ArrayDimension(nullptr) != dime + 1) || dime > MAXVARIABLE_ARRAY - 1) {
		SimpleError("SetArryVals() dimension error!");
		return false;
	}

	for (int ii = 0; ii < count; ii++) {
		arrayIndex[dime] = ii;
		nsv->SetArrayValue(arrayIndex, *(v + ii));
	}
	arrayIndex[dime] = -1;
	return true;
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