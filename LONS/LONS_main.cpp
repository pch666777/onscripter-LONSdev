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
	LOLog_i("main scripter thread will exit.");
 	return 0;
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

	int x = 0x8EB90100;
	int a = (x & 0xff) << 24;
	int b = (x & 0xff00) << 8;
	int c = (x & 0x00ff0000) >> 8;

	//BinArray bin(2, true);
	//bin.WriteInt(1);
	//bin.WriteInt4(2, 3, 4, 5);
	//bin.WriteInt4(6, 7, 8, 9);
	//bin.WriteInt4(10, 11, 12, 13);

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
			//注册spstr事件
			LOShareEventHook ev(LOEventHook::CreateSpstrHook());
			G_hookQue.push_back(ev, LOEventQue::LEVEL_NORMAL);

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

