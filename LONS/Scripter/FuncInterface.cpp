/*
//这是一个接口类，所有命令都继承这个类
//运行时优先尝试脚本类-->图像类-->音频类
*/
#include "FuncInterface.h"
#include "../etc/LOIO.h"
#include <stdarg.h>

std::unordered_map<std::string, FunctionInterface::FuncLUT*> FunctionInterface::baseFuncMap;
FunctionInterface *FunctionInterface::imgeModule = NULL;
FunctionInterface *FunctionInterface::audioModule = NULL;
FunctionInterface *FunctionInterface::scriptModule = NULL;
FunctionInterface *FunctionInterface::fileModule = NULL;    //文件系统
BinArray *FunctionInterface::GloVariableFS = new BinArray(1024, true);
BinArray *FunctionInterface::GloSaveFS = new BinArray(1024, true);
bool FunctionInterface::errorFlag = false;
bool FunctionInterface::breakFlag = false;

LOShareEventHook FunctionInterface::effcetRunHook(new LOEventHook());
LOShareEventHook FunctionInterface::printHook(new LOEventHook());

LOString FunctionInterface::userGoSubName[3] = {"lons_pretextgosub__","lons_textgosub__",""};
std::unordered_set<std::string> FunctionInterface::fileLogSet;
std::unordered_set<std::string> FunctionInterface::labelLogSet;

