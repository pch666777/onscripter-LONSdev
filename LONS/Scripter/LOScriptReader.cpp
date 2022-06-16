#include "LOScriptReader.h"
//#include "../Render/LOLayerData.h"
#include "Buil_in_script.h"
#include <stdarg.h>
//#include <SDL.h>
//#include "LOMessage.h"

//=========== static ========================== //
LOStack<LOScripFile> LOScriptReader::filesList;
std::unordered_map<std::string, int> LOScriptReader::numAliasMap;   //整数别名
std::unordered_map<std::string, int> LOScriptReader::strAliasMap;   //字符串别名，存的是字符串位置
std::vector<LOString> LOScriptReader::strAliasList;  //字符串别名
std::vector<LOString> LOScriptReader::workDirs;
std::unordered_map<std::string, LOScriptPoint*> LOScriptReader::defsubMap;
int LOScriptReader::sectionState ;
bool LOScriptReader::st_globalon; //是否使用全局变量
bool LOScriptReader::st_labellog; //是否使用标签变量
bool LOScriptReader::st_errorsave; //是否使用错误自动保存

LOScripFile* LOScriptReader::AddScript(const char *buf, int length, const char* filename) {
	LOScripFile *file = new LOScripFile(buf, length, filename);
	//设置默认编码，可以消除潜在的错误
	if (filesList.size() == 0 && file) {
		LOString::SetDefaultEncoder(file->GetBuf()->GetEncoder()->codeID);
	}
	filesList.push(file);
	return file;
}

LOScripFile* LOScriptReader::AddScript(LOString *s, const char* filename) {
	LOScripFile *file = new LOScripFile(s, filename);
	filesList.push(file);
	return file;
}

void LOScriptReader::RemoveScript(LOScripFile *f) {
	for (int ii = 0; ii < filesList.size(); ii++) {
		if (f == filesList.at(ii)) {
			filesList.removeAt(ii);
			break;
		}
	}
}

void LOScriptReader::InitScriptLabels() {
	for (int ii = 0; ii < filesList.size(); ii++) {
		filesList.at(ii)->InitLables();
	}
}

 void LOScriptReader::AddWorkDir(LOString dir) {
	 workDirs.push_back(dir);
 }

 LOScriptReader* LOScriptReader::GetScriptReader(LOString &name) {
	 LOScriptReader *reader = nullptr;
	 if (!scriptModule) { //第一个脚本总是main
		 reader = new LOScriptReader;
		 reader->Name = name;
		 reader->sctype = SCRIPT_TYPE::SCRIPT_TYPE_MAIN;
		 scriptModule = reader;
		 return reader;
	 }
	 else {
		 reader = (LOScriptReader*)scriptModule;
		 while (reader) {
			 if (reader->Name == name) return reader;
			 if (!reader->nextReader) break;
			 else reader = reader->nextReader;
		 }
		 reader->nextReader = new LOScriptReader;
		 reader->nextReader->Name = name;
		 return reader->nextReader;
	 }
 }

 LOScriptReader* LOScriptReader::EnterScriptReader(LOString name) {
	 LOScriptReader *reader = GetScriptReader(name);
	 if (reader->moduleState != MODULE_STATE::MODULE_STATE_NOUSE) {
		 LOLog_e(nullptr, "ScriptReader[ %s ]: this name already use!", name.c_str());
		 return nullptr;
	 }
	 return reader;
 }

 //返回下一个有效的脚本解析器
 LOScriptReader* LOScriptReader::LeaveScriptReader(LOScriptReader* reader) {
	 LOScriptReader *baseReader = (LOScriptReader*)scriptModule;
	 if (reader->sctype == SCRIPT_TYPE::SCRIPT_TYPE_NORMAL) {
		 while (baseReader) {
			 if (baseReader->nextReader == reader) {
				 baseReader->nextReader = reader->nextReader;
				 LOLog_i("script thread has exit:%s", reader->Name.c_str());
				 delete reader;
				 return baseReader->nextReader;
			 }
			 else baseReader = baseReader->nextReader;
		 }
	 }
	 else if(reader->sctype == SCRIPT_TYPE::SCRIPT_TYPE_TIMER) {
		 LOLog_i("script thread has nouse:%s", reader->Name.c_str());
		 reader->moduleState = MODULE_STATE::MODULE_STATE_NOUSE;
		 return reader->nextReader;
	 }
	 else {  //main
		 while (baseReader->nextReader) {
			 LOScriptReader *tmp = baseReader->nextReader;
			 baseReader->nextReader = tmp->nextReader;
			 delete tmp;
		 }
		 //main脚本从外部删除，提示信息也是从外部输出
		 return (LOScriptReader*)scriptModule;
	 }

	 return nullptr;
 }


 // ================ class =========================== //

LOScriptReader::LOScriptReader()
{
	//RegisterBaseFunc();
	sctype = SCRIPT_TYPE::SCRIPT_TYPE_NORMAL;
	nslogic = new LogicPointer(LogicPointer::TYPE_IF);
	subStack = new LOStack<LOScriptPoint>;
	loopStack = new LOStack<LogicPointer>;
	ResetBaseConfig();
}

LOScriptReader::~LOScriptReader()
{
	//blocksEvent.InvalidAll();
	while (!isEndSub()) ReadyToBack();
	delete nslogic;
	if(subStack) delete subStack;
	if(loopStack) delete loopStack;
}

int LOScriptReader::MainTreadRunning() {
	if (sctype != SCRIPT_TYPE::SCRIPT_TYPE_MAIN) {
		PrintError("LOScriptReader::MainTreadRunning() must run at main scriptReader!");
		return -1;
	}

	moduleState = MODULE_STATE::MODULE_STATE_RUNNING;
	const char* lables[] = { "define" , "start" };

	int ret = RET_CONTINUE;
	sectionState = SECTION_DEFINE;
	activeReader = this;
	while (moduleState <= MODULE_STATE::MODULE_STATE_RESET) {
		LOString lable(lables[sectionState]);
		if (!ReadyToRun(&lable, LOScriptPoint::CALL_BY_NORMAL))return -1;

		//轮询脚本
		//isAddLine = true;
		while (moduleState < MODULE_STATE::MODULE_STATE_RESET) {

			ret = activeReader->ContinueRun();
			if (ret == RET_RETURN && activeReader->isEndSub()) {  //某个脚本返回
				activeReader = LeaveScriptReader(activeReader);
				if (activeReader == this) break;  //主脚本返回则跳出循环
			}
			else {
				activeReader = activeReader->nextReader;
				//检查下一个脚本解析是否处于闲置状态，是的话继续下一个
				while(activeReader && activeReader->moduleState == MODULE_STATE_NOUSE) activeReader = activeReader->nextReader;
			}

			if (!activeReader) activeReader = this;
		}

		//=*****************==//

		if (moduleState & MODULE_STATE_RESET) { //重置，到这一步说明其他模块已经重置完成了
			ResetMe();
			//清理所有的事件队列
			//G_ClearAllEventSlots();
			//清理所有的变量
			ONSVariableBase::ResetAll();
			moduleState = MODULE_STATE_NOUSE;
			//等待渲染模块重置
			while (imgeModule->moduleState != MODULE_STATE_RUNNING) {
				SDL_Delay(1);
			}
			//完成重置，恢复其他模块的可操作性
			audioModule->ResetMeFinish();
			moduleState = MODULE_STATE_RUNNING;
			sectionState = SECTION_DEFINE;
		}
		else {
			sectionState++;
			if (sectionState > SECTION_NORMAL) break;
		}

		//=*****************==//
	}

	//退出之前保存全局变量、文件读取列表等
	if(st_labellog) WriteLog(FunctionInterface::LOGSET_LABELLOG);
	UpdataGlobleVariable();
	SaveGlobleVariable();

	if (moduleState & MODULE_STATE::MODULE_STATE_ERROR) return -1;
	else {
		return 0;
	}
} 


