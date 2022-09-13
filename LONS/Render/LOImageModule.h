/*
//图像渲染模块
*/

#include "../Scripter/FuncInterface.h"
#include "../etc/LOString.h"
#include "LOEffect.h"
#include "LOLayer.h"
#include "LOTexture.h"
#include "LOTextDescribe.h"
#include "LOTexture.h"
#include <SDL.h>

#define ONS_VERSION "ver0.5-20220910"


class LOImageModule :public FunctionInterface{
public:
	LOImageModule();
	~LOImageModule();

	int InitImageModule();
	int MainLoop();
	void UpDisplay(double postime);
	void UpDataLayer(LOLayer *layer, Uint32 curTime, int from, int dest, int level);

	enum {
		TEXTURE_IMG = 1,
		TEXTURE_STRING,
		TEXTURE_COLOR,
		//TEXTURE_GEO
		//TEXTURE_DESCRIBE,
	};

	enum {
		IMG_EFFECT_NONE,
		IMG_EFFECT_MONO,  //黑白
		IMG_EFFECT_INVERT, //反色
		IMG_EFFECT_USECOLOR  //着色
	};

	enum {
		PARAM_BGCOPY = 1723 ,
		PARAM_SCREENSHORT ,
	};

	enum {
		SHADER_MOMO,  //黑白
		SHADER_INVERT, //反色
	};

	//printName结构
	class PrintNameMap{
	public:
		std::string *mapName = nullptr;
		std::map<int, LOLayer*> *map = nullptr;
		PrintNameMap(const char *fn) {
			mapName = new std::string(fn);
			map = new std::map<int, LOLayer*>;
		}
		~PrintNameMap() {
			delete mapName;
			delete map;
		}

		void Serialize(BinArray *bin);
		bool DeSerialize(BinArray *bin, int *pos);
	};

	//对话框描述
	struct LOSayWindow {
		int x;
		int y;
		int w;
		int h;
		int textX;
		int textY;
		LOString winstr;
		void reset() {
			x = y = textX = textY = 0;
			w = G_gameWidth;
			h = G_gameHeight;
			winstr.clear();
		}

		void Serialize(BinArray *bin);

	};

	//对话框的状态
	struct LOSayState{
		enum {
			//文字、窗口和图标
			FLAGS_TEXT_SHOW = 1 ,
			FLAGS_WINDOW_SHOW = 2,
			FLAGS_CUR_SHOW = 4,
			//默认print隐藏对话框
			FLAGS_PRINT_HIDE = 8,
			FLAGS_PRINT_BEFOR = 16,
			FLAGS_WINBACK_MODE = 32,
			//textec决定了下一行是否从头开始
			FLAGS_TEXT_TEXEC = 64 ,
			FLAGS_WINDOW_CHANGE = 128 ,
			FLAGS_TEXT_CHANGE = 256,
			FLAGS_TEXT_DISPLAY = 512,
		};
		//当前显示的对话文字
		LOString say;
		//对话框显示、消失时候执行的效果
		std::unique_ptr<LOEffect> winEffect;
		//状态标记
		int flags;
		//对话框所在的位置，大于这个值的sp将被显示在对话框的下方
		int z_order;
		//结尾符号，值的是 / \ @ 三种
		int pageEnd;
		
		void reset() {
			say.clear();
			winEffect.reset();
			flags = FLAGS_PRINT_HIDE | FLAGS_WINDOW_CHANGE;
			z_order = 499;
			pageEnd = 0;
		}
		//相信编译器，自动inline优化
		bool isWinbak() { return flags & FLAGS_WINBACK_MODE;}
		bool isTexec() { return flags & FLAGS_TEXT_TEXEC; }
		bool isPrinHide() {return flags & FLAGS_PRINT_HIDE;}
		bool isTextDispaly() { return flags & FLAGS_TEXT_DISPLAY; }
		bool isWindowChange() { return flags & FLAGS_WINDOW_CHANGE; }
		void setFlags(int f) { flags |= f; }
		void unSetFlags(int f) { flags &= (~f); }
		void Serialize(BinArray *bin);
		bool DeSerialize(BinArray *bin, int *pos);
	};


	int trans_mode;   //透明类型
	int effectRunFalg[2];  //是否正处于特性运行阶段 [0] 1处于 0不处于  [1] 0允许点击时跳过  1不允许跳过

	SDL_mutex* layerQueMutex;	//图层队列锁
	//std::mutex layerTestMute;
	//需要对后台layerdata操作时，要先锁定
	SDL_mutex* layerDataMutex;
	SDL_mutex* doQueMutex;      //添加、展开队列时必须保证没有其他线程进入

	LOLayer *lastActiveLayer;  //上一次被激活的按钮图层，这个值每次进入btnwait时都会被重置
	std::vector<std::unique_ptr<PrintNameMap>> backDataMaps;