//R-ref, I-realref, S-strref, N-normal word, L-label, A-arrayref, C-color
//r-any, i-real, s-string, *-repeat, #-repeat last
static FunctionInterface::FuncLUT func_lut[] = {
	//scripter module
	{"mov",      "\0",         "\0",    &FunctionInterface::movCommand},
	{"mov3",     "\0",         "\0",    &FunctionInterface::movCommand},
	{"mov4",     "\0",         "\0",    &FunctionInterface::movCommand},
	{"mov5",     "\0",         "\0",    &FunctionInterface::movCommand},
	{"mov6",     "\0",         "\0",    &FunctionInterface::movCommand},
	{"mov7",     "\0",         "\0",    &FunctionInterface::movCommand},
	{"mov8",     "\0",         "\0",    &FunctionInterface::movCommand},
	{"mov9",     "\0",         "\0",    &FunctionInterface::movCommand},
	{"mov10",    "\0",         "\0",    &FunctionInterface::movCommand},

	{"movl",      "A,i,#",     "Y,Y,#",    &FunctionInterface::movlCommand},
	{"cos",      "I,i",       "Y,Y",       &FunctionInterface::operationCommand},
	{"dec",      "I",         "Y",         &FunctionInterface::operationCommand},
	{"inc",      "I",         "Y",         &FunctionInterface::operationCommand},
	{"div",      "I,i",       "Y,Y",       &FunctionInterface::operationCommand},
	{"mod",      "I,i",       "Y,Y",       &FunctionInterface::operationCommand},
	{"mul",      "I,i",       "Y,Y",       &FunctionInterface::operationCommand},
	{"rnd",      "I,i",       "Y,Y",       &FunctionInterface::operationCommand},
	{"rnd2",     "I,i,i",     "Y,Y,Y",     &FunctionInterface::operationCommand},
	{"sin",      "I,i",       "Y,Y",       &FunctionInterface::operationCommand},
	{"sub",      "I,i",       "Y,Y",       &FunctionInterface::operationCommand},
	{"tan",      "I,i",       "Y,Y",       &FunctionInterface::operationCommand},
	{"dim",      "A",         "Y",         &FunctionInterface::dimCommand},
	{"add",      "R,r",       "Y,Y",       &FunctionInterface::addCommand},
	{"intlimit", "i,i,i",     "Y,Y,Y",     &FunctionInterface::intlimitCommand},
	{"itoa",     "S,i",       "Y,Y",       &FunctionInterface::itoaCommand},
	{"itoa2",    "S,i",       "Y,Y",       &FunctionInterface::itoaCommand},
	{"len",		 "I,s",       "Y,Y",       &FunctionInterface::lenCommand},
	{"lenword",  "I,s",       "Y,Y",       &FunctionInterface::lenCommand},
	{"mid",		 "S,s,i,i",   "Y,Y,Y,Y",   &FunctionInterface::midCommand},
	{"midword",	 "S,s,i,i",   "Y,Y,Y,Y",   &FunctionInterface::midCommand},
	{"debuglog", "r,*",       "Y,*",       &FunctionInterface::debuglogCommand},
	{"numalias", "N,i",       "Y,Y",       &FunctionInterface::numaliasCommand},
	{"stralias", "N,s",       "Y,Y",       &FunctionInterface::straliasCommand},
	{"gosub",    "L",         "Y",         &FunctionInterface::gotoCommand},
	{"goto",     "L",         "Y",         &FunctionInterface::gotoCommand},
	{"jumpb",     "\0",       "\0",        &FunctionInterface::jumpCommand},
	{"jumpf",     "\0",       "\0",        &FunctionInterface::jumpCommand},
	{"return",    "L",        "N",         &FunctionInterface::returnCommand},
	{"skip",      "i",        "Y",         &FunctionInterface::skipCommand},
	{"tablegoto", "I,L",      "Y,Y,#",     &FunctionInterface::tablegotoCommand},
	{"for",       "\0",       "\0",        &FunctionInterface::forCommand},
	{"next",      "\0",       "\0",        &FunctionInterface::nextCommand},
	{"break",     "\0",       "\0",        &FunctionInterface::breakCommand},
	{"if",        "\0",       "\0",        &FunctionInterface::ifCommand},
	{"notif",     "\0",       "\0",        &FunctionInterface::ifCommand},
	{"else",      "\0",       "\0",        &FunctionInterface::elseCommand},
	{"endif",     "\0",       "\0",        &FunctionInterface::endifCommand},
	{"resettimer","\0",       "\0",        &FunctionInterface::resettimerCommand},
	{"gettimer",  "I",        "Y",         &FunctionInterface::gettimerCommand},
	{"delay",     "i",        "Y",         &FunctionInterface::delayCommand},
	{"wait",      "i",        "Y",         &FunctionInterface::delayCommand},
	{"waittimer", "i",        "Y",         &FunctionInterface::delayCommand},
	{"game",      "\0",       "\0",        &FunctionInterface::gameCommand},
	{"end",       "\0",       "\0",        &FunctionInterface::endCommand},
	{"defsub",    "N",        "Y",         &FunctionInterface::defsubCommand},
	{"linepage",  "\0",       "\0",        &FunctionInterface::linepageCommand},
	//getparam有一些特殊的参数，由函数获取参数
	{"getparam",   "\0",      "\0",        &FunctionInterface::getparamCommand},
	{"labelexist", "I,L",     "Y,Y",       &FunctionInterface::labelexistCommand},
	{"fileexist",  "I,s",     "Y,Y",       &FunctionInterface::fileexistCommand},
	{"fileremove",  "s",      "Y",         &FunctionInterface::fileremoveCommand},
	{"readfile",   "S,s",     "Y,Y",       &FunctionInterface::readfileCommand},
	{"split",      "s,s,r,#", "Y,Y,Y,#",   &FunctionInterface::splitCommand},
	{"getversion", "R",       "Y",         &FunctionInterface::getversionCommand},
	{"getenvironment", "S,S,S",  "Y,N,N",  &FunctionInterface::getenvCommand},
	{"date",       "r,r,r",   "Y,Y,Y",     &FunctionInterface::dateCommand},
	{"time",       "r,r,r",   "Y,Y,Y",     &FunctionInterface::dateCommand},
	{"loadscript", "s,I",     "Y,N",       &FunctionInterface::loadscriptCommand},
	{"atoi",       "I,s",     "Y,Y",       &FunctionInterface::atoiCommand},
	{"_chkval_",   "R,r,s",   "Y,Y,Y",     &FunctionInterface::chkValueCommand},
	{"csel",       "s,L,*",   "Y,Y,*",     &FunctionInterface::cselCommand},
	{"getcselstr", "S,i",     "Y,Y",       &FunctionInterface::getcselstrCommand},
	{"getcselnum", "I",       "Y",         &FunctionInterface::getcselstrCommand},
	{"cselgoto",   "i",       "Y",         &FunctionInterface::getcselstrCommand},
	{"delaygosub", "L,i",     "Y,Y",       &FunctionInterface::delaygosubCommand},
	{"savefileexist",  "I,i", "Y,Y",       &FunctionInterface::savefileexistCommand},
	{"savepoint",  "\0",      "\0",        &FunctionInterface::savepointCommand},
	{"savegame",   "i",       "Y",         &FunctionInterface::savegameCommand},
	{"savegame2",  "i,s",     "Y,Y",       &FunctionInterface::savegameCommand},
	{"savedir",    "s",       "Y",         &FunctionInterface::savedirCommand},
	{"reset",      "\0",      "\0",        &FunctionInterface::resetCommand},
	{"definereset","\0",      "\0",        &FunctionInterface::defineresetCommand},
	{"labellog",   "\0",      "\0",        &FunctionInterface::labellogCommand},
	{"globalon",   "\0",      "\0",        &FunctionInterface::globalonCommand},
	{"_testcmd_s", "s",       "Y",         &FunctionInterface::testcmdsCommand},
	//{"_moveto_",   "s",       "Y",         &FunctionInterface::_movto_Command},
	{"loadgame",   "i",       "Y",         &FunctionInterface::loadgameCommand},
	{"setintvar",  "s,i",     "Y,Y",       &FunctionInterface::setintvarCommand},
	{"saveon",     "\0",      "\0",        &FunctionInterface::saveonCommand},
	{"saveoff",    "\0",      "\0",        &FunctionInterface::saveonCommand},
	{"autosaveoff","\0",      "\0",        &FunctionInterface::saveonCommand},

	{"savetime",   "i,I,I,I,I","Y,Y,Y,Y,Y",&FunctionInterface::savetimeCommand},
	{"getsavestr", "S,i",     "Y,Y",       &FunctionInterface::getsavestrCommand},

	{"pretextgosub", "L",     "Y",         &FunctionInterface::usergosubCommand},
	{"textgosub",    "L",     "Y",         &FunctionInterface::usergosubCommand},
	{"loadgosub",    "L",     "Y",         &FunctionInterface::usergosubCommand},

	//image module
	{"lsp",      "i,s,i,i,i",     "Y,Y,N,N,N",     &FunctionInterface::lspCommand},
	{"lsph",     "i,s,i,i,i",     "Y,Y,N,N,N",     &FunctionInterface::lspCommand},
	{"lspc",     "i,i,s,i,i,i",   "Y,Y,Y,Y,Y,N",   &FunctionInterface::lspCommand},
	{"lsp2",     "i,s,i,i,i,i,i,i", "Y,Y,Y,Y,Y,Y,Y,N",  &FunctionInterface::lsp2Command},
	{"lsp2add",  "i,s,i,i,i,i,i,i", "Y,Y,Y,Y,Y,Y,Y,N",  &FunctionInterface::lsp2Command},
	{"lsp2sub",  "i,s,i,i,i,i,i,i", "Y,Y,Y,Y,Y,Y,Y,N",  &FunctionInterface::lsp2Command},
	{"lsph2",    "i,s,i,i,i,i,i,i", "Y,Y,Y,Y,Y,Y,Y,N",  &FunctionInterface::lsp2Command},
	{"lsph2add", "i,s,i,i,i,i,i,i", "Y,Y,Y,Y,Y,Y,Y,N",  &FunctionInterface::lsp2Command},
	{"lsph2sub", "i,s,i,i,i,i,i,i", "Y,Y,Y,Y,Y,Y,Y,N",  &FunctionInterface::lsp2Command},
	{"print",    "i,i,s",         "Y,N,N",         &FunctionInterface::printCommand},
	{"bg",       "Ns,i,i,s",     "Y,N,N,N",        &FunctionInterface::bgCommand},
	{"csp",      "i",            "Y",              &FunctionInterface::cspCommand},
	{"csp2",     "i",            "Y",              &FunctionInterface::cspCommand},
	{"msp",      "i,i,i,i",      "Y,Y,Y,N",        &FunctionInterface::mspCommand},
	{"msp2",     "i,i,i,i,i,i,i","Y,Y,Y,Y,Y,Y,N",  &FunctionInterface::mspCommand},
	{"amsp",     "i,i,i,i",      "Y,Y,Y,N",        &FunctionInterface::mspCommand},
	{"amsp2",    "i,i,i,i,i,i,i","Y,Y,Y,Y,Y,Y,N",  &FunctionInterface::mspCommand},
	{"cell",     "i,i",          "Y,Y",            &FunctionInterface::cellCommand},
	{"humanz",   "i",            "Y",              &FunctionInterface::humanzCommand},
	{"strsp",    "i,s,i,i,i,i,i,i,i,i,i,i,N,#", "Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,N,#", &FunctionInterface::strspCommand},
	{"strsph",   "i,s,i,i,i,i,i,i,i,i,i,i,N,#", "Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,N,#", &FunctionInterface::strspCommand},
	{"transmode", "Ns",          "Y",              &FunctionInterface::transmodeCommand},
	{"bgcopy",    "",            "",               &FunctionInterface::bgcopyCommand},
	{"spfont",    "Ns,i,i,i,i,i,i", "Y,N,N,N,N,N,N", &FunctionInterface::spfontCommand},
	{"effectskip","i",           "Y",              &FunctionInterface::effectskipCommand},
	{"getspmode", "I,i",         "Y,Y",            &FunctionInterface::getspmodeCommand},
	{"getspsize", "i,I,I,I",     "Y,Y,Y,N",        &FunctionInterface::getspsizeCommand},
	{"getsppos",  "i,I,I",       "Y,Y,Y",          &FunctionInterface::getspposCommand },
	{"getspalpha","i,I",         "Y,Y",            &FunctionInterface::getspalphaCommand },
	{"getspposex","i,I,I,I,I,I", "Y,Y,Y,N,N,N",    &FunctionInterface::getspposexCommand },
	{"getspposex2","i,I,I,I,I,I", "Y,Y,Y,N,N,N",   &FunctionInterface::getspposexCommand },
	{"vsp",       "i,i",         "Y,Y",            &FunctionInterface::vspCommand},
	{"vsp2",      "i,i",         "Y,Y",            &FunctionInterface::vspCommand},
	{"allsphide", "\0",          "\0",             &FunctionInterface::allspCommand},
	{"allspresume",    "\0",        "\0",          &FunctionInterface::allspCommand},
	{"allsp2hide",     "\0",        "\0",          &FunctionInterface::allspCommand},
	{"allsp2resume",   "\0",        "\0",          &FunctionInterface::allspCommand},
	{"windowback",     "\0",        "\0",          &FunctionInterface::windowbackCommand},
	{ "effect",        "i,i,i,s",   "Y,Y,N,N",     &FunctionInterface::effectCommand },
	{ "windoweffect",  "i,i,s",     "Y,N,N",       &FunctionInterface::windoweffectCommand },
	{ "btnwait",       "I",         "Y",           &FunctionInterface::btnwaitCommand },
	{ "btnwait2",      "I",         "Y",           &FunctionInterface::btnwaitCommand },
	{ "spbtn",         "i,i",       "Y,Y",         &FunctionInterface::spbtnCommand },
	{ "texec",         "\0",        "\0",          &FunctionInterface::texecCommand },
	{ "texthide",      "\0",        "\0",          &FunctionInterface::texthideCommand },
	{ "textshow",      "\0",        "\0",          &FunctionInterface::texthideCommand },
	{ "texton",        "\0",        "\0",          &FunctionInterface::textonCommand },
	{ "textoff",       "\0",        "\0",          &FunctionInterface::textonCommand },
	{ "textbtnoff",    "\0",        "\0",          &FunctionInterface::textbtnsetCommand },
	{ "textclear",     "\0",        "\0",          &FunctionInterface::textonCommand },
	{ "textbtnstart",  "i",         "Y",           &FunctionInterface::textbtnsetCommand },
	{ "textbtnwait",   "I",         "Y",           &FunctionInterface::btnwaitCommand },
	{ "setwindow",     "i,i,i,i,i,i,i,i,i,i,i,Ns,i,i,i,i","Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,N,N", &FunctionInterface::setwindowCommand },
	{ "setwindow3",     "i,i,i,i,i,i,i,i,i,i,i,Ns,i,i,i,i","Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,N,N", &FunctionInterface::setwindowCommand },
	{ "setwindow2",    "Ns",        "Y",           &FunctionInterface::setwindow2Command },
	{ "gettag",        "R,R,#",     "Y,N,#",       &FunctionInterface::gettagCommand },
	{ "gettext",       "S",         "Y",           &FunctionInterface::gettextCommand },
	{ "click",         "\0",        "\0",          &FunctionInterface::clickCommand },
	{ "lrclick",       "\0",        "\0",          &FunctionInterface::clickCommand },
	{ "erasetextwindow","i",        "Y",           &FunctionInterface::erasetextwindowCommand },
	{ "getmousepos",   "I,I",       "Y,Y",         &FunctionInterface::getmouseposCommand },
	{ "btndef",        "Ns",        "Y",           &FunctionInterface::btndefCommand },
	{ "btntime",       "i",         "Y",           &FunctionInterface::btntimeCommand },
	{ "btntime2",      "i",         "Y",           &FunctionInterface::btntimeCommand },
	{ "btime",		   "i,i",       "Y,N",         &FunctionInterface::btntimeCommand },
	{ "getpixcolor",   "S,i,i",     "Y,Y,Y",       &FunctionInterface::getpixcolorCommand },
	{ "_chkcolor_",    "i,i,i,i,S", "Y,Y,Y,Y,Y",   &FunctionInterface::chkcolorCommand },
	{ "_mouseclick_",  "i,i,i",     "Y,Y,Y",       &FunctionInterface::mouseclickCommand },
	{ "getscreenshot", "i,i",       "Y,Y",         &FunctionInterface::getscreenshotCommand },
	{ "savescreenshot","s",         "Y",           &FunctionInterface::savescreenshotCommand },
	{ "savescreenshot2","s",        "Y",           &FunctionInterface::savescreenshotCommand },
	{ "deletescreenshot","\0",      "\0",          &FunctionInterface::savescreenshotCommand },
	{ "rubyon",        "i,i,s",     "N,N,N",       &FunctionInterface::rubyCommand },
	{ "rubyon2",       "i,i,s",     "Y,Y,N",       &FunctionInterface::rubyCommand },
	{ "rubyoff",       "\0",        "\0",          &FunctionInterface::rubyCommand },
	{ "getcursorpos",  "I,I",       "Y,Y",         &FunctionInterface::getcursorposCommand },
    { "getcursorpos2", "I,I",       "Y,Y",         &FunctionInterface::getcursorposCommand },
	{ "ispage",        "I",         "Y",           &FunctionInterface::ispageCommand },
	{ "exbtn",         "i,i,s",     "Y,Y,Y",       &FunctionInterface::spbtnCommand },
	{ "exbtn_d",       "s",         "Y",           &FunctionInterface::exbtn_dCommand },
	{ "btn",           "i,i,i,i,i,i,i","Y,Y,Y,Y,Y,Y,Y", &FunctionInterface::btnCommand },
	{ "spstr",         "s",         "Y",           &FunctionInterface::spstrCommand },
	{ "filelog",       "\0",        "\0",          &FunctionInterface::filelogCommand },
	{ "mpegplay",      "s,i",       "Y,N",         &FunctionInterface::aviCommand },
	{ "avi",           "s,i",       "Y,N",         &FunctionInterface::aviCommand },
	//movie命令的参数不符合规则，将由命令本身提取参数
    { "movie",         "\0",         "\0",         &FunctionInterface::movieCommand },
	{ "spevent",       "i,s,s,s,i", "Y,Y,Y,N,N",   &FunctionInterface::speventCommand },
	{ "quake",         "i,i",       "Y,Y",         &FunctionInterface::quakeCommand },
	{ "quakex",        "i,i",       "Y,Y",         &FunctionInterface::quakeCommand },
	{ "quakey",        "i,i",       "Y,Y",         &FunctionInterface::quakeCommand },
	{ "ld",            "N,s,i,i,s", "Y,Y,N,N,N",   &FunctionInterface::ldCommand },
	{ "cl",            "N,i,i,s",   "Y,N,N,N",     &FunctionInterface::clCommand },
	{ "humanorder",    "s,i,i,i",   "Y,N,N,N",     &FunctionInterface::humanorderCommand },
	{ "caption",       "s",         "Y",           &FunctionInterface::captionCommand },
	{ "textspeed",     "i",         "Y",           &FunctionInterface::textspeedCommand },
    { "action",        "i,s,i,#",   "Y,Y,Y,#",     &FunctionInterface::actionCommand },
    { "actionloop",    "i,s,i",     "Y,Y,Y",       &FunctionInterface::actionloopCommand },

	//audio
	{ "bgm",           "s",         "Y",           &FunctionInterface::bgmCommand },
	{ "play",          "s",         "Y",           &FunctionInterface::bgmCommand },
	{ "mp3loop",       "s",         "Y",           &FunctionInterface::bgmCommand },
	{ "bgmonce",       "s",         "Y",           &FunctionInterface::bgmonceCommand },
	{ "mp3",           "s",         "Y",           &FunctionInterface::bgmonceCommand },
	{ "mp3save",       "s",         "Y",           &FunctionInterface::bgmonceCommand },
	{ "playonce",      "s",         "Y",           &FunctionInterface::bgmonceCommand },
	{ "bgmstop",       "\0",        "Y",           &FunctionInterface::bgmstopCommand },
	{ "mp3stop",       "\0",        "Y",           &FunctionInterface::bgmstopCommand },
	{ "playstop",      "\0",        "Y",           &FunctionInterface::bgmstopCommand },
	{ "loopbgm",       "s,s",       "Y,Y",         &FunctionInterface::loopbgmCommand },
	{ "loopbgmstop",   "\0",        "",            &FunctionInterface::loopbgmstopCommand },
	{ "bgmfadein",     "i",         "Y",           &FunctionInterface::bgmfadeCommand },
	{ "mp3fadein",     "i",         "Y",           &FunctionInterface::bgmfadeCommand },
	{ "bgmfadeout",    "i",         "Y",           &FunctionInterface::bgmfadeCommand },
	{ "mp3fadeout",    "i",         "Y",           &FunctionInterface::bgmfadeCommand },
	{ "wave",          "s",         "Y",           &FunctionInterface::waveCommand },
	{ "wavestop",      "\0",        "Y",           &FunctionInterface::waveCommand },
	{ "waveloop",      "s",         "Y",           &FunctionInterface::waveCommand },
	{ "bgmdownmode",   "i",         "Y",           &FunctionInterface::bgmdownmodeCommand },
	{ "bgmvol",        "i",         "Y",           &FunctionInterface::voicevolCommand },
	{ "mp3vol",        "i",         "Y",           &FunctionInterface::voicevolCommand },
	{ "sevol",         "i",         "Y",           &FunctionInterface::voicevolCommand },
	{ "voicevol",      "i",         "Y",           &FunctionInterface::voicevolCommand },
	{ "chvol",         "i,i",       "Y,Y",         &FunctionInterface::chvolCommand },
	{ "dwave",         "i,s",       "Y,Y",         &FunctionInterface::dwaveCommand },
	{ "dwaveload",     "i,s",       "Y,Y",         &FunctionInterface::dwaveloadCommand },
	{ "dwaveloop",     "i,s",       "Y,Y",         &FunctionInterface::dwaveCommand },
	{ "dwaveplay",     "i",         "Y",           &FunctionInterface::dwaveplayCommand },
	{ "dwaveplayloop", "i",         "Y",           &FunctionInterface::dwaveplayCommand },
	{ "dwavestop",     "i",         "N",           &FunctionInterface::dwavestopCommand },
	{ "getbgmvol",     "I",         "Y",           &FunctionInterface::getvoicevolCommand },
	{ "getmp3vol",     "I",         "Y",           &FunctionInterface::getvoicevolCommand },
	{ "getsevol",      "I",         "Y",           &FunctionInterface::getvoicevolCommand },
	{ "getvoicevol",   "I",         "Y",           &FunctionInterface::getvoicevolCommand },
	{ "stop",          "\0",        "",            &FunctionInterface::stopCommand },
	//lons添加了的获取某个音轨是否处于播放状态
	{ "getvoicestate", "I,i",       "Y,Y",         &FunctionInterface::getvoicestateCommand },
	{ "getvoicefile",  "S,i",       "Y,Y",         &FunctionInterface::getvoicefileCommand },
	{ "getrealvol",    "I,i",       "Y,Y",         &FunctionInterface::getrealvolCommand },

	//file
	{ "nsadir",        "s",         "Y",           &FunctionInterface::nsadirCommand },
	{ "addnsadir",     "s",         "Y",           &FunctionInterface::nsadirCommand },
	{ "nsa",           "\0",        "",            &FunctionInterface::nsaCommand },
	{ "ns2",           "\0",        "",            &FunctionInterface::nsaCommand },
	{ "ns3",           "\0",        "",            &FunctionInterface::nsaCommand },

	{NULL, NULL, NULL, NULL},
};


