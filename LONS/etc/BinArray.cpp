#include "BinArray.h"

char BinArray::litte[2] = { 1,2 };

//���˳λ��С����λ
#define ISBIG *(int16_t*)litte==0x0102

//���ٽ���
//#define XCHANGE2 val=((val&0xff)<<8)|((val&0xff00)>>8)
//#define XCHANGE3 val=((val&0xff)<<16)|((val&0xff0000)>>16)|(val&0xff00)
#define XCHANGE4(X) ((X&0xff)<<24)|((X&0xff00)<<8)|(X>>24)|((X&0x00ff0000)>>8)

BinArray::BinArray(int prepsize, bool isstream){
	NewSelf(prepsize, isstream);
}

BinArray::BinArray(const char *buf, int length) {
	if (length == -1) {
		length = 0;
		while (buf[length] != 0) length++;
	}
	if (length == 0)NewSelf(8,false);
	else if(length > 0) {
		NewSelf(length + BIN_PREPLEN, false);
		realLen = length;
		memcpy(bin, buf, realLen);
	}
	else {
		errerinfo = "BinArray(const char *buf, int length) length less than zero!";
	}
}

void BinArray::NewSelf(int prepsize, bool isstream) {
	prepLen = prepsize;
	realLen = 0;
	bin = (char*)calloc(1, prepLen);
	isStream = isstream;
	errerinfo = nullptr;
}

void BinArray::Clear(bool resize) {
	if (resize) {
		if (bin) free(bin);
		NewSelf(8,false);
	}
	else memset(bin, 0, prepLen);
	realLen = 0;
	errerinfo = nullptr;
}


BinArray::~BinArray()
{
	if (bin) free(bin);
}

void BinArray::GetCharIntArray(char *buf, int len, int *pos, bool isbig) {
	if (*pos > realLen - 1) {
		errerinfo = "out of range!";
		*pos = -2;
	}
	else if (*pos < 0) {
		errerinfo = "position less than zero!";
		*pos = -2;
	}
	else {
		int olen = len;
		memset(buf, 0, len);
		if (len + (*pos) > realLen - 1) len = realLen - (*pos);
		memcpy(buf, bin + (*pos), len);
		(*pos) += len;
		if (*pos >= realLen) *pos = -1;
		//ȷ���Ƿ���Ҫ��ת�ֽ�����
		if (olen > 1) {
			bool machineBigen = false;
			char test[] = { 11,22 };
			if (*(uint16_t*)(test) == 0x1122) machineBigen = true;
			if (!isbig && machineBigen) ReverseBytes(buf, olen);  //�����Ǵ��Ҫ���С��
			if (isbig && !machineBigen) ReverseBytes(buf, olen);  //������С��Ҫ������
		}
		//if (isbig && !isBig && olen > 1) ReverseBytes(buf, olen);
		//if(!isbig && isBig && olen > 1) ReverseBytes(buf, olen);
	}
}

//������д���ֽڼ�
void BinArray::WriteCharIntArray(char *buf, int len, int *pos, bool isbig) {
	int tpos = realLen;
	if (!pos) pos = &tpos;

	//��鳤�ȣ����ӳ���
	if ((*pos) + len > prepLen) AddMemory(len);

	//����Ƿ���Ҫ����ֵ����С�����ò�ƥ�����Ҫ����ֵ
	if (len > 1 && isbig != (ISBIG)) {
		//���֧��4��4�ֽ�
		char nbuf[16];
		//�����ֽ�
		for (int ii = 0; ii < len; ii++) nbuf[ii] = buf[(len - 1) - ii];
		memcpy(bin + (*pos), nbuf, len);
	}
	else memcpy(bin + (*pos), buf, len);


	(*pos) += len;
	if ( *pos > realLen) realLen = *pos;
}


char BinArray::GetChar(int *pos) {
	char tmp = 0;
	GetCharIntArray(&tmp, 1, pos,false);
	return tmp;
}

bool BinArray::GetBool(int *pos) {
	return (bool)GetChar(pos);
}

float BinArray::GetFloat(int *pos, bool isbig) {
	float tmp = 0.0;
	GetCharIntArray((char*)(&tmp), sizeof(tmp), pos, isbig);
	return tmp;
}

