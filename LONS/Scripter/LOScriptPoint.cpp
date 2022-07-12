#include "LOScriptPoint.h"

//每隔LINEINTERVAL做一个记录，方便任意buf查询位于哪一行
#define LINEINTERVAL 80


LOScriptPoint::LOScriptPoint() {
	file = NULL;
	s_buf = 0;
	s_line = 0;
}

LOScriptPoint::~LOScriptPoint() {
}

void LOScriptPointCall::Serialize(BinArray *bin) {

	int len = bin->Length() + 4;
	//'poin', len, version
	bin->WriteInt3(0x6E696F70, 0, 1);
	bin->WriteLOString(&file->Name);
	bin->WriteLOString(&name);
	//重新获取行信息，因为运行的时候 c_line可能是错误的
	auto line = file->GetLineInfo(c_buf, 0, false);
	if (!line.buf) {
		LOLog_e("Get running point info error! file[%s],lable[%s],at buf[%d]", file->Name.c_str(), name.c_str(), c_buf - s_buf);
	}
	//相对行
	bin->WriteInt(line.lineID - s_line);
	//相对于行首有多少长度
	bin->WriteInt(c_buf - line.buf);
	//call类型
	bin->WriteInt(callType);

	//记录后4个字节核验，我想不会有人在脚本尾部写个savepoint吧
	bin->WriteInt(*(int*)c_buf);

	bin->WriteInt(bin->Length() - len, &len);

}

LOScriptPointCall::LOScriptPointCall() {
	callType = CALL_BY_NORMAL;
	//当前执行到的行
	c_line = 0;
	//当前执行到的位置
	c_buf = nullptr;
}

LOScriptPointCall::LOScriptPointCall(LOScriptPoint *p) {
	name = p->name;
	s_buf = p->s_buf;
	s_line = p->s_line;
	file = p->file;
	c_line = s_line;
	c_buf = s_buf;
}

LOScriptPointCall::~LOScriptPointCall() {

}


// *** ================ *** //
LogicPointer::LogicPointer(int t) {
	forVar = nullptr;
	dstVar = nullptr;
	reset();
	flags = t;
}

LogicPointer::~LogicPointer() {
	reset();
}

void LogicPointer::reset() {
	flags = 0;
	pointLine = 0;
	step = 1;
	if (forVar) delete forVar;
	if (dstVar) delete dstVar;
	forVar = nullptr;
	dstVar = nullptr;
	point = nullptr;
	label = nullptr;
}

void LogicPointer::SetRet(bool it) {
	if (it) flags |= TYPE_RESULT_TRUE;
	else flags &= (~TYPE_RESULT_TRUE);
}

//void LogicPointer::SetPoint(LOScriptPointCall *p) {
//	point = p->c_buf;
//	pointLine = p->c_line;
//	label = p;
//}
//
//void LogicPointer::BackToPoint(LOScriptPointCall *p) {
//	p->p
//}

void LogicPointer::Serialize(BinArray *bin) {
	int len = bin->Length() + 4;
	//'lpos', len, version
	bin->WriteInt3(0x736F706C, 0 , 1);
	auto line = label->file->GetLineInfo(point, 0, false);

	//属于哪个label的
	bin->WriteLOString(&label->name);
	//相对于label的位置
	bin->WriteInt(line.lineID - label->s_line);
	//相对于行首的位置
	bin->WriteInt(point - line.buf);

	bin->WriteInt(step);
	if (forVar) bin->WriteInt(forVar->GetTypeRefid());
	else bin->WriteInt(0);
	if (dstVar) bin->WriteInt(dstVar->GetTypeRefid());
	else bin->WriteInt(0);

	bin->WriteInt(bin->Length() - len, &len);
}


//最后一个参数表示如果有重复的标签是否用重复标签替代，默认为第一个标签生效
LOScripFile::LOScripFile(const char *cbuf, int len, const char *filename) {
	scriptbuf.assign(cbuf, len);
	Name.assign(filename);

	//根据脚本的字符编码获得对应的编码器，最大搜索128kb
	if (len > 1048576) len = 1048576;
	
	scriptbuf.SetEncoder(LOCodePage::GetEncoder(LOCodePage::ENCODER_GBK)); //先来个默认的顶着
	CreateRootLabel();
}

void LOScripFile::CreateRootLabel() {
	//添加一个根标签
	LOScriptPoint *root = new LOScriptPoint();
	root->name = "__init__";
	root->s_buf = scriptbuf.c_str();
	root->s_line = 0;
	root->file = this;
	labels[root->name] = root;
}

