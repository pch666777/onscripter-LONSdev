/*
//基本的命令实现
*/
#include "LOScriptReader.h"
#include "../etc/LOIO.h"
#ifdef WIN32
#include <Windows.h>
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846264338327950288   /**< pi */
#endif // !M_PI

extern void LonsSaveLayer(BinArray *bin);
extern void LonsSaveImageModule(BinArray *bin);

int LOScriptReader::dateCommand(FunctionInterface *reader) {
	auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	struct tm *ptm = localtime(&tt);
	ONSVariableRef *v1 = GetParamRef(0);
	ONSVariableRef *v2 = GetParamRef(1);
	ONSVariableRef *v3 = GetParamRef(2);
	if (isName("date")) {
		int val = (ptm->tm_year + 1900) % 100;
		v1->SetAutoVal((double)val);
		v2->SetAutoVal((double)(ptm->tm_mon + 1));
		v3->SetAutoVal((double)(ptm->tm_mday));
	}
	else {
		v1->SetAutoVal((double)(ptm->tm_hour));
		v2->SetAutoVal((double)(ptm->tm_min));
		v3->SetAutoVal((double)(ptm->tm_sec));
	}
	return RET_CONTINUE;
}

int LOScriptReader::getenvCommand(FunctionInterface *reader) {
	ONSVariableRef *v = reader->GetParamRef(0);
	LOString s = "LONS-S" + std::to_string(SCRIPT_VERSION);
	v->SetValue(&s);
	if (reader->GetParamCount() > 1) {

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
		s.assign("windows");
#elif __ANDROID__
		s.assign("android");
#elif __linux__
		s.assign("linux");
#else
		s.assign("unkonw_system");
#endif

		ONSVariableRef *v = reader->GetParamRef(1);
		v->SetValue(&s);
	}
	return RET_CONTINUE;
}

int LOScriptReader::getversionCommand(FunctionInterface *reader) {
	ONSVariableRef *v = GetParamRef(0);
	v->SetAutoVal(296.0);  //以这个版本为模板
	return RET_CONTINUE;
}

int LOScriptReader::splitCommand(FunctionInterface *reader) {
	LOString s = reader->GetParamStr(0);
	LOString tag = reader->GetParamStr(1);
	std::vector<LOString> list = s.Split(tag);
	for (int ii = 0; ii < reader->GetParamCount() - 2; ii++) {
		ONSVariableRef *v = GetParamRef(ii + 2);
		if (ii < list.size()) v->SetAutoVal( &list.at(ii) );
		else v->SetAutoVal((LOString*)NULL);
	}
	list.clear();
	return RET_CONTINUE;
}

//int LOScriptReader::readfileCommand(FunctionInterface *reader) {
//	ONSVariableRef *v = reader->GetParamRef(0);
//	LOString s = reader->GetParamStr(1);
//
//	int len;
//	char *buf = ReadFileOnce(s.c_str(), &len);
//	LOString res;
//	if (buf) {
//		res.assign(buf, len);
//		delete[] buf;
//	}
//
//	v->SetValue(&res);
//	return RET_CONTINUE;
//}

int LOScriptReader::fileremoveCommand(FunctionInterface *reader) {
	LOString s = reader->GetParamStr(0);
	FileRemove(s.c_str());
	return RET_CONTINUE;
}

//int LOScriptReader::fileexistCommand(FunctionInterface *reader) {
//	ONSVariableRef *v = reader->GetParamRef(0);
//	LOString s = reader->GetParamStr(1);
//	FILE *f = OpenFile(s.c_str(), "rb");
//	if (f) {
//		fclose(f);
//		v->SetValue(1.0);
//	}
//	else v->SetValue(0.0);
//	return RET_CONTINUE;
//}


int LOScriptReader::labelexistCommand(FunctionInterface *reader) {
	ONSVariableRef *v = reader->GetParamRef(0);
	LOScriptPoint  *p = reader->GetParamLable(1);
	if (p) v->SetValue(1.0);
	else v->SetValue(0.0);
	return RET_CONTINUE;
}

int LOScriptReader::getparamCommand(FunctionInterface *reader) {
	//转移运行点到之前的位置
	if (!ChangePointer(1)) {
		LOLog_e("You can not use [getparam] command in non child functions");
		return RET_ERROR;
	}
	int count = reader->GetParamCount();
	for (int ii = 0; ii < count; ii++) {
		ONSVariableRef *v1 = reader->GetParamRef(ii);
		ONSVariableRef *v2 = ParseVariableBase();
		v1->SetValue(v2);
		delete v2;
		if (!NextComma(true)) break;
	}
	//返回原运行点
	ChangePointer(0);
	return RET_CONTINUE;
}

