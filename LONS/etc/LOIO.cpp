/*
//只管IO的
*/

#include "LOIO.h"


LOString LOIO::ioReadDir;
LOString LOIO::ioWriteDir;
LOString LOIO::ioWorkDir;
LOString LOIO::ioSaveDir;

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
	fread(bin->bin, 1, len, f);
	bin->SetLength(len);
	fclose(f);
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


void LOIO::InitLPKStream(BinArray *bin) {
	bin->Clear(false);
	//LPKS,version,预留
	bin->WriteInt3(0x534B504C, 1, 0);
	bin->WriteInt(0);   //预留
}

bool LOIO::CheckLPKHeader(BinArray *bin, int *pos) {
	*pos = 0;
	//LPKS
	if (bin->GetInt(*pos) != 0x534B504C) return false;
	*pos = 16;
	return true;
}

//LONS的envdata跟ONS是完全不同的
void LonsSaveEnvData() {
	std::unique_ptr<BinArray> unibin(new BinArray(1024, true));
	BinArray *bin = unibin.get();
	LOIO::InitLPKStream(bin);
	//envdata的版本
	bin->WriteInt(1);
	//savedir，这个必须先写出，因为一些存储的文件会放在这个文件夹里
	bin->WriteLOString(&LOIO::ioSaveDir);
	//下面这些都是预留的
	bin->WriteInt(0);
	bin->WriteInt(0);
	bin->WriteInt(0);
	bin->WriteInt(0);
	bin->WriteInt(0);
	bin->WriteInt(0);
	bin->WriteInt(0);
	bin->WriteInt(0);
	bin->WriteInt(0);

	LOString s("envdataL");
	FILE *f = LOIO::GetWriteHandle(s, "wb");
	if (f) {
		fwrite(bin->bin, 1, bin->Length(), f);
		fflush(f);
		fclose(f);
	}
}


void LonsReadEnvData() {
	LOString s("envdataL");
	FILE *f = LOIO::GetWriteHandle(s, "rb");
	if (f) {
		int pos = 0;
		std::unique_ptr<BinArray> bin(BinArray::ReadFile(f, 0, 0x7ffffff));
		fclose(f);

		if (!bin || !LOIO::CheckLPKHeader(bin.get(), &pos)) return;

		bin->GetInt(&pos);
		LOIO::ioSaveDir = bin->GetLOString(&pos);
	}
}