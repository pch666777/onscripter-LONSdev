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
	//���»�ȡ����Ϣ����Ϊ���е�ʱ�� c_line�����Ǵ����
	int lineID = 0;
	const char *lineStart = nullptr;
	file->GetBufLine(c_buf, &lineID, lineStart);
	if (!lineStart) {
		LOLog_e("Get running point info error! file[%s],lable[%s],at buf[%d]", file->Name.c_str(), name.c_str(), c_buf - s_buf);
	}
	//�����
	bin->WriteInt(lineID - s_line);
	//����������ж��ٳ���
	bin->WriteInt(c_buf - lineStart);
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
	step = 1;   //Ĭ�ϵ�����
	var = nullptr;
	type = t;
	lableName = nullptr;
}

LogicPointer::~LogicPointer() {
	if (var) delete var;
	if (dstvar)delete dstvar;
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

//��ȡָ����ַ��Ӧ����
//ָ����buf���洢��ID�ı������洢����buf�ı���
void LOScripFile::GetBufLine(const char *buf, int *lineID, const char* &lineStart) {
	if(lineID) *lineID = -1;
	if (lineStart) lineStart = nullptr;
	const char *startbuf = scriptbuf.c_str();
	const char *endbuf = startbuf + scriptbuf.length();

	if (buf > startbuf && buf < endbuf) {
		//��λ������ʼ��
		int startLine = 0;
		for (int ii = 0; ii < lineInfo.size() - 1; ii++) {
			startLine = lineInfo.at(ii).lineID;
			if (buf >= lineInfo.at(ii).buf && buf < lineInfo.at(ii + 1).buf) {
				startbuf = lineInfo.at(ii).buf;
				break;
			}
		}
		//��λ�������ĳһ��
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