FunctionInterface::FunctionInterface() {
	RegisterBaseFunc();
	moduleState = MODULE_STATE_NOUSE;
	logMem = nullptr;
}

FunctionInterface::~FunctionInterface() {
	if (logMem) delete logMem;
	logMem = nullptr;
}

void FunctionInterface::RegisterBaseFunc() {
	if (baseFuncMap.size() > 0) return;
	FuncLUT *ptr = &func_lut[0];
	while (ptr->cmd) {
		//printf(ptr->cmd);
		baseFuncMap[LOString(ptr->cmd)] = ptr;
		ptr++;
	}
}

FunctionInterface::FuncLUT* FunctionInterface::GetFunction(LOString &func) {
	auto iter = baseFuncMap.find(func);
	if (iter == baseFuncMap.end()) return NULL;
	else return iter->second;
}

int FunctionInterface::RunCommand(FuncBase it) {
	int ret = (this->*it)(this);
	if (ret == RET_VIRTUAL && imgeModule) ret = (imgeModule->*it)(this);
	if (ret == RET_VIRTUAL && audioModule) ret = (audioModule->*it)(this);
	if (ret == RET_VIRTUAL && fileModule) ret = (fileModule->*it)(this);
	return ret;
}


LOScriptPoint  *FunctionInterface::GetParamLable(int index) {
	return NULL;
}

