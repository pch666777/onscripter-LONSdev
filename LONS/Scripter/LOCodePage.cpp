#include "LOCodePage.h"

unsigned short CodePageFullMark_utf8[] = { 65292,12290,65311,65281,65306,65307,
12289,65374,65286,65312,65283,8230,8220,8221,8216,8217,12317,12318,65287,12304,
12305,12298,12299,65308,65310,65117,65118,12308,12309,12296,12297,65339,65341,
12300,12301,65371,65373,12310,12311,12302,12303,65295,65372,65340,65343,65103,
65101,65102,65507,65099,65097,65098,65076,12288,0};

unsigned char LOCodePage::charactTable[256];
char LOCodePage::charactOperator[9] = { '+','-','*','/','^','(',')','[',']' };

LOCodePage::LOCodePage()
{
	InitCharact();
	codepage = CODE_UTF8;
}

LOCodePage::~LOCodePage()
{
}


LOCodePage* LOCodePage::CheckCodeAndGet(char *buf, int len) {
	int codep = CODE_GBK;
	return new LOCodePageGBK;
}

//检查字符类型是否已经初始化，没有则初始化
void LOCodePage::InitCharact() {
	if (charactTable[0] == 0 && charactTable[0x7f] == CHARACTER_CONTROL
		&& charactTable[0x5f] == CHARACTER_LETTER) return;

	memset(charactTable, CHARACTER_MULBYTE, 256);

	charactTable[0] = CHARACTER_ZERO;
	for (int ii = 1; ii < 0x20; ii++) charactTable[ii] = CHARACTER_CONTROL;
	charactTable['\t'] = CHARACTER_SPACE;
	charactTable['\r'] = CHARACTER_SPACE;
	charactTable[0x20] = CHARACTER_SPACE;
	for (int ii = 0x21; ii <= 0x2f; ii++) charactTable[ii] = CHARACTER_SYMBOL;
	for (int ii = '0'; ii <= '9'; ii++) charactTable[ii] = CHARACTER_NUMBER;
	for (int ii = 0x3a; ii <= 0x40; ii++) charactTable[ii] = CHARACTER_SYMBOL;
	for (int ii = 'A'; ii <= 'Z'; ii++) charactTable[ii] = CHARACTER_LETTER;
	for (int ii = 0x5b; ii <= 0x60; ii++) charactTable[ii] = CHARACTER_SYMBOL;
	for (int ii = 'a'; ii <= 'z'; ii++) charactTable[ii] = CHARACTER_LETTER;
	for (int ii = 0x7b; ii <= 0x7e; ii++) charactTable[ii] = CHARACTER_SYMBOL;
	charactTable[0x7f] = CHARACTER_CONTROL;
	charactTable[0x5f] = CHARACTER_LETTER;   //下划线作为字母处理


}

char* LOCodePage::SkipSpace(char *buf) {
	while (buf[0] == ' ' || buf[0] == '\t' || buf[0] == '\r') buf++;
	return buf;
}

char* LOCodePage::GetWord(std::string *s, char *buf) {
	s->clear();
	int ctype, type, ulen;
	type = ctype = GetCharacter(buf);
	while (buf[0] != 0) {
		if (ctype != type) return buf;
		else if (type == CHARACT_TYPE::CHARACTER_MULBYTE)ulen = GetCharLen(buf);
		else ulen = 1;
		s->append(buf, ulen);
		buf += ulen;
		type = GetCharacter(buf);
	}
	return buf;
}

char* LOCodePage::GetWordUntil(std::string *s, char *buf, int stop) {
	s->clear();
	int ulen, type = GetCharacter(buf);
	while (!(type & stop)) {
		if (type == CHARACT_TYPE::CHARACTER_MULBYTE) {  //多字节的话要判断字节数
			ulen = GetCharLen(buf);
		}
		else ulen = 1;
		s->append(buf, ulen);
		buf += ulen;
		type = GetCharacter(buf);
	}
	return buf;
}