int LOScriptReader::defsubCommand(FunctionInterface *reader) {
	LOString s = reader->GetParamStr(0).toLower();
	LOScriptPoint *p = GetScriptPoint(s);
	defsubMap[s] = p;
	return RET_CONTINUE;
}

int LOScriptReader::endCommand(FunctionInterface *reader) {
	//scriptModule->moduleState = MODULE_STATE_EXIT;
	//audioModule->ResetMe();
	//imgeModule->moduleState = MODULE_STATE_EXIT;
	return RET_RETURN;
}

int LOScriptReader::gameCommand(FunctionInterface *reader) {
	return ReadyToBack();
}

int LOScriptReader::delayCommand(FunctionInterface *reader) {
	int val = reader->GetParamInt(0);
	if (val > 0) {
		//创建一个时间阻塞事件
		//除waittimer外，delay和wait均可以左键跳过
		LOShareEventHook ev(LOEventHook::CreateTimerHook(val, !reader->isName("waittimer")));
		G_hookQue.push_back(ev, LOEventQue::LEVEL_NORMAL);
		reader->waitEventQue.push_back(ev, LOEventQue::LEVEL_NORMAL);
	}
	return RET_CONTINUE;
}

int LOScriptReader::gettimerCommand(FunctionInterface *reader) {
	//STEADY_CLOCK::time_point t1 = STEADY_CLOCK::now();
	//auto it = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - ttimer);
	//int val = it.count() & 0xfffffff;
	Uint64 pos = SDL_GetPerformanceCounter() - ttimer;
	Uint64 perHtickTime = SDL_GetPerformanceFrequency() / 1000;
	double val = (double)pos / perHtickTime;
	ONSVariableRef *v = GetParamRef(0);
	v->SetValue((double)val);
	return RET_CONTINUE;
}

int LOScriptReader::resettimerCommand(FunctionInterface *reader) {
	//ttimer = STEADY_CLOCK::now();
	ttimer = SDL_GetPerformanceCounter();
	return RET_CONTINUE;
}

int LOScriptReader::endifCommand(FunctionInterface *reader) {
	std::unique_ptr<LogicPointer> p(loopStack.pop());
	if (!p || !p->isIFthen()) {
		SimpleError("if...then...else....endif Mismatch!");
		return RET_ERROR;
	}
	return RET_CONTINUE;
}

int LOScriptReader::elseCommand(FunctionInterface *reader) {
	if (NextStartFrom('\n')) { //标准ns else 会接命令
		LogicPointer *p = loopStack.top();
		if (!p || !p->isIFthen()) {
			LOLog_e("if...then...else ....endif Mismatch!");
			return RET_ERROR;
		}
		//else 要执行假的部分，所以返回继续执行
		if (!p->isRetTrue()) return RET_CONTINUE;
		//逻辑跳过
		int jump = LogicJump(false);
		if (jump < 0) {
			SimpleError("if...then...else...endif Mismatch!");
			return RET_ERROR;
		}
		loopStack.pop(true);
		return RET_CONTINUE;
	}
	else {
		if (normalLogic.isRetTrue()) NextLineStart();  //条件成立时不执行
		return RET_CONTINUE;
	}
	return RET_CONTINUE;
}

int LOScriptReader::ifCommand(FunctionInterface *reader) {
	//if (currentLable->c_line == 352) {
	//	int debugbreak = 1;
	//}

	bool ret = ParseLogicExp();
	if (isName("notif")) ret = !ret;
	if (NextStartFrom("then")) {
		LogicPointer *p = loopStack.push(new LogicPointer(LogicPointer::TYPE_IFTHEN));
		p->SetRet(ret);
		//条件成立继续执行，直到else或者endif
		if (ret) return RET_CONTINUE;  
		//不成立跳到else 或者endif
		int jump = LogicJump(true);
		if (jump < 0) {
			SimpleError("if...then  else then ....endif Mismatch!");
			return RET_ERROR;
		}
		if (jump == 1) return elseCommand(reader); //有else执行else
		 //没有直接释放
		loopStack.pop(true);
		return RET_CONTINUE;
	}
	else {  //
		normalLogic.SetRet(ret);
		//if条件不成立则下一行
		if (!normalLogic.isRetTrue()) NextLineStart();  
		return RET_CONTINUE;
	}
}