double BinArray::GetDouble(int *pos, bool isbig) {
	double tmp = 0.0;
	GetCharIntArray((char*)(&tmp), sizeof(tmp), pos, isbig);
	return tmp;
}

short int BinArray::GetShortInt(int *pos, bool isbig) {
	short int tmp = 0;
	GetCharIntArray((char*)(&tmp), sizeof(tmp), pos,isbig);
	return tmp;
}


int  BinArray::GetInt(int *pos, bool isbig) {
	int tmp = 0;
	GetCharIntArray((char*)(&tmp), sizeof(tmp), pos,isbig);
	return tmp;
}



int8_t BinArray::GetInt8(int *pos) {
	int8_t tmp = 0;
	GetCharIntArray((char*)(&tmp), sizeof(tmp), pos,false);
	return tmp;
}

int16_t BinArray::GetInt16(int *pos, bool isbig) {
	int16_t tmp = 0;
	GetCharIntArray((char*)(&tmp), sizeof(tmp), pos,isbig);
	return tmp;
}


int32_t BinArray::GetInt32(int *pos, bool isbig) {
	int32_t tmp = 0;
	GetCharIntArray((char*)(&tmp), sizeof(tmp), pos,isbig);
	return tmp;
}

int64_t BinArray::GetInt64(int *pos, bool isbig) {
	int64_t tmp = 0;
	GetCharIntArray((char*)(&tmp), sizeof(tmp), pos,isbig);
	return tmp;
}


//��ת�ֽ�����
void BinArray::ReverseBytes(char *buf, int len) {
	char *tmp = new char[len];
	memcpy(tmp, buf, len);
	for (int ii = 0; ii < len; ii++) buf[ii] = tmp[(len-1) - ii];
	delete []tmp;
}

//����\0��posָ��\0�ĺ�һ���ֽ�
void BinArray::GetString(std::string &s, int *pos){
	if (*pos > realLen - 1) {
		errerinfo = "out of range!";
		*pos = -2;
	}
	else if (*pos < 0) {
		errerinfo = "position less than zero!";
		*pos = -2;
	}
	int len = 0;
	while (bin[(*pos) + len] != 0 && (*pos) + len < realLen) len++;
	s.assign(bin + (*pos), len);
	(*pos) += (len+1); //����\0
	if ( *pos >= realLen) *pos = -1;
}


LOString BinArray::GetLOString(int *pos) {
	LOString s;
	GetString(s, pos);
	if (pos >= 0) {
		s.SetEncoder(LOCodePage::GetEncoder(GetChar(pos)));
	}
	return s;
}


LOString* BinArray::GetLOStrPtr(int *pos) {
	LOString *s = new LOString();
	GetString(*s, pos);
	if (pos && *pos >= 0) {
		s->SetEncoder(LOCodePage::GetEncoder(GetChar(pos)));
	}
	return s;
}


void BinArray::AddMemory(int len) {
	int destlen;
	if (!isStream) destlen = realLen + len + BIN_PREPLEN;
	else {
		destlen = realLen + len;
		if (destlen < 10000000) destlen *= 2;  //10MB����ֱ�ӷ���
		else destlen = destlen + 10000000;    //10MB�������ֻ��10MB
	}
	bin = (char*)realloc(bin, destlen);
	memset(bin + realLen, 0, destlen - realLen);  //��������ڴ���0
	prepLen = destlen;
}

void BinArray::Append(const char *buf, int len) {
	if (realLen + len > prepLen) AddMemory(len);
	memcpy(bin + realLen, buf, len);
	realLen += len;
}

void BinArray::Append(BinArray *v) {
	Append(v->bin, v->Length());
}


int BinArray::WriteChar(char v, int *pos) {
	WriteCharIntArray(&v, 1, pos, false);
	return 1;
}

int BinArray::WriteInt(int v, int *pos, bool isbig) {
	WriteCharIntArray((char*)(&v), sizeof(v), pos, isbig);
	return sizeof(v);
}