LOString FunctionInterface::GetParamStr(int index) {
	ONSVariableRef *v = GetParamRef(index);
	if (v && v->GetStr()) return *(v->GetStr());
	else return LOString();
}


int FunctionInterface::GetParamInt(int index) {
	ONSVariableRef *v = GetParamRef(index);
	if (v) return (int)v->GetReal();
	else return -1;
}

double FunctionInterface::GetParamDoublePer(int index) {
	int vv = GetParamInt(index);
	return (double)vv / 100;
}

int FunctionInterface::GetParamColor(int index) {
	ONSVariableRef *v = GetParamRef(index);
	if (v) {
		LOString *s = v->GetStr();
		if (s) {
			char *buf = (char*)(intptr_t)(s->c_str());
			if (buf[0] == '#') buf++;
			int sum = 0;
			while (buf[0]) {
				char hh = tolower(*buf);
				if (hh >= 'a' && hh <= 'f') sum = sum * 16 + hh - 87;
				else if (hh >= '0' && hh <= '9') sum = sum * 16 + hh - '0';
				else return sum;
				buf++;
			}
			return sum;
		}
	}
	return 0;
}

int FunctionInterface::GetParamCount() {
	return ((intptr_t)paramStack.top()) & 0xff;
}

//获取指定位置的ref变量，失败返回NULL
ONSVariableRef *FunctionInterface::GetParamRef(int index) {
	int max = GetParamCount();
	if (index > max) {
		FatalError("GetParamRef() out of range! max is %d, index is %d.", max, index);
		return NULL;
	}
	return paramStack.at(paramStack.size() - 1 - max + index);
}


