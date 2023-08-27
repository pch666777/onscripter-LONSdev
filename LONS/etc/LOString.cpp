/*
//带编码信息的字符串，c++是否总是陷入造轮子的怪圈？
*/
#include "LOString.h"
#include <string.h>

//已经经过InitCharact生成
unsigned char LOString::charactTable[256] = {
	0x40, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x10, 0x20, 0x20, 0x20, 0x10, 0x20, 0x20,
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
	0x10, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
	0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
	0x04, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x04, 0x04, 0x04, 0x04, 0x01,
	0x04, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x04, 0x04, 0x04, 0x04, 0x20,
	0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
	0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
	0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
	0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
	0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
	0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
	0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
	0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08
};

int LOString::defaultEncoder = LOCodePage::ENCODER_UTF8;

char LOString::charactOperator[9] = { '+','-','*','/','^','(',')','[',']' };


void LOString::SetDefaultEncoder(int codeID) {
	defaultEncoder = codeID;
}

LOString::LOString() {
	_encoder = LOCodePage::GetEncoder(defaultEncoder);
}

LOString::~LOString() {
	//printf("~LOString()\n");
}

LOString::LOString(const char *ptr):std::string(ptr) {
	_encoder = LOCodePage::GetEncoder(defaultEncoder);
	//printf("0x%llx\n", &_encoder);
}

LOString::LOString(const char *ptr, int slen) : std::string(ptr, slen) {
	_encoder = LOCodePage::GetEncoder(defaultEncoder);
}

LOString::LOString(const std::string &str) : std::string(str) {
	_encoder = LOCodePage::GetEncoder(defaultEncoder);
}

LOString::LOString(const LOString &str) : std::string(str){
	_encoder = str._encoder;
	//if(&str) _encoder = str._encoder;
	//else _encoder = LOCodePage::GetEncoder(LOString::defaultEncoder);
}

LOString::LOString(const LOString &str, int start, int slen) : std::string(str.c_str() + start, slen) {
	_encoder = str._encoder;
}

LOString::LOString(const char *ptr, LOCodePage *enc) :std::string(ptr){
	_encoder = enc;
}

//int LOString::GetCharacter(char ch) {
//	return charactTable[(uint8_t)ch];
//}

int LOString::GetCharacter(int cur) {
	if (cur < 0 || cur >= length()) return CHARACTER_NONE;
	else return charactTable[(uint8_t)(at(cur))];
}

int LOString::GetCharacter(const char *buf) {
	return charactTable[(uint8_t)(buf[0])];
}

void LOString::SetEncoder(LOCodePage *encoder) {
	_encoder = encoder;
}

bool LOString::checkCurrent(int cur) {
	if (cur < 0 || cur >= length()) return false;
	else return true;
}

int LOString::SkipSpace(int &current) {
	while (checkCurrent(current)){
		char c = at(current);
		if (c == ' ' || c == '\t' || c == '\r') current++;
		else return current;
	}
	return -1;
}

const char* LOString::SkipSpace(const char *buf) {
	while (buf[0] == ' ' || buf[0] == '\t' || buf[0] == '\r') buf++;
	return buf;
}



//获取跟当前字符同样类型的单词
LOString LOString::GetWord(int &current) {
	LOString s;
	current = GetWordBase(s, current, 0, GETWORD_LIKE);
	return s;
}

LOString LOString::GetWord(const char *&buf) {
	LOString s;
	buf = GetWordBase(s, buf, 0, GETWORD_LIKE);
	return s;
}



//知道指定的符号才停止
LOString LOString::GetWordUntil(int &current, int stop) {
	LOString s;
	current = GetWordBase(s, current, stop, GETWORD_UNTILL);
	return s;
}

LOString LOString::GetWordUntil(const char *&buf, int stop) {
	LOString s;
	buf = GetWordBase(s, buf, stop, GETWORD_UNTILL);
	return s;
}

