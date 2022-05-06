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


//ONScripter ons;

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
	for (int ii = 0; ii < LOLayer::LAYER_BASE_COUNT; ii++) {
		int fullid = GetFullID(LOLayer::LAYER_CC_USE, ii, 255, 255);
		G_baseLayer[ii] = LOLayer::CreateLayer(fullid);
	}
}


//释放初始化的变量
void GlobalFree() {
	//清理字体
	LOFont::FreeAllFont();
	//释放根图层
	for (int ii = 0; ii < LOLayer::LAYER_BASE_COUNT; ii++) {
		delete G_baseLayer[ii];
	}
}


void FreeModules(LOFileModule *&filemodule, LOScriptReader *&reader, LOImageModule *&imagemodule, LOAudioModule *&audiomodule) {
	if (filemodule) delete filemodule;
	filemodule = NULL;

	if (reader) {
		LOScriptReader::LeaveScriptReader(reader);
		delete reader;
		reader = nullptr;
	}

	if (imagemodule) delete imagemodule;
	imagemodule = NULL;

	if (audiomodule)delete audiomodule;
	audiomodule = NULL;
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
	reader->moduleState = FunctionInterface::MODULE_STATE_NOUSE;
	LOLog_i("main scripter thread has exit.");
	//LOLog_i("%d\n", SDL_GetTicks());
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

	//G_InitSlots();   //初始化线程同步事件槽

	//auto *eventManager = new LOEventManager();
	//for (int ii = 0; ii < 10; ii++) eventManager->AddEvent(new LOEvent1(11, 1), 0);

	//int level, index;
	//level = index = 0;

	//auto *e = eventManager->GetNextEvent(&level, &index);
	//int count = 1;
	//while (e) {
	//	printf("%d\n",count);
	//	e = eventManager->GetNextEvent(&level, &index);
	//	count++;
	//}

	GlobalInit();


	while (exitflag == -1) {
		//reset的时候直接整个重来
		FreeModules(filemodule, reader, imagemodule,audiomodule);
		//初始化IO，必须优先进行IO，因为后面要读文件
		filemodule = new LOFileModule;

		//初始化脚本模块
		reader = LOScriptReader::EnterScriptReader(MAINSCRIPTER_NAME);
		imagemodule = new LOImageModule;
		//ResetTimer();
		//加载脚本
		if (reader->DefaultStep()) {
			LOLog_i("script module init ok.");
			//uint32_t tttt = GetTimer();
			//ResetTimer();

			//初始化渲染模块
			if (imagemodule->InitImageModule()) {
				LOLog_i("image module init ok.");

				char data[] = { 0xCE,0XD2,0XCA,0XC7,0XD2,0XBB, 0XB8, 0XF6, 0XB2, 0XE2, 0XCA,0XD4,0XA1,0XA3, 0 };
				LOString s = "default.ttf";
				LOString str(data);
				str.SetEncoder(LOCodePage::GetEncoder(LOCodePage::ENCODER_GBK));

				LOTextStyle style;
				LOTextTexture tex;
				tex.CreateTextDescribe(&str, &style, &s);

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
				else exitflag = -4;
			}
			else exitflag = -3;
		}
		else {
			LOLog_e("LONS can't find [nscript.dat] or [0.txt~99.txt]!");
			exitflag = -2;
		}
	}

	FreeModules(filemodule, reader, imagemodule, audiomodule);
	//G_DestroySlots();  //释放线程同步信号槽

	GlobalFree();

	if (exitflag == -1) exitflag = 0;
	return exitflag;
}

