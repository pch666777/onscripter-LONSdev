#include "BinArray.h"

int16_t BinArray::cpuOrder = 0x1122;

//大端顺位，小端逆位   
#define IsBigOrder *(char*)(&cpuOrder)==0x11
#define IsCpuOrder *(int16_t*)dataPtr==cpuOrder
//使用0x1122作为字节顺序判断，大端为11 22   小端为 22 11
#define notCpuOrder *(int16_t*)dataPtr!=cpuOrder
#define RealLen *(uint32_t*)(dataPtr + 4)
#define PreLen *(uint32_t*)(dataPtr + 8)
#define Flags *(int16_t*)(dataPtr + 2)

//快速交换
#define XCHANGE2(X) ((X<<8)&0xff00)|((X>>8)&0xff)
#define XCHANGE4(X) ((X<<24)&0xff000000)|((X<<8)&0xff0000)|((X>>8)&0xff00)|((X>>24)&0xff)

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
		NewSelf(length + BIN_DEFAULT_LEN, false);
		RealLen = length;
		memcpy(bin, buf, RealLen);
	}
	else {
		errerinfo = "BinArray(const char *buf, int length) length less than zero!";
	}
}

void BinArray::NewSelf(int prepsize, bool isstream) {
	dataPtr = (char*)calloc(1, prepsize + BIN_DATA);

	bin = dataPtr + BIN_DATA;
	RealLen = 0;
	PreLen = prepsize;
	if (isstream) Flags |= FLAGS_STREAM;

	//CPU是小端模式的话，内存为 22 11，大端模式为 11 22
	//这里采用的CPU的默认order
	*(int16_t*)dataPtr = 0x1122;
	errerinfo = nullptr;
}

void BinArray::Clear(bool resize) {
	if (resize) {
		if (dataPtr) free(dataPtr);
		NewSelf(8, false);
	}
	else memset(bin, 0, PreLen);
	RealLen = 0;
	errerinfo = nullptr;
}


BinArray::~BinArray()
{
	if (dataPtr) free(dataPtr);
}

uint32_t BinArray::Length() {
	if (dataPtr) return RealLen;
	return 0;
}

void BinArray::SetLength(uint32_t len) {
	if (dataPtr) RealLen = len;
}

//颠倒字节
void BinArray::XChangeN(char *buf, int n) {
	for (int ii = 0; ii <= n / 2; ii++) {
		char c = buf[ii];
		buf[ii] = buf[n - 1 - ii];
		buf[n - 1 - ii] = c;
	}
}

bool BinArray::CheckPosition(int pos) {
	if (pos > RealLen) {
		errerinfo = "out of range!";
		return false;
	}
	return true;
}

void BinArray::SetOrder(bool isBig) {
	if (isBig) {
		dataPtr[0] = 0x11;
		dataPtr[1] = 0x22;
	}
	else {
		dataPtr[0] = 0x22;
		dataPtr[1] = 0x11;
	}
}

void BinArray::SetCpuOrder() {
	*(int16_t*)dataPtr = cpuOrder;
}

int BinArray::GetIntAuto(int *pos) {
	int val = 0;
	if (!CheckPosition(pos[0] + 4)) return val;
	val = *(int*)(bin + pos[0]);
	if (notCpuOrder) val = XCHANGE4(val);
	pos[0] += 4;
	return val;
}

int32_t BinArray::GetInt32Auto(int *pos) {
	return GetIntAuto(pos);
}

int16_t BinArray::GetInt16Auto(int *pos) {
	int16_t val = 0;
	if (!CheckPosition(pos[0] + 2)) return val;
	val = *(int16_t*)(bin + pos[0]);
	if (notCpuOrder) val = XCHANGE2(val);
	pos[0] += 2;
	return val;
}