int LOScriptReader::breakCommand(FunctionInterface *reader) {
	std::unique_ptr<LogicPointer> p(loopStack.pop());
	if (!p || (!p->isFor() && !p->isWhile()) ) {
		SimpleError("[break] Not used in the [for] loop!\n");
		return RET_ERROR;
	}
	JumpNext();
	return RET_CONTINUE;
}

int LOScriptReader::nextCommand(FunctionInterface *reader) {
	LogicPointer *p = loopStack.top();
	if (!p || !p->isFor()) {
		LOLog_e("[next] and [for] command not match!");
		return RET_ERROR;
	}
	//递增变量
	p->forVar->SetValue(p->forVar->GetReal() + p->step);

	//比较条件
	int comparetype = ONSVariableRef::LOGIC_LESSANDEQUAL;
	if (p->step < 0) comparetype = ONSVariableRef::LOGIC_BIGANDEQUAL;

	if (p->forVar->Compare(p->dstVar, comparetype,false)) {
		currentLable->c_buf = p->point;
		currentLable->c_line = p->pointLine;
	}
	else {
		loopStack.pop(true);
	}
	return RET_CONTINUE;
}

int LOScriptReader::forCommand(FunctionInterface *reader) {
	LogicPointer *p = loopStack.push(new LogicPointer(LogicPointer::TYPE_FOR));

	p->forVar = ParseVariableBase(); //%

	if (!NextStartFrom('=')) {
		SimpleError("[for] command Missing '='");
		return RET_ERROR;
	}

	NextAdress();
	p->forVar->SetValue((double)ParseIntVariable()); // = number
	if (!NextStartFrom("to")) {
		SimpleError("[for] command Missing 'to'!");
		return RET_ERROR;
	}

	p->dstVar = ParseVariableBase();  //dest
	if (!p->dstVar->isReal()) {
		SimpleError("[for] command the target must be an integer!");
		return RET_ERROR;
	}
	if (NextStartFrom("step")) p->step = ParseIntVariable(); //step
	//step 为负数则大于等于目标，正数则小于等于
	int comparetype = ONSVariableRef::LOGIC_LESSANDEQUAL;
	if (p->step < 0) comparetype = ONSVariableRef::LOGIC_BIGANDEQUAL;
	if (p->forVar->Compare(p->dstVar, comparetype,false)) {
		p->point = currentLable->c_buf;
		p->pointLine = currentLable->c_line;
		p->label = currentLable;
	}
	else { //跳过不执行的部分
		loopStack.pop(true);
		JumpNext();
	}
	
	return RET_CONTINUE;
}

void LOScriptReader::JumpNext() {
	int deep = 0;
	while (!NextStartFrom('\0')) {
		NextLineStart();
		if (NextStartFrom("next")) { //找到跟for匹配的next则跳出循环
			if (deep == 0) {
				NextLineStart();
				break;
			}
			else deep--;
		}
		if (NextStartFrom("for")) deep++;
	}
}

int LOScriptReader::tablegotoCommand(FunctionInterface *reader) {
	int val = reader->GetParamInt(0);
	if (val < 0 || val > reader->GetParamCount() - 2) {
		SimpleError("tablegoto is a error value:%d\n", val);
		return RET_ERROR;
	}
	else {
		LOScriptPoint *p = reader->GetParamLable(val + 1);
		gosubCore(p, true);
		return RET_CONTINUE;
	}
}

//skip 1，next line
int LOScriptReader::skipCommand(FunctionInterface *reader) {
	int count = reader->GetParamInt(0);
	if (count == 0) count = 1;
	while (count > 0) {
		NextLineStart();
		count--;
	}
	while (count < 0) {
		BackLineStart();
		currentLable->c_buf--; // \n  now
		currentLable->c_buf--; // befor -n
		currentLable->c_line--;
		BackLineStart();
		count++;
	}
	return RET_CONTINUE;
}

int LOScriptReader::returnCommand(FunctionInterface *reader) {

	LOScriptPoint *p = NULL;
	if (reader->GetParamCount() > 0) p = reader->GetParamLable(0);
	int ret = ReadyToBack();
	if (p) {
		gosubCore(p, true);
		return ret;
	}
	else return ret;
}

