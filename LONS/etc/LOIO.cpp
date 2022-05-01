/*
//只管IO的
*/

#include "LOIO.h"


LOString LOIO::ioReadDir;
LOString LOIO::ioWriteDir;
LOString LOIO::ioWorkDir;

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

FILE* LOIO::GetWriteHandle(LOString fn, const char *mode) {
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