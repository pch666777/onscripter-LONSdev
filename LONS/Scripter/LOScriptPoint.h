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
	LOScriptPoint();
	virtual ~LOScriptPoint();

	//��ǩ����
	LOString name;
	//���е�ʵ�ʿ�ʼ��λ��
	const char* s_buf;
	//���е㿪ʼ����
	int s_line;
	LOScripFile* file;  //λ���ĸ��ű���
};


//call���ͣ�ͬһ��LOScriptPoint�����в�ͬ��call����
class LOScriptPointCall :public LOScriptPoint {
public:
	LOScriptPointCall();
	LOScriptPointCall(LOScriptPoint *p);
	~LOScriptPointCall();

	//������Ƿ���ȷ
	void CheckCurrentLine();
	void Serialize(BinArray *bin);
	bool DeSerialize(BinArray *bin, int *pos);

	int callType;
	//��ǰִ�е�����
	int c_line;
	//��ǰִ�е���λ��
	const char* c_buf;
	//�п�ʼ��λ��
	const char* c_buf_start;
};


//�߼����е�
class LogicPointer
{
public:
	LogicPointer(int t);
	~LogicPointer();
	enum {
		TYPE_FOR = 1,
		TYPE_IF = 2,
		TYPE_IFTHEN = 4,
		TYPE_ELSE = 8,
		TYPE_WHILE = 16,
		//if���жϽ�������������Ϊ�棬û���������Ϊ��
		TYPE_RESULT_TRUE = 32
	};

	void SetFlags(int f) { flags |= f; }
	void UnSetFlags(int f) { flags &= (~f); }
	bool isFor() { return flags & TYPE_FOR; }
	bool isIF() { return flags & TYPE_IF; }
	bool isIFthen(){ return flags & TYPE_IFTHEN; }
	bool isElse() { return flags & TYPE_ELSE; }
	bool isWhile() { return flags & TYPE_WHILE; }
	bool isRetTrue() { return flags & TYPE_RESULT_TRUE; }
	void reset();
	void SetRet(bool it);
	////���ü�¼�㣬�����������һ��У��
	void SetPoint(LOScriptPointCall *p);
	////���ؼ�¼��
	void BackToPoint(LOScriptPointCall *p);
	void Serialize(BinArray *bin);
	bool LoadSetPoint(LOScriptPoint *p, int r_line, int r_buf);

	//forѭ��step����������
	int step;
	//for %1 = 1 to %2 step n�е�%1
	ONSVariableRef *forVar;
	//for �е� %2�����Ա���̬�ı�
	ONSVariableRef *dstVar;
private:
	int flags;
	//����ڱ�ǩs_line������
	int relativeLine;
	//��������׵�ǰ����
	int relativeByte;
	//����buf
	const char *lineStart;
	//ѭ�����ĸ���ǩ��
	LOScriptPoint *label;
};



class LOScripFile {
public:
	//�и�buf�Ĺ�ϵ
	struct LineData{
		LineData(int id, const char* bin) {
			lineID = id;
			buf = bin;
		}
		int lineID = -1;
		const char* buf = nullptr;
	};

	LOScripFile(const char *cbuf, int len, const char *filename);
	LOScripFile(LOString *s, const char *filename);
	~LOScripFile();

	LOString Name;
	LOScriptPoint *FindLable(std::string &lname);
	LOScriptPoint *FindLable(const char *lname);
	LOString *GetBuf() { return &scriptbuf; }
	void InitLables(bool lableRe = false);
	//��ȡָ���е���Ϣ����Ҫ�ṩ�кŻ���buf
	//AMD zen1 1400  100�β���release��ʱ0.5ms -->�м�¼�趨Ϊ80��
	LineData GetLineInfo(const char *buf, int lineID, bool isLine);
private:
	LOString scriptbuf;
	//ÿ100�д�һ����¼��
	std::vector<LineData> lineInfo;
	std::unordered_map<std::string, LOScriptPoint*> labels;

	void ClearLables();
	void CreateRootLabel();

	//���ַ���λ���У����ص���lineInfo������
	LineData* MidFindBaseIndex(const char *buf, int dstLine, bool isLine);
	//��ָ������ʼλ�ò��ҵ�Ŀ�꣬startBuf��startLine�ᱻ�޸�Ϊ�ҵ���Ŀ�꣬Ӧ����MidFindBaseIndex������Ч�Լ��
	void NextFindDest(const char *dst, int dstLine, const char *&startBuf, int &startLine, bool isLine);
};





#endif // __LOSCRIPTPOINT_H__
