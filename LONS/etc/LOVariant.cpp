/*
//变体型
*/
#include "LOVariant.h"


LOVariant::LOVariant(){
	bytes = nullptr;
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
	NewMem(len);
	bytes[CFG_RECORED] = TYPE_CHARS;
	memcpy(bytes + CFG_DATAS, bin, len);
}

char LOVariant::GetChar() {
	return *(char*)(bytes + CFG_DATAS);
}

const char* LOVariant::GetChars(int *len) {
	*len = *(int*)(bytes);
	(*len) -= CFG_DATAS;
	return (const char*)bytes;
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
	NewMem(sizeof(double));
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
	NewMem(str->length());
	bytes[CFG_RECORED] = TYPE_STRING;
	memcpy(bytes + CFG_DATAS, str->c_str(), str->length());
}

std::string LOVariant::GetString() {
	int len = *(int*)bytes;
	len -= CFG_DATAS;
	return std::string((const char*)(bytes + CFG_DATAS), len);
}


LOString LOVariant::GetLOString() {
	int len = *(int*)bytes;
	len -= CFG_DATAS + 1;
	LOString str((const char*)(bytes + CFG_DATAS + 1), len);
	str.SetEncoder(LOCodePage::GetEncoder(bytes[CFG_DATAS]));
	return str;
}


void LOVariant::SetLOString(LOString *str) {
	if (!str) {
		FreeMem();
		return;
	}
	NewMem(str->length() + 1);
	bytes[CFG_RECORED] = TYPE_LOSTRING;
	bytes[CFG_DATAS] = str->GetEncoder()->codeID;
	memcpy(bytes + CFG_DATAS + 1, str->c_str(), str->length());
}