int LOScriptReader::jumpCommand(FunctionInterface *reader) {
	const char *buf = currentLable->c_buf;
	const char *sbuf = scriptbuf->c_str();
	const char *ebuf = scriptbuf->e_buf();
	int cline = currentLable->c_line;
	bool isnext = isName("jumpf");

	while (buf > sbuf && buf < ebuf) {
		if (buf[0] == '~') {
			currentLable->c_buf = buf;
			currentLable->c_line = cline;
			return RET_CONTINUE;
		}
		else if (buf[0] == '\n') {
			if (isnext) cline++;
			else cline--;
		}

		if (isnext) buf += scriptbuf->ThisCharLen(buf);
		else buf -= scriptbuf->ThisCharLen(buf);
	}
	SimpleError("jumpb faild! not find '~' Symbol!\n");
	return RET_ERROR;
}

int LOScriptReader::gotoCommand(FunctionInterface *reader) {
	//if (currentLable->c_line == 63) {
	//	int debuglog = 0;
	//}

	LOScriptPoint *p = reader->GetParamLable(0);
	if (p) {
		gosubCore(p, isName("goto"));
		return RET_CONTINUE;
	}
	else {
		SimpleError("not find label:\"%s\"\n", reader->GetParamStr(0).c_str());
	}
	return RET_ERROR;
}

int LOScriptReader::gosubCore(LOScriptPoint *p, bool isgoto) {
	if (isgoto) ChangePointer(p);
	else ReadyToRun(p);
	return RET_CONTINUE;
}

int LOScriptReader::straliasCommand(FunctionInterface *reader) {
	LOString s = reader->GetParamStr(0);
	LOString val = reader->GetParamStr(1);
	strAliasList.push_back(val);
	strAliasMap[s] = strAliasList.size() - 1;
	return RET_CONTINUE;
}

int LOScriptReader::numaliasCommand(FunctionInterface *reader) {
	LOString s = reader->GetParamStr(0);
	int val = reader->GetParamInt(1);
	numAliasMap[s] = val;
	return RET_CONTINUE;
}

int LOScriptReader::debuglogCommand(FunctionInterface *reader) {
	//LOString errs = "at file:[ " + reader->GetCurrentFile() + " ], line:[ " + std::to_string(reader->currentLable->current_line) + " ], debug:";
	LOString errs;
	for (int ii = 0; ii < reader->GetParamCount(); ii++) {
		ONSVariableRef *v = GetParamRef(ii);
		if (v->vtype == ONSVariableRef::TYPE_INT) {
			errs.append("   %%" + std::to_string(v->nsvId) + "=" + std::to_string((int)v->GetReal()));
		}
		else if (v->vtype == ONSVariableRef::TYPE_STRING) {
			errs.append("   $" + std::to_string(v->nsvId) + "=\"");
			LOString *s = v->GetStr();
			if (s) errs.append(*s);
			errs.append("\"");
		}
		else if (v->vtype == ONSVariableRef::TYPE_ARRAY) {
			errs.append("   ?" + std::to_string(v->nsvId));
			for (int ii = 0; ii < MAXVARIABLE_ARRAY && v->GetArrayIndex(ii) >= 0; ii++) {
				errs.append("[" + std::to_string(v->GetArrayIndex(ii)) + "]");
			}
			errs.append("=" + std::to_string((int)v->GetReal()));
		}
		else if (v->vtype == ONSVariableRef::TYPE_REAL) {
			errs.append("   number=" + std::to_string((int)v->GetReal()));
		}
		else if (v->vtype == ONSVariableRef::TYPE_STRING_IM) {
			LOString *s = v->GetStr();
			if (s) errs.append("   \"" + *s + "\"");
			else errs.append("\"\"");
		}
		else {
			SimpleError("at file:[ %s ],line:[ %d ],What information do you want to debug output?",
				GetCurrentFile().c_str(), currentLable->c_line);
		}
	}
	errs.append("\n");
	LOLog_i(errs.c_str());
	//SDL_Log(errs.c_str());
	return RET_CONTINUE;
}

int LOScriptReader::midCommand(FunctionInterface *reader) {
	ONSVariableRef *v1 = reader->GetParamRef(0);
	LOString tag = reader->GetParamStr(1);
	int startpos = reader->GetParamInt(2);
	int len = reader->GetParamInt(3);

	LOString s;
	if (reader->isName("midword")) s = tag.substrWord(startpos, len);
	else s = tag.substr(startpos, len);

	v1->SetAutoVal(&s);
	return RET_CONTINUE;
}