LOString LOString::substr(int start, int len) {
	LOString s;
	if (!checkCurrent(start) && len <= 0) return s;
	s =  std::string::substr(start, len);
	s._encoder = _encoder;
	return s;
}

LOString LOString::substr(const char*buf, int len) {
	LOString s(buf, len);
	s._encoder = _encoder;
	return s;
}

//全角符号算一个char
LOString LOString::substrWord(int start, int len) {
	LOString s;
	if (!checkCurrent(start)) return s;
	int tcur = 0;  //确定正确的开始位置
	for (int ii = 0; ii < start; ii++) {
		tcur += ThisCharLen(tcur);
	}
	int cur = tcur;
	for (int ii = 0; ii < len; ii++) {
		cur += ThisCharLen(cur);
	}
	return substr(tcur,cur - tcur);
}

LOString LOString::GetWordStill(int &current, int still) {
	LOString s;
	current = GetWordBase(s, current, still, GETWORD_STILL);
	return s;
}

LOString LOString::GetWordStill(const char *&buf, int still) {
	LOString s;
	buf = GetWordBase(s, buf, still, GETWORD_STILL);
	return s;
}


int LOString::GetWordBase(LOString &s, int current, int stop, int type) {
	s.clear();
	if (!checkCurrent(current)) return -1;
	
	const char *buf = c_str();
	int start = current;
	if (type == GETWORD_LIKE) stop = GetCharacter(current);
	else if (type == GETWORD_UNTILL) stop = ~stop;

	while ( current < length() && (GetCharacter(current) & stop)) {
		current += _encoder->GetCharLen(buf + current);
	}
	s = substr(start, current - start);
	return current;
}

const char* LOString::GetWordBase(LOString &s, const char *buf, int stop, int type) {
	s.clear();
	const char *ebuf = e_buf();
	const char *obuf = buf;

	if (type == GETWORD_LIKE) stop = GetCharacter(buf);
	else if (type == GETWORD_UNTILL) stop = ~stop;

	while ( buf < ebuf && (GetCharacter(buf) & stop)) {
		buf += _encoder->GetCharLen(buf);
	}
	s = substr(obuf, buf - obuf);
	return buf;
}


LOString LOString::toLower() {
	LOString s = *this;
	const char *buf = c_str();
	for (int ii = 0; ii < length(); ) {
		int ulen = _encoder->GetCharLen(buf + ii);
		if (ulen == 1 && buf[ii] >= 'A' && buf[ii] <= 'Z') {
			s[ii] = buf[ii] + 0x20;
		}

		ii += ulen;
	}
	return s;
}

LOString LOString::toLowerAndRePathSymble(char pathSymble) {
	LOString s = *this;
	const char *buf = c_str();
	for (int ii = 0; ii < length(); ) {
		int ulen = _encoder->GetCharLen(buf + ii);
		if (ulen == 1) {
			if(buf[ii] >= 'A' && buf[ii] <= 'Z') s[ii] = buf[ii] + 0x20;
			else if (buf[ii] == '\\' || buf[ii] == '/') s[ii] = pathSymble;
		}

		ii += ulen;
	}
	return s;
}

LOString LOString::RePathSymble(char pathSymble){
    LOString s = *this;
    const char *buf = c_str();
    for (int ii = 0; ii < length(); ) {
        int ulen = _encoder->GetCharLen(buf + ii);
        if (ulen == 1) {
            if (buf[ii] == '\\' || buf[ii] == '/') s[ii] = pathSymble;
        }

        ii += ulen;
    }
    return s;
}


bool LOString::IsOperator(int current) {
	if (!checkCurrent(current)) return false;

	for (int ii = 0; ii < sizeof(charactOperator); ii++) {
		if (at(current) == charactOperator[ii]) return true;
	}
	if (current < length() - 4 && GetCharacter(current + 3) != CHARACTER_LETTER) {
		char ch;
		const char *s1 = "modMOD";
		for (int ii = 0; ii < 3; ii++) {
			ch = at(current + ii);
			if (ch != s1[ii] && ch != s1[ii + 3])return false;
		}
		return true;
	}
	return false;
}

