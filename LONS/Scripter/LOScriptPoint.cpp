#include "LOScriptPoint.h"

//ÿ��LINEINTERVAL��һ����¼����������buf��ѯλ����һ��
#define LINEINTERVAL 80


LOScriptPoint::LOScriptPoint() {
	file = NULL;
	s_buf = 0;
	s_line = 0;
}

LOScriptPoint::~LOScriptPoint() {
}

void LOScriptPointCall::CheckCurrentLine() {
	LOScripFile::LineData data = file->GetLineInfo(c_buf, 0, false);
	if (data.lineID >= 0) c_line = data.lineID;
	else LOLog_e("LOScriptPointCall::CheckCurrentLine() can't find right line ID!");
}

LOString *LOScriptPointCall::GetScriptStr() {
	if (!file) return nullptr;
	if (callType == CALL_BY_EVAL) return (LOString*)file;
	else return file->GetBuf();
}

void LOScriptPointCall::Serialize(BinArray *bin) {
	//'poin', len, version
	int len = bin->WriteLpksEntity("poin", 0, 1);
	bin->WriteLOString(&file->Name);
	bin->WriteLOString(&name);
	//��ȡ����к�����
	auto data = file->GetLineInfo(c_buf, 0, false);
	if (data.lineID < 0) {
		LOLog_e("LOScriptPointCall::Serialize() get line info error!");
		return;
	}
	//�����
	bin->WriteInt(data.lineID - s_line);
	//����������ж��ٳ���
	bin->WriteInt(c_buf - data.buf);
	//call����
	bin->WriteInt(callType);

	//��¼��4���ֽں��飬���벻�������ڽű�β��д��savepoint��
	bin->WriteInt(*(int*)c_buf);

	bin->WriteInt(bin->Length() - len, &len);

}



LOScriptPointCall::LOScriptPointCall() {
	callType = CALL_BY_NORMAL;
	//��ǰִ�е�����
	c_line = 0;
	//��ǰִ�е���λ��
	c_buf = nullptr;
	//eval�ı��
	e_id = 0;
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
	if (callType == CALL_BY_EVAL) delete (LOString*)file;
	file = nullptr;
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
	LOScripFile::LineData data = p->file->GetLineInfo(p->c_buf, 0, false);
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
	//flag����
	bin->WriteInt(flags);

	//�����ĸ�label��
	bin->WriteLOString(&label->name);
	//�����label��λ��
	bin->WriteInt(relativeLine);
	//��������׵�λ��
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
	auto data = label->file->GetLineInfo(nullptr, label->s_line + relativeLine, true);
	if (!data.buf) return false;
	lineStart = data.buf;
}


//���һ��������ʾ������ظ��ı�ǩ�Ƿ����ظ���ǩ�����Ĭ��Ϊ��һ����ǩ��Ч
LOScripFile::LOScripFile(const char *cbuf, int len, const char *filename) {
	scriptbuf.assign(cbuf, len);
	Name.assign(filename);

	//���ݽű����ַ������ö�Ӧ�ı��������������128kb
	if (len > 1048576) len = 1048576;
	
	scriptbuf.SetEncoder(LOCodePage::GetEncoder(LOCodePage::ENCODER_GBK)); //������Ĭ�ϵĶ���
	CreateRootLabel();
}

void LOScripFile::CreateRootLabel() {
	//���һ������ǩ
	LOScriptPoint *root = new LOScriptPoint();
	root->name = "__init__";
	root->s_buf = scriptbuf.c_str();
	root->s_line = 0;
	root->file = this;
	labels[root->name] = root;
}

//����һ���ļ�
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

	//���������еı�ǩ
	int linecount = 1;
	const char *buf = scriptbuf.c_str();
	const char *ebuf = buf + scriptbuf.length();
	LOCodePage *encoder = scriptbuf.GetEncoder();

	//�����������Ҫ����һ��ʱ��ģ�7M�ı��ڵ����ϴ�Լ�� 500- 600����
	while (buf < ebuf - 1) {
		buf = scriptbuf.SkipSpace(buf);
		//��¼��
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

			//��ǩ����
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

	//�������һ�У���������
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
	//�����Ч��
	if (isLine && (dstLine < 1 || dstLine > lineInfo.back().lineID)) return nullptr;
	if (!isLine && (buf < lineInfo.front().buf || buf > lineInfo.back().buf)) return nullptr;
	if (lineInfo.size() == 1) return &lineInfo.front();

	int left = 0;
	int right = lineInfo.size() - 1;
	int mid = right / 2;
	while (true) {
		//�����ұ�
		if ((isLine && dstLine >= lineInfo.at(mid).lineID) ||
			(!isLine && buf >= lineInfo.at(mid).buf)) {
			// (mid + mid + 1 ) / 2 ���ǵ���mid�������Ѿ�������β����
			if (right - mid <= 1) return &lineInfo.at(mid);
			left = mid;
		}
		else {
			//�������
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