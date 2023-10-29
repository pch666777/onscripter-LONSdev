#include "LOScriptReader.h"
#include "../etc/LOIO.h"
#include "../etc/LOTimer.h"
#include "../etc/LOMessage.h"
#include "Buil_in_script.h"
#include <stdarg.h>
#include <SDL_mixer.h>
//#include <SDL.h>
//#include "LOMessage.h"


//=========== static ========================== //
std::unordered_map<std::string, int> LOScriptReader::numAliasMap;   //整数别名
std::unordered_map<std::string, int> LOScriptReader::strAliasMap;   //字符串别名，存的是字符串位置
std::vector<LOString> LOScriptReader::strAliasList;  //字符串别名
std::vector<LOString> LOScriptReader::workDirs;
std::unordered_map<std::string, LOScriptPoint*> LOScriptReader::defsubMap;
int LOScriptReader::sectionState ;
bool LOScriptReader::st_globalon; //是否使用全局变量
bool LOScriptReader::st_labellog; //是否使用标签变量
bool LOScriptReader::st_errorsave; //是否使用错误自动保存
bool LOScriptReader::st_saveonflag = false;
bool LOScriptReader::st_autosaveoff_flag = false;
bool st_skipflag = false;   //快进标记
//bool st_fastPrint = true;  //快速print模式，会导致print时帧数暴涨
int LOScriptReader::gloableBorder = 200;
LOScriptReader::SaveFileInfo LOScriptReader::s_saveinfo;
int G_lineLog = 1;
bool st_linePageFlag = false;

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
         SDL_LogError(0, "ScriptReader[ %s ]: this name already use!", name.c_str());
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
                 SDL_Log("script thread has exit:%s", reader->Name.c_str());
				 delete reader;
				 return baseReader->nextReader;
			 }
			 else baseReader = baseReader->nextReader;
		 }
	 }
	 else if(reader->sctype == SCRIPT_TYPE::SCRIPT_TYPE_TIMER) {
         SDL_Log("script thread has nouse:%s", reader->Name.c_str());
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
:normalLogic(LogicPointer::TYPE_IF)
{
	sctype = SCRIPT_TYPE::SCRIPT_TYPE_NORMAL;
    currentLable = nullptr ;
	ResetBaseConfig();
}

LOScriptReader::~LOScriptReader()
{
	while (!isEndSub()) ReadyToBack();
	loopStack.clear(true);
}

int LOScriptReader::MainTreadRunning() {
	if (sctype != SCRIPT_TYPE::SCRIPT_TYPE_MAIN) {
		FatalError("LOScriptReader::MainTreadRunning() must run at main scriptReader!");
		return -1;
	}

	ChangeModuleState(MODULE_STATE_RUNNING);
	//开始之前先读取全局变量
	ReadGlobleVarFile();

	bool isLableStart = true;
	const char* lables[] = { "define" , "start" };
	int ret = RET_CONTINUE;
	sectionState = SECTION_DEFINE;
	activeReader = this;

	//section循环
	while (!isModuleExit()) {

		//是否从指定标签开始
		if (isLableStart) {
			LOString lable(lables[sectionState]);
			if (!ReadyToRun(&lable, LOScriptPoint::CALL_BY_NORMAL))return -1;
		}

		//============轮询循环===========================
		while (!isModuleExit() && !isStateChange()) {

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
		//============轮询循环==结束=======================

		//检查循环退出是否由某些事件引起的
		if (isStateChange()) {
			if (isModuleLoading()) {
				LoadReset();
				LoadCore(s_saveinfo.id);
				isLableStart = false;
			}
			else if (isModuleReset()) {
				ResetMe();
				sectionState = SECTION_DEFINE;
				isLableStart = true;
			}
			//重置状态
			if (!isModuleError() && !isModuleExit()) {
				ChangeModuleState(MODULE_STATE_RUNNING);
				ChangeModuleFlags(0);
			}
		}
		else {
			isLableStart = true;
			sectionState++;
			if (sectionState > SECTION_NORMAL) break;
		}

		//=*****************==//
	}
	//--------------------------------------//
	//退出之前保存全局变量、文件读取列表等
	//没有错误才保存
	if (isModuleError()) return  -1;

	if(st_labellog) WriteLog(FunctionInterface::LOGSET_LABELLOG);
	if (st_globalon) {
		UpdataGlobleVariable();
		SaveGlobleVariable();
	}
	//保存环境
	LonsSaveEnvData();
	return 0;
} 


//是否运行到RunScript应该返回的位置
bool LOScriptReader::isEndSub() {
	return subStack.size() == 0;;
}

//准备转到下一个运行点
void LOScriptReader::ReadyToRun(LOScriptPoint *label, int callby) {
    //LOLog_i("ReadyToRun: %s,%x",label->name.c_str(),callby) ;
	LOScriptPointCall call(label);
	call.callType = callby;
	subStack.push_back(call);
	currentLable = &subStack.at(subStack.size() - 1);
	scriptbuf = currentLable->GetScriptStr();
}

bool LOScriptReader::ReadyToRun(LOString *lname, int callby) {
	if (!lname || lname->length() == 0) return false;
	LOScriptPoint *p = LOScriptPointCall::GetScriptPoint(*lname);
	if (!p) {
		FatalError("not lable name:[%s]", lname->c_str());
		return false;
	}
	ReadyToRun(p, callby);
	return true;
}


bool LOScriptReader::ReadyToRunEval(LOString *eval) {
	subStack.emplace_back();
	LOScriptPointCall *p = &subStack.at(subStack.size() - 1);
	p->callType = LOScriptPointCall::CALL_BY_EVAL;
	p->i_index = LOScripFile::AddEval(eval);
	p->s_line = p->c_line = 0;
	scriptbuf = p->GetScriptStr();
	p->s_buf = p->c_buf = scriptbuf->c_str();
	currentLable = p;
	return true;
}

//void LOScriptReader::ReadyToEval(LOScriptPointCall &p) {
//	evalStack.push_back(p);
//}

//准备返回上一个运行点，同时抛弃本个运行点
int LOScriptReader::ReadyToBack() {
	LOScriptPointCall lastCall = subStack.at(subStack.size() - 1);
	//不允许eval在这里退栈，会报一个错误
	if (lastCall.isEval()) {
		FatalError("Eval sub call can't pop at LOScriptReader::ReadyToBack()");
		return RET_ERROR;
	}
	subStack.pop_back();
	if (!isEndSub()) {
		currentLable = &subStack.at(subStack.size() - 1);
		scriptbuf = currentLable->GetScriptStr();
	}
	else currentLable = nullptr;

	int ret = RET_RETURN | ((lastCall.callType & 0xff) << 8);
	return ret;
}


bool LOScriptReader::ReadyToBackEval() {
	if (!currentLable->isEval()) return false;
	currentLable->freeEval();
	subStack.pop_back();
	currentLable = &subStack.at(subStack.size() - 1);
	scriptbuf = currentLable->GetScriptStr();
	return true;
}



//LOScriptPoint* LOScriptReader::GetScriptPoint(LOString lname) {
//	lname = lname.toLower();
//	LOScriptPoint *p = NULL;
//	for (int ii = 0; ii < filesList.size(); ii++) {
//		p = filesList.at(ii)->FindLable(lname);
//		if (p) return p;
//	}
//	LOLog_i("no lable:%s", lname.c_str());
//	return p;
//}

int LOScriptReader::GetCurrentLableIndex() {
	int index = 0;
	while (index < subStack.size()) {
		if (currentLable == &subStack.at(subStack.size() - 1 - index)) return index;
		index++;
	}
	return index;
}

LOScriptPoint  *LOScriptReader::GetParamLable(int index) {
	LOString s = GetParamStr(index);
	if (s.length() == 0) return NULL;
	return LOScriptPointCall::GetScriptPoint(s);
}


//转移运行点位置，0表示转到top，1表示top的前一个
bool LOScriptReader::ChangePointer(int esp) {
	int index = subStack.size() - 1 - esp;
	if (index < 0 || index > subStack.size() - 1) return false;
	currentLable = &subStack.at(index);
	scriptbuf = currentLable->GetScriptStr();
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

    //if (GetCurrentLine() == 29) {
    //    int debugint = 0;
    //}

	bool hasnormal, haslabel, hasvariable, mustref, isstr;
	int allow_type, paramcount = 0;
	const char *fromparam = param;
	const char *fromuse = used;
	const char *lastparam, *lastused;

	while (param[0] != 0) {
		if (paramcount == 0) {
			if (used[0] == 'N' && !NextSomething()) break;
			else if (used[0] == 'Y' && !NextSomething()) {
				FatalError("function paragram %d type error!", paramcount + 1);
				return false;
			}
		}
		
		lastparam = param; lastused = used;
		isstr = hasvariable = hasnormal = haslabel = false;
		allow_type = ONSVariableRef::GetTypeAllow(param, mustref);

		for (int ii = 0; param[ii] != ',' && param[ii] != 0; ii++) {
			if (param[ii] == 'N') hasnormal = true;
			else if (param[ii] == 'L') haslabel = true;
			else {
				hasvariable = true;
				if (param[ii] == 's' || param[ii] == 'r') isstr = true;
			}
		}
		//get it
		ONSVariableRef *v = nullptr;
		if (hasnormal) 
			v = TryNextNormalWord();
		if (!v && haslabel) v = ParseLabel2();
		if (!v && hasvariable) {
			v = ParseVariableBase(isstr);
			if (!v ||  !v->isAllowType(allow_type)) {
				FatalError("[%s] paragram %d type error!\n",curCmdbuf, paramcount + 1);
				ClearParams(true);
				return false;
			}
			if (mustref && !v->isRef()) {
				FatalError("[%s] paragram %d must be ref!\n", curCmdbuf, paramcount + 1);
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
			FatalError("function paragram %d type error!", paramcount + 1);
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
				TextPushParams();
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
	case LINE_ZERO:
		//到达脚本的尾部，通常是eval中的，尝试从eval中退出，如果失败，则脚本无法继续进行下去了
		if (!ReadyToBackEval()) {
			FatalError("faild ReadyToBackEval() at LOScriptReader::ContinueRun()");
		}
		return ret;
	}

	//正式有效的命令前，先执行阻塞事件
	ret = ContinueEvent();
	if (ret != RET_VIRTUAL) return ret;
	ret = RET_CONTINUE;

	switch (nextType)
	{
	case LINE_TEXT:
		//输出信息
		//if (currentLable->c_line == 0) {
		//	ret = 0;
		//}
		if (G_lineLog > 0) SDL_Log("[%d]:[showtext]", currentLable->c_line);
		//首先尝试获取TagString，然后开始获取文字内容，期间可以反复进入eval模式，直到文字开始显示
		//遇到文字 --> pretext -->返回后进入文字显示
		TagString = scriptbuf->GetTagString(currentLable->c_buf);
		// '/'只有首次显示才执行pretextgosub
		//line_text执行pretextgosub --> ReturnEvent执行textcommand --> 等待事件完成，执行textgosub
		imgeModule->GetModValue(MODVALUE_PAGEEND, &callby);
		if ((callby & 0xff) == '/') { //直接进入textcommand
			TextPushParams();
			ret = imgeModule->textCommand(this);
			ClearParams(false);
		}
		else ReadyToRun(&userGoSubName[USERGOSUB_PRETEXT], LOScriptPoint::CALL_BY_PRETEXT_GOSUB);
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
		if (!InserCommand()) {
			FatalError("eval command faild!");
			return RET_ERROR;
		}
		break;
	default:
		curCmd = GetLineFromCurrent();
		SDL_LogError(0, "unknow line type at line:%d-->%s", currentLable->c_line, curCmd.c_str());
		currentLable->c_buf = scriptbuf->NextLine(currentLable->c_buf);
		//if (isAddLine) currentLable->c_line++;
		currentLable->c_line++;
		break;
	}
	return ret;
}


//因为采用了轮询来模拟多线程脚本，所以部分需等待的命令是通过事件存储的
int LOScriptReader::ContinueEvent() {
	int index = 0;
	bool isfinish = false;
	bool hasEvent = false;
	bool isloop = true;
	auto iter = waitEventQue.begin();

	while (isloop) {
		isloop = false;

		LOShareEventHook ev = waitEventQue.GetEventHook(iter, false);
		if (!ev) break;

		hasEvent = true;
		//快进模式下，textbtnwai会立即完成（点击空白区域）
		if (st_skipflag) {
			if (ev->isTextBtnWait() && !ev->isStateAfterFinish()) {
				ev->InvalidMe();
				LOUniqEventHook qev(LOEventHook::CreateBtnClickEvent( -1, 0, 0));
				imgeModule->RunFunc(ev.get(), qev.get());
			}
		}

		//如果ev带有timer则验证是否超时
		if (ev->catchFlag & LOEventHook::ANSWER_TIMER) {
			if (CheckTimer(ev.get(), 2)) {
				if (ev->enterEdit()) {
					//如果是等待音频的，要检查音频播放是否完成，没完成时间+100ms
					if ((ev->catchFlag & LOEventHook::ANSWER_SEPLAYOVER) && Mix_Playing(0) > 0) {
						//printf("add!\n");
						ev->GetParam(0)->SetInt(ev->GetParam(0)->GetInt() + 100);
						ev->closeEdit();
					}
					else RunFuncBase(ev.get(), nullptr); //已经完成
				}
			}
		}
		else if (ev->catchFlag == LOEventHook::ANSWER_NONE) {
			if (ev->param2 == LOEventHook::FUN_BGM_AFTER) {
				audioModule->PlayAfter();
				ev->InvalidMe();
			}
		}

		//这里有个小技巧，如果hook是 finish状态，表示需要由脚本线程处理余下的过程
		//如果是Invalid()表示已经处理完成了，不需要再次额外处理
		if (ev->isFinish()) {
			RunEventAfterFinish(ev.get());
			isfinish = true;
		}
		else if (ev->isInvalid()) isfinish = true;
		
	}

	//返回continue将会阻塞线程
	if (isfinish) return RET_CONTINUE;

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
	LOScriptPoint *p = LOScriptPointCall::GetScriptPoint(*pname);
	if (p) {
		LOScriptReader *scripter = LOScriptReader::EnterScriptReader(threadName);
		scripter->gosubCore(p, false);
		scripter->moduleState = MODULE_STATE_RUNNING;
        SDL_Log("create scripter thread:%s", threadName.c_str());
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
	if (ch == '\0') return LINE_ZERO;
	int type = scriptbuf->GetCharacter(buf);
	if (type == LOCodePage::CHARACTER_LETTER) return LINE_CAMMAND;
	else if (type == LOCodePage::CHARACTER_MULBYTE) return LINE_TEXT;
	if (ch == ',') return LINE_CONNECT;   //空格作为间隔，或者某些有意义的符号
	return LINE_UNKNOW;
}

int LOScriptReader::RunCommand(const char *&buf) {
	//if (currentLable->c_line == 39447) {
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
				FatalError("no label for this function:%s!\n", curCmdbuf);
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
			//空格隔断，或者‘,’号
			buf = scriptbuf->SkipSpace(buf);
			if (buf[0] == ',') buf++;

            if(G_lineLog) SDL_Log("[%d]:[%s]\n", currentLable->c_line, tmpcmd.c_str());
			//if (currentLable->current_line == 54) {
			//	int debugbreak = 1;
			//}
			if (!PushParams(ptr->param, ptr->used))return RET_ERROR;
			int ret = FunctionInterface::RunCommand(ptr->method);
			ClearParams(false);
			return ret;
		}
		else {
            SDL_Log("[%s]:[%d]:[%s] is not supported yet!!\n",
				GetCurrentFile().c_str(), currentLable->c_line, tmpcmd.c_str());
			return RET_WARNING;
		}

		return RET_ERROR;
	}
	FatalError("empty command!");
	return RET_ERROR;
}

LOString LOScriptReader::GetCurrentFile() {
	if (currentLable && currentLable->file()) {
		return currentLable->file()->Name;
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
    //严格来说，只有','才是符合要求的标记，但是ons有大量的历史非标准写法，所以需要一些特殊支持
	const char* buf = currentLable->c_buf;
	buf = scriptbuf->SkipSpace(buf);
	if (buf[0] == ',') {
		currentLable->c_buf = buf + 1;
		return true;
	}
    //要判断是否是非标准写法，比如空格或者 "与数字紧密相邻，或者数字与"紧密相连，只要不是命令，都可以尝试
    bool nonStand = false ;
    int charType = scriptbuf->GetCharacter(buf);
    if (charType == LOCodePage::CHARACTER_NUMBER) nonStand = true ;
    else if(buf[0] == '"' || buf[0] == '%' ) nonStand = true ; //$是否应该增加？
    //命令肯定不可能从 " % 0-9开始，因此，是安全的
    if(nonStand){
        currentLable->c_buf = buf;
        return true;
    }
    if (!isTry) SDL_LogError(0,"NextComma not at a comma!");

    return false ;
}

bool LOScriptReader::isName(const char* name) {
	return (curCmd.compare(name) == 0);
}


ONSVariableRef* LOScriptReader::ParseIntExpression(const char *&buf, bool isstrAlias) {
	std::vector<LOUniqVariableRef> s2;
	buf = GetRPNstack2(&s2, buf, isstrAlias);
	//buf = GetRPNstack2(&s2, buf, isstrAlias);
    //auto ss = TransformStack(&s2);
	CalculatRPNstack(&s2);
	if (s2.size() == 0) return nullptr;
	ONSVariableRef *ref = (*s2.begin()).release();
	return ref;
}


//isstr决定是文字类型还是整数类型，默认都是整数类型的，文字类型只会出现在文字变量的首位置和+号之后
ONSVariableRef *LOScriptReader::ParseVariableBase(bool isstr, bool isInDialog) { //在对话中  '/'  符号的意义是不一样的
	const char *buf, *sbuf;
	LOString ts;
	double tval;

//	if (currentLable->c_line == 38463) {
//		int bbq = 1;
//	}

	sbuf = buf = scriptbuf->SkipSpace(currentLable->c_buf);
	//如果立即遇到',' 那么认为是默认值
	if (buf[0] == ',') {
		ONSVariableRef *ref = new ONSVariableRef();
		if (isstr) ref->SetImVal(nullptr);
		else ref->SetImVal(0.0);
		currentLable->c_buf = buf;
		return ref;
	}

	//遇到立即数或者立即文字跳出循环，别名也是立即数。优先尝试%XX $XX立即数，速度最快
	//表达式计算功能强大，但是速度比较慢，而且需要更多的内存
	char op[] = { 0,0,0,0 };
	int curtype, ii, alias, aret = 0;
	for (ii = 0; ii < 4; ii++) {
		curtype = ONSVariableRef::GetYFtype(buf, ii == 0);
		//只有两种情况会继续，一种是整数表达式，一种是文字表达式
		if (curtype == ONSVariableRef::YF_IntRef) { op[ii] = '%'; buf++; }
		else if (curtype == ONSVariableRef::YF_StrRef) { 
			//文字表达式只出现在行首，不允许出现 %$1这种情况
			if (ii != 0) {
				FatalError("Expression error!");
				return nullptr;
			}
			op[ii] = '$'; buf++;
		}
		else break;  //其他的均转表达式处理
	}

	//是别名，尝试展开别名
	switch (curtype){
	case ONSVariableRef::YF_Alias:
		ts = scriptbuf->GetWordStill(buf, LOCodePage::CHARACTER_LETTER | LOCodePage::CHARACTER_NUMBER);
		aret = -3; 
		break;
	case ONSVariableRef::YF_Int:
		tval = scriptbuf->GetReal(buf); aret = -1;
		break;
	case ONSVariableRef::YF_Str:
		ts = scriptbuf->GetString(buf); aret = -2;
		break;
	case ONSVariableRef::YF_Negative:
		tval = scriptbuf->GetRealNe(buf); aret = -1;
		break;
	default:
		break;
	}

	//检查立即处理模式的后一个对象是否为符号，符号交给表达式处理
	buf = scriptbuf->SkipSpace(buf);
	//如果是对话，要检查是否 '/' 符号，是的话也是终止符
	if (aret != 0 && ONSVariableRef::GetYFtype(buf, false) == ONSVariableRef::YF_Oper) {
		if (isInDialog && buf[0] == '/') aret = aret; //对话终止符
		else aret = 0;
	}

	if (aret == 0) {
		buf = sbuf;
		ONSVariableRef *v = ParseIntExpression(buf, isstr);
		currentLable->c_buf = buf;
		return v;
	}
	else { //立即模式成功
		//解释别名
		if (aret == -3) aret = GetAliasRef(ts, isstr && ii == 0, alias);
		if (aret == 0) {
			FatalError("Expression error!");
			return nullptr;
		}
		//设置数值
		ONSVariableRef *v = new ONSVariableRef();
		if (aret == ALIAS_INT) v->SetImVal((double)alias);
		else if (aret == ALIAS_STR) v->SetImVal(&strAliasList[alias]);
		else if (aret == -2) v->SetImVal(&ts);
		else v->SetImVal(tval);
		//解释数值
		for (ii = 3; ii >= 0; ii--) {
			if (op[ii] == '%')  v->UpImToRef(ONSVariableRef::TYPE_INT_REF);
			else if (op[ii] == '$') v->UpImToRef(ONSVariableRef::TYPE_STR_REF);
		}
		currentLable->c_buf = buf;
		return v;
	}
	return nullptr;
}

//只允许数值型
int LOScriptReader::ParseIntVariable() {
	std::unique_ptr<ONSVariableRef> v(ParseVariableBase(ALIAS_INT));
	if (!v || !v->isReal()) {
		FatalError("LOScriptReader::ParseIntVariable() get int faild!");
		return -1;
	}
	return (int)v->GetReal();
}


LOString LOScriptReader::ParseStrVariable() {
	std::unique_ptr<ONSVariableRef> v(ParseVariableBase(ALIAS_STR));
	if (!v || !v->isStr()) {
		FatalError("LOScriptReader::ParseIntVariable() get int faild!");
		return LOString();
	}
	LOString *tmp = v->GetStr();
	if(tmp) return LOString( *(v->GetStr()));
	else return LOString();
}

//返回的字符串不包含'*'
LOString LOScriptReader::ParseLabel(bool istry) {
	if (NextStartFrom('*')) {
		while(CurrentOP() == '*')NextAdress();
		//支持数字_，字母，全角文字混合，可能还有别的？
		LOString tmp;
		const char *buf = currentLable->c_buf;
		while (true) {
			//符合条件的
			if ((buf[0] >= '0' && buf[0] <= '9') || (buf[0] >= 'a' && buf[0] <= 'z') ||
				buf[0] >= 'A' && buf[0] <= 'Z' || buf[0] == '_') {
				tmp.append(buf, 1);
				buf++;
			}
			else break;
		}
		currentLable->c_buf = buf;
		return tmp;
	}
	else if (NextStartFrom('$') || NextStartFrom('"')) {
		LOString tmp = ParseStrVariable();
		int ulen = 0;
		while ( tmp[ulen] == '*') ulen++;
		return LOString(tmp.c_str() + ulen);
	}
	else if(!istry){
		FatalError("it's not a label or a string value!");
	}
	return LOString();
}

ONSVariableRef *LOScriptReader::ParseLabel2() {
	LOString tmp = ParseLabel(true);
	if (tmp.length() > 0) {
		ONSVariableRef *v = new ONSVariableRef();
		v->SetImVal(&tmp);
		return v;
	}
	else return NULL;
}


//获取别名是有文字和整数之分的
int LOScriptReader::GetAliasRef(LOString &s, bool isstr, int &out) {
	LOString ls = s.toLower();
	//整数别名只查找整数，文字别名先查找文字，再查找整数
	if (isstr) {
		auto iter = strAliasMap.find(ls);
		if (iter != strAliasMap.end()) {
			out = iter->second;
			return ALIAS_STR;
		}
	}
	auto iter = numAliasMap.find(ls);
	if (iter != numAliasMap.end()) {
		out = iter->second;
		return ALIAS_INT;
	}

	return 0;
}

int LOScriptReader::GetAliasRef(const char *&buf, bool isstr, int &out) {
	const char *obuf = buf;
	scriptbuf->SkipSpace(buf);
	LOString s = scriptbuf->GetWordStill(buf, LOCodePage::CHARACTER_LETTER | LOCodePage::CHARACTER_NUMBER);
	int ret = GetAliasRef(s, isstr, out);
	//失败了，重置buf位置
	if(ret == 0) buf = obuf;
	return ret;
}


const char* LOScriptReader::GetRPNstack2(std::vector<LOUniqVariableRef> *s2, const char *buf, bool isstrAlia) {
	std::vector<LOUniqVariableRef> s1;
	LOUniqVariableRef v;
	int curAllow, tint, curType, tdouble;
	LOString ts;

	bool isfirst = true;
	bool strAlia = isstrAlia;
	bool isOpAdd = false;
	s1.emplace_back(new ONSVariableRef());   //插入一个空对象
	curType = 0;

	while (true) {
		buf = scriptbuf->SkipSpace(buf);
		//根据之前的类型，获取下一个允许的类型
		curAllow = ONSVariableRef::GetYFnextAllow(curType);
        //表达式的起始，总是允许一个'('
        if(isfirst) curAllow |= ONSVariableRef::YF_Left_PA;
		//获取当前的语法类型
		curType = ONSVariableRef::GetYFtype(buf, isfirst);

		//尝试解释别名，别名总是可行的，唯一需要区分的只有文字别名还是整数别名
		if (curType == ONSVariableRef::YF_Alias) {
			//别名就相当于值，那么首先允许的类型要是值类型，否则无法继续下去
            if ((curAllow & ONSVariableRef::YF_Value) == 0) break ;
			//获取别名的值
			int ret = GetAliasRef(buf, strAlia, tint);
			if(ret == 0) curType = ONSVariableRef::YF_Error;   //别名获取失败，无法继续了
			else if (ret == ALIAS_INT) {
				v.reset(new ONSVariableRef());
				v->SetImVal((double)tint);
				curType = ONSVariableRef::YF_Int;
			}
			else {
				v.reset(new ONSVariableRef());
				v->SetImVal(&strAliasList[tint]);
				curType = ONSVariableRef::YF_Str;
			}
		}

		//表达式无法再继续了，不一定是错误
		if ((curType & curAllow) == 0 || curType == ONSVariableRef::YF_Error || curType == ONSVariableRef::YF_Break) break;

		//根据类型获取参数
		if(!v) v.reset(new ONSVariableRef());  //先生成一个，由后面处理

		if (curType & (ONSVariableRef::YF_Negative | ONSVariableRef::YF_Int)) {
			//整数处理
			if (v->isNone()) {
				if (curType == ONSVariableRef::YF_Negative) v->SetImVal(scriptbuf->GetRealNe(buf));
				else v->SetImVal(scriptbuf->GetReal(buf));
			}
			s2->push_back(std::move(v)); //已经经过别名解释
			//s2->push(v.release()); 
		}
		else if (curType & ONSVariableRef::YF_Str) {
			//文本处理
			if (v->isNone()) {
				ts = scriptbuf->GetString(buf);
				v->SetImVal(&ts);
			}
			s2->push_back(std::move(v));
		}
		else if (curType & (ONSVariableRef::YF_IntRef | ONSVariableRef::YF_StrRef | ONSVariableRef::YF_Array | ONSVariableRef::YF_Oper)) {
			//符号处理
			buf += v->SetOperator(buf);
			if (v->GetOperator() == '+') isOpAdd = true;
			//如果是数组，则压入一个特殊符号
			if (curType == ONSVariableRef::YF_Array) s2->emplace_back(new ONSVariableRef(ONSVariableRef::TYPE_ARRAY_FLAG, 0));
			auto iter = s1.end() - 1;
			if (v->GetOrder() > (*iter)->GetOrder()) s1.push_back(std::move(v));  //当前符号优先级高，则直接入临时，相当于在正式栈优先执行
			//%$是平级的，因此需要直接加入
			else if (v->GetOrder() == (*iter)->GetOrder() && v->GetOrder() == ONSVariableRef::ORDER_GETVAR) s1.push_back(std::move(v));
			else {
				//当前优先级低于临时栈的
				while (v->GetOrder() <= (*iter)->GetOrder() && (*iter)->GetOperator() != '(' &&
					(*iter)->GetOperator() != '[') {
					s2->push_back( std::move(*iter));
					s1.erase(iter--);
				}
				s1.push_back(std::move(v));
			}
		}
		else if (curType & ONSVariableRef::YF_Left_PA) { //'('
			buf += v->SetOperator(buf);
			s1.push_back(std::move(v));
		}
		else if (curType & ONSVariableRef::YF_Right_PA) {  //')'
			buf++;
            //auto iiit = TransformStack(&s1);
			PopRPNstackUtill(&s1, s2, '(');
		}
		else if (curType & ONSVariableRef::YF_Left_SQ) { //'['
			buf += v->SetOperator(buf);
			PopRPNstackUtill(&s1, s2, '?');
			s1.push_back(std::move(v));
		}
		else if (curType & ONSVariableRef::YF_Right_SQ) {//']'
			buf++;
			PopRPNstackUtill(&s1, s2, '[');
		}

		//左括号总是可以接受一个负数
		if (curType == ONSVariableRef::YF_Left_PA) isfirst = true;
		else isfirst = false;

		//文字别名之后出现在'+'后面，因此+号之后会重置文本别名类型
		if (isOpAdd) strAlia = isstrAlia;
		else strAlia = false; //有符号之后就只能是整数别名

		//已经push的v都应该为 nullptr ;
		v.reset();
		isOpAdd = false;
	}
    //行首括号、多层括号会出错
	PopRPNstackUtill(&s1, s2, '\0');
	if (s2->size() == 0 || !s2->back() || !s2->back()->isOperator())
		FatalError("Expression error");
	
	return buf;
}


//计算逆波兰式
void LOScriptReader::CalculatRPNstack(std::vector<LOUniqVariableRef> *stack) {
	auto iter = stack->begin();
	int docount;  //操作数的个数
	ONSVariableRef *op, *v1, *v2;
	op = v1 = v2 = nullptr;

	//if (currentLable->c_line == 47) {
	//	int bbq = 1;
	//}
//    while(iter != stack->end()){
//        op = *iter ;
//        op->isOperator() ? printf("%c ", op->GetOperator()) : printf("%d ",(int)op->GetReal()) ;
//        iter++ ;
//    }
//    iter = stack->begin();
//    printf("\n");
    //return ;

	while (iter != stack->end()) {
		op = (*iter).get();
		if (op->isOperator()) {
			//运算符位于首个符号是不可能的
			if (iter == stack->begin()) 
				FatalError("Expression error!");
			//该符号需要的操作数
			docount = op->GetOpCount();
			//检查操作数的数量是否还符合要求
			if (stack->size() < docount + 1) {
				FatalError("[ %c ] Insufficient operands required by operator!", op->GetOperator());
				return;
			}
			else if (docount == 1) {
				v1 = (*(iter - 1)).get();
				if (op->GetOperator() == '?') {
					//找到数组操作的起始标记
					while ( (*iter)->isArrayFlag() == false) iter--;
					iter = stack->erase(iter);   //删除数组标记
					v1 = (*iter).get();  //被操作的数组编号
					v1->DownRefToIm(ONSVariableRef::TYPE_REAL_IM);
					v1->UpImToRef(ONSVariableRef::TYPE_ARRAY_REF);
					v1->InitArrayIndex();
					iter++;
					for (int ii = 0; ii < MAXVARIABLE_ARRAY && !(*iter)->isOperator(); ii++) {
						v1->SetArrayIndex((int)(*iter)->GetReal(), ii);
						iter = stack->erase(iter);  //删除，同时指向下一个指针
					}
				}
				else if((*iter)->GetOperator() == '$') v1->UpImToRef(ONSVariableRef::TYPE_STR_REF);
				else if ((*iter)->GetOperator() == '%') v1->UpImToRef(ONSVariableRef::TYPE_INT_REF);
				else FatalError("%s","LOScriptReader::CalculatRPNstack() unknow docount = 1 type!");
				iter = stack->erase(iter);  //单操作数只删除符号
			}
			else{
				v1 = (*(iter - 2)).get();
				v2 = (*(iter - 1)).get();
				if(!v1->Calculator(v2, op->GetOperator() ,false )) break;
				iter--;  //指向开始删除的位置
				iter = stack->erase(iter); //删除后一个数
				iter = stack->erase(iter); //删除后一个符号
			}
		}
		else iter++;
	}

	//计算完成后堆栈只能剩一个元素
	if (stack->size() > 1) FatalError("Stack error after calculation, please check the formula!");
	return ;
}

void LOScriptReader::PopRPNstackUtill(std::vector<LOUniqVariableRef> *s1, std::vector<LOUniqVariableRef> *s2, char op) {
	auto iter = s1->end()-1;
	while (s1->size() > 1) {
		if ((*iter)->GetOperator() == op) {
			if (op != '?') {
				s1->erase(iter);   //抛弃'('  '['
			}
			return;
		}
		s2->push_back(std::move(*iter));
		s1->erase(iter--);  //符号转移到s2，不删除
	}
	if (op == '\0') {
		s1->clear();
		return;
	}
	FatalError("missing match '%c'", op);
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

//op应该是同一种类型，不能出现 +abc 或者abc+这种情况
bool LOScriptReader::NextStartFrom(const char* op) {
	const char *buf = currentLable->c_buf;
	buf = scriptbuf->SkipSpace(buf);
	while (op[0] != 0) {
		if (tolower(op[0]) != tolower(buf[0])) return false;
		op++;
		buf++;
	}
    int optype = scriptbuf->GetCharacter(op-1);
    //字母后面不能跟随字母
    if(optype == LOCodePage::CHARACTER_LETTER && scriptbuf->GetCharacter(buf) == LOCodePage::CHARACTER_LETTER) return false ;
    //符号后面不能跟随符号
    else if(optype == LOCodePage::CHARACTER_SYMBOL && scriptbuf->GetCharacter(buf) == LOCodePage::CHARACTER_SYMBOL){
        //'= < >'后面可以跟 ? % $ - "
        if(*(op-1) == '=' || *(op-1) == '<' || *(op-1) == '>'){
            if(!(buf[0] == '?' || buf[0] == '%' || buf[0] == '$' || buf[0] == '"' || buf[0] == '-')) return false ;
        }
        else return false ;
    }
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
		ONSVariableRef *v = new ONSVariableRef();
		LOString s = scriptbuf->substr(buf, 7);
		v->SetImVal(&s);
		currentLable->c_buf = buf + 7;
		return v;
	}
	else if (NextStartFrom('"') || NextStartFrom('$')) return ParseVariableBase(ALIAS_INT);
	else {
		NextNormalWord(false);
		auto iter = strAliasMap.find(word);
		if (iter == strAliasMap.end()) {
			ONSVariableRef *v = new ONSVariableRef();
			v->SetImVal(&word);
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

LOString LOScriptReader::GetLineFromCurrent() {
	const char *buf = currentLable->c_buf;
	const char *ebuf = buf;
	while (ebuf[0] != '\r' && ebuf[0] != '\n' && ebuf[0] != '\0') ebuf++;
	LOString s(buf, ebuf - buf);
	s.SetEncoder(scriptbuf->GetEncoder());
	return s;
}

void LOScriptReader::GotoLine(int lineID) {
	int lineID_t;
	const char *lineStart = nullptr;
	//currentLable->file->GetBufLine()
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
	subStack.pop_back();
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
	std::unique_ptr<ONSVariableRef> v1, v2;
	while (true) {
		v1.reset(ParseVariableBase(true));
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
			FatalError("Logical expression error!");
			return false;
		}

		v2.reset(ParseVariableBase(true));
		if (!v2) {
			FatalError("Logical expression error!");
			return false;
		}
		int ret0 = v1->Compare(v2.get(), comtype, false);
		if (nextconet == ONSVariableRef::LOGIC_AND) ret &= ret0;
		else if (nextconet == ONSVariableRef::LOGIC_OR) ret |= ret0;
		if (NextStartFrom("&&") || NextStartFrom("&")) nextconet = ONSVariableRef::LOGIC_AND;
		else if (NextStartFrom("||") || NextStartFrom("|")) nextconet = ONSVariableRef::LOGIC_OR;
		else break;
	}

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

			ONSVariableRef *v1 = ParseVariableBase(false);
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
		//LOIO::GetPathForRead(fn) ;
		bin = ReadFile(&fn, false);
		if (!bin && ii < 10) {
			fn = "0" + fn;
			bin = ReadFile(&fn, false);
		}

		if (bin) {
			LOScripFile::AddScript(bin->bin, bin->Length(), fn.c_str());
            SDL_Log("scripter[%s] has read.\n", fn.c_str());
			delete bin;
			isok = true;
		}
	}

	//然后考虑通常的脚本
	if (!isok) {
		fn = "nscript.dat";
		//LOIO::GetPathForRead(fn) ;
		//SDL_Log("script is:%s" , fn.c_str()) ;
		bin = ReadFile(&fn, false);
		if (bin) {
			unsigned char *buf = (unsigned char*)bin->bin;
			for (int ii = 0; ii < bin->Length() - 1; ii++) {
				buf[ii] ^= 0x84;
			}
            //FILE *f = LOIO::GetSaveHandle("00.txt", "wb") ;
            //if(f){
            //    fwrite(bin->bin, 1, bin->Length(), f);
            //    fflush(f);
            //    fclose(f);
            //}
			LOScripFile::AddScript(bin->bin, bin->Length(), fn.c_str());
            SDL_Log("scripter[%s] has read.\n", fn.c_str());
			delete bin;
			isok = true;
		}
	}

	//加入内置系统脚本
	char *tmp = (char*)(intptr_t)(__buil_in_script__);
	LOScripFile::AddScript(tmp, strlen(__buil_in_script__), "Buil_in_script.h");
	return isok;
}

void LOScriptReader::GetGameInit(int &w, int &h) {
	w = 640; h = 480;
	//if (filesList.size() == 0) return;

	gloableBorder = 200;

	LOScriptPoint *p = LOScriptPointCall::GetScriptPoint("__init__");
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
					gloableBorder = scriptbuf->GetInt(buf);
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

bool LOScriptReader::InserCommand() {
	std::unique_ptr<ONSVariableRef> v(ParseVariableBase(true));
	if (!v || !v->isStr()) return false;
	LOString *s = v->GetStr();
	//不执行任何操作
	if (s->length() <= 0) return true;
	InserCommand(s);
	return true;
}

void LOScriptReader::InserCommand(LOString *incmd) {
	if (!incmd || incmd->length() == 0) return;
	//进入eval后不能使用goto gosub saveon saveoff savepoint命令
	ReadyToRunEval(incmd);
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
	subStack.clear();

	normalLogic.reset();
	loopStack.clear();

	//清理def设置
	defsubMap.clear();

	//清理别名
	numAliasMap.clear();
	strAliasList.clear();
	strAliasMap.clear();
	ResetBaseConfig();
}


//这个函数只能从主脚本调用
void LOScriptReader::LoadReset() {
	if (this != scriptModule) {
        SDL_LogError(0, "LOScriptReader::LoadReset() must call by main script reader!");
		return;
	}

	//移除其他所有脚本解析
	while (nextReader) {
		LeaveScriptReader(nextReader);
	}
	//清空call堆栈
	while (!isEndSub()) ReadyToBack();
	//清除运行状态
	subStack.clear();
	normalLogic.reset();
	loopStack.clear();
	nextReader = nullptr;
	//清除所有事件
	waitEventQue.invalidClear();
	//重置变量的值


	//不清除各种 define设置
}

//获取自身报告
LOString LOScriptReader::GetReport() {
    LOString report = msg_script_thread + Name + "\n";
	if (currentLable) {
        report.append(msg_script_file);
		report.append(currentLable->file()->Name.c_str());
		report.append("\n");
		report += "line: " + std::to_string(currentLable->c_line) + "\n--> ";
		//获取一行
		int len = 0;
		const char *current = currentLable->c_buf;
		BackLineStart();
		while (currentLable->c_buf[len] != 0 && currentLable->c_buf[len] != '\n') len++;
		//注意，提供给SDL的必须都是utf8编码，不然在某些平台上可能会导致crash，比如安卓
		LOString tstr(currentLable->c_buf, len);
        tstr.SetEncoder(scriptbuf->GetEncoder()) ;
		tstr.SelfToUtf8();
		report.append(tstr.c_str(), tstr.length());
		currentLable->c_buf = current;
		report.append("\n");
	}
    //这里是错误的，应该是utf8编码
	if(scriptbuf) report.SetEncoder(scriptbuf->GetEncoder());
	return report;
}


//致命错误，弹出对话框，程序退出，生成error.txt
void FatalError(const char *fmt, ...) {
	int pos = 0;
	std::unique_ptr<BinArray> bin(new BinArray(1048));

	va_list argptr;
	va_start(argptr, fmt);
	pos = vsnprintf(bin->bin + pos, 1048, fmt, argptr);
	va_end(argptr);
	bin->SetLength(pos);
	bin->Append("\n", 1);

	LOString errfn("error.txt");
	FILE *f = LOIO::GetWriteHandle(errfn, "wb");
	if (f) fwrite(bin->bin, 1, bin->Length(), f);

	//添加最多5行的错误信息
	if (FunctionInterface::scriptModule) {
		LOString temp = ((LOScriptReader*)FunctionInterface::scriptModule)->GetReport();
		const char* ebuf = temp.GetLineBuf(6);
		if (!ebuf) ebuf = temp.e_buf();
		bin->Append(temp.c_str(), ebuf - temp.c_str());
		bin->Append("\n......\n", 8);
        bin->Append(msg_more_errorinfo, strlen(msg_more_errorinfo));

		if (f) fprintf(f, "script:\n%s\n", temp.c_str());
	}

	//显示错误信息
    SDL_LogError(0, "%s", bin->bin) ;
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, msg_error_title, bin->bin, nullptr);
	//生成错误报告
	if (f) {
		fprintf(f, "LONS version:%s\n",ONS_VERSION);
		fclose(f);
	}
	//设置错误标记，以便程序退出
	FunctionInterface::errorFlag = true;
	if (FunctionInterface::audioModule) FunctionInterface::audioModule->ResetMe();
}


//一些关键性的警告信息
void SimpleWarning(const char *fmt, ...) {
    int pos = 0;
    std::unique_ptr<BinArray> bin(new BinArray(1048));

    va_list argptr;
    va_start(argptr, fmt);
    pos = vsnprintf(bin->bin + pos, 1048, fmt, argptr);
    va_end(argptr);
    bin->SetLength(pos);
    bin->Append("\n", 1);

    SDL_Log("%s", bin->bin);
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, msg_warning_title, bin->bin, nullptr);
}



//一些事件的处理
int LOScriptReader::RunFunc(LOEventHook *hook, LOEventHook *e) {
	return LOEventHook::RUNFUNC_CONTINUE;
}


//有不少事件是需要在渲染线程响应后，再到脚本线程继续处理（为了线程安全）
int LOScriptReader::RunEventAfterFinish(LOEventHook *e) {
	if (e->catchFlag & LOEventHook::ANSWER_BTNCLICK) RunFuncBtnSetVal(e);
	else if (e->catchFlag & LOEventHook::ANSWER_PRINGJMP) {
		//响应此事件的有文字和print
		if (e->param2 == LOEventHook::FUN_TEXT_ACTION) {
			//printf("***********text funish***********\n");
            //SDL_Log("text finish get!");
			RunFuncSayFinish(e);
		}
		else;
	}
	//视频播放或者调过的清理过程,由脚本线程调用渲染模块（因为线程安全）
	else if (e->catchFlag & LOEventHook::ANSWER_VIDEOFINISH) {
		e->param2 = LOEventHook::FUN_Video_Finish_After;
		imgeModule->RunFunc(e, nullptr);
	}
	else if (e->catchFlag & LOEventHook::ANSWER_SEPLAYOVER_NORMAL) { //loopbgm
		audioModule->RunFunc(nullptr, e);
	}
	return 0;
}


//变量赋值事件，这个函数应该从脚本线程执行
int LOScriptReader::RunFuncBtnSetVal(LOEventHook *hook) {
	//跟btnwaithook的原始参数个数有关
	int fix = 5;
	int refid = hook->GetParam(LOEventHook::PINDS_REFID)->GetInt();
	LOUniqVariableRef ref(ONSVariableRef::GetRefFromTypeRefid(refid));
	LOVariant *var = hook->GetParam(LOEventHook::PINDS_BTNVAL + fix);
	if (var->IsType(LOVariant::TYPE_INT)) {
		//LOLog_i("btnwait is:%d", var->GetInt());
		ref->SetValue((double)var->GetInt());
		//printf("btn set time:%d, var is %d\n", SDL_GetTicks(), var->GetInt());
	}
	else {
		//似乎有字符串版本的命令？
	}
	//切除多余的参数
	hook->paramListCut(fix);
	hook->InvalidMe();
	return 0;
}


//文字显示完成，进入textgosub
int LOScriptReader::RunFuncSayFinish(LOEventHook *hook) {
	// '\''@'才进入等待，'/'不进入等待
	int pageEnd = 0;
	imgeModule->GetModValue(MODVALUE_PAGEEND, &pageEnd);
	if((pageEnd&0xff) != '/') ReadyToRun(&userGoSubName[USERGOSUB_TEXT], LOScriptPoint::CALL_BY_TEXT_GOSUB);

	hook->InvalidMe();
    //关闭rubyline模式
    imgeModule->SimpleEvent(SIMPLE_CLOSE_RUBYLINE, nullptr) ;
	//如果是saveon模式，则添加存档点
	//autosaveoff_flag有效时，saveon/saveoff会失效
	if(st_saveonflag || st_autosaveoff_flag) savepointCommand(this);
	return 0;
}

void LOScriptReader::GetModValue(int vtype, void *val){
    switch (vtype) {
    case MODVALUE_AUTOMODE:
        break;
    }
}


void LOScriptReader::Serialize(BinArray *bin) {
	int len = bin->WriteLpksEntity("scri", 0, 1);
	//
	bin->WriteInt(lastCmdCheckFlag);
	bin->WriteInt((int)sctype);
	//脚本名称
	bin->WriteLOString(&Name);
	//脚本print名称
	bin->WriteLOString(&printName);
	//tagstring
	bin->WriteLOString(&TagString);
	//当前的命令
	bin->WriteLOString(&curCmd);
	//上一个命令
	bin->WriteLOString(&lastCmd);
	//parse嵌套
	bin->WriteInt(parseDeep);
	//高精度计时器，存储的时候不需要那个高的精度，只保留整数即可
	//存储的是经过的毫秒数
	int timeDiff = (int)LOTimer::GetHighTimeDiff(ttimer);
	bin->WriteInt(timeDiff);
	//nextReader不需要
	//当前的运行点在堆栈中的序号
	bin->WriteInt(GetCurrentLableIndex());
	//运行点堆栈
	bin->WriteInt(subStack.size());
	for (int ii = 0; ii < subStack.size(); ii++) subStack.at(ii).Serialize(bin);
	//逻辑堆栈
	bin->WriteInt(loopStack.size());
	for (int ii = 0; ii < loopStack.size(); ii++)loopStack.at(ii)->Serialize(bin);
	//普通的normalLogic只需要存储结果
	bin->WriteChar(normalLogic.isRetTrue());
	//是否有下一个脚本
	if (nextReader) bin->WriteChar(1);
	else bin->WriteChar(0);
	//预留
	bin->WriteInt3(0, 0, 0);
	bin->WriteInt3(0, 0, 0);
	bin->WriteString(nullptr);
	bin->WriteString(nullptr);
	bin->WriteInt(bin->Length() - len, &len);

	//如果链表继续延长
	if(nextReader) nextReader->Serialize(bin);
}


bool LOScriptReader::DeSerialize(BinArray *bin, int *pos, LOEventMap *evmap) {
	int next = -1;
	if (!bin->CheckEntity("scri", &next, nullptr, pos)) {
        SDL_LogError(0, "LOScriptReader::DeSerialize() faild! it's not scripter stream!");
		return false;
	}
	lastCmdCheckFlag = bin->GetIntAuto(pos);
	sctype = (SCRIPT_TYPE)bin->GetIntAuto(pos);

	Name = bin->GetLOString(pos);
	printName = bin->GetLOString(pos);
	TagString = bin->GetLOString(pos);
	curCmd = bin->GetLOString(pos);
	lastCmd = bin->GetLOString(pos);
	parseDeep = bin->GetIntAuto(pos);
	//从时间差回推
	int timeDiff = bin->GetIntAuto(pos);
	ttimer = LOTimer::GetHighTimer();
	ttimer -= LOTimer::perTik64 * 1000 * timeDiff;

	//运行点堆栈
	int index = bin->GetIntAuto(pos);
	int count = bin->GetIntAuto(pos);
	for (int ii = 0; ii < count; ii++) {
		if (!ScCallDeSerialize(bin, pos)) {
            SDL_LogError(0, "LOScriptPointCall DeSerialize faild!");
			return false;
		}
	}
	currentLable = &subStack.at(subStack.size() - index - 1);
	//逻辑堆栈
	count = bin->GetIntAuto(pos);
	for (int ii = 0; ii < count; ii++) {
		if (!LogicCallDeSerialize(bin, pos)) {
            SDL_LogError(0, "LogicPointCall DeSerialize faild!");
			return false;
		}
	}

	//普通的normalLogic，只需要设置值
	normalLogic.SetRet(bin->GetChar(pos));
	char hasnext = bin->GetChar(pos);
	//预留
	*pos = next;
	//继续反序列化下一个脚本
	if (hasnext) {
		nextReader = new LOScriptReader();
		return nextReader->DeSerialize(bin, pos, evmap);
	}
	return true;
}



bool LOScriptReader::ScCallDeSerialize(BinArray *bin, int *pos) {
	int next = -1;
	if (!bin->CheckEntity("poin", &next, nullptr, pos)) return false;
	//ons对在哪个脚本不敏感
	bin->GetLOString(pos);
	//
	LOString labelName = bin->GetLOString(pos);
	LOScriptPoint *p = LOScriptPointCall::GetScriptPoint(labelName);
	if (!p) return false;

	subStack.emplace_back(p);
	LOScriptPointCall *point = &subStack.at(subStack.size() - 1);

	point->c_line = point->s_line + bin->GetIntAuto(pos);
	auto data = point->file()->GetLineInfo(nullptr, point->c_line, true);
	if (!data.buf) {
        SDL_LogError(0, "can't find scripter point line:%d", point->c_line);
		return false;
	}
	point->c_buf = data.buf + bin->GetIntAuto(pos);
	point->callType = bin->GetIntAuto(pos);
	//校验一下
	int ihash = bin->GetIntAuto(pos);
    if (ihash != *(int*)point->c_buf) SDL_Log("scripter point [%s] mamy be error!", labelName.c_str());

	*pos = next;
	return true;
}


bool LOScriptReader::LogicCallDeSerialize(BinArray *bin, int *pos) {
	int next = -1;
	if (!bin->CheckEntity("lpos", &next, nullptr, pos)) return false;

	int flags = bin->GetIntAuto(pos);
	LogicPointer *logic = new LogicPointer(flags);
	loopStack.push(logic);

	LOString labelName = bin->GetLOString(pos);
	LOScriptPoint *p = LOScriptPointCall::GetScriptPoint(labelName);
	if (!p) return false;

	if (!logic->LoadSetPoint(p, bin->GetIntAuto(pos), bin->GetIntAuto(pos))) {
        SDL_LogError(0, "logic scripter point error");
		return false;
	}

	logic->step = bin->GetIntAuto(pos);
	int refID = bin->GetIntAuto(pos);
	if (refID) logic->forVar = ONSVariableRef::GetRefFromTypeRefid(refID);
	refID = bin->GetIntAuto(pos);
	if(refID) logic->dstVar = ONSVariableRef::GetRefFromTypeRefid(refID);

	*pos = next;
	return true;
}