int64_t BinArray::GetInt64Auto(int *pos) {
	int64_t val = 0;
	if (!CheckPosition(pos[0] + 8)) return val;
	val = *(int64_t*)(bin + pos[0]);
	if (notCpuOrder) XChangeN((char*)(&val), 8);
	pos[0] += 8;
	return val;
}

float BinArray::GetFloatAuto(int *pos) {
	float val = 0.0f;
	if (!CheckPosition(pos[0] + 4)) return val;
	val = *(float*)(bin + pos[0]);
	if (notCpuOrder) XChangeN((char*)(&val), 4);
	pos[0] += 4;
	return val;
}

double  BinArray::GetDoubleAuto(int *pos) {
	double val = 0.0;
	if (!CheckPosition(pos[0] + 8)) return val;
	val = *(double*)(bin + pos[0]);
	if (notCpuOrder) XChangeN((char*)(&val), 8);
	pos[0] += 8;
	return val;
}

bool BinArray::GetArrayAuto(void *dst, int elmentCount, int elmentSize, int *pos) {
	int len = elmentCount * elmentSize;
	if (!CheckPosition(pos[0] + len)) return false;
	memcpy(dst, bin + pos[0], len);
	//交换每一个元素的字节顺序
	if (notCpuOrder) {
		char *dst_t = (char*)dst;
		for (int ii = 0; ii < elmentCount; ii++) {
			//2交换1次，3交换1次，4交换2次
			for (int kk = 0; kk <= elmentSize /2 ; kk++) {
				char c = dst_t[kk];
				dst_t[kk] = dst_t[elmentSize - 1 - kk];
				dst_t[elmentSize - 1 - kk] = c;
			}
			dst_t += elmentSize;
		}
	}

	pos[0] += len;
	return true;
}


char BinArray::GetChar(int *pos) {
	char val = 0;
	if (!CheckPosition(pos[0] + 1)) return val;
	val = bin[pos[0]];
	pos[0] += 1;
	return val;
}

bool BinArray::GetBool(int *pos) {
	bool val = false;
	if (!CheckPosition(pos[0] + 1)) return val;
	val = bin[pos[0]];
	pos[0] += 1;
	return val;
}

std::string BinArray::GetString(int *pos) {
	std::string val;
	if (!CheckPosition(pos[0] + 1)) return val;
	//要对字节集的边界做一个检查
	int len = strlen(bin + pos[0]);
	if (pos[0] + len >= RealLen) val.assign(bin + pos[0], RealLen - pos[0]);
	else val.assign(bin + pos[0]);
	pos[0] += val.length() + 1;
	return val;
}

LOString* BinArray::GetLOStrPtr(int *pos) {
	if (!CheckPosition(pos[0] + 2)) return nullptr;
	if (bin[pos[0]] == 0) {
		pos[0] += 2;
		return nullptr;
	}
	else {
		LOString *s = new LOString(bin + pos[0]);
		pos[0] += s->length() + 1;
		s->SetEncoder(LOCodePage::GetEncoder(bin[pos[0]]));
		pos[0]++;
		return s;
	}
}

LOString BinArray::GetLOString(int *pos) {
	LOString tmp;
	if (!CheckPosition(pos[0] + 2)) return tmp;
	if (bin[pos[0]] == 0) {
		pos[0] += 2;
		return tmp;
	}
	else {
		tmp.assign(bin + pos[0]);
		pos[0] += tmp.length() + 1;
		tmp.SetEncoder(LOCodePage::GetEncoder(bin[pos[0]]));
		pos[0]++;
		return tmp;
	}
}


LOVariant* BinArray::GetLOVariant(int *pos) {
	int len = GetIntAuto(pos);
	if (len <= 0) return nullptr;
	if (!CheckPosition(pos[0] + len)) return nullptr;
	LOVariant *v = new LOVariant();
	v->SetMemData(bin + pos[0], len);
	*pos += len;
	return v;
}


