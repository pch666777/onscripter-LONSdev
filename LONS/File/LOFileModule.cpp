/*
//文件系统模块
*/
#include "SiYuanCN.h"
#include "LOCompressInfo.h"
#include "LOFileModule.h"


BinArray *LOFileModule::built_in_font = NULL;

LOFileModule::LOFileModule() {
	FunctionInterface::fileModule = this;
	nsaHasRead = false;
}

LOFileModule::~LOFileModule() {
	delete built_in_font;
	built_in_font = NULL;
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
	if (!temp)temp = ReadFileFromFileSys(fileName->c_str());
	if (!temp && err) SDL_Log("*** LONS not find file:%s   ***\n", fileName->c_str());
	return temp;
}


BinArray *LOFileModule::ReadFileFromFileSys(const char *fileName) {
	LOString tstr(fileName);
	for (int ii = 0; ii < tstr.length(); ii++) {
		if (tstr[ii] == '\\') tstr[ii] = '/';
	}

	if(readDir.length() > 0) tstr = readDir + "/" + tstr ;
	//SDL_Log("read file is:%s",tstr.c_str()) ;
	BinArray *bin = BinArray::ReadFromFile(tstr.c_str());
	/*
	if (!bin && fileName[0] != '/' && fileName[1] != ':') {  //非绝对路径尝试更多
		for (int ii = 0; ii < workDirs.size() && !bin; ii++) {
			tstr = workDirs.at(ii) + "/" + fileName;
			bin = BinArray::ReadFromFile(tstr.c_str());
		}
	}
	 */
	return bin;
}

LOString LOFileModule::ReplacePathSymbol(LOString *fn) {
	LOString tstr(*fn);
	for (int ii = 0; ii < tstr.length(); ii++) {
		if (tstr[ii] == '\\') tstr[ii] = '/';
	}
	return tstr;
}


BinArray* LOFileModule::ReadFileFromRecord(LOString *fn) {
	LOString fnl = fn->toLower();
	LOPackFile::PackIndex index ;
	LOPackFile *file = NULL;
	for (int ii = 0; ii < packFiles.size() && index.length == 0; ii++) {
		 file = (LOPackFile*)packFiles.at(ii);
		 index = file->FindFile(fnl);
	}

	if (index.length == 0) return NULL;

	FILE *f = file->GetHandle();
	BinArray *bin = BinArray::ReadFile(f, index.adress, index.length);

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
		if (built_in_font) return built_in_font;
		BinArray *zbin = new BinArray((char*)(consola_ttf), *consola_ttf + 8);
		LOCompressInfo comp;
		built_in_font = comp.UnZlibCompress(zbin);
		delete zbin;
	}
	return built_in_font;
}


int LOFileModule::nsadirCommand(FunctionInterface *reader) {
	nsaDir = reader->GetParamStr(0);
	return RET_CONTINUE;
}

void LOFileModule::ResetMe() {
	nsaHasRead = false;
	for (auto iter = packFiles.begin(); iter != packFiles.end(); iter++) {
		delete (LOPackFile*)(*iter);
	}
	packFiles.clear();
}


int LOFileModule::nsaCommand(FunctionInterface *reader) {
	if (nsaHasRead) return RET_CONTINUE;
	
	int FileCount = 0;
	LOString s;
	LOPackFile::PACKTYPE packtype = LOPackFile::PACK_ARC;
	if (reader->isName("ns2") || reader->isName("ns3")) packtype = LOPackFile::PACK_NS2;

	for (int ii = 0; ii < 100; ii++) {
		//create file name
		if (packtype == LOPackFile::PACK_ARC) {
			if (ii == 0) s = "arc.nsa";
			else s = "arc" + std::to_string(ii) + ".nsa";
		}
		else if (packtype == LOPackFile::PACK_NS2) {
			if (ii < 10) s = "0" + std::to_string(ii) + ".ns2";
			else s = std::to_string(ii) + ".ns2";
		}
		//try open it
		FILE *f = OpenFileForRead(s.c_str(), "rb");
		if (!f && nsaDir.length() > 0) {
			s = nsaDir + "/" + s;
			f = OpenFileForRead(s.c_str(), "rb");
		}
		if (!f) break;
		//get file index
		LOPackFile *file = new LOPackFile;
		file->Name = s;
		file->ReadFileIndexs(f, packtype);
		packFiles.push_back((intptr_t)file);
		FileCount += file->FilesCount();
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
		fnl = ReplacePathSymbol(&fn);
		FILE *f = OpenFileForRead(fnl.c_str(),"rb");
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
		LOString s;
		bin->GetString(s, &pos);
		s.SetEncoder(fn.GetEncoder());
		ONSVariableRef *v = reader->GetParamRef(0);
		v->SetValue(&s);
		delete bin;
	}
	return RET_CONTINUE;
}