/*
//变体型
*/
#ifndef __H_LOVARIANT_
#define __H_LOVARIANT_

#include <stdint.h>
#include <string.h>
#include <string>
#include "LOString.h"

//前4字节：总长度，第5字节：类型 ，第6字节：数据

class LOVariant{
public:
	enum {
		TYPE_NONE,
		TYPE_CHAR,
		TYPE_CHARS,
		TYPE_INT,
		TYPE_DOUBLE,
		TYPE_FLOAT,
		TYPE_STRING,
		TYPE_LOSTRING,
		TYPE_PTR,
	};

	LOVariant();
	LOVariant(LOString *str);
	LOVariant(int val);
	//需要小心的指明用的哪一个构造函数
	LOVariant(void *ptr);
	LOVariant(const char *ptr, int len);
	LOVariant& operator=(const LOVariant &obj) {
		if (this != &obj) {
			if (obj.bytes) {
				FreeMem();
				int len = *(int*)obj.bytes;
				bytes = new uint8_t[len];
				memcpy(bytes, obj.bytes, len);
			}
			else FreeMem();
		}
		return *this;
	}
	~LOVariant();

	int GetType();
	bool IsValid();
	bool IsType(int type);

	void SetChar(char c);
	void SetChars(const char* bin, int len);
	void SetInt(int val);
	void SetDoule(double val);
	void SetFloat(float val);
	void SetString(std::string *str);
	void SetLOString(LOString *str);
	void SetPtr(void *ptr);

	//所有的get类都不做有效性检查
	char GetChar();
	const char* GetChars(int *len);
	int GetInt();
	double GetDoule();
	float GetFloat();
	std::string GetString();
	LOString GetLOString();
	void* GetPtr();

	//获取原始数据及长度
	uint8_t* GetDataPtr() { return bytes; }
	int GetDataLen();
private:
	enum {
		CFG_RECORED = 4,   //类型
		CFG_DATAS = 5      //数据的起点
	};

	uint8_t *bytes;
	void FreeMem();
	void NewMem(int datalen);
};

#endif // !__H_LOVARIANT_