bool LOString::IsOperator(const char *buf) {
	const char *ebuf = e_buf();
	if (buf >= ebuf) return false;
	for (int ii = 0; ii < sizeof(charactOperator); ii++) {
		if (buf[0] == charactOperator[ii]) return true;
	}
	if (buf < ebuf - 4 && GetCharacter(buf + 3) != CHARACTER_LETTER) {
		const char *s1 = "modMOD";
		for (int ii = 0; ii < 3; ii++) {
			if (buf[ii] != s1[ii] && buf[ii] != s1[ii + 3])return false;
		}
		return true;
	}
	return false;
}

bool LOString::IsMod(const char *buf) {
	const char *ebuf = e_buf();
	if (buf < ebuf - 4 && GetCharacter(buf + 3) != CHARACTER_LETTER) {
		const char *s1 = "modMOD";
		for (int ii = 0; ii < 3; ii++) {
			if (buf[ii] != s1[ii] && buf[ii] != s1[ii + 3])return false;
		}
		return true;
	}
	return false ;
}


int LOString::WordLength() {
	const char *buf = c_str();
	int count = 0;
	for (int ii = 0; ii < length(); ) {
		ii += _encoder->GetCharLen(buf + ii);
		count++;
	}
	return count;
}

//获得两个引号之间的字符串长度
int LOString::GetStringLen(const int cur) {
	if (!checkCurrent(cur)) return 0;

	const char *buf = c_str() + cur;
	const char *obuf = buf;

	while (buf[0] != '"' && buf[0] != '\r' && buf[0] != '\n' && buf[0] != '\0') {
		buf += _encoder->GetCharLen(buf);
	}
	return buf - obuf;
}

int LOString::GetStringLen(const char* buf) {
	const char *obuf = buf;

	while (buf[0] != '"' && buf[0] != '\r' && buf[0] != '\n' && buf[0] != '\0') {
		buf += _encoder->GetCharLen(buf);
	}
	return buf - obuf;
}

LOString LOString::GetString(const char*&buf) {
	const char *ebuf = e_buf();
	const char *obuf = buf;
	buf = SkipSpace(buf);
	LOString s;
	if (buf[0] == '"') {
		buf++;
		while (buf[0] != '"' && buf[0] != '\r' && buf[0] != '\n' && buf < ebuf) {
			buf += _encoder->GetCharLen(buf);
		}
		s = substr(obuf + 1, buf - obuf - 1);
		if (buf[0] == '"') buf++;
	}
	else buf = obuf;
	return s;
}


int LOString::NextLine(int cur) {
	if (!checkCurrent(cur)) return -1;
	const char *buf = c_str();

	while (cur < length()) {
		if (buf[cur] == '\n') {
			if (cur < length() - 1) return cur + 1;
			else return -1;
		}
		cur += _encoder->GetCharLen(buf + cur);
	}
	return cur;
}

const char* LOString::NextLine(const char *buf) {
	const char* ebuf = c_str() + length();
	while (buf < ebuf) {
		if (buf[0] == '\n') {
			if (buf < ebuf - 1) buf++;
			return buf;
		}
		buf += _encoder->GetCharLen(buf);
	}
	return buf;
}

double LOString::GetReal(int &cur) {
	if (!checkCurrent(cur)) {
		cur = -1;
		return 0.0;
	}
	double isum = 0.0;
	while (GetCharacter(cur) == CHARACTER_NUMBER) {
		isum = isum * 10 + (at(cur) - '0');
		cur++;
	}

	if (cur >= length() - 1) return isum;
	if (at(cur) == '.') {
		cur++;
		int iper = 1;
		int dsum = 0;
		while (GetCharacter(cur) == CHARACTER_NUMBER) {
			dsum = dsum * 10 + (at(cur) - '0');
			iper = iper * 10;
			cur++;
		}
		isum += ((double)dsum / iper);
	}
	return isum;
}