	std::unordered_map<int, LOEffect*> effectMap; //特效缓存器
		//前置事件组，用于帧刷新过程中产生的事件，帧刷新完成后在事件处理之前处理
	std::vector<LOShareEventHook> preEventList;
	void GetUseTextrue(LOLayerDataBase *bak, void *data, bool addcount = true);

	LOLayerData* CreateNewLayerData(int fullid, const char *printName);
	LOLayerData* CreateLayerBakData(int fullid, const char *printName);
	LOLayerData* GetLayerData(int fullid, const char *printName);
	LOLayerData* CreateBtnData(const char *printName);

	//获取printName对应的map
	PrintNameMap* GetPrintNameMap(const char *printName);

	bool loadSpCore(LOLayerData *info, LOString &tag, int x, int y, int alpha, bool visiable = false);
	bool loadSpCoreWith(LOLayerDataBase *bak, LOString &tag, int x, int y, int alpha,int eff);

	LOtexture* ScreenShot(int x, int y, int w, int h, int dw, int dh);
	void ScreenShotCountinue(LOEventHook *e);
	LOtextureBase* SurfaceFromFile(LOString *filename);
	int RefreshFrame(double postime);        //刷新帧显示

	bool ParseTag(LOLayerDataBase *bak, LOString *tag);
	bool ParseImgSP(LOLayerDataBase *bak, LOString *tag, const char *buf);

	LOActionText* LoadDialogText(LOString *s,int pageEnd,  bool isAdd);
	bool LoadDialogWin();
	bool SetLayerShow(bool isVisi, int fullid, const char *printName);
	void ClearBtndef();

	void DialogWindowSet(int showtext, int showwin, int showbmp);
	void DialogWindowPrint();
	void EnterTextDisplayMode(bool force = false);
	void LeveTextDisplayMode(bool force = false);
	void ClearDialogText(char flag);
	void RunExbtnStr(LOString *s);
	int RunFunc(LOEventHook *hook, LOEventHook *e);
	int RunFuncSpstr(LOEventHook *hook, LOEventHook *e);
	int RunFuncText(LOEventHook *hook, LOEventHook *e);
	int RunFuncScriptCall(LOEventHook *hook, LOEventHook *e);
	int RunFuncBtnClear(LOEventHook *hook, LOEventHook *e);
	int RunFuncBtnFinish(LOEventHook *hook, LOEventHook *e);

	LOEffect* GetEffect(int id);
	void PrepareEffect(LOEffect *ef, const char *printName);
	bool ContinueEffect(LOEffect *ef, const char *printName, double postime);

	intptr_t GetSDLRender() { return (intptr_t)render; }
	void PrintError(LOString *err);
	void ResetMe();
	void LoadReset();

	void Serialize(BinArray *bin);
	//序列化print执行队列
	void SerializePrintQue(BinArray *bin);
	//序列化渲染模块的状态
	void SerializeState(BinArray *bin);
	bool DeSerializeState(BinArray *bin, int *pos);
	bool DeSerialize(BinArray *bin, int *pos, LOEventMap *evmap);
	void SaveLayers(BinArray *bin);
	bool LoadLayers(BinArray *bin, int *pos, LOEventMap *evmap);

	int lspCommand(FunctionInterface *reader);
	int printCommand(FunctionInterface *reader);
	int printStack(FunctionInterface *reader, int fix);
	int bgCommand(FunctionInterface *reader);
	int cspCommand(FunctionInterface *reader);
	void CspCore(int layerType, int fromid, int endid, const char *print_name);
	int mspCommand(FunctionInterface *reader);
	int cellCommand(FunctionInterface *reader);
	int humanzCommand(FunctionInterface *reader);
	int strspCommand(FunctionInterface *reader);
	int transmodeCommand(FunctionInterface *reader);
	int bgcopyCommand(FunctionInterface *reader);
	int spfontCommand(FunctionInterface *reader);
	int effectskipCommand(FunctionInterface *reader);
	int getspmodeCommand(FunctionInterface *reader);
	int getspsizeCommand(FunctionInterface *reader);
	int getspposCommand(FunctionInterface *reader);
	int vspCommand(FunctionInterface *reader);
	void VspCore(int fullid, const char *print_name, int vals);
	int allspCommand(FunctionInterface *reader);
	int windowbackCommand(FunctionInterface *reader);
	int lsp2Command(FunctionInterface *reader);
	int textCommand(FunctionInterface *reader);
	int effectCommand(FunctionInterface *reader);
	int windoweffectCommand(FunctionInterface *reader);
	int btnwaitCommand(FunctionInterface *reader);
	int spbtnCommand(FunctionInterface *reader);
	int texecCommand(FunctionInterface *reader);
	int texthideCommand(FunctionInterface *reader);
	int textonCommand(FunctionInterface *reader);
	int textbtnsetCommand(FunctionInterface *reader);
	int setwindowCommand(FunctionInterface *reader);
	int setwindow2Command(FunctionInterface *reader);
	int clickCommand(FunctionInterface *reader);
	int erasetextwindowCommand(FunctionInterface *reader);
	int getmouseposCommand(FunctionInterface *reader);
	int btndefCommand(FunctionInterface *reader);
	int btntimeCommand(FunctionInterface *reader);
	int getpixcolorCommand(FunctionInterface *reader);
	int getspposexCommand(FunctionInterface *reader);
	int getspalphaCommand(FunctionInterface *reader);
	int gettextCommand(FunctionInterface *reader);
	int chkcolorCommand(FunctionInterface *reader);
	int mouseclickCommand(FunctionInterface *reader);
	int getscreenshotCommand(FunctionInterface *reader);
	int savescreenshotCommand(FunctionInterface *reader);
	int rubyCommand(FunctionInterface *reader);
	int getcursorposCommand(FunctionInterface *reader);
	int ispageCommand(FunctionInterface *reader);
	int exbtn_dCommand(FunctionInterface *reader);
	int btnCommand(FunctionInterface *reader);
	int spstrCommand(FunctionInterface *reader);
	int filelogCommand(FunctionInterface *reader);

private:
	static bool isShowFps;
	static bool st_filelog;  //是否使用文件记录