int LOScriptReader::itoaCommand(FunctionInterface *reader) {
	ONSVariableRef *v = reader->GetParamRef(0);
	int val = reader->GetParamInt(1);
	if (isName("itoa2")) {
		LOString s = scriptbuf->Itoa2(val);
		v->SetValue(&s);
	}
	else v->SetAutoVal((double)val);
	return RET_CONTINUE;
}

int LOScriptReader::lenCommand(FunctionInterface *reader) {
	ONSVariableRef *v1 = reader->GetParamRef(0);
	LOString s = reader->GetParamStr(1);

	int len = 0;
	if (isName("lenword")) len = s.WordLength();
	else len = s.length();
	v1->SetValue(len);
	return RET_CONTINUE;
}

int LOScriptReader::movCommand(FunctionInterface *reader) {
	while (true) {
		ONSVariableRef *v1 = ParseVariableBase();
		if (!NextComma()) {
			SimpleError("[%s] command not match!\n", curCmd.c_str());
			return RET_ERROR;
		}
		ONSVariableRef *v2 = ParseVariableBase(NULL,v1->isStrRef());
		if (v1 && v1->isRef() && v2) v1->SetValue(v2);
		if (v1) delete v1;
		if (v2) delete v2;
		if (!NextComma(true)) break;
	}
	return RET_CONTINUE;
}

int LOScriptReader::movlCommand(FunctionInterface *reader) {
	ONSVariableRef *v = reader->GetParamRef(0);
	int vcount = reader->GetParamCount() - 1;
	int *vals = new int[vcount];
	for (int ii = 0; ii < vcount; ii++) vals[ii] = reader->GetParamInt(ii + 1);
	v->SetArryVals( vals, vcount);
	delete[] vals;
	return RET_CONTINUE;
}

int LOScriptReader::operationCommand(FunctionInterface *reader) {
	int ret2, ret = 0;
	ONSVariableRef *v1 = reader->GetParamRef(0);

	if (!isName("inc") && !isName("dec")) ret = reader->GetParamInt(1);
	if (isName("rnd2")) ret2 = reader->GetParamInt(2);

	const char* buf = curCmd.c_str();
	uint32_t optype = 0;
	for (int ii = 0; ii < 4; ii++) optype = optype * 256 + (unsigned char)(buf[ii]);

	int val = v1->GetReal();
	switch (optype)
	{
	case 0x61646400: //add,not here,at addCommand()
		break;
	case 0x636F7300: //cos
		val = cos(M_PI*ret / 180.0)*1000.0;
		break;
	case 0x64656300: //dec
		val = val - 1;
		break;
	case 0x64697600: //div
		if (ret == 0) {
			SimpleError("[div] command,the divisor is zero!");
			return RET_ERROR;
		}
		val = val / ret;
		break;
	case 0x696E6300: //inc
		val = val + 1;
		break;
	case 0x6D756C00: //mul
		val = val * ret;
		break;
	case 0x726E6400: //rnd
		val = rand() % ret;
		break;
	case 0x726E6432: //rnd2
		if (ret2 <= ret) {
			SimpleError("[rnd2] command faild! second number is less than first number!");
			return RET_ERROR;
		}
		val = rand() % (ret2 - ret + 1) + ret;
		break;
	case 0x73696E00: //sin
		val = sin(M_PI*ret / 180.0)*1000.0;
		break;
	case 0x73756200: //sub
		val = val - ret;
		break;
	case 0x74616E00:  //tan
		val = tan(M_PI*ret / 180.0)*1000.0;
		break;
	default:
		val = 0;
		break;
	}

	v1->SetValue((double)val);
	return RET_CONTINUE;
}

int LOScriptReader::addCommand(FunctionInterface *reader) {
	ONSVariableRef *v1 = reader->GetParamRef(0);
	ONSVariableRef *v2 = reader->GetParamRef(1);
	ONSVariableRef *v3 = new ONSVariableRef(v1->vtype, v1->nsvId);

	//Calculator会改变ref的类型
	v1->Calculator(v2, '+', false);
	v3->SetValue(v1);
	delete v3;
	return RET_CONTINUE;
}

