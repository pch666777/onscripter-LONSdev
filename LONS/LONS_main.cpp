/*
项目地址：https://gitee.com/only998/onscripter-lons
邮件联系：pngchs@qq.com
编辑时间：2020——2021
*/

#include "Render/LOImageModule.h"
#include "Scripter/LOScriptReader.h"
#include "File/LOFileModule.h"
#include "Audio/LOAudioModule.h"
#include "etc/LOEvent1.h"
#include "etc/LOLog.h"

#include "etc/LOVariant.h"

#include <SDL.h>
#include <chrono>

#include "Render/LOTextDescribe.h"

#ifdef ANDROID
#include <android/log.h>
#include <unistd.h>
#endif

//#ifdef WIN32
//
//#endif // 


extern void LonsReadEnvData();

void GetIntSet(int *a, int *b, char *buf) {
	//int c = *a >> 16;
	//if (!(c >> 16)) {
	//	std::string val;
	//	buf = GetWord(&val, buf);
	//	buf = GetWord(&val, buf);
	//	*a = CharToInt(val.c_str());
	//	if (b) {
	//		buf = GetWord(&val, buf);
	//		buf = GetWord(&val, buf);
	//		*b = CharToInt(val.c_str());
	//	}
	//}
}

//读取lons.cfg的配置文件
void ReadConfig() {
	//FILE *f = tryOpenFile("lons.cfg", "r");
	//if (!f)return;
	//char *line = new char[256];
	//std::string keywork;

	//while (fgets(line, 255, f)) {
	//	char *buf = SkipSpace(line);
	//	buf = GetWord(&keywork, buf);
	//	if (keywork == "fullscreen") GetIntSet(&G_cfgIsFull,nullptr, buf);
	//	else if (keywork == "window") GetIntSet(&G_cfgwindowW,&G_cfgwindowH,buf);
	//	else if (keywork == "outline")GetIntSet(&G_cfgoutlinePix, nullptr, buf);
	//	else if (keywork == "shadow") GetIntSet(&G_cfgfontshadow, nullptr, buf);
	//	else if (keywork == "fps") GetIntSet(&G_cfgfps, nullptr, buf);
	//	else if (keywork == "ratio") GetIntSet(&G_gameRatioW, &G_gameRatioH, buf);
	//	else if (keywork == "position") GetIntSet(&G_cfgposition, nullptr, buf);
	//	else if (keywork == "logfile") GetIntSet(&G_useLogFile, nullptr, buf);
	//}
	//fclose(f);
	//delete[] line;
}


//必要的变量初始化
void GlobalInit() {
	//初始化根图层
	LOLayer::InitBaseLayer();
	//读取基本环境
	LonsReadEnvData();
}


//释放初始化的变量
void GlobalFree() {
	//清理字体
	LOFont::FreeAllFont();
	//释放根图层
	for (int ii = 0; ii < LOLayer::LAYER_BASE_COUNT; ii++) {
		delete G_baseLayer[ii];
	}

	//保存环境
	
}


int ScripterThreadEntry(void *ptr) {
	//新的线程均需要初始化随机数种子，这是一个容易坑的位置
	char *srand_ptr = new char[64];
	int timeval = time(NULL);
	srand((intptr_t)srand_ptr ^ timeval);   //初始化随机数种子
	delete [] srand_ptr;

	//init labels
	LOScriptReader::InitScriptLabels();
	LOScriptReader *reader = (LOScriptReader*)ptr;
	reader->MainTreadRunning();
	LOLog_i("main scripter thread has exit.");
	reader->ChangeModuleFlags(0);
	reader->ChangeModuleState(FunctionInterface::MODULE_STATE_NOUSE);
 	return 0;
}


//注册基本的事件钩子
void RegisterBaseHook() {
	//注册脚本模块呼叫hook
	LOShareEventHook ev(LOEventHook::CreateScriptCallHook());
	G_hookQue.push_N_front(ev);
}



int main(int argc, char **argv) {
	SDL_Log("LONS engine has been run from the main() function!\n");
	//check base type byte len
	if (sizeof(int) != 4) SDL_Log("The basic data type [int] length does not meet the requirements!!!\n");
	if (sizeof(double) != 8) SDL_Log("The basic data type [double] length does not meet the requirements!!!\n");

	ReadConfig();
	//if (G_useLogFile) UseLogFile();

	//SDL_Log("LONS version %s(%d.%02d)\n", ONS_VERSION, NSC_VERSION / 100, NSC_VERSION % 100);
	LOLog_i("***==========LONS,New generation of high performance ONScripter engine==========*** \n");
	// ================ test ================== //
	//G_EventQue.AddEvent(nullptr);
	// ================ start ================== //
	LOFileModule *filemodule = NULL;
	LOScriptReader *reader = NULL;
	LOImageModule *imagemodule = NULL;
	LOAudioModule *audiomodule = NULL;

	int exitflag = -1;

	srand((unsigned)time(NULL));   //初始化随机数种子

	GlobalInit();


	//LOShareEventHook e1(new LOEventHook());
	//e1->param1 = 1;
	//LOShareEventHook e2(new LOEventHook());
	//e2->param1 = 2;
	//LOShareEventHook e3(new LOEventHook());
	//e3->param1 = 3;
	//LOShareEventHook e4(new LOEventHook());
	//e4->param1 = 4;
	//LOShareEventHook e5(new LOEventHook());
	//e5->param1 = 5;

	//LOEventQue que;
	//que.push_N_front(e3);
	//que.push_N_back(e1);
	//que.push_N_front(e4);
	//que.push_H_front(e2);
	//que.push_H_back(e5);

	//auto iter = que.begin();
	//while (true) {
		//LOShareEventHook ev = que.GetEventHook(iter, false);
	//	LOShareEventHook ev = que.TakeOutEvent();
	//	if (!ev) break;
	//}


	//初始化IO，必须优先进行IO，因为后面要读文件
	filemodule = new LOFileModule;

	//初始化脚本模块
	reader = LOScriptReader::EnterScriptReader(MAINSCRIPTER_NAME);

	//实例化渲染模块
	imagemodule = new LOImageModule();

	//加载脚本
	if (reader->DefaultStep()) {
		LOLog_i("script module init ok.");
		//初始化渲染模块
		if (imagemodule->InitImageModule()) {
			LOLog_i("image module init ok.");
			//事件支持
			RegisterBaseHook();

			//初始化音频模块
			audiomodule = new LOAudioModule;
			if (audiomodule->InitAudioModule()) {
				LOLog_i("audiomodule module init ok.");
				//启动脚本线程
				SDL_CreateThread(ScripterThreadEntry, "script", (void*)reader);
				audiomodule->moduleState = FunctionInterface::MODULE_STATE_RUNNING;

				//渲染及事件开始在主线程循环
				imagemodule->MainLoop();
				exitflag = 0;
			}
			else LOLog_e("init audio module faild!");
		}
		else LOLog_e("init render module faild!");
	}
	else LOLog_e("No valid script!");

	GlobalFree();

	if (audiomodule) delete audiomodule;
	if (imagemodule) delete imagemodule;
	if (reader) delete reader;
	if (filemodule) delete filemodule;

	return exitflag;
}

