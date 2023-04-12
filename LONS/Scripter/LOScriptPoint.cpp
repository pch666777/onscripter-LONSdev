#include "LOScriptPoint.h"

//每隔LINEINTERVAL做一个记录，方便任意buf查询位于哪一行
#define LINEINTERVAL 80
//脚本有两种，一种是完整的脚本，总是需要存储
//一种是eval，只是临时的存储
std::vector<LOScripFile*> fileList;
std::vector<LOString*> evalList;

LOScriptPoint::LOScriptPoint() {
	i_index = -1;
	s_buf = 0;
	s_line = 0;
}


void LOScriptPointCall::CheckCurrentLine() {
	LOScripFile *file = fileList[i_index];
	LOScripFile::LineData data = file->GetLineInfo(c_buf, 0, false);
	if (data.lineID >= 0) c_line = data.lineID;
	else LOLog_e("LOScriptPointCall::CheckCurrentLine() can't find right line ID!");
}

LOString *LOScriptPointCall::GetScriptStr() {
	if (callType == CALL_BY_EVAL) return evalList[i_index];
	else return fileList[i_index]->GetBuf();
}

void LOScriptPointCall::Serialize(BinArray *bin) {
	//'poin', len, version
	int len = bin->WriteLpksEntity("poin", 0, 1);
	LOScripFile *file = fileList[i_index];
	bin->WriteLOString(&file->Name);
	bin->WriteLOString(&name);
	//获取相对行和行首
	auto data = file->GetLineInfo(c_buf, 0, false);
	if (data.lineID < 0) {
		LOLog_e("LOScriptPointCall::Serialize() get line info error!");
		return;
	}
	//相对行
	bin->WriteInt(data.lineID - s_line);
	//相对于行首有多少长度
	bin->WriteInt(c_buf - data.buf);
	//call类型
	bin->WriteInt(callType);

	//记录后4个字节核验，我想不会有人在脚本尾部写个savepoint吧
	bin->WriteInt(*(int*)c_buf);

	bin->WriteInt(bin->Length() - len, &len);

}


//这样很不好，不过考虑到call的位置由std::vector管理，内部有大量的拷贝和析构，因此不能在析构函数中释放变量
//必须手动管理
void LOScriptPointCall::freeEval() {
	if (callType == CALL_BY_EVAL) {
		delete evalList[i_index];
		evalList[i_index] = nullptr;
	}
}


LOScripFile *LOScriptPointCall::file() {
	if (i_index < 0 || callType == CALL_BY_EVAL || i_index >= fileList.size()) return nullptr;
	return fileList[i_index];
}


LOScriptPoint* LOScriptPointCall::GetScriptPoint(LOString lname) {
	lname = lname.toLower();
	LOScriptPoint *p = nullptr;
	for (int ii = 0; ii < fileList.size(); ii++) {
		p = fileList.at(ii)->FindLable(lname);
		if (p) return p;
	}
	return nullptr;
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
	i_index = p->i_index;
	c_line = s_line;
	c_buf = s_buf;
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
	step = 1;
	relativeLine = relativeByte = 0;
	if (forVar) delete forVar;
	if (dstVar) delete dstVar;
	forVar = nullptr;
	dstVar = nullptr;
	lineStart = nullptr;
	//label = nullptr;
}

void LogicPointer::SetRet(bool it) {
	if (it) flags |= TYPE_RESULT_TRUE;
	else flags &= (~TYPE_RESULT_TRUE);
}


void LogicPointer::SetPoint(LOScriptPointCall *p) {
	LOScripFile::LineData data =fileList[p->i_index]->GetLineInfo(p->c_buf, 0, false);
	if (data.buf && data.lineID == p->c_line) {
		relativeLine = data.lineID - p->s_line;
		lineStart = data.buf;
		relativeByte = p->c_buf - lineStart;
		label = p;
	}
	else LOLog_e("LogicPointer::SetPoint() check lineID error,system is %d,actually is %d.", p->c_line, data.lineID);
}

void LogicPointer::BackToPoint(LOScriptPointCall *p) {
	p->c_line = p->s_line + relativeLine;
	p->c_buf = lineStart + relativeByte;
}

