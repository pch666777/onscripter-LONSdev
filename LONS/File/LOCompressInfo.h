/*
//解压缩使用
*/

#include "../etc/BinArray.h"

class LOCompressInfo{
public:
	LOCompressInfo();
	~LOCompressInfo();

	void InitLZSS();
	void Clear();
	int GetBit(BinArray *bin, int bitlen);
	BinArray *UncompressLZSS(BinArray *bin, int uncom_size);
	BinArray *ZlibCompress(BinArray *bin);
	BinArray *UnZlibCompress(BinArray *bin);
	std::string GetCdata(BinArray *bin);

	int mask;      //当前的bit掩码位
	int r;         //滑动窗口位置
	char* decomp_buf;  //解压时用的缓冲区
	char* decomp_dic;  //解压时用的字典
private:
	int bufsize;       //缓冲区大小
	int dicsize;      //字典大小
	int maxrenum;     //允许的最大重复长度
	int dicKey;      //字典的当前值
	int byteCount;   //当前已经读取到的字节位置
	uint32_t bitCount; //累计循环的bit计数
	uint32_t bitSize;  //最大的bit计数

	uint32_t CopyCharToPtr(char *dst, BinArray *bin, int start, int len);
};