LOString FunctionInterface::StringFormat(int max, const char *format, ...) {
	va_list ap;
	va_start(ap, format);

	char *tmp = new char[max];
	memset(tmp, 0, max);
	int len = vsnprintf(tmp, max - 1, format, ap);
	tmp[len] = 0;
	LOString rets(tmp);
	delete[] tmp;
	va_end(ap);
	return rets;
}



//应由文件系统实现
BinArray *FunctionInterface::ReadFile(LOString *fileName, bool err) {
	if (fileModule) {
		return fileModule->ReadFile(fileName, err);
	}
	else {
		return LOIO::ReadAllBytes(*fileName);
	}
}

BinArray* LonsReadFile(LOString &fn) {
	if (FunctionInterface::fileModule) {
		return FunctionInterface::fileModule->ReadFile(&fn, false);
	}
	else {
		return LOIO::ReadAllBytes(fn);
	}
}




void FunctionInterface::SetExitFlag(int flag) {
	//中断所有模块运行
	if (audioModule) {
		audioModule->ResetMe();
	}
	if (scriptModule) scriptModule->ChangeModuleFlags(flag);
	if (imgeModule) imgeModule->ChangeModuleFlags(flag);
	//LOEvent1::SetExitFlag(1);
}


void FunctionInterface::ReadLog(int logt) {
	std::unordered_set<std::string> *logset = nullptr;
	const char *fn = nullptr;
	if (logt == LOGSET_FILELOG) {
		fn = "NScrflog.dat";
		logset = &fileLogSet;
	}
	else if (logt == LOGSET_LABELLOG) {
		fn = "NScrllog.dat";
		logset = &labelLogSet;
	}
	else return;

	/*
	FILE *f = OpenFileForRead(fn, "rb");
	if (f) {
		std::unique_ptr<BinArray> ptr(BinArray::ReadFile(f, 0, 0x7fffffff));
		if (ptr->Length() > 0) {
			int count = 0;
			int len = 0;
			while (ptr->bin[len] != 0xa) {
				count = count * 10 + ptr->bin[len] - '0';
				len++;
			}
			len++;

			std::string tmp;
			for (int ii = 0; ii < count && len < ptr->Length(); ii++) {
				if (ptr->bin[len] == '"') len++;
				char *buf = ptr->bin + len;
				while (ptr->bin[len] != '"') {
					ptr->bin[len] ^= 0x84;
					len++;
				}
				tmp.assign(buf, ptr->bin + len - buf);
				logset->insert(tmp);
				len++;
			}
		}
		fclose(f);
	}
	*/
}