void LogicPointer::Serialize(BinArray *bin) {
	int len = bin->WriteLpksEntity("lpos", 0, 1);
	//flag优先
	bin->WriteInt(flags);

	//属于哪个label的
	bin->WriteLOString(&label->name);
	//相对于label的位置
	bin->WriteInt(relativeLine);
	//相对于行首的位置
	bin->WriteInt(relativeByte);

	bin->WriteInt(step);
	if (forVar) bin->WriteInt(forVar->GetTypeRefid());
	else bin->WriteInt(0);
	if (dstVar) bin->WriteInt(dstVar->GetTypeRefid());
	else bin->WriteInt(0);

	bin->WriteInt(bin->Length() - len, &len);
}


bool LogicPointer::LoadSetPoint(LOScriptPoint *p, int r_line, int r_buf) {
	relativeLine = r_line;
	relativeByte = r_buf;
	label = p;
	auto data = fileList[label->i_index]->GetLineInfo(nullptr, label->s_line + relativeLine, true);
	if (!data.buf) return false;
	lineStart = data.buf;
	return true;
}

LOScripFile::LOScripFile() {
	i_index = -1;
}

//最后一个参数表示如果有重复的标签是否用重复标签替代，默认为第一个标签生效
//LOScripFile::LOScripFile(const char *cbuf, int len, const char *filename) {
//	scriptbuf.assign(cbuf, len);
//	Name.assign(filename);
//
//	//根据脚本的字符编码获得对应的编码器，最大搜索128kb
//	if (len > 1048576) len = 1048576;
//	
//	scriptbuf.SetEncoder(LOCodePage::GetEncoder(LOCodePage::ENCODER_GBK)); //先来个默认的顶着
//	CreateRootLabel();
//}

void LOScripFile::CreateRootLabel() {
	//添加一个根标签
	LOScriptPoint *root = new LOScriptPoint();
	root->name = "__init__";
	root->s_buf = scriptbuf.c_str();
	root->s_line = 0;
	root->i_index = i_index;
	labels[root->name] = root;
}

////创建一个文件
//LOScripFile::LOScripFile(LOString *s, const char *filename) {
//	scriptbuf.assign(*s);
//	scriptbuf.SetEncoder(s->GetEncoder());
//	if (!filename) Name = "Anonymous_" + LOString::RandomStr(4);
//	else Name.assign(filename);
//	InitLables();
//}

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
			label->i_index = i_index;

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
		if ((isLine && dstLine >= lineInfo.at(mid).lineID) ||
			(!isLine && buf >= lineInfo.at(mid).buf)) {
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


LOScripFile* LOScripFile::AddScript(const char *buf, int length, const char* filename) {
	LOScripFile *file = new LOScripFile();
	fileList.push_back(file);
	file->i_index = fileList.size() - 1;
	file->scriptbuf.assign(buf, length);
	file->Name.assign(filename);

	//根据脚本的字符编码获得对应的编码器，最大搜索128kb
	if (length > 1048576) length = 1048576;

	file->scriptbuf.SetEncoder(LOCodePage::GetEncoder(LOCodePage::ENCODER_GBK)); //先来个默认的顶着

	//设置LOString的默认编码，这是十分必要的
	if (file->i_index == 0) LOString::SetDefaultEncoder(file->scriptbuf.GetEncoder()->codeID);

	file->CreateRootLabel();
	return file;
}


int LOScripFile::AddEval(LOString *s) {
	for (int ii = 0; ii < evalList.size(); ii++) {
		if (evalList[ii] == nullptr) {
			evalList[ii] = new LOString(*s);
			return ii;
		}
	}
	evalList.push_back(new LOString(*s));
	return evalList.size() - 1;
}


bool LOScripFile::HasEval() {
	for (int ii = 0; ii < evalList.size(); ii++) {
		if (evalList[ii]) return true;
	}
	return false;
}


void LOScripFile::InitScriptLabels() {
	for (int ii = 0; ii < fileList.size(); ii++) {
		fileList[ii]->InitLables();
	}
}