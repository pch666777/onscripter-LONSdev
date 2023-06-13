/*
//只管IO的
*/

#include "LOIO.h"


LOString LOIO::ioReadDir;    //游戏文件所在的目录
LOString LOIO::ioWriteDir;   //游戏写入文件的目录，可能存在存档、LOG等文件不能写入ioReadDir的情况，因此将其分开
                             //因此可以设定与游戏目录无关的存档目录

LOString LOIO::ioSaveDir;    //由游戏内savedir设定的目录，非必要不应该从外部设定，很可能会导致问题

LOIO::LOIO() {
}

LOIO::~LOIO() {
}


void LOIO::GetPathForRead(LOString &s) {
	if (ioReadDir.length() > 0) s = ioReadDir + "/" + s;
}

void LOIO::GetPathForWrite(LOString &s) {
	if (ioWriteDir.length() > 0) s = ioWriteDir + "/" + s;
}

FILE* LOIO::GetReadHandle(LOString fn, const char *mode) {
	GetPathForRead(fn);
	return fopen(fn.c_str(), mode);
}

//这里要注意加入fn有由脚本提供的全角文件名，那么可能有编码问题
FILE* LOIO::GetWriteHandle(LOString fn, const char *mode) {
	GetPathForWrite(fn);
	return fopen(fn.c_str(), mode);
}

FILE* LOIO::GetSaveHandle(LOString fn, const char *mode) {
	if (ioSaveDir.length() > 0) {
		auto coder = fn.GetEncoder();
		fn = ioSaveDir + "/" + fn;
		fn.SetEncoder(coder);
	}
	GetPathForWrite(fn);
	return fopen(fn.c_str(), mode);
}


BinArray* LOIO::ReadAllBytes(LOString &fn) {
	FILE *f = GetReadHandle(fn, "rb");
	if (!f) return nullptr;
	fseek(f, 0, SEEK_END);
	size_t len = ftell(f);
	if (len > 0x7fffffff) {
		fclose(f);
		return nullptr;
	}
	fseek(f, 0, SEEK_SET);

	BinArray *bin = new BinArray(len);
	int rlen = fread(bin->bin, 1, len, f);
	bin->SetLength(len);
	fclose(f);
	//安卓上似乎有一个问题，fn即使为空路径也能执行到这里
	if(rlen <= 0){
		delete bin ;
		bin = nullptr ;
	}
	return bin;
}


bool LOIO::WriteAllBytes(LOString &fn, BinArray *bin) {
	if (!bin) return false;
	FILE *f = GetWriteHandle(fn, "rb");
	if (!f) return false;

	fwrite(bin->bin, 1, bin->Length(), f);
	fflush(f);
	fclose(f);
	return true;
}


void LOIO::ReadBytes(BinArray *&bin, FILE *f, int position, int length) {
	if (bin && bin->Length() > 0) {
		delete bin;
		bin = nullptr;
	}
	if (f) {
		if (fseek(f, position, SEEK_SET) != 0) return;
		if(!bin) bin = new BinArray(length);
		int len = 0;
		len	= fread(bin->bin, 1, length, f);
		bin->SetLength(len);
	}
}