void BinArray::AddMemory(int len) {
	//浪费一点点空间，避免频繁重新分配内存
	len += BIN_DEFAULT_LEN;
	//增加的大小
	int addLen;
	//对字节流有一个优化
	if ((Flags)& FLAGS_STREAM) {
		//10MB以下直接翻倍,10MB以上最多只加10MB
		addLen = PreLen;
		if (addLen > 10000000) addLen = 10000000;
		while (addLen < len) {
			if (addLen < 10000000) addLen *= 2;
			else addLen += 10000000;
		}
	}
	else addLen = len;

	//BIN_DATA + preLen才是上次分配的大小
	//printf("BinArray resize:%d ++-> %d\n", PreLen + BIN_DATA, addLen);
	dataPtr = (char*)realloc(dataPtr, PreLen + BIN_DATA + addLen);
	//多出来的内存置0
	memset(dataPtr + BIN_DATA + PreLen, 0, addLen); 

	bin = dataPtr + BIN_DATA;
	PreLen = PreLen + addLen;
}

void BinArray::Append(const char *buf, int len) {
	if (RealLen + len > PreLen) AddMemory(len);
	memcpy(bin + RealLen, buf, len);
	RealLen += len;
}

void BinArray::Append(BinArray *v) {
	Append(v->bin, v->Length());
}




//==============================================
int BinArray::WriteUnOrder(void *src_t, int *pos, int len) {
	//int testP = RealLen;
	//if (testP == 8227) {
	//	int breakii = 0;
	//}
	int tpos = RealLen;
	if (!pos) pos = &tpos;

	if (pos[0] + len > PreLen) AddMemory(len);
	memcpy(bin + pos[0], src_t, len);

	pos[0] += len;
	if (pos[0] > RealLen) RealLen = pos[0];

	//if (RealLen - testP > 5000) {
	//	int breaki = 0;
	//}
	return len;
}

int BinArray::WriteBool(bool v, int *pos) {
	char c = (char)v;
	return WriteUnOrder(&c, pos, 1);
}

int BinArray::WriteChar(char v, int *pos) {
	return WriteUnOrder(&v, pos, sizeof(char));
}

int BinArray::WriteInt(int v, int *pos) {
	return WriteUnOrder(&v, pos, sizeof(int));
}

int BinArray::WriteInt2(int v1, int v2, int *pos) {
	int list[] = { v1,v2 };
	return WriteUnOrder(list, pos, sizeof(int) * 2);
}

int BinArray::WriteInt3(int v1, int v2, int v3, int *pos) {
	int list[] = { v1,v2,v3 };
	return WriteUnOrder(list, pos, sizeof(int) * 3);
}

int BinArray::WriteInt4(int v1, int v2, int v3, int v4, int *pos) {
	int list[] = { v1,v2,v3,v4 };
	return WriteUnOrder(list, pos, sizeof(int) * 4);
}

int BinArray::WriteInt16(int16_t v, int *pos) {
	return WriteUnOrder(&v, pos, sizeof(int16_t));
}

int BinArray::WriteInt32(int32_t v, int *pos) {
	return WriteUnOrder(&v, pos, sizeof(int32_t));
}

int BinArray::WriteInt64(int64_t v, int *pos) {
	return WriteUnOrder(&v, pos, sizeof(int64_t));
}

int BinArray::WriteFloat(float v, int *pos) {
	return WriteUnOrder(&v, pos, sizeof(float));
}

int BinArray::WriteDouble(double v, int *pos) {
	return WriteUnOrder(&v, pos, sizeof(double));
}

int BinArray::WriteCString(const char *buf, int *pos) {
	if (!buf) {
		char c = 0;
		return WriteUnOrder(&c, pos, 1);
	}
	return WriteUnOrder((void*)buf, pos, strlen(buf) + 1);
}


int BinArray::WriteString(std::string *s, int *pos) {
	if (!s) {
		char c = 0;
		return WriteUnOrder(&c, pos, 1);
	}
	else return WriteUnOrder( (void*)s->c_str(), pos, s->length() + 1);
}


