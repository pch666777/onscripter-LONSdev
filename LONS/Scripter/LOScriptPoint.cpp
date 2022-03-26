#include "LOScriptPoint.h"

LOScriptPoint::LOScriptPoint(int t, bool canf) {
	file = NULL;
	c_buf = 0;
	c_line = 0;
	s_buf = 0;
	s_line = 0;
	//canfree = canf;
}

LOScriptPoint::~LOScriptPoint() {
}

//LOScriptPoint* LOScriptPoint::CopyMe() {
//	LOScriptPoint *tmp = new LOScriptPoint(type, canfree);
//	tmp->label_header = label_header;
//	tmp->start_address = start_address;
//	tmp->start_line = start_line;
//	tmp->current_line = current_line;
//	tmp->current_address = current_address;
//	return tmp;
//}

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
	LOScriptPoint *root = new LOScriptPoint(LOScriptPoint::CALL_BY_NORMAL, false);
	root->name = "__init__";
	root->s_buf = scriptbuf.c_str();
	root->s_line = 0;
	root->c_buf = scriptbuf.c_str();
	root->c_line = 0;
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
	//���������еı�ǩ
	int linecount = 1;
	const char *buf = scriptbuf.c_str();
	const char *ebuf = buf + scriptbuf.length();
	LOCodePage *encoder = scriptbuf.GetEncoder();

	//�����������Ҫ����һ��ʱ��ģ�7M�ı��ڵ����ϴ�Լ�� 500- 600����
	while (buf < ebuf - 1) {
		buf = scriptbuf.SkipSpace(buf);
		if (buf[0] == '*') {
			while (buf[0] == '*') buf++;
			auto *label = new LOScriptPoint(LOScriptPoint::CALL_BY_NORMAL, false);
			label->name = scriptbuf.GetWordUntil(buf, LOCodePage::CHARACTER_ZERO | LOCodePage::CHARACTER_CONTROL | LOCodePage::CHARACTER_SPACE | LOCodePage::CHARACTER_SYMBOL);
			buf = scriptbuf.NextLine(buf);
			linecount++;

			label->name = label->name.toLower();
			label->s_buf = buf;
			label->s_line = linecount;
			label->c_line = linecount;
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


//��ȡָ��λ���������в�����������е�λ�ã�ʧ�ܷ���-1
int LOScripFile::GetPointLineAndPosition(LOScriptPoint *p, int *position) {
	const char *linestart = p->s_buf;
	const char *maxbuf = scriptbuf.c_str() + scriptbuf.length();
	if (p->c_buf >= maxbuf) return -1;

	int reline = 0;  //���������
	while (linestart < maxbuf) {
		const char* lineend = linestart;
		while (lineend[0] != '\n' && lineend < maxbuf) lineend += scriptbuf.ThisCharLen(lineend);
		if (lineend >= maxbuf) break;

		if (p->c_buf >= linestart && p->c_buf <= lineend) {
			*position = p->c_buf - linestart;
			return reline;
		}
		reline++;
		linestart = lineend + 1;
	}

	return -1;
}