char* LOCodePage::GetWordStill(std::string *s, char *buf, int still) {
	s->clear();
	int ulen, type = GetCharacter(buf);
	while (type & still) {
		if (type == CHARACT_TYPE::CHARACTER_MULBYTE) ulen = GetCharLen(buf);
		else ulen = 1;
		s->append(buf, ulen);
		buf += ulen;
		type = GetCharacter(buf);
	}
	return buf;
}

int LOCodePage::GetCharacter(const char *buf) {
	return charactTable[(uint8_t)(buf[0])];
}

bool LOCodePage::IsOperator(const char *buf) {
	for (int ii = 0; ii < sizeof(charactOperator); ii++) {
		if (buf[0] == charactOperator[ii]) return true;
	}
	if (buf[0] == 'm' && buf[1] == 'o' && buf[2] == 'd'
		&& GetCharacter(buf + 3) != CHARACTER_LETTER) return true;
	return false;
}

char* LOCodePage::NextLine(char *buf, std::string *s) {
	if (s) s->clear();
	int ulen, type = GetCharacter(buf);
	while (true) {
		if (buf[0] == '\n' || buf[0] == 0) {
			if (buf[0] != 0) buf++;
			return buf;
		}
		else if (type == CHARACT_TYPE::CHARACTER_MULBYTE) {
			ulen = GetCharLen(buf);
		}
		else ulen = 1;
		if (s)s->append(buf, ulen);
		buf += ulen;
		type = GetCharacter(buf);
	}
	return buf;
}

void LOCodePage::ToLower(std::string *s) {
	int count = 0;
	const char* buf = s->c_str();
	while (buf[count] != 0) {
		if (buf[count] >= 'A' && buf[count] <= 'Z')(*s)[count] = buf[count] + 0x20;
		count += GetCharLen(buf + count);
	}
}

char* LOCodePage::GetRealNumber(double *v, char *buf) {
	*v = 0.0;
	buf = SkipSpace(buf);
	int isum = 0;
	while (GetCharacter(buf) == CHARACTER_NUMBER) {
		isum = isum * 10 + (buf[0] - '0');
		buf++;
	}
	if (buf[0] == '.') {
		buf++;
		int iper = 1;
		int dsum = 0;
		while (GetCharacter(buf) == CHARACTER_NUMBER) {
			dsum = dsum * 10 + (buf[0] - '0');
			iper = iper * 10;
			buf++;
		}
		*v = (double)dsum / iper + isum;
	}
	else *v = (double)isum;
	return buf;
}

int LOCodePage::GetStringLen(char *buf) {
	char *obuf = buf;
	while (buf[0] != '"' && buf[0] != '\r' && buf[0] != '\n' && buf[0] != '\0') {
		buf += GetCharLen(buf);
	}
	return buf - obuf;
}

//不要修改char*指向的内容，不要释放！你知道自己在干什么！
char* LOCodePage::ConstCharptrToCharptr(const char *ptr) {
	intptr_t p = (intptr_t)ptr;
	return (char*)p;
}

void LOCodePage::TrimEnd(std::string &s) {
	int nlen = s.length();
	if (nlen < 0) return;
	const char* buf = s.c_str() + s.length() - 1;
	for (int ii = nlen - 1; ii >= 0 && GetCharacter(buf) == CHARACTER_SPACE; ii--) {
		nlen--;
		buf--;
	}
	if (nlen != s.length()) s = s.substr(0, nlen);
}

int LOCodePage::GetCharLen(const char* ch) {
	unsigned char chh = *ch;
	if (((chh >> 7) & 1) == 0) return 1;
	if (((chh >> 4) & 15) == 15) return 4;  //11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
	if (((chh >> 5) & 7) == 7) return 3; //1110xxxx 10xxxxxx 10xxxxxx
	if (((chh >> 6) & 3) == 3) return 2; //110xxxxx 10xxxxxx
	return 1;
}