//是否运行到RunScript应该返回的位置
bool LOScriptReader::isEndSub() {
	return subStack->size() == 0;
	//uintptr_t ptr = (uintptr_t)subStack->top();
	//if (ptr < 0x8) return true;
	//else return false;
}

//准备转到下一个运行点
void LOScriptReader::ReadyToRun(LOScriptPoint *label, int callby) {
    //LOLog_i("ReadyToRun: %s,%x",label->name.c_str(),callby) ;
	if (callby != LOScriptPoint::CALL_BY_NORMAL) subStack->push((LOScriptPoint*)callby);
	subStack->push(label);
	scriptbuf = label->file->GetBuf();
	currentLable = subStack->top();
	currentLable->c_buf = currentLable->s_buf;
	currentLable->c_line = currentLable->s_line;
}

bool LOScriptReader::ReadyToRun(LOString *lname, int callby) {
	if (!lname || lname->length() == 0) return false;
	LOScriptPoint *p = GetScriptPoint(*lname);
	if (!p) {
		PrintError("not lable name:[%s]", lname->c_str());
		return false;
	}
	ReadyToRun(p, callby);
	return true;
}

//准备返回上一个运行点，同时抛弃本个运行点
int LOScriptReader::ReadyToBack() {
	uintptr_t callby = 0;
	LOScriptPoint *last = subStack->pop();
	currentLable = NULL;
	if (!isEndSub()) {
		callby = (uintptr_t)subStack->top();
		if (callby < 0x10) subStack->pop();
		else callby = LOScriptPoint::CALL_BY_NORMAL;
		currentLable = subStack->top();
		scriptbuf = currentLable->file->GetBuf();

		if (callby == LOScriptPoint::CALL_BY_EVAL) {//删除eval的点
			RemoveScript(last->file);
			delete last->file;
		}
	}

	int ret = RET_RETURN | ((callby & 0xff) << 8);
	return ret;
}

LOScriptPoint* LOScriptReader::GetScriptPoint(LOString lname) {
	lname = lname.toLower();
	LOScriptPoint *p = NULL;
	for (int ii = 0; ii < filesList.size(); ii++) {
		p = filesList.at(ii)->FindLable(lname);
		if (p) return p;
	}
	LOLog_i("no lable:%s", lname.c_str());
	return p;
}

LOScriptPoint  *LOScriptReader::GetParamLable(int index) {
	LOString s = GetParamStr(index);
	if (s.length() == 0) return NULL;
	return GetScriptPoint(s);
}


//转移运行点位置，0表示转到top，1表示top的前一个
bool LOScriptReader::ChangePointer(int esp) {
	//寻找真正的位置
	int index = subStack->size() - 1;
	int available = 0;
	while (index >= 0 && available < esp) {
		//Beware of negative and positive problems
		if (((uintptr_t)subStack->at(index)) > 0x10) available++;
		index--;
	}
	if (index < 0) return false;

	currentLable = subStack->at(index);
	scriptbuf = currentLable->file->GetBuf();
	return true;
}


void LOScriptReader::ClearParams(bool isall) {
	if (isall) {
		while (paramStack.size() > 0) {
			ONSVariableRef *v = paramStack.pop();
			if ((intptr_t)v > 0x10) delete v;
		}
	}
	else {
		if (paramStack.size() > 0) {
			intptr_t paramcount = (intptr_t)paramStack.pop();
			for (int ii = 0; ii < paramcount && paramStack.size() > 0; ii++) delete paramStack.pop();
		}
	}
}


//根据参数将变量推入paramStack中，完成后推入一个NULL,出错返回假
bool LOScriptReader::PushParams(const char *param, const char* used) {
	if (param[0] == 0) {
		paramStack.push(NULL);
		return true;
	}

	//if (currentLable->c_line == 184968) {
	//	int debugbreak = 0;
	//}

	bool hasnormal, haslabel, hasvariable;
	int allow_type, paramcount = 0;
	const char *fromparam = param;
	const char *fromuse = used;
	const char *lastparam, *lastused;

	while (param[0] != 0) {
		if (paramcount == 0) {
			if (used[0] == 'N' && !NextSomething()) break;
			else if (used[0] == 'Y' && !NextSomething()) {
				PrintError("function paragram %d type error!", paramcount + 1);
				return false;
			}
		}
		
		lastparam = param; lastused = used;
		hasvariable = hasnormal = haslabel = false;
		allow_type = ONSVariableRef::GetTypeAllow(param);

		for (int ii = 0; param[ii] != ',' && param[ii] != 0; ii++) {
			if (param[ii] == 'N') hasnormal = true;
			else if (param[ii] == 'L') haslabel = true;
			else hasvariable = true;
		}
		//get it
		ONSVariableRef *v = nullptr;
		if (hasnormal) 
			v = TryNextNormalWord();
		if (!v && haslabel) v = ParseLabel2();
		if (!v && hasvariable) {
			v = ParseVariableBase();
			if (!v ||  !(v->vtype & allow_type)) {
				PrintError("[%s] paragram %d type error!\n",curCmdbuf, paramcount + 1);
				ClearParams(true);
				return false;
			}
		}
		//push it
		if (v) {
			paramcount++;
			paramStack.push(v);
		}
		else {
			PrintError("function paragram %d type error!", paramcount + 1);
			ClearParams(true);
			return false;
		}
		//check next
		while (param[0] != ',' && param[0] != 0) param++;
		if (param[0] == ',') param++;
		while (used[0] != ',' && used[0] != 0) used++;
		if (used[0] == ',') used++;

		if (used[0] == 0) {
			paramStack.push((ONSVariableRef*)paramcount);
			return true;
		}
		else if (used[0] == 'Y') {
			NextComma();
		}
		else if (used[0] == 'N') {
			if (!NextComma(true)) {
				paramStack.push((ONSVariableRef*)paramcount);
				return true;
			}
			used++;
		}
		else if (used[0] == '*') {
			if (!NextComma(true)) {
				paramStack.push((ONSVariableRef*)paramcount);
				return true;
			}
			param = fromparam;
			used = fromuse;
		}
		else if (used[0] == '#') {
			if (!NextComma(true)) {
				paramStack.push((ONSVariableRef*)paramcount);
				return true;
			}
			param = lastparam;
			used = lastused;
		}
	}
	return true;
}

//根据特殊的返回返回值确定操作
int LOScriptReader::ReturnEvent(int ret, const char *&buf, int &line) {
	int call_by = (ret >> 8) & 0xff;
	ret = ret & 0xff;
	if (ret == RET_WARNING) buf = TryToNextCommand(buf);
	else if (ret == RET_ERROR) {
		if (!isEndSub()) ReadyToBack();
	}
	else if (ret == RET_RETURN) {
		if (isEndSub()) return ret;
		if (call_by) {
			if (call_by == LOScriptPoint::CALL_BY_PRETEXT_GOSUB) {
				line += TextPushParams(buf);
				ret = imgeModule->textCommand(this);
				ClearParams(false);
				//ReadyToRun(&userGoSubName[USERGOSUB_TEXT], LOScriptPoint::CALL_BY_TEXT_GOSUB);
			}
		}
	}
	return ret;
}


