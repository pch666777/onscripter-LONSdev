/*
项目地址：https://github.com/pch666777/onscripter-LONSdev
邮件联系：pngchs@qq.com
编辑时间：2020——2023
*/

#include "Render/LOImageModule.h"
#include "Scripter/LOScriptReader.h"
#include "File/LOFileModule.h"
#include "Audio/LOAudioModule.h"
#include "etc/LOEvent1.h"
#include "etc/LOLog.h"
#include "etc/LOIO.h"
#include "etc/LOMessage.h"

#include "etc/LOVariant.h"

#include <SDL.h>
#include <chrono>

#include "Render/LOTextDescribe.h"

#ifdef ANDROID
#include <android/log.h>
#include <unistd.h>
#endif

#ifdef WIN32
extern void LoadLibs();
#endif // 


extern void LonsReadEnvData();
extern int G_lineLog;

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

//读取lons.cfg的配置文件,ANSI only，do not use any two byte char
void ReadConfig() {
	LOString s("lons.cfg");
	FILE *f = LOIO::GetReadHandle(s, "rb");
	if (!f) return;
	std::unique_ptr<BinArray> bin(BinArray::ReadFile(f, 0, 1000000));
	fclose(f);
	if (!bin) return;
	s.assign(bin->bin, bin->Length());

	const char *buf = s.SkipSpace(s.c_str());
	const char *ebuf = s.c_str() + s.length();
	LOString key;

	while (buf < ebuf) {
		buf = s.SkipSpace(buf);
        if(buf[0] == ';'){ //跳过注释
            buf = s.NextLine(buf);
            continue ;
        }

        key = s.GetWord(buf);
		buf = s.SkipSpace(buf);
		if (buf >= ebuf - 1 || buf[0] != '=') return;
		buf++;

		buf = s.SkipSpace(buf);

        if (key == "window") { //window=1440*900，设置窗口大小
			G_destWidth = s.GetInt(buf);
			buf = s.SkipSpace(buf) + 1; //skip ','
			G_destHeight = s.GetInt(buf);
		}
        //不需要设置控制台项，只需要在终端/控制台允许LONS，即可有标准输出
        else if(key == "debug"){ //设置debug等级，目前只有0和非0两个区分
            if(s.GetInt(buf) != 0) G_lineLog = 1 ;
            else G_lineLog = 0 ;
        }
        else if(key == "ratio"){ //画面比例 ratio=4:3，目前工作不正常
            G_gameRatioW = s.GetInt(buf);
            buf = s.SkipSpace(buf) + 1;
            G_gameRatioH = s.GetInt(buf);
        }
        else if(key == "fullscreen"){//全屏
            G_fullScreen = s.GetInt(buf) ;
        }
        else if(key == "playcmd"){
            G_playcmd = s.GetWord(buf);
        }

		buf = s.NextLine(buf);
	}
}


//必要的变量初始化
void GlobalInit() {
	//初始化根图层
	LOLayer::InitBaseLayer();
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
	LOScripFile::InitScriptLabels();
	LOScriptReader *reader = (LOScriptReader*)ptr;
	reader->MainTreadRunning();
    SDL_Log("%s", "main scripter thread has exit.");
	reader->ChangeModuleFlags(0);
	reader->ChangeModuleState(FunctionInterface::MODULE_STATE_NOUSE);
 	return 0;
}


int main(int argc, char **argv) {
    SDL_Log("%s", msg_has_run);
    SDL_Log("%s%s", msg_work_dir_is, LOIO::ioReadDir.c_str()) ;
	//check base type byte len
    if (sizeof(int) != 4) SDL_Log("%s", msg_base_int_no);
    if (sizeof(double) != 8) SDL_Log("%s", msg_base_double_no);

	//某些平台可能需要一些初始化的操作
#ifdef WIN32
	//setlocale(LC_ALL, "gbk");
	//LoadLibs();
#endif // WIN32

	ReadConfig();
	//if (G_useLogFile) UseLogFile();

    SDL_Log("LONS %s", ONS_VERSION);
    SDL_Log("%s", msg_lons_wecomle);
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


	//初始化IO，必须优先进行IO，因为后面要读文件
	filemodule = new LOFileModule;

	//初始化脚本模块
	reader = LOScriptReader::EnterScriptReader(MAINSCRIPTER_NAME);

	//实例化渲染模块
	imagemodule = new LOImageModule();

	//加载脚本
	if (reader->DefaultStep()) {
        SDL_Log("script module init ok.");
		//初始化渲染模块
		if (imagemodule->InitImageModule()) {
            SDL_Log("image module init ok.");
            //初始化音频模块，初始化失败不影响运行，只是没有声音
			audiomodule = new LOAudioModule;
            if(!audiomodule->InitAudioModule())SimpleWarning("%s", msg_init_audio_faild);
            else SDL_Log("audiomodule module init ok.");
            //读取基本环境
            LonsReadEnvData();
            //启动脚本线程
            SDL_CreateThread(ScripterThreadEntry, "script", (void*)reader);
            audiomodule->moduleState = FunctionInterface::MODULE_STATE_RUNNING;

            //渲染及事件开始在主线程循环
            imagemodule->MainLoop();
            exitflag = 0;
		}
        else FatalError("%s", msg_init_render_faild);
	}
    else FatalError("%s", msg_init_noscript);

	GlobalFree();

	if (audiomodule) delete audiomodule;
	if (imagemodule) delete imagemodule;
	if (reader) delete reader;
	if (filemodule) delete filemodule;

	return exitflag;
}

