#include "LOScriptPoint.h"

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
	int lineID = 0;
	const char *lineStart = nullptr;
	file->GetBufLine(c_buf, &lineID, lineStart);
	if (!lineStart) {
		LOLog_e("Get running point info error! file[%s],lable[%s],at buf[%d]", file->Name.c_str(), name.c_str(), c_buf - s_buf);
	}
	//相对行
	bin->WriteInt(lineID - s_line);
	//相对于行首有多少长度
	bin->WriteInt(c_buf - lineStart);
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
	//adress = nullptr;
	startLine = -1;
	relAdress = 0;
	dstvar = nullptr;
	step = 1;   //默认递增量
	var = nullptr;
	type = t;
	lableName = nullptr;
}

LogicPointer::~LogicPointer() {
	if (var) delete var;
	if (dstvar)delete dstvar;
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
		if (linecount % 100 == 0) {
			LineData it;
			it.buf = buf;
			it.lineID = linecount;
			lineInfo.push_back(it);
		}

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

//获取指定地址对应的行
//指定的buf，存储行ID的变量，存储行首buf的变量
void LOScripFile::GetBufLine(const char *buf, int *lineID, const char* &lineStart) {
	if(lineID) *lineID = -1;
	if (lineStart) lineStart = nullptr;
	const char *startbuf = scriptbuf.c_str();
	const char *endbuf = startbuf + scriptbuf.length();

	if (buf > startbuf && buf < endbuf) {
		//定位搜索起始行
		int startLine = 0;
		for (int ii = 0; ii < lineInfo.size() - 1; ii++) {
			startLine = lineInfo.at(ii).lineID;
			if (buf >= lineInfo.at(ii).buf && buf < lineInfo.at(ii + 1).buf) {
				startbuf = lineInfo.at(ii).buf;
				break;
			}
		}
		//定位到具体的某一行
		const char *nextbuf = scriptbuf.NextLine(startbuf);
		while (buf < endbuf) {
			if (buf >= startbuf && buf < nextbuf) {
				if (lineID) *lineID = startLine;
				if (lineStart) lineStart = startbuf;
				return;
			}
			else {
				startbuf = nextbuf;
				nextbuf = scriptbuf.NextLine(startbuf);
				lineStart++;
			}
		}
	}
}