int LOScriptReader::ContinueRun() {
	//LOString cmd;
	int ret = RET_CONTINUE;
	int callby = 0;
	LOString sss;

	//遇到一些空行直接就跳过去了，不需要检查event
	int nextType = IdentifyLine(currentLable->c_buf);

	switch (nextType) {
	case LINE_NOTES:case LINE_LABEL:case LINE_END:
		currentLable->c_buf = scriptbuf->NextLine(currentLable->c_buf);
		//if (isAddLine) currentLable->c_line++;
		currentLable->c_line++;
		return ret;
	case LINE_CONNECT:
		currentLable->c_buf++;
		return ret;
	}

	//正式有效的命令前，先执行阻塞事件
	ret = ContinueEvent();
	if (ret != RET_VIRTUAL) return ret;
	ret = RET_CONTINUE;

	switch (nextType)
	{
	case LINE_TEXT:
		//这个过程看起来有点奇怪，显示文本前我们必须进入 pretextgosub ，等到从pretextgosub返回时再进入到文字显示
		//文字显示后进入 textgosub，预先准备好tagstring
		TagString = scriptbuf->GetTagString(currentLable->c_buf);
		ReadyToRun(&userGoSubName[USERGOSUB_PRETEXT], LOScriptPoint::CALL_BY_PRETEXT_GOSUB);
		return RET_CONTINUE;
	case LINE_CAMMAND:
		ret = RunCommand(currentLable->c_buf);
		ret = ReturnEvent(ret, currentLable->c_buf, currentLable->c_line);
		if (isEndSub()) return ret;
		break;
	case LINE_JUMP:
		currentLable->c_buf++;
		break;
	case LINE_EXPRESSION:
		sss = ParseStrVariable();
		InserCommand(&sss);
		break;
	default:
		currentLable->c_buf = scriptbuf->NextLine(currentLable->c_buf);
		//SDL_LogError(0, "unknow line type at line:%d,buf[0]:%s", currentLable->current_line, cmd.c_str());
		//if (isAddLine) currentLable->c_line++;
		currentLable->c_line++;
		break;
	}
	return ret;
}


//因为采用了轮询来模拟多线程脚本，所以部分需等待的命令是通过事件存储的
int LOScriptReader::ContinueEvent() {
	int level = LOEventQue::LEVEL_NORMAL;
	int index = 0;
	bool isfinish = false;
	bool hasEvent = false;
	bool isloop = true;

	while (isloop) {
		isloop = false;

		LOShareEventHook ev = waitEventQue.GetEventHook(index, level, false);
		if (!ev) break;

		hasEvent = true;

		//如果ev带有timer则验证是否超时
		if (ev->catchFlag & LOEventHook::ANSWER_TIMER) {
			if (CheckTimer(ev.get(), 5)) RunFuncBase(ev.get(), nullptr);
		}

		//这里有个小技巧，如果hook是 finish状态，表示需要由脚本线程处理余下的过程
		//如果是Invalid()表示已经处理完成了，不需要再次额外处理
		if (ev->isFinish()) {
			if (ev->catchFlag & LOEventHook::ANSWER_BTNCLICK) RunFuncBtnSetVal(ev.get());
			else if (ev->catchFlag & LOEventHook::ANSWER_PRINGJMP) {
				//响应此事件的有文字和print
				if (ev->param2 == LOEventHook::FUN_TEXT_ACTION) {
					//printf("***********text funish***********\n");
					RunFuncSayFinish(ev.get());
				}
				else;
			}
			isfinish = true;
		}
		else if (ev->isInvalid()) isfinish = true;
		
	}

	//返回continue将会阻塞线程
	if (isfinish) {
		waitEventQue.arrangeList();
		return RET_CONTINUE;
	}

	//事件没完成根据有无事件做处理
	if (hasEvent) {
		//有事件，但是是单线程脚本，延迟一下
		//多线程脚本应该马上去执行下一个
		if (sctype == SCRIPT_TYPE::SCRIPT_TYPE_MAIN && !nextReader) SDL_Delay(1);
		//阻塞的
		return RET_CONTINUE;
	}
	return RET_VIRTUAL;   //没有阻塞事件，进入下一个命令
}


void LOScriptReader::NewThreadGosub(LOString *pname, LOString threadName) {
	if (threadName.length() == 0) threadName = LOString::RandomStr(8);
	LOScriptPoint *p = GetScriptPoint(*pname);
	if (p) {
		LOScriptReader *scripter = LOScriptReader::EnterScriptReader(threadName);
		scripter->gosubCore(p, false);
		scripter->moduleState = MODULE_STATE_RUNNING;
		LOLog_i("create scripter thread:%s", threadName.c_str());
	}
}


//识别出从指定位置的语句类型
int LOScriptReader::IdentifyLine(const char *&buf) {
	buf = scriptbuf->SkipSpace(buf);
	char ch = buf[0];
	if (ch == ';') return LINE_NOTES;
	if (ch == '*')return LINE_LABEL;
	if (ch == ':')return LINE_CONNECT;
	if (ch == '`')return LINE_TEXT;
	if (ch == '\n')return LINE_END;
	if (ch == '[')return LINE_TEXT;
	if (ch == '$')
		return LINE_EXPRESSION;
	if (ch == '~') return LINE_JUMP;
	if(ch == '\\' || ch == '@' || ch == '/') return LINE_TEXT;
	int type = scriptbuf->GetCharacter(buf);
	if (type == LOCodePage::CHARACTER_LETTER) return LINE_CAMMAND;
	else if (type == LOCodePage::CHARACTER_MULBYTE) return LINE_TEXT;
	return LINE_UNKNOW;
}

int LOScriptReader::RunCommand(const char *&buf) {
	//if (currentLable->current_line == 67) {
	//	int deebgu = 0;
	//}

	LOString tmpcmd = scriptbuf->GetWordStill(buf, LOCodePage::CHARACTER_LETTER | LOCodePage::CHARACTER_NUMBER).toLower();
	if (tmpcmd.length() > 0) {
		//优先尝试用户定义的函数，这样可以实现一些特殊自定义的系统
		auto useit = defsubMap.find(tmpcmd);
		if (useit != defsubMap.end()) {
			curCmd = tmpcmd;
			curCmdbuf = curCmd.c_str();

			if (!useit->second) {
				PrintError("no label for this function:%s!\n", curCmdbuf);
				return RET_ERROR;
			}
			buf = scriptbuf->SkipSpace(buf);
			gosubCore(useit->second, false);
			return RET_CONTINUE;
		}
		//

		FuncLUT *ptr = GetFunction(tmpcmd);
		if (ptr) {
			lastCmd = curCmd;
			curCmd = tmpcmd;
			curCmdbuf = curCmd.c_str();
			buf = scriptbuf->SkipSpace(buf);
			LOLog_i("[%d]:[%s]\n", currentLable->c_line, tmpcmd.c_str());
			//if (currentLable->current_line == 54) {
			//	int debugbreak = 1;
			//}
			if (!PushParams(ptr->param, ptr->used))return RET_ERROR;
			int ret = FunctionInterface::RunCommand(ptr->method);
			ClearParams(false);
			return ret;
		}
		else {
			LOLog_i("[%s]:[%d]:[%s] is not supported yet!!\n",
				GetCurrentFile().c_str(), currentLable->c_line, tmpcmd.c_str());
			return RET_WARNING;
		}

		return RET_ERROR;
	}
}

LOString LOScriptReader::GetCurrentFile() {
	if (currentLable && currentLable->file) {
		return currentLable->file->Name;
	}
	return LOString();
}


//获取下一个关键词，参数用于确定是否往下移动
void LOScriptReader::NextWord(bool movebuf) {
	const char* buf = currentLable->c_buf;
	buf = scriptbuf->SkipSpace(buf);
	word = scriptbuf->GetWord(buf);
	if (movebuf) currentLable->c_buf = buf;
}


