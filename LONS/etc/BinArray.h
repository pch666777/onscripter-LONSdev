
#ifndef __BINARRAY_ONLY998_H__
#define __BINARRAY_ONLY998_H__

#include <stdlib.h>
#include <stdint.h>
#include <string>
#include "LOString.h"
#include "LOVariant.h"

//本质上是一个连续的char数组
class BinArray
{
public:
	enum {
		FLAGS_STREAM = 1,

	};

	//字节集的默认大小和是否为流模式
	BinArray(int prepsize = 8 , bool isstream = false);

	//用内存中创建字节集，长度为-1表示自动检测字符串
	BinArray(const char *buf,int length = -1);

	~BinArray();

	void Clear(bool resize);
	uint32_t Length();
	void SetLength(uint32_t len);

	//设置字节集的order模式，字节顺序只对auto读取时有用，模式默认时小端的
	void SetOrder(bool isBig);
	//恢复默认的字节顺序
	void SetCpuOrder();

	//与order无关的获取
	char GetChar(int *pos);
	bool GetBool(int *pos);
	std::string GetString(int *pos);
	//LOString GetLOString(int *pos);
	LOString* GetLOStrPtr(int *pos);
	LOVariant* GetLOVariant(int *pos);

	//根据BinArray的order自动调整读取的内容到CPU order
	int GetIntAuto(int *pos);
	int GetIntAuto(int pos) { return GetIntAuto(&pos); }
	int16_t GetInt16Auto(int *pos);
	int16_t GetInt16Auto(int pos) { return GetInt16Auto(&pos); }
	int32_t GetInt32Auto(int *pos);
	int64_t GetInt64Auto(int *pos);
	float   GetFloatAuto(int *pos);
	double  GetDoubleAuto(int *pos);
	bool GetArrayAuto(void *dst, int elmentCount, int elmentSize, int *pos);

	void Append(const char *buf,int len);
	void Append(BinArray *v);

	//不调整order的write
	int WriteChar(char v,int *pos = NULL);
	int WriteBool(bool v, int *pos = NULL);
	int WriteInt(int v, int *pos = NULL);
	int WriteInt2(int v1, int v2, int *pos = NULL);
	int WriteInt3(int v1, int v2, int v3, int *pos = NULL);
	int WriteInt4(int v1, int v2, int v3, int v4, int *pos = NULL);
	int WriteInt16(int16_t v, int *pos = NULL);
	int WriteInt32(int32_t v, int *pos = NULL);
	int WriteInt64(int64_t v, int *pos = NULL);
	int WriteFloat(float v, int *pos = NULL);
	int WriteDouble(double v, int *pos = NULL);
	//C风格的字符串 结尾的0一起写入
	int WriteCString(const char *buf, int *pos = NULL);
	int WriteString(std::string *s, int *pos = NULL);
	int WriteLOString(LOString *s, int *pos = NULL);
	int WriteLOVariant(LOVariant *v, int *pos = NULL);
	//LPKS格式的操作,mark只写入4字节，返回的是len被写入的位置
	int WriteLpksEntity(const char *mark, int len, int version);
	void InitLpksHeader();
	//检查LPKS的文件头，检查通过的话会设置字节集的字节顺序
	bool CheckLpksHeader(int *pos);
	bool CheckEntity(const char *mark,int *next, int *version, int *pos);
	bool JumpEntity(const char *mark, int *pos);
	int GetNextEntity(int *pos);
	//调整order的write


	//其他
	bool WriteToFile(const char *name);

	//static BinArray* ReadFromFile(const char* name);
	static BinArray* ReadFile(FILE *f,int pos,int len);

	char *bin;
private:
	enum {
		BIN_DEFAULT_LEN = 20,
		//数据起始的位置
		BIN_DATA = 12,
		//当前设置的内存序列（大端，还是小端）
		//BIN_ORDER = 0,
		//标记位，总长2字节
		//BIN_FLAGS = 2,
		//BIN_REALLEN = 4,
		//BIN_PRELEN = 8,
	};
	//检测是当前CPU是大端还是小端的标记
	static int16_t cpuOrder;

	//实际数据的存放地址
	char *dataPtr;
	////实际使用的长度，指读写到的位置
	//uint32_t *realLenPtr;
	////已经准备的长度
	//uint32_t *preLenPtr;
	const char *errerinfo;   //上一次错误信息

	void NewSelf(int prepsize, bool isstream);
	//void ReverseBytes(char *buf,int len);
	//void GetCharIntArray(char *buf, int len, int *pos, bool isbig);


	void WriteCharIntArray(char *buf, int len, int *pos, bool isbig);
	void AddMemory(int len);
	bool CheckPosition(int pos);
	void XChangeN(char *buf, int n);
	int WriteUnOrder(void *src_t, int *pos, int len);
};


#endif // !__BINARRAY_ONLY998_H__

