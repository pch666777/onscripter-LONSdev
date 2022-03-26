/*
//打包的文件
*/
#ifndef __LOPACKFILE_H__
#define __LOPACKFILE_H__

#include "../etc/LOString.h"

#include <mutex>
#include <unordered_map>
#include <stdio.h>

class LOPackFile
{
public:
	struct PackIndex{
		char flag = 0;
		uint32_t adress = 0;
		uint32_t length = 0;
		uint32_t compresslen = 0;
	};

	enum PACKTYPE {
		PACK_NONE,
		PACK_ARC,   //arc.nsa
		PACK_NS2,   //xxx.ns2
	};

	LOPackFile();
	~LOPackFile();
	bool ReadFileIndexs(FILE *f, PACKTYPE t);
	FILE * GetHandle() { return filePtr; }
	PackIndex FindFile(std::string &s);
	int FilesCount() { return indexMap.size(); }

	std::string Name;
private:
	std::mutex _mutex;  //显然不能同时让两个线程读取同一个压缩包
	PACKTYPE _type;
	uint32_t dataStart;
	std::unordered_map<std::string, PackIndex> indexMap;

	bool ReadARCindexs();
	bool ReadNS2index();

	FILE *filePtr;
};



#endif // !__LOPACKFILE_H__