void LOScriptReader::NextNormalWord(bool movebuf) {
	const char* buf = currentLable->c_buf;
	buf = scriptbuf->SkipSpace(buf);
	if(scriptbuf->GetCharacter(buf) == LOCodePage::CHARACTER_LETTER) {
		word = scriptbuf->GetWordStill(buf, LOCodePage::CHARACTER_LETTER | LOCodePage::CHARACTER_NUMBER);
	}
	if (movebuf) currentLable->c_buf = buf;
}


bool LOScriptReader::NextSomething() {
	const char* buf = currentLable->c_buf;
	buf = scriptbuf->SkipSpace(buf);
	if (buf[0] == ':' || buf[0] == ';' || buf[0] == '\n' || buf[0] == 0) return false;
	return true;
}

//非尝试模式时会报错
bool LOScriptReader::NextComma(bool isTry) {
	bool spaceisComma = false;
	if (scriptbuf->GetCharacter(currentLable->c_buf) == LOCodePage::CHARACTER_SPACE)spaceisComma = true;
	const char* buf = currentLable->c_buf;
	buf = scriptbuf->SkipSpace(buf);
	if (buf[0] == ',') {
		currentLable->c_buf = buf + 1;
		return true;
	}
	else if (!isTry) {
		//有一些麻烦的空格即为分隔的写法，真不想支持
		if (spaceisComma) return true;
		LOLog_i("NextComma not at a comma!");
		return false;
	}
	else {
		currentLable->c_buf = buf;
		return false;
	}
}

bool LOScriptReader::isName(const char* name) {
	return (curCmd.compare(name) == 0);
}


ONSVariableRef* LOScriptReader::ParseIntExpression(const char *&buf, bool isalias) {
	LOStack<ONSVariableRef> s2;
	buf = GetRPNstack(&s2, buf, true);
	//auto ss = TransformStack(&s2);
	CalculatRPNstack(&s2);
	return *(s2.begin());
}


ONSVariableRef *LOScriptReader::ParseVariableBase(ONSVariableRef *ref, bool str_alias) {
	//if (currentLable->c_line == 310401) {
	//	int debugbreak = 111;
	//}

	const char *buf, *obuf;
	buf = obuf = scriptbuf->SkipSpace(currentLable->c_buf);
	int ret, vardeep = 1;
	bool isok = false;
	bool isstr_alias = false;
	
	//优先尝试%XX $XX立即数，速度最快
	char vart = buf[0];
	if (vart == '%' || vart == '$' || vart == '"' || vart == '-') buf++;
	char ch = buf[0];
	if (ch == '%' && (vart == '%' || vart == '$')) {  //最多搜索两层，更多的交给表达式处理
		buf++;
		vardeep++; 
	}
	if(vart == '"') buf = TryGetImmediateStr(ret, buf, isok);
	else buf = TryGetImmediate(ret, buf, true, isok);
	if (!isok && str_alias) buf = TryGetStrAlias(ret, buf, isstr_alias); //这里有个风险，同名的整数别名可能会覆盖掉文字别名
	if (isok || isstr_alias) {
		if(!ref) ref = new ONSVariableRef;
		if (vart == '-' && !isstr_alias) ret = 0 - ret;
		if (vardeep > 1) { //获取最里面一层的%值
			ref->SetRef(ONSVariableRef::TYPE_INT, ret);
			ret = ref->GetReal();
		}
		//包装结果
		if (vart == '"') {
			LOString s = scriptbuf->substr(obuf + 1, ret);
			ref->SetImVal(&s);
		}
		else if (isstr_alias)ref->SetImVal(&strAliasList[ret]);
		else if (vart == '$') ref->SetRef(ONSVariableRef::TYPE_STRING, ret);
		else if(vart == '%') ref->SetRef(ONSVariableRef::TYPE_INT, ret);
		else ref->SetImVal((double)ret);
		currentLable->c_buf = buf;
		return ref;
	}

	//if (GetCharacter(buf) != CHARACTER_NUMBER)return NULL; //不是数字开头，也不是别名

	//尝试表达式
	buf = obuf;
	ONSVariableRef *v = ParseIntExpression(buf, true);
	currentLable->c_buf = buf;
	if (!ref) return v;
	else {
		ref->CopyFrom(v);
		delete v;
		return ref;
	}
}

//只允许数值型
int LOScriptReader::ParseIntVariable() {
	ONSVariableRef *v = ParseVariableBase();
	if (!v) {
		PrintError("Is not a valid expression");
		return 0;
	}
	if (!v->isReal()) PrintError("You should use an integer value!");
	int ret = (int)v->GetReal();
	delete v;
	return ret;
}


LOString LOScriptReader::ParseStrVariable() {
	ONSVariableRef *v = ParseVariableBase(NULL,true);
	if (!v) {
		PrintError("Is not a valid expression");
		return LOString();
	}
	if (!v->isStr()) PrintError("You should use an string value!");
	LOString s;
	LOString *vs = v->GetStr();
	if (vs) s.assign(*vs);
	delete v;
 	return s;
}

//返回的字符串不包含'*'
LOString LOScriptReader::ParseLabel(bool istry) {
	if (NextStartFrom('*')) {
		while(CurrentOP() == '*')NextAdress();
		NextNormalWord(true);
		return LOString(word);
	}
	else if (NextStartFrom('$') || NextStartFrom('"')) {
		LOString tmp = ParseStrVariable();
		int ulen = 0;
		while ( tmp[ulen] == '*') ulen++;
		return LOString(tmp.c_str() + ulen);
	}
	else if(!istry){
		PrintError("it's not a label or a string value!");
	}
	return LOString();
}

ONSVariableRef *LOScriptReader::ParseLabel2() {
	LOString tmp = ParseLabel(true);
	if (tmp.length() > 0) {
		ONSVariableRef *v = new ONSVariableRef(ONSVariableRef::TYPE_STRING_IM);
		v->SetValue(&tmp);
		return v;
	}
	else return NULL;
}

//尝试取得立即数
const char* LOScriptReader::TryGetImmediate(int &val, const char *buf, bool isalias, bool &isok) {
	const char *obuf = buf;
	isok = false;

	buf = scriptbuf->SkipSpace(buf);
	int type = scriptbuf->GetCharacter(buf);
	if (type == LOCodePage::CHARACTER_NUMBER) {
		double vv = scriptbuf->GetReal(buf);
		buf = scriptbuf->SkipSpace(buf);
		if (scriptbuf->IsOperator(buf)) return obuf; //后面有计算符号，说明是复杂算式
		val = (int)vv;
		isok = true;
		return buf;
	}
	else if (buf[0] == '-') { //负数
		int ret;
		buf = TryGetImmediate(ret, buf + 1, false, isok);
		if (isok) {
			val = 0 - ret;
			return buf;
		}
	}
	else if (isalias && type == LOCodePage::CHARACTER_LETTER) {
		LOString s = scriptbuf->GetWordStill(buf, LOCodePage::CHARACTER_LETTER | LOCodePage::CHARACTER_NUMBER);
		buf = scriptbuf->SkipSpace(buf);
		if (scriptbuf->IsOperator(buf) ) return obuf;  //后面有计算符号，说明是复杂算式
		val = GetAliasRef(s, false, isok);
		if (isok) return buf;
		else return obuf;
	}
	else return obuf;
	return obuf;
}

int LOScriptReader::GetAliasRef(LOString &s, bool isstr, bool &isok) {
	std::unordered_map<std::string, int> *tmap = &numAliasMap;
	if (isstr) tmap = &strAliasMap;

	auto iter = tmap->find(s);
	if (iter == tmap->end()) {
		isok = false;
		return -1;
	}
	else {
		isok = true;
		return iter->second;
	}
}