double LOString::GetReal(const char *&buf) {
	const char *ebuf = e_buf();
	if (buf > ebuf) return 0.0;
	double isum = 0.0;
	while (GetCharacter(buf) == CHARACTER_NUMBER) {
		isum = isum * 10 + (buf[0] - '0');
		buf++;
	}

	if (buf >= ebuf) return isum;
	if (buf[0] == '.') {
		buf++;
		int iper = 1;
		int dsum = 0;
		while (GetCharacter(buf) == CHARACTER_NUMBER) {
			dsum = dsum * 10 + (buf[0] - '0');
			iper = iper * 10;
			buf++;
		}
		isum += ((double)dsum / iper);
	}
	return isum;
}

double LOString::GetRealNe(const char *&buf) {
	if (buf[0] == '-') {
		buf++;
		return 0.0 - GetReal(buf);
	}
	return GetReal(buf);
}


LOString LOString::TrimEnd() {
	int cur = length() - 1;
	while (cur >= 0 && GetCharacter(cur) == CHARACTER_SPACE) {
		cur--;
	}
	if (cur != length() - 1) return substr(0, cur + 1);
	else return *this;
}

int LOString::GetInt(int &cur, int size) {
	const char *buf = c_str() + cur;
	int val = GetInt( buf , size);
	cur = buf - c_str();
	return val;
}

int LOString::GetInt(const char *&buf, int size) {
	if (buf >= e_buf()) return 0;
	if (size <= 0) size = 32;
	int sum = 0;
	for (int ii = 0; buf[0] != 0 && ii < size && buf[0] >= '0' && buf[0] <= '9'; ii++) {
		sum = sum * 10 + (buf[0] - '0');
		buf++;
	}
	return sum;
}


int LOString::GetHexInt(int &cur, int size) {
	const char *buf = c_str() + cur;
	int val = GetHexInt(buf, size);
	cur = buf - c_str();
	return val;
}

int LOString::GetHexInt(const char *&buf, int size) {
	if (buf >= e_buf()) return 0;
	if (size <= 0) size = 8;
	int sum = 0;
	for (int ii = 0; buf[0] != 0 && ii < size; ii++) {
		char ch = tolower(buf[0]);
		if (ch >= 'a' && ch <= 'f') sum = sum * 16 + ch - 87;
		else if (ch >= '0' && ch <= '9') sum = sum * 16 + ch - '0';
		else return sum;
		buf++;
	}
	return sum;
}


LOString LOString::GetTagString(int &cur) {
	SkipSpace(cur);
	if (at(cur) == '[') {
		int ocur = cur;
		cur++;
		SkipSpace(cur);
		while (cur < length() && at(cur) != ']') cur += _encoder->GetCharLen(c_str() + cur);
		if (cur > ocur) return substr(ocur, cur - 1);
	}
	return LOString();
}

//char版本速度要快一倍这样，调试时更方便
LOString LOString::GetTagString(const char *&buf) {
	LOString s;
	const char *ebuf = c_str() + length();
	if (buf[0] == '[') {
		buf++;
		const char *obuf = buf;
		while (buf < ebuf && buf[0] != ']') buf += _encoder->GetCharLen(buf);
		if(buf > obuf) s = substr(obuf - c_str(), buf - obuf);
		if (buf[0] == ']') buf++;
	}
	return s;
}

int LOString::ThisCharLen(int cur) {
	if (!checkCurrent(cur)) return 0;
	const char *buf = c_str() + cur;
	return _encoder->GetCharLen(buf);
}

int LOString::ThisCharLen(const char *buf) {
	return _encoder->GetCharLen(buf);
}


std::vector<LOString> LOString::Split(LOString &tag) {
	//无效的分割，不产生结果
	std::vector<LOString> list;
	if (&tag == NULL || tag.length() == 0 || length() == 0) return list;
	int cur = 0;
	int last = 0;
	while (cur < length()) {
		if (compare(cur, tag.length(), tag.c_str()) == 0) {
			LOString s = substr(last, cur - last);
			list.push_back(s);
			cur += tag.length();
			last = cur;
		}
		else cur += _encoder->GetCharLen(c_str() + cur);
	}
	//最后增加一个
	list.push_back(substr(last, cur - last));
	return list;
}


