/*
//只管IO的
*/

#ifndef __LOIO_H__
#define __LOIO_H__

#include "LOString.h"
#include "BinArray.h"

class LOIO
{
public:
	LOIO();
	~LOIO();

	static void GetPathForRead(LOString &s);
	static void GetPathForWrite(LOString &s);
	static FILE* GetReadHandle(LOString fn, const char *mode);
	static FILE* GetWriteHandle(LOString fn, const char *mode);
	static BinArray* ReadAllBytes(LOString &fn);
	static void ReadBytes(BinArray *&bin, FILE *f, int position, int length);
	static bool WriteAllBytes(LOString &fn, BinArray *bin);

	static LOString ioReadDir;
	static LOString ioWriteDir;
	static LOString ioWorkDir;
private:

};

//接住文件模块读取文件，函数主体不在IO中实现，只是在这里声明，方便使用
extern BinArray* LonsReadFile(LOString &fn);

//尝试从封包中读取，函数主体不在IO中实现，只是在这里声明，方便使用
extern BinArray* LonsReadFileFromPack(LOString &fn);

//获取一些内建的文件，函数主体不在IO中实现，只是在这里声明，方便使用
extern BinArray* LonsGetBuiltMem(int type);
#endif // !__LOIO_H__