const char*  LOScriptReader::TryGetImmediateStr(int &len, const char *buf, bool &isok) {
	const char *obuf = buf;
	isok = false;
	len = scriptbuf->GetStringLen(buf);
	buf += len;
	if (buf[0] == '"') buf++;
	buf = scriptbuf->SkipSpace(buf);
	if (buf[0] == '+') return obuf;
	else {
		isok = true;
		return buf;
	}
}

const char* LOScriptReader::TryGetStrAlias(int &ret, const char *buf, bool &isok) {
	const char *obuf = buf;
	isok = false;
	if (strAliasMap.size() < 1) return obuf;
	if (scriptbuf->GetCharacter(buf) == LOCodePage::CHARACTER_LETTER) {
		LOString s = scriptbuf->GetWordStill(buf, LOCodePage::CHARACTER_LETTER | LOCodePage::CHARACTER_NUMBER);
		ret = GetAliasRef(s, true, isok);
		if (isok) return buf;
		else return obuf;
	}
	return obuf;
}

//获取逆波兰式的堆栈
const char* LOScriptReader::GetRPNstack(LOStack<ONSVariableRef> *s2, const char *buf, bool isalias) {
	//初始化符号栈
	LOStack<ONSVariableRef> s1;
	ONSVariableRef *v = new ONSVariableRef;
	LOString s;
	bool first = true;
	s1.push(v);
	//===================//
	int type;
	while (true) {
		buf = scriptbuf->SkipSpace(buf);
		//LOLog_i("char is:%d",buf[0]) ;
		type = scriptbuf->GetCharacter(buf);
		//LOLog_i("type is:%d",type) ;
		if (type == LOCodePage::CHARACTER_NUMBER || (buf[0] == '-' && first)) { //必须处理首个负数
			double tdd;
			if (buf[0] == '-') {
				buf++;
				tdd = 0 - scriptbuf->GetReal(buf);
			}
			else tdd = scriptbuf->GetReal(buf);
			v = new ONSVariableRef(ONSVariableRef::TYPE_REAL, tdd);
			s2->push(v);  //操作数立即入栈
			//检查符号栈，如果是% $则全部弹出这两个符号
			//while (s1.top()->oper == '%' || s1.top()->oper == '$') s2->push(s1.pop());

			//下一个符号不是运算符则停止
			buf = scriptbuf->SkipSpace(buf);
			if (!scriptbuf->IsOperator(buf)) break;
		}
		else if (type == LOString::CHARACTER_SYMBOL || scriptbuf->IsMod(buf) ) { //因为有mod这种特殊的，所以不能只使用类型判断
			v = new ONSVariableRef;
			buf += v->GetOperator(buf);
			if (v->isOperator()) {
				//数组用的方括号不在这里处理
				if (v->oper == '(') {
					s1.push(v);
				}
				else if (v->oper == ')') PopRPNstackUtill(&s1, s2, '(');
				else if (v->oper == '[') {
					PopRPNstackUtill(&s1, s2, '?');
					s1.push(v);
				}
				else if (v->oper == ']') PopRPNstackUtill(&s1,s2,'[');
				else {
					//如果是数组，则压入一个特殊符号
					if (v->oper == '?') {
						s2->push(new ONSVariableRef(ONSVariableRef::TYPE_ARRAY_FLAG));
					}
					//
					auto iter = s1.end() - 1;
					if (v->order > (*iter)->order) s1.push(v);
					else if (v->order == (*iter)->order && v->order == ONSVariableRef::ORDER_GETVAR) s1.push(v);
					else {
						while (v->order <= (*iter)->order && (*iter)->oper != '(' &&
							(*iter)->oper != '[') {
							s2->push(*iter);
							s1.erase(iter--);
						}
						s1.push(v);
					}
				}

			}
			else if (buf[0] == '"') { //string
				v->vtype = ONSVariableRef::TYPE_STRING_IM;
				LOString s = scriptbuf->GetString(buf);
				v->SetValue(&s);
				s2->push(v);
				buf = scriptbuf->SkipSpace(buf);
				if (!scriptbuf->IsOperator(buf)) break;
			}
			else {
				delete v;
				break;
			}

		}
		else if (isalias && type == LOCodePage::CHARACTER_LETTER) { //允许别名则处理别名
			s = scriptbuf->GetWordStill(buf, LOCodePage::CHARACTER_LETTER | LOCodePage::CHARACTER_NUMBER);
			//别名作为立即数处理
			bool isok;
			v = new ONSVariableRef(ONSVariableRef::TYPE_REAL, GetAliasRef(s, false, isok));
			s2->push(v);
		}
		else {
			//正常语句结束
			char ch = buf[0];
			if (ch == ',' || ch == '\n' || ch == ':') break; 
			else LOLog_i("line at [%d] may be have error.\n", currentLable->c_line);
			break;
		}

		first = false;
	}
	PopRPNstackUtill(&s1, s2, '\0');
	if (!s2->top() || !s2->top()->isOperator()) 
		PrintError("Expression error");
	return buf;
}


//计算逆波兰式
void LOScriptReader::CalculatRPNstack(LOStack<ONSVariableRef> *stack) {
	auto iter = stack->begin();
	int docount;  //操作数的个数
	ONSVariableRef *op,*v1 = nullptr,*v2 = nullptr;

	while (iter != stack->end()) {
		op = (*iter);
		if (op->isOperator()) {
			//运算符位于首个符号是不可能的
			if (iter == stack->begin()) 
				PrintError("Expression error!");
			//该符号需要的操作数
			docount = op->GetOpCount();
			//检查操作数的数量是否还符合要求
			if (stack->size() < docount + 1) {
				PrintError("[ %c ] Insufficient operands required by operator!", (*iter)->oper);
				return;
			}
			else if (docount == 1) {
				v1 = *(iter - 1);
				if ((*iter)->oper == '?') {
					//找到数组操作的起始标记
					while ((*iter)->vtype != ONSVariableRef::TYPE_ARRAY_FLAG) iter--;
					stack->erase(iter);   //删除数组标记
					v1 = (*iter);  //被操作的数组编号
					v1->DownRefToIm(ONSVariableRef::TYPE_REAL);
					v1->InitArrayIndex();
					iter++;
					for (int ii = 0; ii < MAXVARIABLE_ARRAY && !(*iter)->isOperator(); ii++) {
						v1->SetArrayIndex((int)(*iter)->GetReal(), ii);
						stack->erase(iter, true);  //删除，同时指向下一个指针
					}
					v1->UpImToRef(ONSVariableRef::TYPE_ARRAY);
				}
				else if((*iter)->oper == '$') v1->UpImToRef(ONSVariableRef::TYPE_STRING);
				else if ((*iter)->oper == '%') v1->UpImToRef(ONSVariableRef::TYPE_INT);
				else PrintError("%s","LOScriptReader::CalculatRPNstack() unknow docount = 1 type!");
				stack->erase(iter, true);  //单操作数只删除符号
			}
			else{
				v1 = *(iter - 2);
				v2 = *(iter - 1);
				if(!v1->Calculator(v2, (*iter)->oper ,false )) break;
				iter--;  //指向开始删除的位置
				stack->erase(iter, true); //删除后一个数
				stack->erase(iter, true); //删除后一个符号
			}
		}
		else iter++;
	}

	//计算完成后堆栈只能剩一个元素
	if (stack->size() > 1) PrintError("Stack error after calculation, please check the formula!");
	return ;
}

