/*
//变体型
*/
#include "LOVariant.h"


LOVariant::LOVariant(){
	bytes = nullptr;
}

LOVariant::LOVariant(LOString *str) {
	bytes = nullptr;
	SetLOString(str);
}

LOVariant::LOVariant(int val) {
	bytes = nullptr;
	SetInt(val);
}

LOVariant::LOVariant(void *ptr) {
	bytes = nullptr;
	SetPtr(ptr);
}

LOVariant::LOVariant(const char *ptr, int len) {
	bytes = nullptr;
	SetChars(ptr, len);
}


LOVariant::~LOVariant(){
	FreeMem();
}

void LOVariant::FreeMem() {
	if (bytes) delete[] bytes;
	bytes = nullptr;
}


void LOVariant::NewMem(int datalen) {
	FreeMem();
	int maxlen = CFG_DATAS + datalen;
	bytes = new uint8_t[maxlen];
	memset(bytes, 0, maxlen);
	*(int32_t*)(bytes) = maxlen;
}


//取出变体型的类型
int LOVariant::GetType() {
	if (!bytes) return TYPE_NONE;
	else return bytes[CFG_RECORED];
}


bool LOVariant::IsValid() {
	return (bytes != nullptr);
}

bool LOVariant::IsType(int type) {
	if (!bytes) return false;
	return (bytes[CFG_RECORED] == (type & 0xff));
}

void LOVariant::SetChar(char c) {
	NewMem(1);
	bytes[CFG_RECORED] = TYPE_CHAR;
	bytes[CFG_DATAS] = (uint8_t)c;
}

void LOVariant::SetChars(const char* bin, int len) {
	//尾部追加一个\0，方便使用
	NewMem(len + 1);
	bytes[CFG_RECORED] = TYPE_CHARS;
	memcpy(bytes + CFG_DATAS, bin, len);
}

char LOVariant::GetChar() {
	return *(char*)(bytes + CFG_DATAS);
}

const char* LOVariant::GetChars(int *len) {
	int tlen = *(int*)(bytes);
	tlen -= CFG_DATAS;
	tlen -= 1;   //不包括 \0
	if (len) *len = tlen;
	return (const char*)(bytes + CFG_DATAS);
}

void LOVariant::SetInt(int val) {
	NewMem(sizeof(int));
	bytes[CFG_RECORED] = TYPE_INT;
	*(int*)(bytes + CFG_DATAS) = val;
}

int LOVariant::GetInt() {
	return *(int*)(bytes + CFG_DATAS);
}

void LOVariant::SetDoule(double val) {
	if(bytes[CFG_RECORED] != TYPE_DOUBLE) NewMem(sizeof(double));
	bytes[CFG_RECORED] = TYPE_DOUBLE;
	*(double*)(bytes + CFG_DATAS) = val;
}

double LOVariant::GetDoule() {
	return *(double*)(bytes + CFG_DATAS);
}


void LOVariant::SetFloat(float val) {
	NewMem(sizeof(float));
	bytes[CFG_RECORED] = TYPE_FLOAT;
	*(float*)(bytes + CFG_DATAS) = val;
}

float LOVariant::GetFloat() {
	return *(float*)(bytes + CFG_DATAS);
}


void LOVariant::SetString(std::string *str) {
	if (!str) {
		FreeMem();
		return;
	}
	//追加\0
	NewMem(str->length() + 1);
	bytes[CFG_RECORED] = TYPE_STRING;
	memcpy(bytes + CFG_DATAS, str->c_str(), str->length());
}

std::string LOVariant::GetString() {
	int len = *(int*)bytes;
	len -= CFG_DATAS;
	len -= 1;
	return std::string((const char*)(bytes + CFG_DATAS), len);
}

void LOVariant::SetLOString(LOString *str) {
	if (!str) {
		FreeMem();
		return;
	}
	//前面追加一个语言，后面追加一个\0
	NewMem(str->length() + 2);
	bytes[CFG_RECORED] = TYPE_LOSTRING;
	bytes[CFG_DATAS] = str->GetEncoder()->codeID;
	memcpy(bytes + CFG_DATAS + 1, str->c_str(), str->length());
}

LOString LOVariant::GetLOString() {
	int len = *(int*)bytes;
	len -= CFG_DATAS;
	len -= 2; //去掉一个语言，一个\0
	LOString str((const char*)(bytes + CFG_DATAS + 1), len);
	str.SetEncoder(LOCodePage::GetEncoder(bytes[CFG_DATAS]));
	return str;
}


//这里ptr固定为一个int64
void LOVariant::SetPtr(void *ptr) {
	NewMem(sizeof(int64_t));
	bytes[CFG_RECORED] = TYPE_PTR;
	*(int64_t*)(bytes + CFG_DATAS) = (int64_t)ptr;
}


void LOVariant::SetMemData(void *mem, int len) {
	FreeMem();
	//提供的len是总长度
	NewMem(len - CFG_RECORED);
	memcpy(bytes + CFG_RECORED, mem, len - CFG_RECORED);
}


void* LOVariant::GetPtr() {
	int64_t val = *(int64_t*)(bytes + CFG_DATAS);
	if (sizeof(intptr_t) == 4) return (void*)(val & 0xffffffff);
	else return (void*)val;
}


int LOVariant::GetDataLen() {
	if (!bytes) return 0;
	return *(int32_t*)bytes;
}