void FunctionInterface::WriteLog(int logt) {
	std::unordered_set<std::string> *logset = nullptr;
	const char *fn = nullptr;
	if (logt == LOGSET_FILELOG) {
		fn = "NScrflog.dat";
		logset = &fileLogSet;
	}
	else if (logt == LOGSET_LABELLOG) {
		fn = "NScrllog.dat";
		logset = &labelLogSet;
	}
	else return;
	if (logset->empty()) return;

	std::unique_ptr<BinArray> ptr(new BinArray(1024, true));
	std::unique_ptr<char> ubuf(new char[1024]);

	std::string tmp = std::to_string(logset->size());
	ptr->Append(tmp.c_str(), tmp.length());
	ptr->WriteChar(0xa);

	for (auto iter = logset->begin(); iter != logset->end(); iter++) {
		char *dst = ubuf.get();
		dst[0] = '"'; dst++;
		const char *src = (*iter).c_str();
		while (src[0] != 0) {
			dst[0] = src[0] ^ 0x84;
			dst++;
			src++;
		}
		dst[0] = '"';
		ptr->Append(ubuf.get(), (*iter).length() + 2);
	}

	/*
	FILE *f = OpenFileForWrite(fn, "wb");
	if (f) {
		fwrite(ptr->bin, 1, ptr->Length(), f);
		fflush(f);
		fclose(f);
	}
	*/
}