void LOScriptReader::PopRPNstackUtill(LOStack<ONSVariableRef> *s1, LOStack<ONSVariableRef> *s2, char op) {
	auto iter = s1->end()-1;
	while (s1->size() > 1) {
		if ((*iter)->oper == op) {
			if (op != '?') {
				s1->erase(iter,true);   //抛弃'('  '['
			}
			return;
		}
		s2->push(*iter);
		s1->erase(iter--);  //符号转移到s2，不删除
	}
	if (op == '\0') {
		s1->clear(true);
		return;
	}
	PrintError("missing match '%c'", op);
}



std::vector<ONSVariableRef*> LOScriptReader::TransformStack(LOStack<ONSVariableRef> *s1) {
	std::vector<ONSVariableRef*> s;
	for (auto iter = s1->begin(); iter != s1->end(); iter++) s.push_back(*iter);
	return s;
}

bool LOScriptReader::NextStartFrom(char op) {
	currentLable->c_buf = scriptbuf->SkipSpace(currentLable->c_buf);
	return ( currentLable->c_buf[0] == op);
}

bool LOScriptReader::NextStartFrom(const char* op) {
	const char *buf = currentLable->c_buf;
	buf = scriptbuf->SkipSpace(buf);
	while (op[0] != 0) {
		if (tolower(op[0]) != tolower(buf[0])) return false;
		op++;
		buf++;
	}
	if (scriptbuf->GetCharacter(buf) == LOCodePage::CHARACTER_LETTER) return false; //下一个依然是字母说明是连续的字母
	currentLable->c_buf = buf;
	return true;
}

bool LOScriptReader::NextStartFromStrVal() {
	return (NextStartFrom('"') || NextStartFrom('$'));
}

//普通单词不包括 stralias
ONSVariableRef* LOScriptReader::TryNextNormalWord() {
	const char* buf =  currentLable->c_buf;
	buf = scriptbuf->SkipSpace(buf);
	if (buf[0] == '#') {
		ONSVariableRef *v = new ONSVariableRef(ONSVariableRef::TYPE_STRING_IM);
		LOString s = scriptbuf->substr(buf, 7);
		v->SetValue(&s);
		currentLable->c_buf = buf + 7;
		return v;
	}
	else if (NextStartFrom('"') || NextStartFrom('$')) return ParseVariableBase();
	else {
		NextNormalWord(false);
		auto iter = strAliasMap.find(word);
		if (iter == strAliasMap.end()) {
			ONSVariableRef *v = new ONSVariableRef(ONSVariableRef::TYPE_STRING_IM);
			v->SetValue(&word);
			currentLable->c_buf += word.length();
			return v;
		}
	}
	return NULL;
}

bool LOScriptReader::LineEndIs(const char*op) {
	const char *buf = scriptbuf->NextLine(currentLable->c_buf);
	if (buf[0] == '\n') buf--;
	while (scriptbuf->GetCharacter(buf) == LOCodePage::CHARACTER_SPACE) buf--;
	buf -= strlen(op) -1;
	if (scriptbuf->GetCharacter(buf - 1) == LOCodePage::CHARACTER_LETTER) return false; //前一个是字母
	while (op[0] != 0) {
		if (tolower(op[0]) != tolower(buf[0])) return false;
		op++;
		buf++;
	}
	return true;
}

void LOScriptReader::BackLineStart() {
	const char *buf = currentLable->c_buf;
	while (buf[0] == '\n') buf--;  //已经在行尾了，往前一个符号
	while (buf[0] != '\n') buf -= scriptbuf->GetEncoder()->GetLastCharLen(buf);
	buf++;
	currentLable->c_buf = buf;
}

int LOScriptReader::LogicJump(bool iselse) {
	int deep = 0;  //if then 的嵌套层数
	while (CurrentOP() != 0) {
		NextLineStart();
		if (NextStartFrom("if") && LineEndIs("then")) deep++;
		//else 后面直接换行就是接的
		else if (iselse && NextStartFrom("else")) {
			const char *buf = currentLable->c_buf;
			buf = scriptbuf->SkipSpace(buf);
			if (buf[0] == '\n' && deep == 0)return 1; //没有深层嵌套才返回
			else if (buf[0] == '\0') return -1;
		}
		else if (NextStartFrom("endif")) {
			if (deep == 0) return 2;
			else deep--;
		}
	}
	return -1;
}


void LOScriptReader::NextLineStart() {
	currentLable->c_buf = scriptbuf->NextLine(currentLable->c_buf);
	currentLable->c_line++;
}

//用于goto等强制跳转
bool LOScriptReader::ChangePointer(LOScriptPoint *label) {
	subStack->pop();
	ReadyToRun(label);
	return true;
}

//切换当前堆栈
//bool LOScriptReader::ChangeStackData(int totype) {
//	subStack = &stackData[totype];
//	loopStack = &loopStackData[totype];
//	if (subStack->size() > 0) currentLable = subStack->top();
//	return true;
//}


bool LOScriptReader::ParseLogicExp() {
	int comtype,ret = 1; //默认为真
	int nextconet = ONSVariableRef::LOGIC_AND;
	ONSVariableRef *v1 = nullptr;
	ONSVariableRef *v2 = nullptr;
	while (true) {
		if (v1) delete v1;
		v1 = ParseVariableBase(NULL,true);
		if (NextStartFrom("==")) comtype = ONSVariableRef::LOGIC_EQUAL;
		else if (NextStartFrom("!=") || NextStartFrom("<>"))comtype = ONSVariableRef::LOGIC_UNEQUAL;
		else if (NextStartFrom(">="))comtype = ONSVariableRef::LOGIC_BIGANDEQUAL;
		else if (NextStartFrom("<="))comtype = ONSVariableRef::LOGIC_LESSANDEQUAL;
		else if (NextStartFrom('>')) {
			comtype = ONSVariableRef::LOGIC_BIGTHAN;
			NextAdress();
		}
		else if (NextStartFrom('<')) {
			comtype = ONSVariableRef::LOGIC_LESS;
			NextAdress();
		}
		else if (NextStartFrom('=')) {
			comtype = ONSVariableRef::LOGIC_EQUAL;
			NextAdress();
		}
		else {
			PrintError("Logical expression error!");
			return false;
		}

		if (v2) delete v2;
		v2 = ParseVariableBase(NULL, true);
		if (!v2) {
			PrintError("Logical expression error!");
			return false;
		}
		int ret0 = v1->Compare(v2, comtype,false);
		if (nextconet == ONSVariableRef::LOGIC_AND) ret &= ret0;
		else if (nextconet == ONSVariableRef::LOGIC_OR) ret |= ret0;
		if (NextStartFrom("&&") || NextStartFrom("&")) nextconet = ONSVariableRef::LOGIC_AND;
		else if (NextStartFrom("||") || NextStartFrom("|")) nextconet = ONSVariableRef::LOGIC_OR;
		else break;
	}

	if (v1) delete v1;
	if (v2) delete v2;
	if (ret == 0) return false;
	else return true;
}

LOString LOScriptReader::GetTextTagString(){
	return TagString;
}

//部分文本需展开$符号
bool LOScriptReader::ExpandStr(LOString &s) {
	LOCodePage *encoder = s.GetEncoder();
	LOString *tmp = nullptr;
	bool isexp = false;
	int ulen;
	const char* buf = s.c_str();
	const char* backbuf = currentLable->c_buf;

	while (buf[0] != 0) {
		if (buf[0] == '$' || buf[0] == '%') {
			isexp = true; //进入表达式解析模式
			if (!tmp) tmp = new LOString(s.c_str(), buf - s.c_str());
			tmp->SetEncoder(s.GetEncoder());

			LOString *optr = scriptbuf;
			scriptbuf = &s;
			currentLable->c_buf = buf;

			ONSVariableRef *v1 = ParseVariableBase();
			ONSVariableBase::AppendStrCore(tmp, v1->GetStr());
			delete v1;

			buf = currentLable->c_buf; //当前到达的位置
			scriptbuf = optr;
		}
		else {
			ulen = encoder->GetCharLen(buf);
			if (isexp) tmp->append(buf, ulen);
			buf += ulen;
		}
	}

	if (isexp) {
		s.assign(*tmp);
		delete tmp;
	}

	currentLable->c_buf = backbuf;

	return isexp;
}