int BinArray::WriteLOString(LOString *s, int *pos) {
	if (!s || s->length() == 0) {
		int16_t v = 0;
		return WriteUnOrder(&v, pos, 2);
	}
	else {
		//debug use
		if (s->GetEncoder()->codeID == LOCodePage::ENCODER_UTF8) {
			const char *buf = s->c_str();
			const char *end = buf + s->length();
			auto *encoder = s->GetEncoder();
			while (buf < end) {
				int ulen = encoder->GetCharLen(buf);
				if (ulen < 1 || ulen > 3) {
					int errbreak = 1;
				}
				buf += ulen;
			}
		}

		int len = WriteUnOrder((void*)s->c_str(), pos, s->length() + 1);
		char code = s->GetEncoder()->codeID;
		if(pos) (*pos) += len;
		WriteUnOrder(&code, pos, 1);
		return len + 1;
	}
}


int BinArray::WriteLOVariant(LOVariant *v, int *pos) {
	if (!v || v->GetDataPtr()) {
		int v = 0;
		return WriteUnOrder(&v, pos, 4);
	}
	else return WriteUnOrder((void*)v->GetDataPtr(), pos, v->GetDataLen());
}






BinArray* BinArray::ReadFile(FILE *f, int pos, int len) {
	//长度不因大于文件的长度
	fseek(f, 0, SEEK_END);
	if (len > ftell(f) || len < 0) len = ftell(f);

	BinArray *sbin = new BinArray(len, false);
	//文件位置
	fseek(f, pos, SEEK_SET);
	int sumlen = 0;
	while (sumlen < len) {
		int rlen = fread(sbin->bin + sumlen, 1, len - sumlen, f);
		if (rlen <= 0) break;  //可能读到文件尾了
		sumlen += rlen;
	}
	sbin->SetLength(len);
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


//mark只写入4字节，返回的是len被写入的位置
int BinArray::WriteLpksEntity(const char *mark, int len, int version) {
	int markv = *(int*)mark;
	//if (!(ISBIG)) markv = XCHANGE4(markv);
	int outlen = Length() + 4;
	WriteInt3(markv, len, version);
	return outlen;
}

void BinArray::InitLpksHeader() {
	Clear(false);
	SetCpuOrder();
	//LPKS，字节顺序和标记，版本
	WriteLpksEntity("LPKS", *(int*)dataPtr, 1);
	//预留
	WriteInt(0);
}

bool BinArray::CheckLpksHeader(int *pos) {
	if (!CheckPosition(4) || !CheckEntity("LPKS", nullptr, nullptr, pos)) return false;
	dataPtr[0] = bin[4];
	dataPtr[1] = bin[5];
	pos[0] += 4;
	return true;
}


bool BinArray::CheckEntity(const char *mark, int *next, int *version, int *pos) {
	if (!CheckPosition(pos[0] + 12)) return false;
	int markv = *(int*)mark;
	//if (!(ISBIG)) markv = XCHANGE4(markv);
	//printf("%x,%x\n", markv, *(int*)(bin + pos[0]));
	if (markv != *(int*)(bin + pos[0])) return false;
	pos[0] += 4;
	if (next) *next = pos[0] + *(int*)(bin + pos[0]);
	pos[0] += 4;
	if (version) *version = *(int*)(bin + pos[0]);
	pos[0] += 4;
	return true;
}


//跳过实体
bool BinArray::JumpEntity(const char *mark, int *pos) {
	int next = -1;
	if (!CheckEntity(mark, &next, nullptr, pos)) return false;
	*pos = next;
	return true;
}


int BinArray::GetNextEntity(int *pos) {
	if (!CheckPosition(pos[0] + 4)) return -1;
	int next = pos[0] + *(int*)(bin + pos[0]);
	pos[0] += 4;
	return next;
}