/*
//带编码信息的字符串，c++是否总是陷入造轮子的怪圈？
*/
#ifndef __LOSTRING_H__
#define __LOSTRING_H__

#include "LOCodePage.h"
#include <string>
#include <memory>
#include <random>   //c++ 11


class LOString :public std::string
{
public:

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

	LOString();
	LOString(const char *ptr);
	LOString(const char *ptr, LOCodePage *enc);
	LOString(const char *ptr, int slen);
	LOString(const std::string &str);
	LOString(const LOString &str);
	LOString(const LOString &str, int start, int slen);
	~LOString();

	int SkipSpace(int &current);
	const char* SkipSpace(const char *buf);
	LOString GetWord(int &current);
	LOString GetWord(const char *&buf);
	LOString GetWordUntil(int &current, int stop);
	LOString GetWordUntil(const char *&buf, int stop);
	LOString GetWordStill(int &current, int still);
	LOString GetWordStill(const char *&buf, int still);
	LOString toLower();
	LOString GetTagString(int &cur);
	LOString GetTagString(const char *&buf);

	std::vector<LOString> Split(LOString &tag);
	std::vector<LOString> SplitConst(LOString tag) { return Split(tag); };
	bool IsOperator(int current);
	bool IsOperator(const char *buf);
	bool IsMod(const char *buf);
	int WordLength();
	int GetStringLen(const int cur);
	int GetStringLen(const char* buf);
	LOString GetString(const char*&buf);

	int NextLine(int cur);
	const char* NextLine(const char *buf);
	int ThisCharLen(int cur);
	int ThisCharLen(const char *buf);
	double GetReal(int &cur);
	double GetReal(const char *&buf);
	LOString TrimEnd();
	int GetInt(int &cur, int size = 32);
	int GetInt(const char *&buf, int size = 32);
	int GetHexInt(int &cur, int size = 8);
	int GetHexInt(const char *&buf, int size = 8);
	
	int SkipSpaceConst(int current) { return SkipSpace(current); }
	LOString GetWordConst(int current) { return GetWord(current); }
	LOString GetWordUntilConst(int current, int stop) { return GetWordUntil(current, stop); }
	double GetRealConst(int cur) { return GetReal(cur); }

	int GetCharacter(int cur);
	int GetCharacter(const char *buf);
	void SetEncoder(LOCodePage *encoder);
	LOCodePage *GetEncoder() { return _encoder; }
	inline bool checkCurrent(int cur);
	LOString substr(int start, int len);
	LOString substr(const char*buf, int len);
	LOString substrWord(int start, int len);
	LOString Itoa2(int number);
	const char* e_buf() { return c_str() + length(); }

	static void SetStr(LOString *&s1, LOString *&s2, bool ismove);
	static LOString RandomStr(int len);
	static uintptr_t HexToNum(const char* buf);
	static int HashStr(const char* buf);
private:
	enum {
		GETWORD_LIKE,
		GETWORD_UNTILL,
		GETWORD_STILL
	};
	LOCodePage *_encoder;
	static unsigned char charactTable[256];
	static char charactOperator[9]; //符号
	int GetWordBase(LOString &s, int current, int stop, int type);
	const char* GetWordBase(LOString &s, const char *buf, int stop, int type);
};


typedef std::shared_ptr<LOString> LOShareString;

#endif // !__LOSTRING_H__