#ifndef __LOCODEPAGE_H__
#define __LOCODEPAGE_H__

#include <vector>
#include <string>
#include "GBK.h"

extern unsigned short CodePageFullMark_utf8[] ;

//默认为UTF8编码的
class LOCodePage
{
public:
	enum ENCODERTYPE {
		ENCODER_DEFAULT,
		ENCODER_UTF8,
		ENCODER_GBK,
		ENCODER_SHJIS,
		ENCODER_KR,
		ENCODER_BIG5,

		ENCODER_COUNT_SIZE
	};

	enum CHARACT_TYPE {
		CHARACTER_NONE,
		CHARACTER_LETTER = 1,  //字母
		CHARACTER_NUMBER = 2,  //数字
		CHARACTER_SYMBOL = 4,  //符号
		CHARACTER_MULBYTE = 8, //多字节符号的汉字
		CHARACTER_SPACE = 16,  //空格
		CHARACTER_CONTROL = 32, //控制符
		CHARACTER_ZERO = 64     //0
	};

	ENCODERTYPE codeID;

	LOCodePage();
	virtual ~LOCodePage();

	//下一个字的字长，子类必须实现
	virtual int GetCharLen(const char* ch);

	//获取上一个字的字长
	int GetLastCharLen(const char* ch);

	//单个字最长字节，子类必须实现
	virtual int MaxCharLen() { return 4; }

	//获取单个字的utf16，子类必须实现
	virtual unsigned short GetUtf16(const char*buf,int *len);

	//是否全角符号
	virtual bool isFullMark(unsigned short ch);

	//要实现对应编码的itoa2
	virtual std::string Itoa2(int number);

	//是否半角符号
	bool isHalfMark(unsigned short ch);

	//子类无需实现
	virtual std::vector<unsigned short>* GetUtf16Array(std::string *s);

	//子类无需实现
	int GetUtf8Word(unsigned char* dst, unsigned short src);

	//子类无需实现
	std::string GetUtf8String(std::string *s);

	static LOCodePage* CheckCodeAndGet(char *buf, int len);
	static LOCodePage* GetEncoder(int encoder);
private:
	static LOCodePage *baseEnder[ENCODER_COUNT_SIZE];  //同类型的编码器只会出现一个

	void InitCharact();
};

class LOCodePageGBK :public LOCodePage {
public:
	LOCodePageGBK();
	~LOCodePageGBK();

	int MaxCharLen() { return 2; }
	int GetCharLen(const char* ch);
	unsigned short GetUtf16(const char*buf, int *len);
	std::string Itoa2(int number);
private:
	unsigned short* gbk_2_utf16;

};


#endif // !__LOCODEPAGE_H__
