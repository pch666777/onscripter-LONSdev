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
	//代码页
	enum CodePage{
		CODE_UTF8,   //utf8
		CODE_GBK,    //gbk
		CODE_SFJIS,  //日文编码
		CODE_KR,     //韩文编码
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

	CodePage codepage;

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
	std::string* GetUtf8String(std::string *s);

	//
	static char* SkipSpace(char *buf);
	static int CharToInt(const char* buf, int size = -1);
	static int HexToInt(const char* &buf, int maxlen = 8);
	static int HexToInt(char* &buf, int maxlen = 8);

	char* GetWord(std::string *s, char *buf);
	char* GetWordUntil(std::string *s, char *buf, int stop);
	char* GetWordStill(std::string *s, char *buf, int still);
	int GetCharacter(const char *buf);
	bool IsOperator(const char *buf);
	char* NextLine(char *buf, std::string *s);
	void ToLower(std::string *s);
	char* GetRealNumber(double *v, char *buf);
	int GetStringLen(char *buf);
	char* ConstCharptrToCharptr(const char *ptr);
	void TrimEnd(std::string &s);

	static LOCodePage* CheckCodeAndGet(char *buf, int len);
private:
	static unsigned char charactTable[256];  //0-255每一个字符的类型
	static char charactOperator[9]; //符号
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