int BinArray::WriteInt3(int v1, int v2, int v3, int *pos, bool isbig) {
	//Ԥ�Ƚ���ֵ
	if (isbig != (ISBIG)) {
		v1 = XCHANGE4(v1);
		v2 = XCHANGE4(v2);
		v3 = XCHANGE4(v3);
	}
	int val[] = { v1,v2,v3 };
	WriteCharIntArray((char*)(val), sizeof(int) * 3, pos, ISBIG);
	return sizeof(int) * 3;
}

int BinArray::WriteInt16(int16_t v, int *pos, bool isbig) {
	WriteCharIntArray((char*)(&v), 2, pos, isbig);
	return 2;
}

int BinArray::WriteInt32(int32_t v, int *pos, bool isbig) {
	WriteCharIntArray((char*)(&v), 4, pos, isbig);
	return 4;
}

int BinArray::WriteInt64(int64_t v, int *pos, bool isbig) {
	WriteCharIntArray((char*)(&v), 8, pos, isbig);
	return 8;
}

int BinArray::WriteFloat(float v, int *pos, bool isbig) {
	WriteCharIntArray((char*)(&v), sizeof(v), pos, isbig);
	return sizeof(v);
}

int BinArray::WriteDouble(double v, int *pos, bool isbig) {
	WriteCharIntArray((char*)(&v), sizeof(v), pos, isbig);
	return sizeof(v);
}

int BinArray::WriteBool(bool v, int *pos, bool isbig) {
	WriteCharIntArray((char*)(&v), 1, pos, isbig);
	return 1;
}


int BinArray::WriteString(const char *buf, int *pos, bool isbig) {
	int tpos = realLen;
	if (!pos) pos = &tpos;
	int len = strlen(buf) + 1;
	if (*pos + len > prepLen) AddMemory(len);
	memcpy(bin + (*pos), buf, len);
	*pos += len;
	if (*pos > realLen) realLen = *pos;
	return len;
}

int BinArray::WriteLOString(LOString *v, int *pos) {
	int tpos = realLen;
	if (!pos) pos = &tpos;

	int len;
	if (!v || v->length() == 0) { //���ַ������߳���Ϊ0
		WriteChar(0, pos);
		len = 1; 
	}
	else {
		//�ַ����������� \0
		WriteString(v->c_str(), pos);
		//����ID
		WriteChar(v->GetEncoder()->codeID & 0xff, pos);
		len = v->length() + 2;
	}
	return len;
}


int BinArray::WriteLOVariant(LOVariant *v, int *pos) {
	int tpos = realLen;
	if (!pos) pos = &tpos;

	int len = 0;
	if (v) len = v->GetDataLen();
	//��
	if (len == 0) {
		WriteChar(0, pos);
		return 1;
	}
	else {
		if (*pos + len > prepLen) AddMemory(len);
		memcpy(bin + (*pos), v->GetDataPtr(), len);
		*pos += len;
		if (*pos > realLen) realLen = *pos;
	}
	return len;
}


BinArray* BinArray::ReadFile(FILE *f, int pos, int len) {
	//���Ȳ�������ļ��ĳ���
	fseek(f, 0, SEEK_END);
	if (len > ftell(f)) len = ftell(f);

	BinArray *sbin = new BinArray(len + BIN_PREPLEN, false);
	//�ļ�λ��
	fseek(f, pos, SEEK_SET);
	int sumlen = 0;
	while (sumlen < len) {
		int rlen = fread(sbin->bin + sumlen, 1, len - sumlen, f);
		if (rlen <= 0) break;  //���ܶ����ļ�β��
		sumlen += rlen;
	}
	sbin->realLen = sumlen;
	return sbin;
}

bool BinArray::WriteToFile(const char *name) {
	if (!bin || Length() == 0) return false;
	FILE *f = fopen(name, "wb");
	if (!f) return false;
	fwrite(bin, 1, Length(), f);
	fflush(f);
	fclose(f);
	return true;
}

//����ֽڼ�û�дﵽָ����������0���
int BinArray::WriteFillEmpty(int len, int *pos) {
	int npos = 0;
	if (!pos) pos = &npos;
	if (*pos + len > prepLen) AddMemory(len);
	if (realLen < (*pos + len)) {  //˵�����������
		memset(bin + realLen, 0, (*pos + len) - realLen);  //����������0
		realLen = (*pos + len);
		(*pos) += len;
	}
	return len;
}