LOString LOString::Itoa2(int number) {
	LOString s(_encoder->Itoa2(number));
	s._encoder = _encoder;
	return s;
}


void LOString::SetStr(LOString *&s1, LOString *&s2, bool ismove) {
	if (s1) delete s1;
	s1 = NULL;
	if (ismove) {
		s1 = s2;
		s2 = NULL;
	}
	else if (s2 && s2->length() > 0) s1 = new LOString(s2->c_str(), s2->GetEncoder());
}


LOString LOString::RandomStr(int len) {
	//随机数种子完全可能没有初始化
	//这里利用new出来的地址是唯一的这个特性来混合随机数
	char *tmp = new char[len + 2];
	memset(tmp, 0, len + 2);
	for (int ii = 0; ii < len; ii++) {
		unsigned int val = rand();
		val ^= (intptr_t)tmp;
		tmp[ii] = val % 26 + 65;
	}
	LOString s(tmp);
	delete[] tmp;
	return s;
}

uintptr_t LOString::HexToNum(const char* buf) {
	uintptr_t sum = 0;
	while (buf[0] != 0) {
		char cc = tolower(buf[0]);
		sum *= 16;
		if (cc >= '0' && cc <= '9') sum += (cc - '0');
		else if (cc >= 'a' && cc <= 'f') sum += (cc - 'a' + 10);
		else return 0;
		buf++;
	}
	return sum;
}


int LOString::HashStr(const char* buf) {
	int hash = 0x7BCA5D32;
	while (buf[0] != 0) {
		hash = (hash << 4) ^ (hash >> 28) ^ buf[0];
		buf++;
	}
	return hash;
}

int LOString::HashStr() {
	return HashStr(c_str());
}

//转换路径分隔符风格
LOString LOString::PathTypeTo(PATH_TYPE ptype) {
	int len = length();
	char *temp = new char[len + 1];
	memcpy(temp, c_str(), len + 1);

	char a, b;
	if (ptype == PATH_WINDOWS) {
		a = '/'; b = '\\';
	}
	else {
		a = '\\'; b = '/';
	}

	for (int ii = 0; ii < len; ii += ThisCharLen(temp + ii)) {
		if (temp[ii] == a) temp[ii] = b;
	}
	LOString s(temp);
	s.SetEncoder(GetEncoder());
	delete[] temp;
	return s;
}


//获取指定行的行首buf，超出范围返回null，注意效率很低
const char* LOString::GetLineBuf(int line) {
	const char* buf = c_str();
	const char* ebuf = e_buf();
	if (line <= 0) return buf;

	int count = 0;
	while (buf < ebuf) {
		buf = NextLine(buf);
		count++;
		if (count == line) return buf;
	}
	return nullptr;
}

void LOString::SelfToUtf8(){
	if(length() > 1 && GetEncoder()->codeID != LOCodePage::ENCODER_UTF8){
		std::string s = GetEncoder()->GetUtf8String(this) ;
		this->assign(s.c_str()) ;
	}
}


LOString LOString::ToUtf8() {
	LOString s;
	if (GetEncoder()->codeID == LOCodePage::ENCODER_UTF8) {
		s.assign(this->c_str());
		s.SetEncoder(LOCodePage::GetEncoder(LOCodePage::ENCODER_UTF8));
	}
	else {
		s = GetEncoder()->GetUtf8String(this);
		s.SetEncoder(LOCodePage::GetEncoder(LOCodePage::ENCODER_UTF8));
	}
	return s;
}


LOString LOString::GetRightOfChar(char c) {
	//只考虑英文
	for (int ii = length() - 1; ii >= 0; ii--) {
		if (at(ii) == c) {
			LOString s(c_str() + ii + 1);
			s.SetEncoder(GetEncoder());
			return s;
		}
	}
	return LOString();
}