bool FunctionInterface::CheckTimer(LOEventHook *e, int waittmer) {
	Uint32 postime = SDL_GetTicks() - e->timeStamp;
	//postime = 1;
	int maxtime = e->GetParam(0)->GetInt();
	if (postime >= maxtime) return true;
	else if (postime + waittmer >= maxtime) {
		//进入等待，直到满足时间要求
		while (postime < maxtime) {
			//cpu干点活
			int sum = rand();
			for (int ii = 0; ii < 2000; ii++) {
				sum ^= ii;
				if (ii % 2) sum++;
				else sum += 2;
			}
			postime = SDL_GetTicks() - e->timeStamp;
		}
		return true;
	}
	return false;
}

//向各模块分发事件
int FunctionInterface::RunFuncBase(LOEventHook *hook, LOEventHook *e) {
	if (hook->param1 == LOEventHook::MOD_RENDER) return imgeModule->RunFunc(hook, e);
	else if (hook->param1 == LOEventHook::MOD_SCRIPTER)return scriptModule->RunFunc(hook, e);
	else if (hook->param1 == LOEventHook::MOD_AUDIO)return audioModule->RunFunc(hook, e);
	return LOEventHook::RUNFUNC_CONTINUE;
}


void FunctionInterface::ChangeModuleState(int s) {
	//低4位置0，保持高位不动
	int high = moduleState & 0xFFFFFFF0;
	moduleState = high | s;
}


void FunctionInterface::ChangeModuleFlags(int f) {
	//保持低位不动
	int low = moduleState & 0xf;
	moduleState = low | f;
}