//创建一个文件
LOScripFile::LOScripFile(LOString *s, const char *filename) {
	scriptbuf.assign(*s);
	scriptbuf.SetEncoder(s->GetEncoder());
	if (!filename) Name = "Anonymous_" + LOString::RandomStr(4);
	else Name.assign(filename);
	InitLables();
}

LOScripFile::~LOScripFile() {
	ClearLables();
}

void LOScripFile::ClearLables() {
	for (auto iter = labels.begin(); iter != labels.end(); iter++) {
		delete (iter->second);
	}
}

void LOScripFile::InitLables(bool lableRe) {
	ClearLables();
	lineInfo.clear();

	//搜索出所有的标签
	int linecount = 1;
	const char *buf = scriptbuf.c_str();
	const char *ebuf = buf + scriptbuf.length();
	LOCodePage *encoder = scriptbuf.GetEncoder();

	//这个过程是需要花费一定时间的，7M文本在电脑上大约是 500- 600毫秒
	while (buf < ebuf - 1) {
		buf = scriptbuf.SkipSpace(buf);
		//记录点
		if ((linecount - 1) % LINEINTERVAL == 0) lineInfo.emplace_back(linecount, buf);

		if (buf[0] == '*') {
			while (buf[0] == '*') buf++;
			auto *label = new LOScriptPoint();
			label->name = scriptbuf.GetWordUntil(buf, LOCodePage::CHARACTER_ZERO | LOCodePage::CHARACTER_CONTROL | LOCodePage::CHARACTER_SPACE | LOCodePage::CHARACTER_SYMBOL);
			buf = scriptbuf.NextLine(buf);
			linecount++;

			label->name = label->name.toLower();
			label->s_buf = buf;
			label->s_line = linecount;
			label->file = this;

			//标签加入
			if (!lableRe) {
				auto iter = labels.find(label->name);
				if (iter == labels.end()) labels[label->name] = label;
				else delete label;
			}
			else labels[label->name] = label;
		}
		else {
			buf = scriptbuf.NextLine(buf);
			linecount++;
		}
	}

	//增加最后一行，方便搜索
	lineInfo.emplace_back(linecount + 1, buf + 1);
}

LOScriptPoint *LOScripFile::FindLable(std::string &lname) {
	auto iter = labels.find(lname);
	if (iter == labels.end()) return NULL;
	return iter->second;
}

LOScriptPoint *LOScripFile::FindLable(const char *lname) {
	std::string tmp(lname);
	return FindLable(tmp);
}

LOScripFile::LineData* LOScripFile::MidFindBaseIndex(const char *buf, int dstLine, bool isLine) {
	//检查有效性
	if (isLine && (dstLine < 1 || dstLine > lineInfo.back().lineID)) return nullptr;
	if (!isLine && (buf < lineInfo.front().buf || buf > lineInfo.back().buf)) return nullptr;
	if (lineInfo.size() == 1) return &lineInfo.front();

	int left = 0;
	int right = lineInfo.size() - 1;
	int mid = right / 2;
	while (true) {
		//落在右边
		if ((isLine && dstLine <= lineInfo.at(mid).lineID) ||
			(!isLine && buf <= lineInfo.at(mid).buf)) {
			// (mid + mid + 1 ) / 2 还是等于mid，所以已经搜索到尾部了
			if (right - mid <= 1) return &lineInfo.at(mid);
			left = mid;
		}
		else {
			//落在左边
			right = mid;
		}

		mid = (left + right) / 2;
	}
	return nullptr;
}


void LOScripFile::NextFindDest(const char *dst, int dstLine, const char *&startBuf, int &startLine, bool isLine) {
	const char *endbuf = scriptbuf.c_str() + scriptbuf.length();
	while (startBuf < endbuf) {
		const char *nextbuf = scriptbuf.NextLine(startBuf);
		if((isLine && dstLine == startLine) || (!isLine && dst >= startBuf && dst < nextbuf )) return ;
		startBuf = nextbuf;
		startLine++;
	}
}

LOScripFile::LineData LOScripFile::GetLineInfo(const char *buf, int lineID, bool isLine) {
	LineData *index = MidFindBaseIndex(buf, lineID, isLine);
	LineData data(-1, nullptr);
	if (!index) return data;
	data = *index;
	NextFindDest(buf, lineID, data.buf, data.lineID, isLine);
	return data;
}