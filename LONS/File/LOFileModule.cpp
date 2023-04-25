/*
//文件系统模块
*/
#include "SiYuanCN.h"
#include "LOCompressInfo.h"
#include "LOFileModule.h"
#include "../etc/LOIO.h"
#ifdef WIN32
#include <Windows.h>
#endif


//BinArray *LOFileModule::built_in_font = NULL;

LOFileModule::LOFileModule() {
	FunctionInterface::fileModule = this;
	nsaHasRead[0] = nsaHasRead[1] = nsaHasRead[2] = false;
}

LOFileModule::~LOFileModule() {
	for (int ii = 0; ii < packFiles.size(); ii++) {
		LOPackFile *file = (LOPackFile*)packFiles.at(ii);
		delete file;
	}
	packFiles.clear();
}

//BinArray *LOFileModule::ReadFile(const char *fileName, bool err) {
//	LOString s;
//	s.assign(fileName);
//	return ReadFile(&s, err);
//}

BinArray *LOFileModule::ReadFile(LOString *fileName, bool err) {
	BinArray *temp = ReadFileFromRecord(fileName);
	if (!temp)temp = ReadFileFromFileSys(fileName);
	if (!temp && err) SDL_Log("*** LONS not find file:%s   ***\n", fileName->c_str());
	return temp;
}


BinArray *LOFileModule::ReadFileFromFileSys(LOString *fn) {
	LOString truePath = fn->PathTypeTo(LOString::PATH_LINUX);
	//LOLog_i("file is:%s", truePath.c_str()) ;
	BinArray *bin = LOIO::ReadAllBytes(truePath);
	return bin;
}

BinArray* LOFileModule::ReadFileFromRecord(LOString *fn) {
	LOString fnl = fn->toLowerAndRePathSymble('\\');
	LOPackFile::PackIndex index ;
	LOPackFile *file = NULL;
	for (int ii = 0; ii < packFiles.size() && index.length == 0; ii++) {
		 file = (LOPackFile*)packFiles.at(ii);
		 index = file->FindFile(fnl);
	}

	if (index.length == 0) return NULL;

	FILE *f = file->GetHandle();
	BinArray *bin = nullptr;
	LOIO::ReadBytes(bin, f, index.adress, index.length);

	if (!bin) return bin;
	else if (index.flag == NO_COMPRESSION) return bin;
	else if (index.flag == LZSS_COMPRESSION) {
		LOCompressInfo zout;
		BinArray *zbin = zout.UncompressLZSS(bin, index.compresslen);
		delete bin;
		return zbin;
		//LONS::printFormat("LZSS uncompress fail:%s\n", fn->c_str());
		//LONS::printInfo(LONS::fmOutText);
	}
	else {
		SDL_Log("Unsupported compression type:%s,%d\n", fnl.c_str(), index.flag);
	}
}


BinArray *LOFileModule::GetBuiltMem(int type) {
	if (type == BUILT_FONT) {
		BinArray *zbin = new BinArray((char*)(consola_ttf), *consola_ttf + 8);
		LOCompressInfo comp;
		BinArray *tbin = comp.UnZlibCompress(zbin);
		delete zbin;
		//由使用者自行处理
		return tbin;
	}
	return nullptr;
}


int LOFileModule::nsadirCommand(FunctionInterface *reader) {
	nsaDir = reader->GetParamStr(0);
	return RET_CONTINUE;
}

void LOFileModule::ResetMe() {
	nsaHasRead[0] = nsaHasRead[1] = nsaHasRead[2] = false;
	for (auto iter = packFiles.begin(); iter != packFiles.end(); iter++) {
		delete (LOPackFile*)(*iter);
	}
	packFiles.clear();
}