int LOScriptReader::intlimitCommand(FunctionInterface *reader) {
	int no = reader->GetParamInt(0);
	int min = reader->GetParamInt(1);
	int max = reader->GetParamInt(2);
	if (min >= max) SimpleError("[intlimit] command use an error range!");
	ONSVariableBase *nsv = ONSVariableBase::GetVariable(no);
	nsv->SetLimit(min, max);
	return RET_CONTINUE;
}


int LOScriptReader::dimCommand(FunctionInterface *reader) {
	//LOLog_i("dim command has enter!") ;
	ONSVariableRef *v = reader->GetParamRef(0);
	v->DimArray();
	return RET_CONTINUE;
}

//返回的是增加的行数
int LOScriptReader::TextPushParams(const char *&buf) {
	int currentEndFlag, lineadd = 0;

	//获取要显示的文字
	LOString text;

	buf = scriptbuf->SkipSpace(buf);
	const char *obuf = buf;
	const char *ebuf = scriptbuf->e_buf();

	while (buf < ebuf) {
		if (buf[0] == '\\') {  //强结束符
			currentEndFlag = buf[0];
			buf++;
			break;
		}
		else if (buf[0] == '@') {
			currentEndFlag = buf[0]; //将检查下一个符号是否 \n，\n遇到texec时从下一行开始，否则接着开始
			buf++;
			obuf = scriptbuf->SkipSpace(buf);
			if (obuf[0] == '\n' || obuf[0] == ';') currentEndFlag |= 0x1000; //换行符号
			break;
		}
		else if (buf[0] == '/') {
			currentEndFlag = buf[0];
			buf++;
			break;
		}
		else if (buf[0] == '\n') {
			buf++;
			lineadd++;
			//check next line start english?
			buf = scriptbuf->SkipSpace(buf);
			if (scriptbuf->ThisCharLen(buf) == 1) {
				currentEndFlag = '/';
				break;
			}
		}
		else {
			int ulen = scriptbuf->ThisCharLen(buf);
			text.append(buf, ulen);
			buf += ulen;
		}
	}

	text = text.TrimEnd();

	ONSVariableRef *v1 = new ONSVariableRef(ONSVariableRef::TYPE_STRING_IM);
	if (text.length() > 0) ExpandStr(text);
	v1->SetValue(&text);
	paramStack.push(v1);

	v1 = new ONSVariableRef(ONSVariableRef::TYPE_REAL);
	v1->SetImVal((double)currentEndFlag);
	paramStack.push(v1);

	paramStack.push((ONSVariableRef*)(2));

	return lineadd;
}


int LOScriptReader::usergosubCommand(FunctionInterface *reader) {
	if (reader->isName("pretextgosub")) userGoSubName[USERGOSUB_PRETEXT] = reader->GetParamStr(0);
	else if(reader->isName("textgosub")) userGoSubName[USERGOSUB_TEXT] = reader->GetParamStr(0);
	else if(reader->isName("loadgosub")) userGoSubName[USERGOSUB_LOAD] = reader->GetParamStr(0);
	return RET_CONTINUE;
}

int LOScriptReader::loadscriptCommand(FunctionInterface *reader) {
	int ret = 0;
	LOString fname = reader->GetParamStr(0);
	BinArray *bin = ReadFile(&fname,true);
	if (bin) {
		if (bin->Length() > 0) {
			LOScripFile *file = AddScript(bin->bin, bin->Length(), fname.c_str());
			file->InitLables();
			ret = 1;
		}
		delete bin;
	}

	if (reader->GetParamCount() > 1) {
		ONSVariableRef *v = GetParamRef(1);
		v->SetValue((double)ret);
	}
	return RET_CONTINUE;
}


int LOScriptReader::gettagCommand(FunctionInterface *reader) {
	int count = reader->GetParamCount();
	auto list = TagString.SplitConst("/");
	for (int ii = 0; ii < count; ii++) {
		ONSVariableRef *v = reader->GetParamRef(ii);
		if (ii < list.size()) v->SetAutoVal(&list.at(ii));
		else if (v->isStrRef())v->SetValue((LOString*)NULL);
		else v->SetValue(0.0);
	}
	return RET_CONTINUE;
}


int LOScriptReader::atoiCommand(FunctionInterface *reader) {
	ONSVariableRef *v = reader->GetParamRef(0);
	LOString s = reader->GetParamStr(1);
	v->SetAutoVal(&s);
	return RET_CONTINUE;
}