	//lsp时使用的样式
	LOTextStyle spStyle;
	//lsp使用的字体名称
	LOString    spFontName;
	//对话使用的样式
	LOTextStyle sayStyle;
	LOString    sayFontName;
	//对话框
	LOSayWindow sayWindow;
	//对话框状态
	LOSayState sayState;

	LOtexture *fpstex;

	LOString btndefStr;     //btndef定义的按钮文件名
	//LOString exbtn_dStr;
	int BtndefCount;
	//int exbtn_count;
	int btnOverTime;
	bool btnUseSeOver;
	bool reflashNow;
	//bool exbtn_d_hasrun;

	int dialogDisplayMode;
	bool effectSkipFlag;    //是否允许跳过效果（单击时）
	bool textbtnFlag;    //控制textbtnwait时，文字按钮是否自动注册
	int  textbtnValue;   //文字按钮的值，默认为1
	std::vector<int> *allSpList;   //allsphide、allspresume命令使用的列表
	std::vector<int> *allSpList2;  //allsp2hide、allsp2resume命令使用的列表
	//char pageEndFlag[2];  //'\' or '@' or '/' 0位置反映真实的换页符号，1位置反映根据处理后的换页符号
	int shaderList[20];
	int mouseXY[2];
	//LOSurface *screenshotSu;       //屏幕截图
	//int debugEventCount;  //检查事件队列中的事件数量，以免忘记移除事件
	
	SDL_Window *window;
	SDL_Renderer *render;
	int max_texture_width;
	int max_texture_height;
	LOString titleStr;

	//抓取图像的遮片
	LOShareTexture effectTex;
	//遮片纹理
	LOShareTexture effmakTex;
	Uint32 tickTime;

	void ResetViewPort();
	void CaleWindowSize(int scX, int scY, int srcW, int srcH, int dstW, int dstH, SDL_Rect *result);
	void ShowFPS(double postime);
	bool InitFps();
	void ResetConfig();
	void FreeFps();
	void TextureFromFile(LOLayerDataBase *bak);
	void TextureFromCache(LOLayerDataBase *bak);
	//void TextureFromEmpty(LOLayerData *info);
	void TextureFromControl(LOLayerDataBase *bak, LOString *s);
	void TextureFromColor(LOLayerDataBase *bak, LOString *s);
	void TextureFromSimpleStr(LOLayerDataBase *bak, LOString *s);
	void TextureFromActionStr(LOLayerDataBase *bak, LOString *s);
	//LOtextureBase* TextureFromNSbtn(LOLayerInfo*info, LOString *s);

	void ScaleTextParam(LOLayerData *info, LOTextStyle *fontwin);
	LOtextureBase* EmptyTexture(LOString *fn);
	const char* ParseTrans(int *alphaMode, const char *buf);

	int ExportQuequ(const char *print_name, LOEffect *ef, bool iswait, bool isIM = false);
	void DoPreEvent(double postime);
	void CaptureEvents(SDL_Event *event);
	void HandlingEvents();
	int SendEventToHooks(LOEventHook *e, int flags);
	int SendEventToLayer(LOEventHook *e);
	//int SendEventToHooks(LOEventHook *e);
	bool TranzMousePos(int xx, int yy);
	void CutDialogueAction();
	int CutPrintEffect(LOEventHook *hook, LOEventHook *e);
	bool WaitStateEvent();
};