int LOFileModule::nsaCommand(FunctionInterface *reader) {
	//不管执行的是哪一个，*.arc *.ns2 *.ns3都会被打开
	int FileCount = 0;
	LOString s;
	//LOPackFile::PACKTYPE packtype = LOPackFile::PACK_ARC;
	//if (reader->isName("ns2") || reader->isName("ns3")) packtype = LOPackFile::PACK_NS2;
	for (int pakType = LOPackFile::PACK_ARC; pakType <= LOPackFile::PACK_NS2; pakType++) {
		//防止封包被反复读取
		if (nsaHasRead[pakType]) continue;
		//read it
		for (int ii = 0; ii < 100; ii++) {
			//create file name
			if (pakType == LOPackFile::PACK_ARC) {
				if (ii == 0) s = "arc.nsa";
				else s = "arc" + std::to_string(ii) + ".nsa";
			}
			else if (pakType == LOPackFile::PACK_NS2) {
				if (ii < 10) s = "0" + std::to_string(ii) + ".ns2";
				else s = std::to_string(ii) + ".ns2";
			}
			else break;
			//try open it
			FILE *f = LOIO::GetReadHandle(s, "rb");
			if (!f && nsaDir.length() > 0) {
				s = nsaDir + "/" + s;
				f = LOIO::GetReadHandle(s, "rb");
			}
			if (!f) break;

			LOPackFile *file = new LOPackFile;
			file->Name = s;
			file->ReadFileIndexs(f, (LOPackFile::PACKTYPE)pakType);
			packFiles.push_back((intptr_t)file);
			FileCount += file->FilesCount();
		}

		nsaHasRead[pakType] = true;
	}
	return RET_CONTINUE;
}


int LOFileModule::fileexistCommand(FunctionInterface *reader) {
	LOString fn = reader->GetParamStr(1);
	LOString fnl = fn.toLower();
	LOPackFile::PackIndex index;
	for (int ii = 0; ii < packFiles.size() && index.length == 0; ii++) {
		LOPackFile *file = (LOPackFile*)packFiles.at(ii);
		index = file->FindFile(fnl);
	}

	if (index.length == 0) { //check file sys
		fnl = fn.PathTypeTo(LOString::PATH_LINUX);
		FILE *f = LOIO::GetReadHandle(fnl,"rb");
		if (f) {
			index.length = 1;
			fclose(f);
		}
	}

	ONSVariableRef *v = reader->GetParamRef(0);
	if (index.length == 0) v->SetValue(0.0);
	else v->SetValue(1.0);

	return RET_CONTINUE;
}


int LOFileModule::readfileCommand(FunctionInterface *reader) {
	LOString fn = reader->GetParamStr(1);
	BinArray *bin = ReadFile(&fn);
	if (bin) {
		int pos = 0;
		//utf8 bom
		if (bin->Length() > 3 && bin->bin[0] == (char)0xef 
			&& bin->bin[1] == (char)0xbb && bin->bin[2] == (char)0xbf) pos = 3;
		LOString s = bin->GetString(&pos);
		s.SetEncoder(fn.GetEncoder());
		ONSVariableRef *v = reader->GetParamRef(0);
		v->SetValue(&s);
		delete bin;
	}
	return RET_CONTINUE;
}


BinArray* LonsReadFileFromPack(LOString &fn) {
	if (FunctionInterface::fileModule) {
		auto *it = (LOFileModule*)FunctionInterface::fileModule;
		return it->ReadFileFromRecord(&fn);
	}
	else return nullptr;
}


BinArray* LonsGetBuiltMem(int type) {
	if (FunctionInterface::fileModule) {
		auto *it = (LOFileModule*)FunctionInterface::fileModule;
		return it->GetBuiltMem(type);
	}
	else return nullptr;
}


int LOFileModule::savedirCommand(FunctionInterface *reader) {
	LOIO::ioSaveDir = reader->GetParamStr(0);
	LOString fpath = LOIO::ioSaveDir;
	LOIO::GetPathForWrite(fpath);
	//创建文件夹的具体代码跟平台有关
#ifdef WIN32
	CreateDirectoryA(fpath.c_str(), NULL);
#endif
	return RET_CONTINUE;
}