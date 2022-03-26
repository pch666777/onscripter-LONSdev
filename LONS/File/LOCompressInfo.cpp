/*
//解压缩使用
*/

#include "LOCompressInfo.h"
#include "zlib.h"
#include "../etc/LOLog.h"

LOCompressInfo::LOCompressInfo() {
	decomp_buf = NULL;
	decomp_dic = NULL;
	Clear();
}

void LOCompressInfo::Clear() {
	if (decomp_buf) delete[] decomp_buf;
	if (decomp_dic) delete[] decomp_dic;
	mask = 0;      //当前的bit掩码位
	r = 0;
	decomp_buf = NULL;  //解压时用的缓冲区
	decomp_dic = NULL;  //解压时用的字典
	bufsize = 0;       //缓冲区大小
	dicsize = 0;
	maxrenum = 0;     //允许的最大重复长度
	dicKey = 0;
	byteCount = 0;
	bitCount = 0;
	bitSize = 0;
}

LOCompressInfo::~LOCompressInfo()
{
	if (decomp_buf) delete[] decomp_buf;
	if (decomp_dic) delete[] decomp_dic;
}

void LOCompressInfo::InitLZSS() {
	Clear();
	bufsize = 1 << 8;
	dicsize = 4096;
	maxrenum = (1 << 4) + 1;
	decomp_buf = new char[bufsize];
	decomp_dic = new char[dicsize];
	memset(decomp_dic, 0, dicsize);
	memset(decomp_buf, 0, bufsize - maxrenum);
	r = bufsize - maxrenum;
}

int LOCompressInfo::GetBit(BinArray *bin, int bitlen) {
	static int dicByte;
	int ret = 0;

	for (int ii = 0; ii < bitlen; ii++) {
		if (mask == 0) { //完成一个byte的循环
			if (bitSize == bitCount) { //next group data
				bitSize = CopyCharToPtr(decomp_dic, bin, byteCount, dicsize);
				if (bitSize == 0) return -1;
				bitCount = 0;
			}
			dicByte = decomp_dic[bitCount++];
			byteCount++;
			mask = 1 << 7;
		}

		ret <<= 1;
		if (dicByte & mask) ret++;
		mask >>= 1;
	}
	return ret;
}

BinArray *LOCompressInfo::UncompressLZSS(BinArray *bin, int uncom_size) {
	if (!bin) return bin;

	InitLZSS();
	BinArray *zbin = new BinArray(uncom_size);
	int repos, relen, c, zcount = 0;

	while (zcount < uncom_size) {
		if (GetBit(bin, 1)) {
			if ((c = GetBit(bin, 8)) == -1) break;
			zbin->bin[zcount++] = c;
			decomp_buf[r++] = c;
			r &= 0xff;
		}
		else {
			if ((repos = GetBit(bin, 8)) == -1) break;
			if ((relen = GetBit(bin, 4)) == -1) break;
			for (int ii = 0; ii <= relen + 1 && zcount < uncom_size; ii++) {
				c = decomp_buf[(repos + ii) & 0xff];
				zbin->bin[zcount++] = c;
				decomp_buf[r++] = c;
				r &= 0xff;
			}
			//int debugbre = 0;
		}
	}

	zbin->SetLength(uncom_size);

	//FILE *f = fopen("d:/test_lons.bmp", "wb");
	//fwrite(zbin->bin, 1, zbin->Length(), f);
	//fclose(f);
	return zbin;
}


uint32_t LOCompressInfo::CopyCharToPtr(char *dst, BinArray *bin, int start, int len) {
	if (len > bin->Length() - start) len = bin->Length() - start;
	if (len > 0) memcpy(dst, bin->bin, len);
	return len;
}


BinArray *LOCompressInfo::ZlibCompress(BinArray *bin) {
	int destlen = bin->Length() * 1.2 + 8;
	BinArray *dest = new BinArray(destlen);
	int ret = compress((Bytef*)(dest->bin + 8), (uLongf*)(&destlen), (Bytef*)bin->bin, bin->Length());
	if (ret == 0) {
		dest->WriteInt(destlen);
		dest->WriteInt(bin->Length());
		dest->SetLength(destlen + 8);
		return dest;
	}
	else {
		delete dest;
		return NULL;
	}
}

BinArray *LOCompressInfo::UnZlibCompress(BinArray *bin) {
	uLong compresslen = *(int*)(bin->bin);
	uLong sourcelen = *(int*)(bin->bin + 4);
	BinArray *source = new BinArray(sourcelen);

	Bytef *dst = (Bytef*)source->bin ;
	Bytef *src = (Bytef*)bin->bin + 8;
	int ret = uncompress(dst, &sourcelen, src, compresslen);
	if (ret == 0) {
		source->SetLength(sourcelen);
		return source;
	}
	else {
		delete source;
		return NULL;
	}
}

std::string LOCompressInfo::GetCdata(BinArray *bin) {
	std::string s = "uint32_t bindata[] = {\n";
	char *buf = new char[32];
	int pos = 0;
	int count = 0;
	while (pos >= 0) {
		int val = bin->GetInt32(&pos);
		memset(buf, 0, 32);
		sprintf(buf, "0x%x,", val);
		s.append(buf);
		if (count % 10 == 9) s.append("\n");
		count++;
	}
	s.append("\n};");
	return s;
}