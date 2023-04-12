#include "LOScriptPoint.h"

//ÿ��LINEINTERVAL��һ����¼����������buf��ѯλ����һ��
#define LINEINTERVAL 80
//�ű������֣�һ���������Ľű���������Ҫ�洢
//һ����eval��ֻ����ʱ�Ĵ洢
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


//�����ܲ��ã��������ǵ�call��λ����std::vector�����ڲ��д����Ŀ�������������˲����������������ͷű���
//�����ֶ�����
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
	//��ǰִ�е�����
	c_line = 0;
	//��ǰִ�е���λ��
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
	auto data = fileList[label->i_index]->GetLineInfo(nullptr, label->s_line + relativeLine, true);
	if (!data.buf) return false;
	lineStart = data.buf;
	return true;
}

LOScripFile::LOScripFile() {
	i_index = -1;
}

//���һ��������ʾ������ظ��ı�ǩ�Ƿ����ظ���ǩ�����Ĭ��Ϊ��һ����ǩ��Ч
//LOScripFile::LOScripFile(const char *cbuf, int len, const char *filename) {
//	scriptbuf.assign(cbuf, len);
//	Name.assign(filename);
//
//	//���ݽű����ַ������ö�Ӧ�ı��������������128kb
//	if (len > 1048576) len = 1048576;
//	
//	scriptbuf.SetEncoder(LOCodePage::GetEncoder(LOCodePage::ENCODER_GBK)); //������Ĭ�ϵĶ���
//	CreateRootLabel();
//}

void LOScripFile::CreateRootLabel() {
	//���һ������ǩ
	LOScriptPoint *root = new LOScriptPoint();
	root->name = "__init__";
	root->s_buf = scriptbuf.c_str();
	root->s_line = 0;
	root->i_index = i_index;
	labels[root->name] = root;
}

////����һ���ļ�
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
			label->i_index = i_index;

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


LOScripFile* LOScripFile::AddScript(const char *buf, int length, const char* filename) {
	LOScripFile *file = new LOScripFile();
	fileList.push_back(file);
	file->i_index = fileList.size() - 1;
	file->scriptbuf.assign(buf, length);
	file->Name.assign(filename);

	//���ݽű����ַ������ö�Ӧ�ı��������������128kb
	if (length > 1048576) length = 1048576;

	file->scriptbuf.SetEncoder(LOCodePage::GetEncoder(LOCodePage::ENCODER_GBK)); //������Ĭ�ϵĶ���

	//����LOString��Ĭ�ϱ��룬����ʮ�ֱ�Ҫ��
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