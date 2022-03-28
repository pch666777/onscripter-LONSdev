/*
//图像渲染模块
*/

#include "../Scripter/FuncInterface.h"
#include "../etc/LOString.h"
#include "LOEffect.h"
#include "LOLayer.h"
#include "LOTexture.h"
#include "LOFontBase.h"
#include "LOFontInfo.h"
#include "LOSurface.h"
#include "LOTexture.h"
#include <SDL.h>

#define ONS_VERSION "ver0.5-20220116"


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
		PRE_EVENT_PREPRINTOK = 1,
		PRE_EVENT_EFFECTCONTIUE,
	};

	enum {
		IMG_EFFECT_NONE,
		IMG_EFFECT_MONO,  //黑白
		IMG_EFFECT_INVERT, //反色
		IMG_EFFECT_USECOLOR  //着色
	};

	enum {
		DISPLAY_MODE_NORMAL = 0,
		DISPLAY_MODE_TEXT = 1,
		//DISPLAY_MODE_AFTERCLEAR = 4,
	};

	enum {
		PARAM_BGCOPY = 1723 ,
		PARAM_SCREENSHORT ,
	};

	enum {
		SHADER_MOMO,  //黑白
		SHADER_INVERT, //反色
	};


	int z_order;    //对话框所在的位置，大于这个值的sp将被显示在对话框的下方
	int trans_mode;   //透明类型
	bool winbackMode;
	LOString winstr;   //对话窗口
	SDL_Rect winoff;  //对话框的坐标，长宽
	int winState;  //对话框的状态，0表示没有显示，1表示已经显示
	int winEraseFlag;   //print的时候对话框是否隐藏，0为不隐藏，1为隐藏且先执行，2为隐藏且随print执行
	int effectRunFalg[2];  //是否正处于特性运行阶段 [0] 1处于 0不处于  [1] 0允许点击时跳过  1不允许跳过
	LOEffect* winEffect;     //对话框显示、消失时候执行的效果

	SDL_mutex* layerQueMutex;	//图层队列锁
	SDL_mutex* btnQueMutex;     //按钮队列操作锁
	SDL_mutex* presentMutex;    //SDL_RenderPresent时不能执行创建纹理，编辑纹理等操作
	SDL_mutex* doQueMutex;      //添加、展开队列时必须保证没有其他线程进入

	//LOEffect currentEffect;

	LOLayer *lonsLayers[LOLayer::LAYER_BASE_COUNT];
	//int lonsLayers_show[LOLayer::LAYER_BASE_COUNT];  //决定了怎么显示对象
	LOLayer *nssysLayer;      //直接的image按钮位于的图层，包括默认的NS系统
	LOLayer *dialogLayer;     //对话框，系统默认用地立绘
	LOLayer *lsp2Layer;        //lsp2所使用的图层组
	LOLayer *lspLayer;		   //lsp所用的图层组
	LOLayer *bgLayer;          //背景层
	LOLayer *lastActiveLayer;  //上一次被激活的按钮图层，这个值每次进入btnwait时都会被重置
	std::map<int, LOLayer*> btnMap;

	bool breakflag = false;
	bool dialogWinHasChange;
	bool dialogTextHasChange;
	LOString dialogText; //当前显示的文本
	std::unordered_map<int, LOEffect*> effectMap; //特效缓存器

	LOtextureBase* GetUseTextrue(LOLayerInfo *info, void *data, bool addcount = true);

	void RemoveBtn(int fullid);
	//const char* NewSysBtndef();

	LOLayer* FindLayerInBtnQuePosition(int x, int y);
	LOLayer* FindLayerInBase(LOLayer::SysLayerType type, const int *ids);
	LOLayer* FindLayerInBase(int fullid);


	//新建一个layerinfo，如果已经有的话释放掉旧的
	LOLayerInfo *GetInfoNewAndFreeOld(int fullid, const char* print_name);
	LOLayerInfo *GetInfoNewAndNoFreeOld(int fullid, const char* print_name);
	std::vector<LOLayerInfo*> BatchInfoNew(const char* print_name, std::vector<int> *list, bool isfree);
	LOLayerInfo *GetInfoNew(int fullid, const char* print_name);
	LOLayerInfo* GetInfoLayerAvailable(int fullid, const char* cacheN);
	LOLayerInfo* GetInfoLayerAvailable(LOLayer::SysLayerType type, int *ids, const char* cacheN);
	LOLayerInfo* GetInfoUnLayerAvailable(int fullid, const char* cacheN);
	LOLayerInfo* GetInfoUnLayerAvailable(LOLayer::SysLayerType type, int *ids, const char* cacheN);
	void ClearAllLayerInfo();

	void NotUseInfo(LOLayerInfoCacheIndex *minfo);
	uint64_t GetIndexKey(int fullid, const char* print_name);
	uint64_t GetIndexKey(int fullid, int hash);
	LOLayerInfo* LayerInfomation(LOLayer::SysLayerType type, int *ids, const char* cacheN);
	LOLayerInfo* LayerInfomation(int fullid, const char* cacheN);
	BinArray* GetQueLayerUsedState(int sfullID, const char* print_name, int checkIndex);


	bool LayerIsVisable(int fullid, const char* cacheN);

	LOLayerInfoCacheIndex *GetCacheIndexFromPool(int fullid, const char* cacheN);

	int GetInfoCacheFromName(const char *buf);

	void FreeInfoFromAllCache(int fullid);



	LOLayer* GetRootLayer(int fullid);
	bool loadSpCore(LOLayerInfo *info, LOString &tag, int x, int y, int alpha);
	bool loadSpCoreWith(LOLayerInfo *info, LOString &tag, int x, int y, int alpha,int eff);



	//LOSurface* ScreenShot(SDL_Rect *srcRect, SDL_Rect *dstRect);
	//void ScreenShotCountinue(LOEvent1 *e);
	LOSurface* SurfaceFromFile(LOString *filename, bool *ispng = NULL);
	int RefreshFrame(double postime);        //刷新帧显示

	bool ContinueEffect(LOEffect *ef, double postime);

	bool ParseTag(LOLayerInfo *info ,LOString *tag);

	bool ParseImgSP(LOLayerInfo *info, LOString *tag, const char *buf);

	LOtextureBase* RenderText(LOLayerInfo *info, LOFontWindow *fontwin, LOString *s, SDL_Color *color, int cellcount);
	LOtextureBase* RenderText2(LOLayerInfo *info, LOFontWindow *fontwin, LOString *s, int startx);

	bool LoadDialogText(LOString *s, bool isAdd);
	bool LoadDialogWin();
	int ShowLayer(int fullid, const char *printName);
	int HideLayer(int fullid, const char *printName);

	void DialogWindowSet(int showtext, int showwin, int showbmp);
	void DialogWindowPrint();
	void EnterTextDisplayMode(bool force = false);
	void LeveTextDisplayMode(bool force = false);
	void ClearDialogText(char flag);
	void RunExbtnStr(LOString *s);

	LOEffect* GetEffect(int id);
	void UpTest();
	void WindowHasReset();
	void PrepareEffect(LOEffect *ef, const char *printName);
	intptr_t GetSDLRender() { return (intptr_t)render; }
	void PrintError(LOString *err);
	void ResetMe();
	//void InfoCaheSerialize(BinArray *sbin);

	int lspCommand(FunctionInterface *reader);
	int printCommand(FunctionInterface *reader);
	int printStack(FunctionInterface *reader, int fix);
	int bgCommand(FunctionInterface *reader);
	int cspCommand(FunctionInterface *reader);
	void CspCore(LOLayer::SysLayerType sptype, int *cid, const char *print_name);
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
	void VspCore(LOLayer::SysLayerType sptype, int *cid, const char *print_name, int vals);
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
	std::unordered_map<std::string, LOtexture*> texMap;        //材质缓存
	LOFontWindow winFont;       //默认的窗口
	LOFontWindow spFont;
	LOFontManager fontManager;
	LOtextureBase *fpstex;

	LOString btndefStr;     //btndef定义的按钮文件名
	LOString exbtn_dStr;
	int BtndefCount;
	int exbtn_count;
	int btnOverTime;
	bool btnUseSeOver;
	bool exbtn_d_hasrun;

	int dialogDisplayMode;
	bool effectSkipFlag;    //是否允许跳过效果（单击时）
	bool textbtnFlag;    //控制textbtnwait时，文字按钮是否自动注册
	bool texecFlag;
	int  textbtnValue;   //文字按钮的值，默认为1
	std::vector<int> *allSpList;   //allsphide、allspresume命令使用的列表
	std::vector<int> *allSpList2;  //allsp2hide、allsp2resume命令使用的列表
	//char pageEndFlag[2];  //'\' or '@' or '/' 0位置反映真实的换页符号，1位置反映根据处理后的换页符号
	int pageEndFlag;
	int shaderList[20];
	int mouseXY[2];
	LOSurface *screenshotSu;       //屏幕截图
	//int debugEventCount;  //检查事件队列中的事件数量，以免忘记移除事件
	
	SDL_Window *window;
	SDL_Renderer *render;
	int max_texture_width;
	int max_texture_height;
	LOString titleStr;
	SDL_Texture *effectTex;
	SDL_Texture *maskTex;      //遮片纹理
	Uint32 tickTime;
	std::vector<LOEventHook> preEventList;

	LOStack<LOLayerInfoCacheIndex> poolData;  //分配池
	//std::unordered_map<int, LOLayer*> activeLayerMap;
	std::map<uint64_t, LOLayerInfoCacheIndex*> queLayerinfoMap;
	std::vector<int> prinNameList;   //print name 的hash队列
	int poolCurrent;   //分配池的当前位置
	SDL_mutex *poolMutex;  //为了在多线程中使用，添加移除时锁定队列
	//void IncreasePool();
	std::vector<LOLayerInfoCacheIndex*> SortCacheList(std::vector<LOLayerInfoCacheIndex*> *list);
	//LOLayerInfoCacheIndex * GetCacheIndexFromName(int fullid, const char *print_name, int *mapindex);
	void ClearCacheMap(std::vector<LOLayerInfoCacheIndex*> *list);
	std::vector<LOLayerInfoCacheIndex*> FilterCacheQue(const char *print_name,int layertype, int conid, bool isremove);
	std::map<uint64_t, LOLayerInfoCacheIndex*>::iterator queLayerinfoMapStart(const char *print_name);
	bool HasPrintQue(const char *print_name);

	void ResetViewPort();
	void CaleWindowSize(int scX, int scY, int srcW, int srcH, int dstW, int dstH, SDL_Rect *result);
	void ShowFPS(double postime);
	bool InitFps();
	void ResetConfig();
	void FreeFps();
	LOtextureBase* TextureFromFile(LOLayerInfo *info);
	LOtextureBase* TextureFromColor(LOLayerInfo *info);
	LOtextureBase* TextureFromSimpleStr(LOLayerInfo*info, LOString *s);
	LOtextureBase* TextureFromNSbtn(LOLayerInfo*info, LOString *s);

	void ScaleTextParam(LOLayerInfo *info, LOFontWindow *fontwin);
	LOtextureBase* EmptyTexture(LOString *fn);
	const char* ParseTrans(int *alphaMode, const char *buf);
	void FreeLayerInfoData(LOLayerInfo *info);

	LOLayer* GetLayerOrNew(int fullid);
	void ExportQuequ2(std::unordered_map<int, LOLayerInfoCacheIndex*> *map);
	int ExportQuequ(const char *print_name, LOEffect *ef, bool iswait);
	void DoDelayEvent(double postime);
	void DoPreEvent(double postime);
	void CaptureEvents(SDL_Event *event);
	bool TranzMousePos(int xx, int yy);
	void CutDialogueAction();
};