//检查变量是否为指定值，不相当则输出错误信息
int LOScriptReader::chkValueCommand(FunctionInterface *reader) {
	ONSVariableRef *v1 = reader->GetParamRef(0);
	ONSVariableRef *v2 = reader->GetParamRef(1);
	if (!v1->Compare(v2, ONSVariableRef::LOGIC_EQUAL,false)) {
		SimpleError("[%s]:[%d]:[%s]\n", currentLable->file->Name.c_str(), currentLable->c_line,reader->GetParamStr(2).c_str());
	}
	return RET_CONTINUE;
}


int LOScriptReader::cselCommand(FunctionInterface *reader) {
	int count = reader->GetParamCount() / 2;
	cselList.clear();
	for (int ii = 0; ii < count; ii++) {
		cselList.push_back(reader->GetParamStr(ii * 2));
		cselList.push_back(reader->GetParamStr(ii * 2 + 1));
	}
	LOScriptPoint *p = GetScriptPoint("customsel");
	gosubCore(p, true);
	return RET_CONTINUE;
}

int LOScriptReader::getcselstrCommand(FunctionInterface *reader) {
	if (reader->isName("getcselnum")) {
		ONSVariableRef *v = reader->GetParamRef(0);
		v->SetValue( (double)(cselList.size() / 2));
	}
	else if(reader->isName("getcselstr")){
		ONSVariableRef *v = reader->GetParamRef(0);
		int index = reader->GetParamInt(1);
		LOString tmp;
		if (index < cselList.size() / 2) {
			tmp = cselList.at(index * 2);
		}
		v->SetValue(&tmp);
	}
	else{
		int index = reader->GetParamInt(0);
		if (index < cselList.size() / 2) {
			LOScriptPoint *p = GetScriptPoint(cselList.at(index * 2 + 1));
			gosubCore(p, true);
		}
		else LOLog_e("[cselgoto] value is out range!");
	}
	return RET_CONTINUE;
}


int LOScriptReader::delaygosubCommand(FunctionInterface *reader) {
	/*
	LOEventParamBtnRef *param = new LOEventParamBtnRef();
	LOEvent1 *e = new LOEvent1(SCRIPTER_DELAY_GOSUB, param);
	param->ptr1 = SDL_GetTicks();
	param->ptr2 = reader->GetParamInt(1);
	param->ref = new ONSVariableRef(ONSVariableRef::TYPE_STRING_IM);
	LOString s = reader->GetParamStr(0);
	param->ref->SetValue(&s);
	reader->blocksEvent.SendToSlot(e);
	*/
	return RET_CONTINUE;
}

int LOScriptReader::savefileexistCommand(FunctionInterface *reader) {
	/*
	ONSVariableRef *v = reader->GetParamRef(0);
	LOString fn = StringFormat(64, "save%d.datl",reader->GetParamInt(1));
	FILE *f = OpenFileForRead(fn.c_str(), "rb");
	if (f) {
		fclose(f);
		v->SetValue(1.0);
	}
	else v->SetValue(0.0);
	*/
	return RET_CONTINUE;
}


int LOScriptReader::savepointCommand(FunctionInterface *reader) {
	//音频、渲染模块进入save模式，不要调整这几个的顺序
	audioModule->ChangeFlagState(MODULE_FLAGE_SAVE | MODULE_FLAGE_CHANNGE);
	imgeModule->ChangeFlagState(MODULE_FLAGE_SAVE | MODULE_FLAGE_CHANNGE);
	scriptModule->ChangeRunState(MODULE_STATE_SAVING);

	//做一些初始化工作
	LOIO::InitLPKStream(GloSaveFS);

	//等待img进入saving
	while (!imgeModule->isStateSaving()) {
		//延迟1ms
		G_PrecisionDelay(1);
	}

	//更新全局变量
	if (st_globalon) UpdataGlobleVariable();
	//存储变量
	if(st_globalon) ONSVariableBase::SaveOnsVar(GloSaveFS, gloableMax, MAXVARIABLE_COUNT - gloableMax);
	else ONSVariableBase::SaveOnsVar(GloSaveFS, 0, MAXVARIABLE_COUNT);
	//存储hook钩子列表
	G_hookQue.SaveHooks(GloSaveFS);
	//存储图层
	LonsSaveLayer(GloSaveFS);
	//存储渲染模块
	LonsSaveImageModule(GloSaveFS);
	//存储脚本模块
	((LOScriptReader*)scriptModule)->Serialize(GloSaveFS);
	//存储音频模块

	return RET_CONTINUE;
}