std::string* LOCodePage::GetUtf8String(std::string *s) {
	std::vector<unsigned short>* vlist = GetUtf16Array(s);
	int slen = 64;
	int clen = 0;
	unsigned char *res = (unsigned char*)malloc(slen);
	for (int ii = 0; ii < vlist->size(); ii++) {
		int ulen = GetUtf8Word(res + clen, vlist->at(ii));
		clen += ulen;
		if (clen > slen - 5) {
			slen = slen * 2;
			res = (unsigned char*)realloc(res, slen);
		}
	}
	std::string *sok = new std::string((char*)res, clen);
	free(res);
	delete vlist;
	return sok;
}


int LOCodePage::CharToInt(const char* buf, int size) {
	if (size <= 0)size = 32;
	int sum = 0;
	for (int ii = 0; ii < size && buf[0] >= '0' && buf[0] <= '9'; ii++) {
		sum = sum * 10 + (buf[0] - '0');
		buf++;
	}
	return sum;
}

int LOCodePage::HexToInt(const char* &buf, int maxlen) {
	int sum = 0;
	while (maxlen > 0) {
		if (tolower(*buf) >= 'a' && tolower(*buf) <= 'f') sum = sum * 16 + tolower(*buf) - 87;
		else if (*buf >= '0' && *buf <= '9') sum = sum * 16 + (*buf) - '0';
		else return sum;
		buf++;
		maxlen--;
	}
	return sum;
}

int LOCodePage::HexToInt(char* &buf, int maxlen) {
	const char *tbuf = buf;
	int ret = HexToInt(tbuf, maxlen);
	buf = (char*)(intptr_t)tbuf;
	return ret;
}

//utf16->utf8
int LOCodePage::GetUtf8Word(unsigned char* dst, unsigned short src) {
	if (src & 0xff80) {
		if (src & 0xf800) {
			// UCS-2 = U+0800 - U+FFFF -> UTF-8 (3 bytes)
			dst[0] = 0xe0 | (src >> 12);
			dst[1] = 0x80 | ((src >> 6) & 0x3f);
			dst[2] = 0x80 | (src & 0x3f);
			dst[3] = 0;
			return 3;
		}

		// UCS-2 = U+0080 - U+07FF -> UTF-8 (2 bytes)
		dst[0] = 0xc0 | (src >> 6);
		dst[1] = 0x80 | (src & 0x3f);
		dst[2] = 0;
		return 2;
	}

	// UCS-2 = U+0000 - U+007F -> UTF-8 (1 byte)
	dst[0] = src;
	dst[1] = 0;
	return 1;
}


unsigned short LOCodePage::GetUtf16(const char*buf, int *len) {
	unsigned short res = 0;
	unsigned char chh = (*buf)&0xff;
	if ((chh >> 7) == 0) { //单字节
		*len = 1;
		res = chh;
	}
	else if ((chh >> 5) == 0x6) { //2个字节
		*len = 2;
		res = (unsigned short)((buf[0] & 0x1f) << 6);
		res |= (buf[1] & 0x3f);
	}
	else if ((chh >> 4) == 0xE) { //3个字节
		*len = 3;
		res = (unsigned short)((buf[0] & 0x0f) << 12);
		res |= (unsigned short)((buf[1] & 0x3f) << 6);
		res |= (buf[2] & 0x3f);
	}
	else if ((chh >> 3) == 0x1E) { //4个字节，超过3字节必须用utf32表示了
		*len = 4;
		//res = (unsigned short)((buf[0] & 0x7) << 18);
		//res |= (unsigned short)((buf[1] & 0x3f) << 12);
		//res |= (unsigned short)((buf[2] & 0x3f) << 6);
		//res |= (buf[3] & 0x3f) ;
	}
	else{  //理论上4个字节足够覆盖了，如果有操作有4个字节的情况可能需要提示一个错误
	}
	return res;
}

