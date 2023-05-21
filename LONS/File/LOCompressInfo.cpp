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

void LOCompressInfo::InitSPB(int buf_size) {
	Clear();
	bufsize = buf_size;
	dicsize = 4096;
	decomp_buf = new char[bufsize];
	decomp_dic = new char[dicsize];
	memset(decomp_dic, 0, dicsize);
	memset(decomp_buf, 0, bufsize);
	//从第4字节开始，前两字节分别是图像宽度和高度
	byteCount = 4;
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


BinArray *LOCompressInfo::UncompressSPB(BinArray *bin, int uncom_size) {
	if (!bin) return nullptr;
	bin->SetOrder(true);
	int width = bin->GetInt16Auto(0);
	int height = bin->GetInt16Auto(2);
	//这是一个24位的BMP图像
	int width_pad = (4 - width * 3 % 4) % 4;
	//每行的像素数量必须对齐4的倍数
	int total_size = (width * 3 + width_pad) * height + 54;

	BinArray *zbin = new BinArray(total_size);
	zbin->SetLength(total_size);
	//bmp文件头
	int pos = 0;
	memset(zbin->bin, 0, 54);
	zbin->SetOrder(false);
	zbin->WriteChar('B', &pos);
	zbin->WriteChar('M', &pos);
	zbin->WriteOrderInt(total_size, &pos);
	pos += 4;
	//头的大小
	zbin->WriteOrderInt(54, &pos);
	zbin->WriteOrderInt(40, &pos);
	//宽度、高度
	zbin->WriteOrderInt(width, &pos); //18-21
	zbin->WriteOrderInt(height, &pos); //22-25
	zbin->WriteOrderInt16(1, &pos);  //26-27
	zbin->WriteOrderInt16(24, &pos); //24位图像
	pos += 4;
	//小端
	zbin->WriteOrderInt(total_size - 54, &pos);

	//==== 开始解压 =========
	InitSPB(width * height + 4);
	int c, count, n, m;
	for (int channel = 0; channel < 3; channel++) {
		count = 0;
		decomp_buf[count++] = c = GetBit(bin, 8);
		while (count < (unsigned int)(width * height)) {
			n = GetBit(bin, 3);
			if (n == -1) break;
			else if (n == 0) {
				decomp_buf[count++] = c;
				decomp_buf[count++] = c;
				decomp_buf[count++] = c;
				decomp_buf[count++] = c;
				continue;
			}
			else if (n == 7) m = GetBit(bin, 1) + 1;
			else m = n + 2;
			//
			for (int j = 0; j < 4; j++) {
				if (m == 8) c = GetBit(bin, 8);
				else {
					int k = GetBit(bin, m);
					if (k & 1) c += (k >> 1) + 1;
					else c -= (k >> 1);
				}
				decomp_buf[count++] = c;
			}
		}

		unsigned char *pbuf = (unsigned char*)zbin->bin + 54;
		pbuf += (width * 3 + width_pad)*(height - 1) + channel;
		unsigned char *psbuf = (unsigned char*)decomp_buf;

		//复制到通道
		for (int line = 0; line < height; line++) {
			if (line & 1) {
				for (int k = 0; k < width; k++, pbuf -= 3) *pbuf = *psbuf++;
				pbuf -= width * 3 + width_pad - 3;
			}
			else {
				for (int k = 0; k < width; k++, pbuf += 3) *pbuf = *psbuf++;
				pbuf -= width * 3 + width_pad + 3;
			}
		}

	}

	//zbin->WriteToFile("666.bmp");
	return zbin;
}


uint32_t LOCompressInfo::CopyCharToPtr(char *dst, BinArray *bin, int start, int len) {
	if (len > bin->Length() - start) len = bin->Length() - start;
	//if (len > 0) memcpy(dst, bin->bin, len);
	if (len > 0) memcpy(dst, bin->bin + start, len);
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
		int val = bin->GetIntAuto(&pos);
		memset(buf, 0, 32);
		sprintf(buf, "0x%x,", val);
		s.append(buf);
		if (count % 10 == 9) s.append("\n");
		count++;
	}
	s.append("\n};");
	return s;
}