const char* LOScriptReader::TryToNextCommand(const char*buf) {
	int ulen;
	while (buf[0] != 0 && buf[0] != '\n' && buf[0] != ':') {
		buf += scriptbuf->ThisCharLen(buf);
		if (buf[0] == '"') {
			const char* tbuf =  currentLable->c_buf;
			currentLable->c_buf = buf;
			ParseStrVariable();
			buf = currentLable->c_buf;
			currentLable->c_buf = tbuf;
		}
	}
	return buf;
}

int LOScriptReader::DefaultStep() {
	bool isok = false;
	LOString fn ;
	BinArray *bin;

	//LOLog_i("read dir is:%s",readDir.c_str()) ;

	//优先考虑00.txt-99.txt
	for (int ii = 0; ii < 100; ii++) {
		fn = std::to_string(ii) + ".txt";
		bin = ReadFile(&fn, false);
		if (!bin && ii < 10) {
			fn = "0" + fn;
			bin = ReadFile(&fn, false);
		}

		if (bin) {
			AddScript(bin->bin, bin->Length(), fn.c_str());
			LOLog_i("scripter[%s] has read.\n", fn.c_str());
			delete bin;
			isok = true;
		}
	}

	//然后考虑通常的脚本
	if (!isok) {
		fn = "nscript.dat";
		bin = ReadFile(&fn, false);
		if (bin) {
			unsigned char *buf = (unsigned char*)bin->bin;
			for (int ii = 0; ii < bin->Length() - 1; ii++) {
				buf[ii] ^= 0x84;
			}
			AddScript(bin->bin, bin->Length(), fn.c_str());
			PrintError("scripter[%s] has read.\n", fn.c_str());
			delete bin;
			isok = true;
		}
	}

	//加入内置系统脚本
	char *tmp = (char*)(intptr_t)(__buil_in_script__);
	AddScript(tmp, strlen(__buil_in_script__), "Buil_in_script.h");
	return isok;
}

void LOScriptReader::GetGameInit(int &w, int &h) {
	w = 640; h = 480;
	if (filesList.size() == 0) return;

	gloableMax = 200;

	LOScriptPoint *p = GetScriptPoint("__init__");
	ReadyToRun(p);
	const char *buf = currentLable->c_buf;
	const char *obuf = buf;
	while(buf[0] != 0 && buf - obuf < 4096){
		buf = scriptbuf->SkipSpace(buf);
		if (buf[0] == ';' && buf[1] == '$') {  //进入设置读取
			buf += 2;
			buf = scriptbuf->SkipSpace(buf);
			LOString s;
			while (buf[0] != 0 && buf[0] != '\n') {
				buf = scriptbuf->SkipSpace(buf);
				s = scriptbuf->GetWord(buf);
				if (s == "mode") {
					if (!strcmp(s.c_str(),"800" )) {
						w = 800; h = 600; buf += 3;
					}
					else if (!strcmp(s.c_str(), "400")) {
						w = 400; h = 300; buf += 3;
					}
					else if (!strcmp(s.c_str(), "320")) {
						w = 320; h = 240; buf += 3;
					}
					else if (!strcmp(s.c_str(), "w720")) {
						w = 1280; h = 720; buf += 4;
					}
					else break;
				}
				else if (s == "value" || s == "g" || s == "G") {
					buf = scriptbuf->SkipSpace(buf);
					gloableMax = scriptbuf->GetInt(buf);
				}
				else if (s == "v" || s == "V") {
					//LONS总是允许最大的变量数，忽略这个
					s = scriptbuf->GetWord(buf);
				}
				else if (s == "s" || s == "S") {
					w = scriptbuf->GetInt(buf);
					while (buf[0] == ',' || buf[0] == ' ' || buf[0] == '\t') buf++;
					h = scriptbuf->GetInt(buf);
				}
				else if (s == "l" || s == "L") {  //LONS标签数不受限制，不用管这个
					buf = scriptbuf->SkipSpace(buf);
					scriptbuf->GetInt(buf);
				}
				else if (s != ",") break;
			}
			break;
		}
		else {
			buf = scriptbuf->NextLine(buf);
		}
	}
	ReadyToBack();
}


int LOScriptReader::FileRemove(const char *name) {
	LOScriptReader *reader = (LOScriptReader*)scriptModule;
	if (reader->Name != Name) return reader->FileRemove(name); //转到主脚本执行，这样可以通过子类函数使用另外的读写函数
	else {
		return remove(name);
	}
}


const char* LOScriptReader::GetPrintName() {
	return activeReader->printName.c_str();
}


intptr_t LOScriptReader::GetEncoder() {
	return (intptr_t)scriptbuf->GetEncoder();
}

const char* LOScriptReader::debugCharPtr(int cur) {
	return scriptbuf->c_str() + cur;
}

void LOScriptReader::InserCommand(LOString *incmd) {
	if (!incmd || incmd->length() == 0) return;
	//插入命令相当于 gosub *随机标签   xxxx   return
	//随机标签名
	LOString lname = LOString::RandomStr(16);
	//生成一个最小脚本，这样save的时候可以将脚本一起存储
	//从inser返回时注意将脚本删除
	LOString tmp = "*" + lname + "\n";
	tmp += *incmd;
	tmp += "\nreturn";

	AddScript(&tmp, nullptr);
	ReadyToRun(&lname, LOScriptPoint::CALL_BY_EVAL);
	//在 LOScriptReader::ReadyToBack()中删除添加的eval
}


void LOScriptReader::ResetBaseConfig() {
	lastCmdCheckFlag = 0;
	parseDeep = 0;
	printName = "_main";
	ttimer = SDL_GetTicks();
	nextReader = nullptr;
	st_globalon = false;
	st_errorsave = false;
	st_labellog = false;
}

//重置脚本模块，注意只应该从主脚本调用这个函数
void LOScriptReader::ResetMe() {
	//移除其他所有脚本解析
	while (nextReader) {
		LeaveScriptReader(nextReader);
	}
	//重置必要的属性
	//blocksEvent.InvalidAll();
	while (!isEndSub()) ReadyToBack();
	delete nslogic;
	if (subStack) delete subStack;
	if (loopStack) delete loopStack;

	nslogic = new LogicPointer(LogicPointer::TYPE_IF);
	subStack = new LOStack<LOScriptPoint>;
	loopStack = new LOStack<LogicPointer>;

	//清理def设置
	defsubMap.clear();

	//清理别名
	numAliasMap.clear();
	strAliasList.clear();
	strAliasMap.clear();
	ResetBaseConfig();
}

//获取自身报告
LOString LOScriptReader::GetReport() {
	LOString report = "scripter thread: " + Name + "\n";
	if (currentLable) {
		report.append("file: ");
		report.append(currentLable->file->Name.c_str());
		report.append("\n");
		report += "line: " + std::to_string(currentLable->c_line) + "\n--> ";
		//获取一行
		int len = 0;
		const char *current = currentLable->c_buf;
		BackLineStart();
		while (currentLable->c_buf[len] != 0 && currentLable->c_buf[len] != '\n') len++;
		report.append(currentLable->c_buf, len);
		currentLable->c_buf = current;
		report.append("\n");
	}
	if(scriptbuf) report.SetEncoder(scriptbuf->GetEncoder());
	return report;
}


