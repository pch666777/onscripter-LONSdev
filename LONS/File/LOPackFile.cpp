/*
//打包的文件
*/

#include "LOPackFile.h"
#include "../etc/BinArray.h"
#include "../etc/LOIO.h"

LOPackFile::LOPackFile() {
	filePtr = NULL;
	_type = PACK_NONE;
}

LOPackFile::~LOPackFile() {
	if (filePtr) fclose(filePtr);
	indexMap.clear();
}


//读取文件的索引记录ns2和arc文件都没有标记，需要根据后缀名判断
bool LOPackFile::ReadFileIndexs(FILE *f, PACKTYPE t) {
	if (_type != PACK_NONE || !f) return false;
	filePtr = f;
	if (t == PACK_ARC) return ReadARCindexs();
	else if (t == PACK_NS2) return ReadNS2index();
	return false;
}


bool LOPackFile::ReadARCindexs() {
	//读取基本信息
	BinArray *bin = BinArray::ReadFile(filePtr, 0, 6);
	int count = (uint16_t)bin->GetInt16(0, true);
	dataStart = (uint32_t)bin->GetInt32(2, true);  //数据的起始地址
	delete bin;

	//读取索引目录
	bin = BinArray::ReadFile(filePtr, 6, dataStart - 6);
	PackIndex index;
	std::string fname;
	int current = 0;
	for (int ii = 0; ii < count; ii++) {
		bin->GetString(fname, &current);
		//LOCodePage::baseEncoder->ToLower(&fname); 压缩工具应该要保证小写
		index.flag = bin->GetChar(&current);
		index.adress = dataStart + (uint32_t)bin->GetInt32(&current, true);
		index.length = (uint32_t)bin->GetInt32(&current, true);
		index.compresslen = (uint32_t)bin->GetInt32(&current, true);
		indexMap[fname] = index;
		//printf("%s", fname.c_str());
	}
	delete bin;
	return true;
}

bool LOPackFile::ReadNS2index() {
	//the indexs lenght
	BinArray *bin = BinArray::ReadFile(filePtr, 0, 4);
	int lenght = bin->GetInt(0);
	delete bin;
	bin = BinArray::ReadFile(filePtr, 0, lenght);

	PackIndex index;
	std::string fname;
	int current = 4;
	uint32_t srcAdress = (uint32_t)bin->GetInt(0);

	while (current < lenght - 6) {

		if (bin->bin[current] == '"') current++;
		int tsart = current;
		while (bin->bin[current] != '"') current++;
		current++;
		fname.assign(bin->bin + tsart, current - tsart - 1);

		index.length = (uint32_t)bin->GetInt(current);
		current += 4;
		index.adress = srcAdress;
		srcAdress += index.length;

		//printf("%08X--%08X\n",index.adress, index.length);
		indexMap[fname] = index;
	}

	delete bin;
	return true;
}


LOPackFile::PackIndex LOPackFile::FindFile(std::string &s) {
	PackIndex index;
	auto iter = indexMap.find(s);
	if (iter != indexMap.end()) return iter->second;
	else return index;
}