//将全局变量写入到全局变量流中
void LOScriptReader::UpdataGlobleVariable() {
	//初始化格式
	LOIO::InitLPKStream(GloVariableFS);
	ONSVariableBase::SaveOnsVar(GloVariableFS, 0, gloableMax);
}


void LOScriptReader::ReadGlobleVarFile() {
	ONSVariableBase::ResetAll();

	LOString fn("gloval.savl");
	FILE *f = LOIO::GetSaveHandle(fn, "rb");
	if (!f) return;

	std::unique_ptr<BinArray> bin(BinArray::ReadFile(f, 0, 0x7ffffff));
	fclose(f);
	if (bin) {
		int pos = 0;
		ReadGlobleVariable(bin.get(), &pos);
	}
}

void LOScriptReader::ReadGlobleVariable(BinArray *bin, int *pos) {
	if (!LOIO::CheckLPKHeader(bin, pos)) {
		LOLog_i("[gloval.savl] not 'LPKS' flag!");
		return;
	}
	if (bin->GetInt(pos) != 0x52415647) {
		LOLog_i("[gloval.savl] not 'GVAR' flag!");
		return;
	}
	bin->GetInt(pos); //version
	int from = bin->GetInt(pos);
	int count = bin->GetInt(pos);
	for (int ii = from; ii < count; ii++) {
		if (!GNSvariable[ii].Deserialization(bin, pos)) {
			LOLog_i("GNSvariable[ii].Deserialization() faild!");
			break;
		}
	}
}

void LOScriptReader::SaveGlobleVariable() {
	LOString fn("gloval.savl");
	FILE *f = LOIO::GetSaveHandle(fn, "wb");
	if (f) {
		fwrite(GloVariableFS->bin, 1, GloVariableFS->Length(), f);
		fflush(f);
		fclose(f);
	}
}


int LOScriptReader::savegameCommand(FunctionInterface *reader) {
	int no = reader->GetParamInt(0);
	LOString tag;
	if (reader->GetParamCount() > 1) tag = reader->GetParamStr(1);

	//存储全局变量
	SaveGlobleVariable();
	//LOString fn;
	//if (saveDir.length() > 0) fn = saveDir + "/" + StringFormat(64, "save%d.datl", no);
	//else fn = StringFormat(64, "save%d.datl", no);

	//FILE *f = OpenFileForWrite(fn.c_str(), "rb");
	//if (f) {
	//	
	//}
	//else LOLog_e("open file to write faild:%s \n", fn.c_str());
	return RET_CONTINUE;
}


//reset的时候必须小心切断所有模块之间的联系
int LOScriptReader::resetCommand(FunctionInterface *reader) {
	//scriptModule->moduleState |= MODULE_STATE_RESET;
	//imgeModule->moduleState |= MODULE_STATE_RESET;
	//audioModule->ResetMe();
	////这里需要等待一下渲染线程置为nouse，不然有可能渲染线程还在尝试读取文件
	//while (imgeModule->moduleState != MODULE_STATE_NOUSE) {
	//	SDL_Delay(1);
	//}
	//fileModule->ResetMe();
	return RET_CONTINUE;
}


//重新读取变量定义部分，在此之前需清除定义
int LOScriptReader::defineresetCommand(FunctionInterface *reader) {
	//filelog labellog globalon textgosub 
	//useescspc  usewheel  ruby
	return RET_CONTINUE;
}


int LOScriptReader::labellogCommand(FunctionInterface *reader) {
	st_labellog = true;
	ReadLog(LOGSET_LABELLOG);
	return RET_CONTINUE;
}


int LOScriptReader::globalonCommand(FunctionInterface *reader) {
	st_globalon = true;
	return RET_CONTINUE;
}

//自动测试命令
int LOScriptReader::testcmdsCommand(FunctionInterface *reader) {
	LOString cmd = reader->GetParamStr(0);
	if (cmd == "globalon_save") {
		UpdataGlobleVariable();
	}
	else if (cmd == "globalon_load") {
		ONSVariableBase::ResetAll();
		int pos = 0;
		ReadGlobleVariable(GloVariableFS, &pos);
	}
	return RET_CONTINUE;
}