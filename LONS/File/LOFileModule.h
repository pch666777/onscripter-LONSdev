/*
//文件系统模块
*/
#ifndef __LOFILEMODULE_H__
#define __LOFILEMODULE_H__

#include "../Scripter/FuncInterface.h"
#include "../etc/BinArray.h"
#include "LOPackFile.h"
#include <SDL.h>
#include <vector>

class LOFileModule :public FunctionInterface
{
public:
	LOFileModule();
	~LOFileModule();

	enum {
		NO_COMPRESSION = 0,
		SPB_COMPRESSION = 1,
		LZSS_COMPRESSION = 2,
		NBZ_COMPRESSION = 4
	};

	/*BinArray *ReadFile(const char *fileName, bool err = true);*/
	BinArray *ReadFile(LOString *fileName, bool err = true);
	BinArray *GetBuiltMem(int type);
	void ResetMe();

	int nsadirCommand(FunctionInterface *reader);
	int nsaCommand(FunctionInterface *reader);
	int fileexistCommand(FunctionInterface *reader);
	int readfileCommand(FunctionInterface *reader);

private:
	static BinArray *built_in_font;
	LOString nsaDir;
	bool nsaHasRead;


	BinArray *ReadFileFromFileSys(const char *fileName);
	BinArray* ReadFileFromRecord(LOString *fn);
	LOString ReplacePathSymbol(LOString *fn);

	std::vector<intptr_t> packFiles;   //不直接使用指针，不然vector可能会有一些奇葩问题
};
#endif // !__LOFILEMODULE_H__