void LOScriptReader::PrintError(const char *fmt, ...) {
	std::unique_ptr<BinArray> uptr(new BinArray(1024));
	memset(uptr.get(), 0, 1024);
	va_list argptr;
	va_start(argptr, fmt);
	vsnprintf(uptr->bin, 1024, fmt, argptr);
	va_end(argptr);

	LOString tmp = GetReport();
	tmp.append(uptr->bin);
	PrintErrorStatic(&tmp);
}

//一些事件的处理
int LOScriptReader::RunFunc(LOEventHook *hook, LOEventHook *e) {
	if (hook->param2 == LOEventHook::FUN_BTNFINISH) return RunFuncBtnFinish(hook, e);

	return LOEventHook::RUNFUNC_CONTINUE;
}

//按钮完成事件
int LOScriptReader::RunFuncBtnFinish(LOEventHook *hook, LOEventHook *e) {
	//来自超时的事件
	LOUniqEventHook ev;
	if (!e) {
		ev.reset(new LOEventHook());
		e = ev.get();
		e->PushParam(new LOVariant(-1));
		//btnval的超时值为-2
		e->PushParam(new LOVariant(-2));
	}

	if (e->catchFlag == LOEventHook::ANSWER_SEPLAYOVER) {
		//没有满足要求
		if (e->param1 != hook->GetParam(LOEventHook::PINDS_SE_CHANNEL)->GetInt()) return LOEventHook::RUNFUNC_CONTINUE;
	}
	//要确定是否清除btndef
	if (strcmp(hook->GetParam(LOEventHook::PINDS_CMD)->GetChars(nullptr), "btnwait2") != 0) {
		if (e->GetParam(1)->GetInt() > 0)
			imgeModule->ClearBtndef(hook->GetParam(LOEventHook::PINDS_PRINTNAME)->GetChars(nullptr));
	}
	//确定是否要设置变量的值，变量设置要转移到脚本线程中
	if (hook->GetParam(LOEventHook::PINDS_REFID) != nullptr) {
		e->paramListMoveTo(hook->GetParamList());
		hook->FinishMe();
	}
	else hook->InvalidMe();
	return LOEventHook::RUNFUNC_FINISH;
}

//变量赋值事件，这个函数应该从脚本线程执行
int LOScriptReader::RunFuncBtnSetVal(LOEventHook *hook) {
	//跟btnwaithook的原始参数个数有关
	int fix = 5;
	int refid = hook->GetParam(LOEventHook::PINDS_REFID)->GetInt();
	LOUniqVariableRef ref(ONSVariableRef::GetRefFromTypeRefid(refid));
	LOVariant *var = hook->GetParam(LOEventHook::PINDS_BTNVAL + fix);
	if (var->IsType(LOVariant::TYPE_INT)) {
		LOLog_i("btnwait is:%d", var->GetInt());
		ref->SetValue((double)var->GetInt());
	}
	else {
		//似乎有字符串版本的命令？
	}
	//切除多余的参数
	hook->paramListCut(fix);
	hook->InvalidMe();
	return 0;
}


int LOScriptReader::RunFuncSayFinish(LOEventHook *hook) {
	ReadyToRun(&userGoSubName[USERGOSUB_TEXT], LOScriptPoint::CALL_BY_TEXT_GOSUB);
	hook->InvalidMe();
	return 0;
}



//按钮事件
//int LOScriptReader::RunFuncBtnFinish(LOEventHook *hook, LOEventHook *e) {
//	LOUniqVariableRef ref(new ONSVariableRef((ONSVariableRef::ONSVAR_TYPE)hook->paramList[0]->GetInt(), hook->paramList[1]->GetInt()));
//	if (e->paramList[1]->IsType(LOVariant::TYPE_INT)) {
//		ref->SetValue((double)e->paramList[1]->GetInt());
//	}
//	hook->InvalidMe();
//	return LOEventHook::RUNFUNC_FINISH;
//}


void LOScriptReader::Serialize(BinArray *bin) {
	bin->WriteInt(0x49524353);  //SCRI
	bin->WriteInt(1);  //version
	bin->WriteInt(0);  //预留
	bin->WriteInt(0);   //预留

	bin->WriteLOString(&this->Name);  //脚本名称
	bin->WriteLOString(&this->printName); //显示队列名称

	bin->WriteInt(0x4B415453); //STAK
	bin->WriteInt(subStack->size()); //count

	int positon = 0;
	int callcount = 0;
	while (callcount < subStack->size()) {
		bin->WriteInt(0x4C4C4143); //CALL

		intptr_t callby = (intptr_t)subStack->at(callcount); 
		bool iseval = false;
		LOScriptPoint *p = nullptr;
		//什么样的call
		if (callby < 0x10) {
			bin->WriteInt(callby);
			if (callby == LOScriptPoint::CALL_BY_EVAL) iseval = true;
			callcount++;
			p = subStack->at(callcount);
		}
		else {
			bin->WriteInt(LOScriptPoint::CALL_BY_NORMAL);
			p = (LOScriptPoint*)callby;
		}

		positon = 0;
		bin->WriteLOString(&p->name);  //标签名
		bin->WriteLOString(&p->file->Name); //文件名，暂时没什么用
		bin->WriteInt(p->file->GetPointLineAndPosition(p, &positon)); //当前位置位于相对于标签有多少行
		bin->WriteInt(positon);  //相对于行有多少偏移
	}

	//逻辑判断

}





//void LOScriptReader::Serialize(BinArray *sbin) {
//	sbin->WriteString("scriptStart");
//	sbin->WriteString(&Name);
//	sbin->WriteString(&printName);
//	sbin->WriteInt32((int32_t)interval);
//	sbin->WriteString(&curCmd);
//	SerializeScriptPoint(sbin, currentLable);
//	sbin->WriteInt(subStack->size());
//	for (int ii = 0; ii < subStack->size(); ii++) {
//		SerializeScriptPoint(sbin, subStack->at(ii));
//	}
//	sbin->WriteInt(loopStack->size());
//	for (int ii = 0; ii < loopStack->size(); ii++) {
//		SerializeLogicPoint(sbin, loopStack->at(ii));
//	}
//}
//
//void LOScriptReader::SerializeScriptPoint(BinArray *sbin, ScriptPoint *p) {
//	sbin->WriteString("scp");
//	if ((uintptr_t)(p) < 10) {
//		sbin->WriteInt(-1);
//		//sbin->WriteInt((int)(p));
//	}
//	else {
//		sbin->WriteInt(p->type);
//		sbin->WriteString(&p->fileName);
//		sbin->WriteString(&p->name);
//		//sbin->WriteInt(p->current_line - p->start_line);
//		sbin->WriteInt(p->current_address - p->start_address);  //相对于标签开始的位置
//		sbin->WriteBool(p->canfree);
//	}
//}
//
//void LOScriptReader::SerializeLogicPoint(BinArray *sbin, LogicPointer *p) {
//	sbin->WriteString("lcp");
//	sbin->WriteInt(p->type);
//	if (p->type == LogicPointer::TYPE_FOR || p->type == LogicPointer::TYPE_WHILE) {
//		sbin->WriteInt(p->relAdress);
//		sbin->WriteString(p->lableName);
//		sbin->WriteInt(p->step);
//		if (p->var) {
//			sbin->WriteBool(true);
//			//p->var->Serialize(sbin);
//		}
//		else sbin->WriteBool(false);
//		if (p->dstvar) {
//			sbin->WriteBool(true);
//			//p->dstvar->Serialize(sbin);
//		}
//		else sbin->WriteBool(false);
//	}
//	else {
//		sbin->WriteBool(p->ifret);
//	}
//}