//获取utf16数组
std::vector<unsigned short>* LOCodePage::GetUtf16Array(std::string *s) {
	int clen = 0;
	int ulen = 0;
	const char *buf = s->c_str();
	std::vector<unsigned short> *arr = new std::vector<unsigned short>;
	while (clen < s->length()){
		arr->push_back(GetUtf16(buf + clen, &ulen));
		clen += ulen;
	}
	return arr;
}

//从最长字节往回逆推，刚好等于指定长度，则为上一个字节的长度
int LOCodePage::GetLastCharLen(const char* ch) {
	for (int ii = MaxCharLen(); ii > 1; ii--) {
		if (GetCharLen(ch - ii) == ii) return ii;
	}
	return 1;
}

bool LOCodePage::isFullMark(unsigned short ch) {
	for (int ii = 0; CodePageFullMark_utf8[ii] != 0; ii++) {
		if (CodePageFullMark_utf8[ii] == ch) return true;
	}
	return false;
}

bool LOCodePage::isHalfMark(unsigned short ch) {
	if ((ch >= 0x41 && ch <= 0x90) || (ch >= 0x61 && ch <= 0x7a)) return false;
	else return true;
}

std::string LOCodePage::Itoa2(int number) {
	//0-9 is:
	//unsigned char fullNum[] = {0xEF,0xBC,0x90,0xEF,0xBC,0x91,0xEF,0xBC,0x92,0xEF,0xBC,0x93,0xEF,0xBC,0x94,
	//0xEF,0xBC,0x95 ,0xEF,0xBC,0x96 ,0xEF,0xBC,0x97 ,0xEF,0xBC,0x98 ,0xEF,0xBC,0x99};
	unsigned char *cbuf = new unsigned char[34];
	unsigned char *buf = cbuf + 30;
	bool isnegative = (number < 0);
	memset(cbuf, 0, 34);

	do {
		int ii = abs(number) % 10;
		buf[0] = 0xef;
		buf[1] = 0xbc;
		buf[2] = 0x90 + ii;

		number /= 10;
		buf -= 3;
	} while (number != 0);

	if (isnegative) {
		buf[0] = 0xe2;
		buf[1] = 0x80;
		buf[2] = 0x94;
	}
	else buf += 3;

	std::string s((char*)buf);
	delete[] cbuf;
	return s;
}


LOCodePageGBK::LOCodePageGBK() {
	codepage = CODE_GBK;
	gbk_2_utf16 = new unsigned short[65536 - 0x8140 + 1];
	int i = 0;
	while (gbk_2_utf16_org[i][0]) {
		gbk_2_utf16[i] = gbk_2_utf16_org[i][1];
		i++;
	}
}

LOCodePageGBK::~LOCodePageGBK() {
	delete[] gbk_2_utf16;
}

int LOCodePageGBK::GetCharLen(const char* ch) {
	unsigned char chh = (*ch) & 0xff;
	if (chh > 0x7f) return 2;
	else return 1;
}

unsigned short LOCodePageGBK::GetUtf16(const char*buf, int *len) {
	*len = GetCharLen(buf);
	if ((*len) == 1) {
		return buf[0] & 0xff;
	}
	else {
		unsigned short in = ((buf[0] & 0xff) << 8) + (buf[1] & 0xff);
		return gbk_2_utf16[in - 0x8140];
	}
}


std::string LOCodePageGBK::Itoa2(int number) {
	//0-9 is:A3 B0 -> A3 B9  negative is A1 AA
	unsigned char *cbuf = new unsigned char[23];
	unsigned char *buf = cbuf + 20;
	bool isnegative = (number < 0);
	memset(cbuf, 0, 23);

	do {
		int ii = abs(number) % 10;
		buf[0] = 0xA3;
		buf[1] = 0xb0 + ii;

		number /= 10;
		buf -= 2;
	} while (number != 0);

	if (isnegative) {
		buf[0] = 0xa1;
		buf[1] = 0xaa;
	}
	else buf += 2;

	std::string s((char*)buf);
	delete[] cbuf;
	return s;
}


