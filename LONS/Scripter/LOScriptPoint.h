#ifndef __LOSCRIPTPOINT_H__
#define __LOSCRIPTPOINT_H__

#include "../etc/LOString.h"
#include "ONSvariable.h"

#include <string>
#include <unordered_map>

class LOScripFile;
//�ű����е㣬ֻ����LOScripFile����
class LOScriptPoint {
public:
	enum {
		//TYPE_LABEL,
		//TYPE_FOR,
		//TYPE_SUB,
		//TYPE_GOSUB,
		CALL_BY_NORMAL,
		CALL_BY_PRETEXT_GOSUB,
		CALL_BY_TEXT_GOSUB,
		CALL_BY_LOAD_GOSUB,
		CALL_BY_EVAL,
	};
	LOScriptPoint(int t, bool canf);
	~LOScriptPoint();

	//LOScriptPoint *CopyMe();

	LOString name;  //��ǩ����
	const char* s_buf;    //���е�ʵ�ʿ�ʼ��λ��
	const char* c_buf;    //���е㵱ǰ���е���
	int s_line;     //���е㿪ʼ����
	int c_line;     //���е㵱ǰ���е�λ��
	LOScripFile* file;  //λ���ĸ��ű���
};


//�߼����е�
class LogicPointer
{
public:
	LogicPointer(int t);
	~LogicPointer();
	enum {
		TYPE_FOR,
		TYPE_IF,
		TYPE_IFTHEN,
		TYPE_ELSE,
		TYPE_WHILE
	};
	//char *adress; //���е��λ�ã��ж��߼�֮��
	int relAdress; //����ڱ�ǩ�ĵ�ַ
	int startLine;  //adress��ʼ����
	int step;   //forѭ��step����������
	int type;   //����
	bool ifret; //if then ���жϽ��
	ONSVariableRef *var;  //forѭ���ĵ�������
	ONSVariableRef *dstvar;  //for��Ŀ�����������Ա���̬�ı�
	LOString *lableName;
private:
};



class LOScripFile {
public:
	LOScripFile(const char *cbuf, int len, const char *filename);
	LOScripFile(LOString *s, const char *filename);
	~LOScripFile();

	LOString Name;
	LOScriptPoint *FindLable(std::string &lname);
	LOScriptPoint *FindLable(const char *lname);
	LOString *GetBuf() { return &scriptbuf; }
	void InitLables(bool lableRe = false);
	int GetPointLineAndPosition(LOScriptPoint *p, int *position);
private:
	LOString scriptbuf;
	std::unordered_map<std::string, LOScriptPoint*> labels;

	void ClearLables();
	void CreateRootLabel();
};





#endif // __LOSCRIPTPOINT_H__
