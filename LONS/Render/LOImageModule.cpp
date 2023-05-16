/*
//图像渲染模块
*/

#include "../etc/LOIO.h"
#include "../etc/LOEvent1.h"
#include "../etc/LOString.h"
#include "../etc/LOTimer.h"
#include "LOImageModule.h"
#include <SDL_image.h>

bool LOImageModule::isShowFps = false;
bool LOImageModule::st_filelog;  //是否使用文件记录
extern void FatalError(const char *fmt, ...);
extern bool st_skipflag;

LOImageModule::LOImageModule(){
	FunctionInterface::imgeModule = this;
	allSpList = NULL;
	allSpList2 = NULL;
	window = NULL;
	render = NULL;
	fpstex = NULL;
	isShowFps = true;
	isFingerEvent = false ;
	//screenshotSu = NULL;

    ResetConfig();

	layerQueMutex = SDL_CreateMutex();
	layerDataMutex = SDL_CreateMutex();
	doQueMutex = SDL_CreateMutex();
	PrintTextureA = PrintTextureB = PrintTextureEdit = nullptr;

	memset(shaderList, 0, sizeof(int) * 20);
}

void LOImageModule::ResetConfig() {
	tickTime = 0;
	trans_mode = LOLayerData::TRANS_TOPLEFT;
	effectSkipFlag = false;
	textbtnFlag = true;
	st_filelog = false;

	spFontName = "default.ttf";
	sayFontName = "default.ttf";
	sayWindow.reset();
	sayState.reset();
	sayStyle.reset();
	sayStyle.xcount = 22;


	textbtnValue = 1;

	btndefStr.clear();
	btnOverTime = 0;
	btnUseSeOver = false;

	if (allSpList) allSpList->clear();
	if (allSpList2) allSpList2->clear();
	allSpList = NULL;
	allSpList2 = NULL;
}

LOImageModule::~LOImageModule(){
	SDL_DestroyMutex(layerQueMutex);
	SDL_DestroyMutex(layerDataMutex);
	SDL_DestroyMutex(doQueMutex);
	FreeFps();
	if (allSpList) delete allSpList;
	if (allSpList2)delete allSpList2;
	if (PrintTextureA) SDL_DestroyTexture(PrintTextureA);
	if (PrintTextureB) SDL_DestroyTexture(PrintTextureB);
	if (PrintTextureEdit) SDL_DestroyTexture(PrintTextureEdit);
}


int LOImageModule::InitImageModule() {
	if (FunctionInterface::scriptModule) scriptModule->GetGameInit(G_gameWidth, G_gameHeight);
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) < 0) {
		SDL_LogError(0, "Couldn't initialize SDL: %s\n", SDL_GetError());
		return 0;
	}
	//初始化时钟
	LOTimer::Init();

	//======初始化窗口==========//
	//获取显示器的分辨率
	SDL_Rect deviceSize;
	if (SDL_GetDisplayBounds(0, &deviceSize) < 0) {
		SDL_Log("SDL_GetDisplayBounds failed: %s", SDL_GetError());
		return 0;
	}

	//windows下默认使用游戏尺寸，其他系统由外部设置
	int winflag;
    //屏幕的类型，0表示PC类，这种平台上默认将以窗口模式运行。1表示移动设备/平板类，这种将以全屏模式运行
    int screenType = 0 ;
#ifdef ANDROID
    screenType = 1;
#endif
    if(screenType == 0){
        if (G_fullScreen) {
            winflag = SDL_WINDOW_FULLSCREEN;
            SDL_GetDisplayBounds(0, &deviceSize);
            G_windowRect.x = SDL_WINDOWPOS_UNDEFINED;
            G_windowRect.y = SDL_WINDOWPOS_UNDEFINED;
        }
        else {
            winflag = SDL_WINDOW_SHOWN;
            if (G_destWidth > 100) deviceSize.w = G_destWidth;
            else deviceSize.w = G_gameWidth;
            if (G_destHeight > 100) deviceSize.h = G_destHeight;
            else deviceSize.h = G_gameHeight;
            G_windowRect.x = SDL_WINDOWPOS_UNDEFINED;
            G_windowRect.y = SDL_WINDOWPOS_UNDEFINED;
        }
    }
    else if(screenType == 1){
        deviceSize.w = G_gameWidth;
        deviceSize.h = G_gameHeight;
        G_windowRect.x = SDL_WINDOWPOS_UNDEFINED;
        G_windowRect.y = SDL_WINDOWPOS_UNDEFINED;
        G_gameRatioW = 1;
        G_gameRatioH = 1;
        winflag = SDL_WINDOW_FULLSCREEN;
    }

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 32);
	//SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	//Use OpenGL 2.1
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

	//SDL_SetHint(SDL_HINT_RENDER_DRIVER, "direct3d11");

	window = SDL_CreateWindow(NULL, G_windowRect.x, G_windowRect.y, deviceSize.w, deviceSize.h, winflag | SDL_WINDOW_OPENGL);
	render = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

	//检查是否支持多重缓冲
	if (SDL_RenderTargetSupported(render) == SDL_FALSE) {
		SDL_LogError(0, "Your device does not support multiple buffering!");
		return 0;
	}

	//检查渲染器的默认格式，不支持默认格式的无法继续运行
	bool checkOK = false;
	SDL_RendererInfo G_renderinfo;
	SDL_GetRendererInfo(render, &G_renderinfo);
	printf("render is %s\n", G_renderinfo.name);
	LOtextureBase::maxTextureW = G_renderinfo.max_texture_width;
	LOtextureBase::maxTextureH = G_renderinfo.max_texture_height;
	LOtextureBase::render = render;

	for (unsigned int ii = 0; ii < G_renderinfo.num_texture_formats; ii++) {
		if (G_renderinfo.texture_formats[ii] == G_Texture_format) {
			checkOK = true;
			break;
		}
	}
	if (!checkOK) {
        SDL_LogError(0, "Your device does not support the specified texture format and cannot continue!");
		return 0;
	}
	//确定默认材质格式的RGBA位置
	GetFormatBit(G_Texture_format, G_Bit);
	SDL_SetRenderDrawColor(render, 0, 0, 0, 255);  //defalt is 0,0,0,0, it will has some problem.

	//初始化ttf
	if (TTF_Init() == -1) {
		SDL_LogError(0, "TTF_Init() faild!");
		return 0;
	}

	ResetViewPort();

	//创建帧缓冲纹理
	PrintTextureA = CreateTexture(render, G_Texture_format, SDL_TEXTUREACCESS_TARGET, G_gameWidth, G_gameHeight);
	PrintTextureB = CreateTexture(render, G_Texture_format, SDL_TEXTUREACCESS_TARGET, G_gameWidth, G_gameHeight);
	PrintTextureEdit = CreateTexture(render, G_Texture_format, SDL_TEXTUREACCESS_STREAMING, G_gameWidth, G_gameHeight);

	titleStr = "ONScripter-LONS " + LOString(ONS_VERSION);
	SDL_SetWindowTitle(window, titleStr.c_str());

 	return 1;
}

void LOImageModule::ResetViewPort() {
	int winWidth, winHeight;
	SDL_GetWindowSize(window, &winWidth, &winHeight);
	CaleWindowSize(G_gameRatioW, G_gameRatioH, G_gameWidth, G_gameHeight, winWidth, winHeight, &G_viewRect);
	//视口尺寸跟游戏尺寸不一致，需要缩放
	//缩放视口以后viewport是按缩放后的大小计算的
	if (G_viewRect.w != G_gameWidth || G_viewRect.h != G_gameHeight) {
		//SDL_RenderSetViewport(renderer, &G_viewRect);
		SDL_RenderSetScale(render, G_gameScaleX, G_gameScaleX);
	}

	SDL_RendererInfo G_renderinfo;
	SDL_GetRendererInfo(render, &G_renderinfo);
	max_texture_height = G_renderinfo.max_texture_height;
	max_texture_width = G_renderinfo.max_texture_height;

	//LOLog_i("G_viewRect is: %d,%d,%d,%d",G_viewRect.x,G_viewRect.y,G_viewRect.w,G_viewRect.h);
	//视口是否位于窗口的其他位置
	SDL_Rect winView;
	winView.x = (int)((double)G_viewRect.x / G_gameScaleX) ;
	winView.y = (int)((double)G_viewRect.y / G_gameScaleY) ;
	winView.w = G_gameWidth;
	winView.h = G_gameHeight;
	SDL_RenderSetViewport(render, &winView);

	//now,we check the result
	SDL_RenderGetViewport(render, &winView);
    if(winView.w != G_gameWidth || winView.h != G_gameHeight) SDL_Log("viewport set error!") ;
	if(  abs( G_gameScaleX * winView.x- G_viewRect.x) > 1 ||
	     abs( G_gameScaleY * winView.y - G_viewRect.y) > 1){
        SDL_Log("viewport not in the expected position, the parameters will be adjusted automatically.") ;
		G_viewRect.x = (int)(G_gameScaleX * winView.x);
		G_viewRect.y = (int)(G_gameScaleY * winView.y); //? is right?
	}

	effectTex.reset(new LOtexture());
	effectTex->CreateDstTexture(G_viewRect.w, G_viewRect.h, SDL_TEXTUREACCESS_TARGET);
	effmakTex.reset();
	FreeFps();
	InitFps();
}

void LOImageModule::CaleWindowSize(int scX, int scY, int srcW, int srcH, int dstW, int dstH, SDL_Rect *result) {
	//使用原始比例
	if (scX == 1 && scY == 1) {
		scX = srcW; scY = srcH;
	}
	//先按比例计算出实际的Y值
	float tyy = (float)scY * srcW / scX;
	int ty = (float)dstW / srcW * tyy;
	//宽度符合要求
	if (ty <= dstH) {
		result->x = 0;
		result->y = (dstH - ty) / 2;
		result->w = dstW;
		result->h = ty;
		G_gameScaleX = (float)dstW / (float)srcW;
		G_gameScaleY = (float)ty / (float)srcH;
	}
	else {
		float txx = (float)scX * srcH / scY;
		int tx = (float)dstH / srcH * txx;
		result->x = (dstW - tx) / 2;
		result->y = 0;
		result->w = tx;
		result->h = dstH;
		G_gameScaleX = (float)tx / (float)srcW;
		G_gameScaleY = (float)dstH / (float)srcH;
	}
}


//渲染主循环在这里实现
int LOImageModule::MainLoop() {
	Uint64 hightTimeNow;
	bool loopflag = true;
	bool reflashNow1 = false;
	//G_fpsnum = 120;
	if (G_fpsnum < 2) G_fpsnum = 2;
	if (G_fpsnum > 120) G_fpsnum = 120;
	double fpstime = 1000.01 / G_fpsnum;
	double posTime;   //从上一帧后，当前帧花费的时间
	SDL_Event ev;
	Uint64 lastTime = 0;
	bool minisize = false;
	
	ChangeModuleState(MODULE_STATE_RUNNING);

	//第一帧要锁住print信号
	printHook->FinishMe();
	printPreHook->FinishMe();

	while (loopflag) {
		hightTimeNow = LOTimer::GetHighTimer();
		posTime = ((double)(hightTimeNow - lastTime)) / LOTimer::perTik64;

		//部分操作要求立即刷新帧，特别是快进模式下
		if(st_skipflag) reflashNow1 = printHook->enterEdit();
		else reflashNow1 = false;

		if (!minisize && (posTime + 0.1 > fpstime || reflashNow1)) {
			//if (posTime < fpstime - 0.1 && reflashNow1) printf("yes\n");
			if (reflashNow1) {
				//printf("yes\n");
				printHook->closeEdit();
			}
			if (RefreshFrame(posTime) == 0) {
				//now do the delay event.like send finish signed.
				DoPreEvent(posTime);
				lastTime = hightTimeNow;
			}
			else {
                SDL_Log("RefreshFrame faild!");
			}
			//if (reflashNow1) reflashNow.store(false);
		}

		//检查模块状态变化
		if (isStateChange()) {
			if (!WaitStateEvent()) loopflag = false;
		}
		
		//处理事件
		if (SDL_PollEvent(&ev)) {
			switch (ev.type) {
			case SDL_QUIT:
				loopflag = false;
				//这个函数要改写
				SetExitFlag(MODULE_FLAGE_EXIT | MODULE_FLAGE_CHANNGE);
				break;
			case SDL_WINDOWEVENT:
				if (ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
					SDL_Log("window size change!\n");
					ResetViewPort();
				}
				else if (ev.window.event == SDL_WINDOWEVENT_MINIMIZED) {
					//最小化必须暂停渲染，不然会导致内存泄漏
					minisize = true;
					SDL_Log("window is be minisize!\n");
				}
				else if (ev.window.event == SDL_WINDOWEVENT_RESTORED) {
					minisize = false;
					SDL_Log("window is be restored!\n");
				}
				break;
			default:
				//封装事件
				CaptureEvents(&ev);
				break;
			}
		}

		//因为有别的线程的事件，所以要放在外部处理事件
		HandlingEvents(false);
		
		//降低CPU的使用率
		if (posTime < fpstime - 1.5) SDL_Delay(1);
		else if (minisize) SDL_Delay(1);
		else {
			int sum = rand();
			for (int ii = 0; ii < 5000; ii++) {
				sum ^= ii;
				if (ii % 2) sum++;
				else sum += 2;
			}
		}
		
	}
	
	//退出前终止效果
	CutPrintEffect(nullptr, nullptr);

	ChangeModuleState(MODULE_STATE_NOUSE);

	//等待其他模块退出
	hightTimeNow = LOTimer::GetHighTimer();
	posTime = 0;
	while (posTime < 2000 && (!scriptModule->isModuleNoUse() || !audioModule->isModuleNoUse())) {
		SDL_Delay(1);
		posTime = LOTimer::GetHighTimeDiff(hightTimeNow);
	}
	//
	//if (scriptModule->isModuleNoUse()) LOLog_i("no use script");
	//if (audioModule->isModuleNoUse()) LOLog_i("not use audio");

    SDL_Log("LONS::MainLoop exit.");
	return -1;
}


int LOImageModule::RefreshFrame(double postime) {

	int lockfalg = 0;
	SDL_LockMutex(layerQueMutex);
	if (lockfalg == 0) {

		//必须小心处理渲染逻辑，不然很容导致程序崩溃，关键是正确处理 print 1 以外的多线程同步问题
		//要判断是否是print x的执行过程，print的第一帧和最后一帧是非常重要的
		LOEffect *ef = nullptr;
		bool isPrinting = !printHook->isStateAfterFinish();
		if(isPrinting) ef = (LOEffect*)printHook->GetParam(0)->GetPtr();

		if (!ef || ef->nseffID == 10 || ef->postime < 0) { //淡入淡出无需特别的处理
			//“ef->postime<0” 说明特效已经运行完成了，但是我们还需要再运行一帧，让图像刷新到纹理A上，这样下一次print才不会出问题

			//printHook处于无效状态，说明是普通的刷新，帧首先刷新到PrintTextureA上，然后再刷新到渲染器上，这是为了print时可以最快速度获取当前的图像
			//在脚本线程展开print队列时，会交换PrintTextureA和PrintTextureB，这样，前台图像就被交换到后台了
			//每一帧刷新前应该使用SDL_RenderClear() 来清空帧
			SDL_SetTextureBlendMode(PrintTextureA, SDL_BLENDMODE_NONE);
			SDL_SetRenderTarget(render, PrintTextureA);
			SDL_SetRenderDrawColor(render, 0, 0, 0, 255);
			SDL_RenderClear(render);
			UpDisplay(postime);

			//如果有淡入淡出效果，再将texB刷新到A上
			if (ef && ef->nseffID == 10 && ef->postime >= 0) {
				SDL_SetTextureBlendMode(PrintTextureB, SDL_BLENDMODE_BLEND);
				SDL_RenderCopy(render, PrintTextureB, nullptr, nullptr);
			}

			//刷新到渲染器上
			SDL_SetRenderTarget(render, nullptr);
			SDL_RenderClear(render);
			SDL_RenderCopy(render, PrintTextureA, nullptr, nullptr);
		}
		else {
			//print特效执行时，帧直接刷新到画布上
			SDL_SetRenderTarget(render, nullptr);
			SDL_SetRenderDrawColor(render, 0, 0, 0, 255);
			SDL_RenderClear(render);
			UpDisplay(postime);

			//第一帧直接覆盖在上方即可
			if (ef->postime == 0) {
				SDL_SetTextureBlendMode(PrintTextureB, SDL_BLENDMODE_NONE);
				SDL_RenderCopy(render, PrintTextureB, nullptr, nullptr);
			}
			else {
				//2.然后texB + edit -> texA, texA的背景设为 0，0，0，0  生成被动态蒙蔽的纹理texA
				SDL_SetTextureBlendMode(PrintTextureEdit, SDL_BLENDMODE_BLEND);
				SDL_SetTextureBlendMode(PrintTextureB, SDL_BLENDMODE_MOD);

				SDL_SetRenderDrawColor(render, 0, 0, 0, 0);
				SDL_SetRenderTarget(render, PrintTextureA);
				SDL_RenderClear(render);
				SDL_RenderCopy(render, PrintTextureEdit, nullptr, nullptr);
				SDL_RenderCopy(render, PrintTextureB, nullptr, nullptr);

				//3.texA再以透明混合的方式刷新到画布上
				SDL_SetRenderTarget(render, nullptr);
				SDL_SetTextureBlendMode(PrintTextureA, SDL_BLENDMODE_BLEND);
				SDL_RenderCopy(render, PrintTextureA, nullptr, nullptr);
			}
		}
		//注意计时的时候，第一帧会需要初始化，需要几十毫秒
		if(isShowFps) ShowFPS(postime);
		SDL_RenderPresent(render);

		SDL_UnlockMutex(layerQueMutex);

		//每完成一帧的刷新，我们检查是否有 print 事件需要处理，如果是print 1则直接通知完成
		//这里有个隐含的条件，在脚本线程展开队列时，绝对不会进入RefreshFrame刷新，所以如果有MSG_Wait_Print表示已经完成print的第一帧刷新
		//如果是print 2-18,我们将检查effect的运行情况
		if (isPrinting) {
			int alpha = 255;
			if (!ef || ef->postime < 0) printHook->FinishMe();  //print 1直接完成
			else if (ef->RunEffect2(PrintTextureEdit, &alpha, postime)) {
				ef->postime = -2; //print 2-8执行完成了，但是还需要运行一帧
			}
			else printHook->closeEdit();

			SDL_SetTextureBlendMode(PrintTextureB, SDL_BLENDMODE_BLEND);
			SDL_SetTextureAlphaMod(PrintTextureB, (Uint8)(alpha & 0xff));
		}
		return 0;
	}
	else if (lockfalg == -1) {
        SDL_Log("fps lock layer faild:%s", SDL_GetError());
	}
	return -1;
}

void LOImageModule::UpDisplay(double postime) {
	tickTime += (int)postime;
	//位于底部的要先渲染
	UpDataLayer(G_baseLayer[LOLayer::LAYER_BG], tickTime, 1023, 0, 0);
	UpDataLayer(G_baseLayer[LOLayer::LAYER_SPRINT], tickTime, 1023, sayState.z_order + 1, 0);
	UpDataLayer(G_baseLayer[LOLayer::LAYER_STAND], tickTime, 1023, 0, 0);
	if (sayState.isWinbak()) {
		UpDataLayer(G_baseLayer[LOLayer::LAYER_OTHER], tickTime, 1023, 0, 0);
		UpDataLayer(G_baseLayer[LOLayer::LAYER_SPRINTEX], tickTime, 1023, 0, 0);
		UpDataLayer(G_baseLayer[LOLayer::LAYER_DIALOG], tickTime, 1023, 0, 0);
		UpDataLayer(G_baseLayer[LOLayer::LAYER_SPRINT], tickTime, sayState.z_order, 0, 0);
		UpDataLayer(G_baseLayer[LOLayer::LAYER_SELECTBAR], tickTime, 1023, 0, 0);
	}
	else {
		UpDataLayer(G_baseLayer[LOLayer::LAYER_SPRINT], tickTime, sayState.z_order, 0, 0);
		UpDataLayer(G_baseLayer[LOLayer::LAYER_SPRINTEX], tickTime, 1023, 0, 0);
		UpDataLayer(G_baseLayer[LOLayer::LAYER_OTHER], tickTime, 1023, 0, 0);
		UpDataLayer(G_baseLayer[LOLayer::LAYER_SELECTBAR], tickTime, 1023, 0, 0);
		UpDataLayer(G_baseLayer[LOLayer::LAYER_DIALOG], tickTime, 1023, 0, 0);
	}
	//UpDataLayer(lonsLayers[LOLayer::LAYER_BUTTON], tickTime, 1023, 0);
	UpDataLayer(G_baseLayer[LOLayer::LAYER_NSSYS], tickTime, 1023, 0, 0);
}

void LOImageModule::UpDataLayer(LOLayer *layer, Uint32 curTime, int from, int dest, int level) {
	if (!layer->childs || layer->childs->size() == 0) return;

	//if (layer == lonsLayers[LOLayer::LAYER_DIALOG]) {
	//	int debugbreak = 1;
	//}
	//注意是从后往前刷新
	auto iter = layer->childs->rbegin();
	while (iter != layer->childs->rend() && iter->second->id[0] > from) iter++;
	while (iter != layer->childs->rend()) {
		LOLayer *lyr = iter->second;
		if (lyr->id[0] < dest) break;

		//debug
		//if (lyr->layerType == LOLayer::LAYER_NSSYS && lyr->id[0] == LOLayer::IDEX_NSSYS_EFFECT + 1) {
		//	int bbk = 1;
		//}

		//在下方的对象先渲染，渲染父对象
		if (lyr->data->cur.texture) {
			//运行图层的action
			if (lyr->data->cur.actions) {
				lyr->DoAction(lyr->data.get(), curTime);
			}
			//if (!lyr->curInfo->texture->isNull()) {
			if (lyr->data->cur.texture->isAvailable()) {
				lyr->ShowMe(render);
			}
		}

		//if(lyr->id[0] == 20) LONS::printError("%d",lyr->childs.size());
		//渲染子对象
		if (lyr->childs && lyr->childs->size() > 0) {
			if (lyr->isVisible() || lyr->isChildVisible()) UpDataLayer(lyr, curTime, 1023, 0, level + 1);
		}

		iter++;
	}
}

void LOImageModule::PrepareEffect(LOEffect *ef, const char *printName) {
	//Target的纹理是不能编辑的，但是专门构造一个特殊纹理，让effect纹理与特殊纹理叠加产生特殊效果
	//SDL_BLENDMODE_MOD  dstRGB = srcRGB * dstRGB dstA = dstA
	//简单来说就是创建一个动态遮片。对于渐变来说，PrintTextureB设置透明度即可，



	//缩放模式  
	//if (IsGameScale()) {
	//	info->bak.SetPosition2(0, 0, 1.0 / G_gameScaleX, 1.0 / G_gameScaleY);
	//}

	//加载遮片
	if (ef->nseffID == 15 || ef->nseffID == 18) {
		LOtextureBase *su = SurfaceFromFile(&ef->mask);
		//遮片不存在按渐变处理
		if (!su) ef->nseffID = 10;
		else {
			SDL_Surface *tmp = ef->Create8bitMask(su->GetSurface(), true);
			ef->masksu.reset(new LOtextureBase(tmp));
			delete su;
		}
	}
}

//完成了返回true, 否则返回false
bool LOImageModule::ContinueEffect(LOEffect *ef, const char *printName, double postime) {
	if (ef) { //print 2-8
		int fullid = GetFullID(LOLayer::LAYER_NSSYS, LOLayer::IDEX_NSSYS_EFFECT, 255, 255);
		LOLayer *lyr = LOLayer::FindLayerInCenter(fullid);
		//maybe has some error!
		if (!lyr) return true;
		//if (ef->RunEffect2(render, lyr->data.get(), PrintTextureEdit, postime)) {
		//	//断开图层连接
		//	if (lyr->parent) lyr->parent->RemodeChild(lyr->GetSelfChildID());
		//	//重置数据
		//	lyr->data->bak.SetDelete();
		//	lyr->data->cur.SetDelete();
		//	return true;
		//}
		//else return false;
		return true;
	}

	return true; //print 1
}


void LOImageModule::ShowFPS(double postime) {
	if (!fpstex && !InitFps()) {
		isShowFps = false;
		return;
	}
	if (postime <= 0) return;

	//fps显示不需要缩放
	int tt = (int)postime;
	int fps = 1000 / postime;

	SDL_Rect rect = { 0,0,0,0 };
	rect.x = fpstex[0].baseW() * 3 + 5; rect.y = 5;
	while (fps > 0) {
		int ii = fps % 10;

		rect.w = (int)((double)fpstex[ii].baseW() / G_gameScaleX);
		rect.h = (int)((double)fpstex[ii].baseH() / G_gameScaleX);
		auto ittt = fpstex[ii].GetTexture();

		////LOLog_i("%d fps number!", ii) ;

		SDL_RenderCopy(render, fpstex[ii].GetTexture(), NULL, &rect);
		rect.x -= rect.w;
		fps = fps / 10;
	}
}

void LOImageModule::FreeFps() {
	delete[] fpstex;
	fpstex = nullptr ;
}

bool LOImageModule::InitFps() {
	fpstex = new LOtexture[10];
	SDL_Color cc = { 0, 255, 0 };
	LOTextStyle fpsStyle = spStyle;
	fpsStyle.xsize = fpsStyle.ysize = G_viewRect.h * 14 / 400;

	for (int ii = 0; ii <= 9; ii++) {
		LOString s = std::to_string(ii);
		fpstex[ii].CreateTextDescribe(&s, &spStyle, &spFontName);
		int w = 0;
		int h = 0;
		fpstex[ii].GetTextSurfaceSize(&w, &h);
		fpstex[ii].CreateSurface(w, h);
		fpstex[ii].RenderTextSimple(0, 0, cc);
		SDL_Rect rect = { 0,0,w,h };
		fpstex[ii].activeTexture(&rect, true);
	}
	return true;
}


void LOImageModule::ScaleTextParam(LOLayerData *info, LOTextStyle *fontwin) {
	fontwin->xsize = G_gameScaleX * fontwin->xsize;
	fontwin->ysize = G_gameScaleX * fontwin->ysize;
	fontwin->xspace = G_gameScaleX * fontwin->xspace;
	fontwin->yspace = G_gameScaleX * fontwin->yspace;
	info->bak.SetPosition2(0, 0, 1.0 / G_gameScaleX, 1.0 / G_gameScaleY);
}


//做一些准备工作，以便更好的加载
bool LOImageModule::ParseTag(LOLayerDataBase *bak, LOString *tag) {
	const char *buf = tag->c_str();
	int alphaMode = trans_mode;
	//LOLog_i("tag is %s",tag->c_str()) ;
	buf = ParseTrans(&alphaMode, tag->c_str());
	if (alphaMode == LOLayerData::TRANS_STRING) {
		bak->SetTextureType(LOtexture::TEX_SIMPLE_STR);
		bak->keyStr.reset(new LOString(buf, tag->GetEncoder()));
	}
	else {
		buf = tag->SkipSpace(buf);
		if (buf[0] == '>') {
			buf = tag->SkipSpace(buf + 1);
			bak->SetTextureType(LOtexture::TEX_COLOR_AREA);
			bak->keyStr.reset(new LOString(buf, tag->GetEncoder()));
		}
		else if (buf[0] == '*') {
			buf++;
			if (buf[0] == 'd') {
				bak->SetTextureType(LOtexture::TEX_DRAW_COMMAND);
			}
			else if (buf[0] == 's') {
				bak->SetTextureType(LOtexture::TEX_ACTION_STR);
				bak->SetAlphaMode(LOLayerData::TRANS_COPY);
			}
			else if (buf[0] == 'S') {
				bak->SetTextureType(LOtexture::TEX_MULITY_STR);
				bak->SetAlphaMode(LOLayerData::TRANS_COPY);
			}
			else if (buf[0] == '>') {
				bak->SetTextureType(LOtexture::TEX_COLOR_AREA);
				bak->SetAlphaMode(LOLayerData::TRANS_COPY);
				//LOLog_i("usecache is %c",info->usecache) ;
			}
			else if (buf[0] == 'b') {
				//"*b;50,100,内容" ;绘制一个NS样式的按钮，通常模式时只显示文字，鼠标悬停时显示变色文字和有一定透明度的灰色
				bak->SetTextureType(LOtexture::TEX_NSSIMPLE_BTN);
				bak->SetAlphaMode(LOLayerData::TRANS_COPY);
			}
			else if (buf[0] == '*') {
				//操作式纹理
				//** 空纹理，基本上只是为了挂载子对象
				bak->SetTextureType(LOtexture::TEX_CONTROL);
			}
			else if (buf[0] == 'v') {
				bak->SetTextureType(LOtexture::TEX_VIDEO);
				bak->SetAlphaMode(LOLayerData::TRANS_COPY);
			}
			else if (buf[0] >= '0' && buf[0] <= '9') {//layer模式
				bak->SetTextureType(LOtexture::TEX_LAYERMODE);
				bak->btnval = tag->GetInt(buf);
			}
			//非layer模式继续
			if (!bak->isTexType(LOtexture::TEX_LAYERMODE)) {
				buf += 2;
				bak->keyStr.reset(new LOString(buf, tag->GetEncoder()));
			}
		}
		else {
			bak->SetTextureType(LOtexture::TEX_IMG);
			bak->SetAlphaMode(alphaMode);
			ParseImgSP(bak, tag, buf);
		}
	}
	return true;
}


const char* LOImageModule::ParseTrans(int *alphaMode, const char *buf) {
	//const char *obuf = buf;
	if (buf[0] == ':') {
		buf++;
		while (buf[0] == ' ')buf++;
		if (buf[0] == 'b')buf++;  //看起来LONS不需要支持
		if (buf[0] == 'f')buf++;  //只加载某一区域
		if (buf[0] == 'a') {
			*alphaMode = LOLayerData::TRANS_ALPHA;
			buf++;
		}
		else if (buf[0] == 'l') {  //左上角
			*alphaMode = LOLayerData::TRANS_TOPLEFT;
			buf++;
		}
		else if (buf[0] == 'r') {  //右上角
			*alphaMode = LOLayerData::TRANS_TOPRIGHT;
			buf++;
		}
		else if (buf[0] == 'c') {  //不使用透明
			*alphaMode = LOLayerData::TRANS_COPY;
			buf++;
		}
		else if (buf[0] == 's') {  //string
			*alphaMode = LOLayerData::TRANS_STRING;
			buf++;
		}
		else if (buf[0] == 'm') {  //遮片
			*alphaMode = LOLayerData::TRANS_MASK;
		}
		else if (buf[0] == '#') { //指定颜色
			*alphaMode = LOLayerData::TRANS_DIRECT;
		}
		else if (buf[0] == '!') { //画板
			*alphaMode = LOLayerData::TRANS_PALLETTE;
		}
		if (buf[0] == ';') buf++;
	}
	return buf;
}

bool LOImageModule::ParseImgSP(LOLayerDataBase *bak, LOString *tag, const char *buf) {
	buf = tag->SkipSpace(buf);
	if (buf[0] == '/') {
		LOActionNS *anim = new LOActionNS;
		LOShareAction ac(anim);
		//检查一下参数是否合法
		buf++;
		anim->cellCount = tag->GetInt(buf);
		if (anim->cellCount == 0) {
			SDL_Log("Animation grid number cannot be 0!\n");
			return false;
		}
		bak->SetAction(ac);

		//读取每格的时间
		//有逗号表示继续对时间进行描述
		if (buf[0] == ',') {
			buf++;
			if (buf[0] == '<') { //对每格时间进行描述 lsp 150,":a/10,<100 200 300 400 500 600 700 800 900 1000>,0;data\erk.png",69,113
				buf++;
				for (int ii = 0; ii < anim->cellCount; ii++) {
					anim->cellTimes.push_back(tag->GetInt(buf));
					buf++;
				}
			}
			else {  //只有一格数表示每格的时间是一样的
				int temp = tag->GetInt(buf);
				for (int ii = 0; ii < anim->cellCount; ii++)
					anim->cellTimes.push_back(temp);
			}
			buf++;
			anim->loopMode = (LOAction::AnimaLoop)(buf[0] - '0'); // 3...no animation
		}
		else { //表示只划分格，不做动画
			for (int ii = 0; ii < anim->cellCount; ii++) 
				anim->cellTimes.push_back(0);
			anim->loopMode = LOAction::LOOP_CELL;
		}
		anim->cellCurrent = 0; //总是指向马上要执行的帧
		anim->cellForward = 1; //1或者-1
		while (buf[0] != ';' && buf[0] != '\0') buf++;
	}
	if (buf[0] == ';') buf++;

	bak->keyStr.reset(new LOString(buf, tag->GetEncoder()));
	return true ;
}


void LOImageModule::ClearBtndef() {
	//由渲染线程进行必要的清理
	LOShareEventHook ev(LOEventHook::CreateBtnClearEvent(0));


	imgeModule->waitEventQue.push_N_back(ev);

	while (!ev->isInvalid()) LOTimer::CpuDelay(0.5);
	return;
}



//tag定义：
//":a/10,400,0;map\ais.png" 普通图像sp
//":s/40,40,0;#00ffff#ffff00$499" 文字sp
//":c;>50,50,#ff00ff" 色块sp
//"*d;rect,50,100" 绘图sp
//"*s;$499" 对话框文字sp，特效跟随setwindow的值
//"*S;文本" 多行sp
//"*>;50,100,#ff00ff#ffffff" 绘制一个色块，并使用正片叠底模式
//"**;_?_empty_?_"  操作式纹理，比如_?_empty_?_表示空纹理， _?_effect_?_ 表示print时的效果纹理
//"*v/200,100;mov\test.mpg"  加载一个视频
bool LOImageModule::loadSpCore(LOLayerData *info, LOString &tag, int x, int y, int alpha, bool visiable) {
	bool ret = loadSpCoreWith(&info->bak, tag, x, y, alpha, 0);
	info->bak.SetVisable(visiable);
	return ret;
}

//给图层在读取存档的时候调用
bool ImgLoadSpForce(LOLayerDataBase *cur, LOString *tag) {
	LOImageModule *img = (LOImageModule*)FunctionInterface::imgeModule;
	return img->loadSpCoreWith(cur, *tag, 0, 0, -1, 0);
}

bool LOImageModule::loadSpCoreWith(LOLayerDataBase *bak, LOString &tag, int x, int y, int alpha, int eff) {
	bak->buildStr.reset(new LOString(tag));

	bak->SetShowType(LOLayerDataBase::SHOW_NORMAL); //简单模式
	ParseTag(bak, &tag);

	GetUseTextrue(bak, nullptr, true);
	//空纹理不参与下面的设置了，以免出现问题
	if (!bak->texture) return false;

	bak->SetPosition(x, y);
	bak->SetDefaultShowSize();
	bak->SetAlpha(alpha);

	//记录
	//queLayerMap[info->fullid] = NULL;
	return true;
}



void LOImageModule::GetUseTextrue(LOLayerDataBase *bak, void *data, bool addcount) {
	if (!bak->isCache()) {
		//唯一性纹理
		std::unique_ptr<LOString> tstr = std::move(bak->keyStr);
		switch (bak->texType){
		case LOtexture::TEX_SIMPLE_STR:
			TextureFromSimpleStr(bak, tstr.get());
			break;
		case LOtexture::TEX_CONTROL:
			TextureFromControl(bak, tstr.get());
			break;
		case LOtexture::TEX_ACTION_STR:
			TextureFromActionStr(bak, tstr.get());
			break;
        case LOtexture::TEX_MULITY_STR:
            TextureFromStrspLine(bak, tstr.get());
            break ;
		case LOtexture::TEX_COLOR_AREA:
			TextureFromColor(bak, tstr.get());
			break;
		case LOtexture::TEX_VIDEO:
			TextureFromVideo(bak, tstr.get());
			break;
		case LOtexture::TEX_LAYERMODE:
			//还没有完成 work here
			break;
		default:
			LOString errs = StringFormat(128, "ONScripterImage::GetUseTextrue() unkown Textrue type:%d", bak->texType);
			FatalError(errs.c_str());
			break;
		}
	}
	else {
		//只有图像才使用纹理缓存
		TextureFromCache(bak);
		if (!bak->texture) {
			if (bak->texType == LOtexture::TEX_IMG) {
				TextureFromFile(bak);
			}
			else {
                SDL_LogError(0, "ONScripterImage::GetUseTextrue() unkown Textrue type:%d", bak->texType);
			}
		}
	}
}

//单行文字

void LOImageModule::TextureFromSimpleStr(LOLayerDataBase *bak, LOString *s) {
	//拷贝一份样式
	LOTextStyle style = spStyle;
	const char *buf = s->c_str();

	//基础设置
	while (buf[0] == '/' || buf[0] == ',' || buf[0] == ' ') buf++;
	style.xsize = s->GetInt(buf);
	while (buf[0] == ',' || buf[0] == ' ') buf++;
	style.ysize = s->GetInt(buf);
	while (buf[0] == ',' || buf[0] == ' ') buf++;
    if (s->GetInt(buf)) style.SetFlags(LOTextStyle::STYLE_SHADOW);
    else style.UnSetFlags(LOTextStyle::STYLE_SHADOW);

	if (buf[0] == ',') { //what?
		buf++;
		style.xspace = s->GetInt(buf);
	}
	if (buf[0] == ',') { //what?
		buf++;
		style.yspace = s->GetInt(buf);
	}
	while (buf[0] == ',') {  //看着老ons源码有这个，不知道是啥
		buf++;
		s->GetInt(buf);
	}
	while (buf[0] == ';' || buf[0] == ' ') buf++;

	//颜色数
	std::vector<SDL_Color> colorList;
	while (buf[0] == '#') {
		buf++;
		int color = s->GetHexInt(buf, 6);
		SDL_Color cc;
		cc.r = (color >> 16) & 0xff;
		cc.g = (color >> 8) & 0xff;
		cc.b = color & 0xff;
		colorList.push_back(cc);
	}
	if (colorList.size() == 0) return ;

	LOShareTexture texture(new LOtexture());
	LOString text(buf);
	text.SetEncoder(s->GetEncoder());
	
	//先创建文字描述
	int w, h;
	if (!texture->CreateTextDescribe(&text, &style, &spFontName)) return;

	texture->GetTextSurfaceSize(&w, &h);
	if (w <= 0 || h <= 0) return;

	texture->CreateSurface(w * colorList.size(), h);

	//将文本渲染到纹理上
	for (int ii = 0; ii < colorList.size(); ii++) {
		texture->RenderTextSimple(w * ii, 0, colorList.at(ii));
	}
	//单字和文字区域已经不需要了
	texture->textData->ClearTexts();
	texture->textData->ClearWords();
	//如果是多颜色的，生成ns动画
	if (colorList.size() > 1) {
		LOActionNS *ac = new LOActionNS();
		ac->cellCount = colorList.size();
		//不会进入动画检测
		ac->setEnble(false);
		bak->SetAction(ac);
	}

	bak->SetNewFile(texture);
	return ;
}


//用于对话的
void LOImageModule::TextureFromActionStr(LOLayerDataBase *bak, LOString *s) {
	LOShareTexture texture(new LOtexture());
	//先创建文字描述
	int w, h;
    if (!texture->CreateTextDescribe(s, &sayStyle, &sayFontName)) return;
	texture->GetTextSurfaceSize(&w, &h);
	if (w <= 0 || h <= 0) return;
	texture->CreateSurface(w, h);
	//默认是白色
	SDL_Color cc = { 255,255,255,255 };
	texture->RenderTextSimple(0, 0, cc);
	//单字和文字区域已经不需要了
	texture->textData->ClearTexts();
	texture->textData->ClearWords();

	//LOString ss("test.bmp");
	//texture->SaveSurface(&ss);

	//添加action动画
	bak->SetNewFile(texture);
	LOActionText *ac = new LOActionText();
	ac->setFlags(LOAction::FLAGS_INIT);
	bak->SetAction(ac);
}

//用于strsp的
void LOImageModule::TextureFromStrspLine(LOLayerDataBase *bak, LOString *s){
    LOShareTexture texture(new LOtexture());
    int w, h;
    if (!texture->CreateTextDescribe(s, &strspStyle, &spFontName)) return;
    texture->GetTextSurfaceSize(&w, &h);
    if (w <= 0 || h <= 0) return;
    //获取目标颜色值
    SDL_Color color[3];
    int count = 0 ;
    const char *buf = bak->btnStr->c_str();
    while(buf[0] != 0 && count < 3){
        if(buf[0] == '#') buf++ ;
        int val = bak->btnStr->GetHexInt(buf, 6);
        color[count].r = (val >> 16) & 0xff;
        color[count].g = (val >> 8) & 0xff;
        color[count].b = val & 0xff;
        color[count].a = 255 ;
        count++ ;
    }
    //默认值
    if(count == 0){
        count = count + 1;
        color[0] = {255,255,255,255};
    }
    texture->CreateSurface(w * count, h);
    for(int ii = 0 ; ii < count; ii++){
        texture->RenderTextSimple(w * ii, 0, color[ii]);
    }
    //单字和文字区域已经不需要了
    texture->textData->ClearTexts();
    texture->textData->ClearWords();
    bak->btnStr.reset();

    bak->SetNewFile(texture);
}

//从标记中生成色块
void LOImageModule::TextureFromColor(LOLayerDataBase *bak, LOString *s) {
	const char *buf = s->SkipSpace(s->c_str());

	int w = s->GetInt(buf);
	while (buf[0] == ',' || buf[0] == ' ') buf++;
	int h = s->GetInt(buf);

	while (buf[0] != '#' && buf[0] != '\0') buf++;
	//无效的
	if (buf[0] != '#') return;
	buf++;
	int color = s->GetHexInt(buf, 6);
	SDL_Color cc;
	cc.r = (color >> 16) & 0xff;
	cc.g = (color >> 8) & 0xff;
	cc.b = color & 0xff;
	cc.a = 255;

	LOShareTexture texture(new LOtexture());
	bak->SetNewFile(texture);
	texture->CreateSimpleColor(w, h, cc);
	return ;
}

void LOImageModule::TextureFromVideo(LOLayerDataBase *bak, LOString *s) {
	const char *buf = s->SkipSpace(s->c_str());
	bak->showWidth = s->GetInt(buf);
	while (buf[0] == ',' || buf[0] == ' ') buf++;
	bak->showHeight = s->GetInt(buf);
	while (buf[0] == ',' || buf[0] == ' ') buf++;
	LOString sufix = s->GetWordStill(buf, LOCodePage::CHARACTER_LETTER);
	while (buf[0] == ';' || buf[0] == ' ') buf++;
	buf = s->SkipSpace(buf);
	//文件名，注意不支持读取封包内的视频
	LOString fn(buf);
	fn.SetEncoder(s->GetEncoder());
	LOIO::GetPathForRead(fn);

	LOActionMovie *m = new LOActionMovie();
	LOShareAction video(m);
	char *errs = m->InitSmpeg(fn.c_str());
	if (errs) {
		FatalError("LOImageModule::TextureFromVideo() faild:%s", errs);
		return;
	}
	bak->SetAction(video);

	LOShareTexture tex(new LOtexture());
	tex->setEmpty(bak->showWidth, bak->showHeight);
	bak->SetNewFile(tex);
	return;
}

void LOImageModule::TextureFromControl(LOLayerDataBase *bak, LOString *s) {
	LOShareTexture texture;
	if (*s == "_?_empty_?_") {
		texture.reset(new LOtexture());
		texture->setEmpty(G_gameWidth, G_gameHeight);
	}
	//else if (*s == "_?_effect_?_") texture = PrintTextureB;
	//else if (*s == "_?_effect_mask_?_") texture = PrintTextureEdit;
	bak->SetNewFile(texture);
}

void LOImageModule::TextureFromFile(LOLayerDataBase *bak) {
    //bool useAlpha;
	LOShareBaseTexture base(SurfaceFromFile(bak->keyStr.get()));
	if (!base) return ;

	//转换透明格式 
	if (bak->alphaMode != LOLayerData::TRANS_COPY && !base->hasAlpha()) {
		if (bak->alphaMode == LOLayerData::TRANS_ALPHA && !base->ispng) {
			base->SetSurface(LOtextureBase::ConverNSalpha(base->GetSurface(), bak->GetCellCount()));
            if(!base->isValid()) SDL_Log("Conver image ns alhpa faild: %s", bak->keyStr->c_str());
		}
        else if(bak->alphaMode == LOLayerData::TRANS_TOPLEFT){
            SDL_Surface *su = LOtexture::CreateTransAlpha(base->GetSurface(), 1) ;
            if(su) base->SetSurface(su) ;
        }
        else if(bak->alphaMode == LOLayerData::TRANS_TOPRIGHT){
            SDL_Surface *su = LOtexture::CreateTransAlpha(base->GetSurface(), 2) ;
            if(su) base->SetSurface(su) ;
        }
	}

	LOString s = bak->keyStr->toLower() + "?" + std::to_string(bak->alphaMode) + ";";
	LOtexture::addTextureBaseToMap(s, base);

	LOShareTexture texture(new LOtexture(base));
	bak->SetNewFile(texture);
	bak->keyStr.reset(new LOString(s));
	return ;
}


void LOImageModule::TextureFromCache(LOLayerDataBase *bak) {
	LOString s = bak->keyStr->toLower() + "?" + std::to_string(bak->alphaMode) + ";";
	LOShareBaseTexture base = LOtexture::findTextureBaseFromMap(s);
	if (base) {
		LOShareTexture texture(new LOtexture(base));
		bak->SetNewFile(texture);
		bak->keyStr.reset(new LOString(s));
	}
}


LOtextureBase* LOImageModule::SurfaceFromFile(LOString *filename) {
//	if(filename->length() == 0){
//		int bbk = 1 ;
//	}

	std::unique_ptr<BinArray> bin(FunctionInterface::fileModule->ReadFile(filename));
	if (!bin) return nullptr;
	LOtextureBase *base = new LOtextureBase(bin->bin, bin->Length());
	if (!base->isValid()) {
		delete base;
		FatalError("LOImageModule::SurfaceFromFile() unsupported file format!");
		return nullptr;
	}
	return base;
}


LOtextureBase* LOImageModule::EmptyTexture(LOString *fn) {
	LOtextureBase *tx = new LOtextureBase();
	LOtexture::addTextureBaseToMap(*fn, tx);
	return tx;
}



LOEffect* LOImageModule::GetEffect(int id) {
	if (id < 2) return NULL;
	LOEffect *ef = new LOEffect;
	ef->id = id;
	//寻找可以匹配的特性
	auto iter = effectMap.find(ef->id);
	if (iter != effectMap.end()) {
		ef->nseffID = iter->second->nseffID;
		ef->time = iter->second->time;
		ef->mask.assign(iter->second->mask);
	}
	else {
		ef->nseffID = id;
		ef->time = 20; //给予一个最小的时间
	}
	//
	if (ef->nseffID < 0 || ef->nseffID > 18) return nullptr;  //无效的特效

	//if (ef->id <= 18) {
	//	ef->nseffID = ef->id;
	//	ef->time = 20; //给予一个最小的时间
	//}
	//else {
	//	auto iter = effectMap.find(ef->id);
	//	if (iter == effectMap.end()) {
	//		delete ef;
	//		ef = NULL;
	//	}
	//	else {
	//		ef->nseffID = iter->second->nseffID;
	//		ef->time = iter->second->time;
	//		ef->mask.assign(iter->second->mask);
	//	}
	//}
	return ef;
}


void LOImageModule::EnterTextDisplayMode(bool force) {
	//TextDispaly在DialogWindowSet中自动设置
	if (!sayState.isTextDispaly() || force) {
		DialogWindowSet(1, 1, 1);
	}
}


void LOImageModule::LeveTextDisplayMode(bool force) {
	//增加TextDispaly模式，减少频繁判断
	if (sayState.isTextDispaly() && (force || sayState.isPrinHide())) {
		DialogWindowSet(0, 0, 0);
	}
}

//控制对话框和文字的显示状态，负数表示不改变，0隐藏，1显示
void LOImageModule::DialogWindowSet(int showtext, int showwin, int showbmp) {
	int fullid, index, haschange = 0;
	//文字显示
	if (showtext >= 0) {
		fullid = GetFullID(LOLayer::LAYER_DIALOG, LOLayer::IDEX_DIALOG_TEXT, 255, 255);
		if (SetLayerShow(showtext, fullid, "_lons")) haschange |= 1;
		//注意检查是否文字已经被改变
		if (sayState.FLAGS_TEXT_CHANGE) haschange |= 1;
	}

	//对话框显示
	if (showwin >= 0) {
		fullid = GetFullID(LOLayer::LAYER_DIALOG, LOLayer::IDEX_DIALOG_WINDOW, 255, 255);
		if (showwin > 0 && sayState.isWindowChange()) {
			LoadDialogWin();
			haschange |= 2;
		}

		if (SetLayerShow(showwin, fullid, "_lons")) haschange |= 2;
	}
	//文字后的符号显示
	if (haschange > 1) {
		DialogWindowPrint();
		sayState.unSetFlags(LOSayState::FLAGS_WINDOW_CHANGE);
		sayState.unSetFlags(LOSayState::FLAGS_TEXT_CHANGE);
	}
	else if (haschange == 1) {
		ExportQuequ("_lons", NULL, true);
		sayState.unSetFlags(LOSayState::FLAGS_TEXT_CHANGE);
	}

	//只有对话框和文字同时隐藏才是离开文字模式
	if (showtext == 0 && showwin == 0) sayState.unSetFlags(LOSayState::FLAGS_TEXT_DISPLAY);
	else sayState.setFlags(LOSayState::FLAGS_TEXT_DISPLAY);
	return;
}

//加载对话框
bool LOImageModule::LoadDialogWin() {
	int fullid = GetFullID(LOLayer::LAYER_DIALOG, LOLayer::IDEX_DIALOG_WINDOW, 255, 255);
	if ( sayWindow.winstr.length() > 0) {
		LOLayerData *info = CreateNewLayerData(fullid, "_lons");
		loadSpCore(info, sayWindow.winstr, sayWindow.x + sayStyle.xruby, sayWindow.y + sayStyle.yruby, 255, true);
		if (info->bak.texType == LOtexture::TEX_COLOR_AREA) {
			if (info->bak.texture) {
				info->bak.texture->setAplhaModel(-1);
				info->bak.texture->setBlendModel(SDL_BLENDMODE_MOD);
				info->bak.texture->setColorModel(255, 255, 255);
			}
		}
	}
	else {
		LOLayerData *info = CreateNewLayerData(fullid, "_lons");
		info->bak.SetDelete();
	}
	return true;
}

//修改图层的显示状态，如果创建了后台队列，则返回真，否则返回假
//强调的是当前显示状态的改变
bool LOImageModule::SetLayerShow(bool isVisi, int fullid, const char *printName) {
	LOLayerData *info = CreateLayerBakData(fullid, printName);
	if (!info) return false;
	if (info->bak.isNewFile()) {
		if (info->bak.isVisiable() == isVisi) return false;
		info->bak.SetVisable(isVisi);
		return true;
	}
	else {
		if (info->cur.isVisiable() == isVisi) return false;
		info->bak.SetVisable(isVisi);
		return true;
	}
}



void LOImageModule::DialogWindowPrint() {
	LOEffect *ef = nullptr;
	if (sayState.winEffect) {
		ef = new LOEffect;
		ef->CopyFrom(sayState.winEffect.get());
		ExportQuequ("_lons", ef, true);
		delete ef;
	}
	else ExportQuequ("_lons", nullptr, true);
}

//文字显示进入队列
LOActionText* LOImageModule::LoadDialogText(LOString *s, int pageEnd, bool isAdd) {
	int fullid = GetFullID(LOLayer::LAYER_DIALOG, LOLayer::IDEX_DIALOG_TEXT, 255, 255);
	LOLayerData *info = CreateNewLayerData(fullid, "_lons");

	//添加模式需要获取上一次的位置
	int lastPos = 0;

	if (isAdd) {
		if (info->cur.texture) {
			lastPos = info->cur.texture->GetTextTextureEnd();
		}
	}

	LOString tag = "*s;" + (*s);
	tag.SetEncoder(s->GetEncoder());

    int destY = sayWindow.textY ;
    if(sayStyle.isRubyOn() || sayStyle.isRubyLine() ) destY += sayStyle.yruby;  //ruby模式下文字的位置要增加
    loadSpCore(info, tag, sayWindow.textX, destY, 255, true);
	if (!info->bak.texture) return nullptr ;

	info->bak.texture->isEdit = true;
	info->bak.texture->setFlags(LOtexture::USE_TEXTACTION_MOD);

	LOActionText *ac = (LOActionText*)info->bak.GetAction(LOAction::ANIM_TEXT);
	ac->initPos = lastPos;
	if (st_skipflag) ac->initPos = 0x7fff;
	//这个速度是每ms应该前进的像素，一个字的像素/时间
	ac->perPix = (double)sayStyle.xsize / G_textspeed;
	ac->setFlags(LOAction::FLAGS_INIT);
	ac->hook.reset(LOEventHook::CreateTextHook(pageEnd, 0));
	
	//文字已经被改变
	sayState.setFlags(LOSayState::FLAGS_TEXT_CHANGE);
	return ac;
}




//创建新图层，会释放原来的老的数据，lsp使用
LOLayerData* LOImageModule::CreateNewLayerData(int fullid, const char *printName) {
	LOLayer *lyr = LOLayer::CreateLayer(fullid);
	lyr->data->bak.SetDelete();
	//去除setdelete的标记，不去除的话某些情况下会导致图层被删除
	//lyr->data->bak.flags = 0;
	auto *map = GetPrintNameMap(printName)->map;
	(*map)[fullid] = lyr;
	return lyr->data.get();
}

//获取信息或者操作原来的对象
LOLayerData* LOImageModule::CreateLayerBakData(int fullid, const char *printName) {
	//首先搜索是否已经在队列中
	auto *map = GetPrintNameMap(printName)->map;
	auto iter = map->find(fullid);
	if (iter != map->end()) return iter->second->data.get();
	//不在的检查是否有图层
	LOLayer *lyr = LOLayer::FindLayerInCenter(fullid);
	if (!lyr) return nullptr;
	//有图层的添加进队列
	(*map)[fullid] = lyr;
	return lyr->data.get();
}


LOLayerData* LOImageModule::GetLayerData(int fullid, const char *printName) {
	//优先搜索已存在的图层
	LOLayer *lyr = LOLayer::FindLayerInCenter(fullid);
	if (lyr) return lyr->data.get();
	//不存在的图层是无法操作也无法获取信息的
	return nullptr;
}


//专为[btn]命令设计的 
LOLayerData* LOImageModule::CreateBtnData(const char *printName) {
	auto *map = GetPrintNameMap(printName)->map;
	//挑一个空白的
	for (int ii = BtndefCount; ii < G_maxLayerCount[1]; ii++) {
		int fid = GetFullID(LOLayer::LAYER_NSSYS, LOLayer::IDEX_NSSYS_BTN, ii, 255);
		LOLayer *lyr = LOLayer::CreateLayer(fid);
		if (!lyr->data->bak.isNewFile()) {
			(*map)[fid] = lyr;
			return lyr->data.get();
		}
	}
    SDL_LogError(0, "LOImageModule::CreateBtnData() no empty btn slot!");
	return nullptr;
}


LOImageModule::PrintNameMap* LOImageModule::GetPrintNameMap(const char *printName) {
	for (int ii = 0; ii < backDataMaps.size(); ii++) {
		if (backDataMaps[ii]->mapName->compare(printName) == 0) return backDataMaps[ii].get();
	}
	backDataMaps.push_back(std::make_unique<PrintNameMap>(printName));
	return backDataMaps[backDataMaps.size() - 1].get();
}

//void LonsSaveImageModule(BinArray *bin) {
//	LOImageModule *img = (LOImageModule*)FunctionInterface::imgeModule;
//	img->SerializePrintQue(bin);
//	img->SerializeState(bin);
//}
void LOImageModule::Serialize(BinArray *bin) {
    //存储状态，状态先存储的，因为读取的时候有些图层是依赖状态的
    SerializeState(bin);
	//存储图层
	SaveLayers(bin);
	//存储队列
	SerializePrintQue(bin);
}

bool LOImageModule::DeSerialize(BinArray *bin, int *pos, LOEventMap *evmap) {
    //一些模块状态，大部分都不需要覆盖
    if (!DeSerializeState(bin, pos)) {
        SDL_LogError(0, "Image module state DeSerialize faild!");
        return false;
    }
    //读取图层
    if (!LoadLayers(bin, pos, evmap)) {
        FatalError("LoadLayers faild!");
        return false;
    }
    //读取printQue
    int count = bin->GetIntAuto(pos);
    for (int ii = 0; ii < count; ii++) {
        std::string s = bin->GetString(pos);
        PrintNameMap *pmap = GetPrintNameMap(s.c_str());
        if (!pmap || !pmap->DeSerialize(bin, pos)) {
            FatalError("PrintMap DeSerialize faild!");
            return false;
        }
    }
    return true;
}

void LOImageModule::PrintNameMap::Serialize(BinArray *bin) {
	//名称优先
	bin->WriteString(mapName);
	int len = bin->WriteLpksEntity("pque", 0, 1);
	bin->WriteInt(map->size());
	//写入fullid
	for (auto iter = map->begin(); iter != map->end(); iter++) bin->WriteInt(iter->first);
	bin->WriteInt(bin->Length() - len, &len);
}


bool LOImageModule::PrintNameMap::DeSerialize(BinArray *bin, int *pos) {
	int next = -1;
	if (!bin->CheckEntity("pque", &next, nullptr, pos)) return false;
	map->clear();
	int count = bin->GetIntAuto(pos);
	for (int ii = 0; ii < count; ii++) {
		int fid = bin->GetIntAuto(pos);
		LOLayer *lyr = LOLayer::GetLayer(fid);
		if (lyr) (*map)[fid] = lyr;
	}
	*pos = next;
    return true;
}


//加载图层也要在图像模块
void LOImageModule::SaveLayers(BinArray *bin) {
	//LYRS,len, version
	int len = bin->WriteLpksEntity("FORC", 0, 1);
	//首先存储前台
	bin->WriteInt(LOLayer::LAYER_BASE_COUNT);
	for (int ii = 0; ii < LOLayer::LAYER_BASE_COUNT; ii++) {
		G_baseLayer[ii]->SerializeForce(bin);
	}
	bin->WriteInt(bin->Length() - len, &len);

	//存储后台
	len = bin->WriteLpksEntity("BAKE", 0, 1);
	bin->WriteInt(LOLayer::layerCenter.size());
	for (auto iter = LOLayer::layerCenter.begin(); iter != LOLayer::layerCenter.end(); iter++) {
		iter->second->SerializeBak(bin);
	}
	bin->WriteInt(bin->Length() - len, &len);
}


bool LOImageModule::LoadLayers(BinArray *bin, int *pos, LOEventMap *evmap) {
	int next = -1;
	if (!bin->CheckEntity("FORC", &next, nullptr, pos)) return false;
	//高版本的可能无法在低版本上读取
	int count = bin->GetIntAuto(pos);
	for (int ii = 0; ii < count; ii++) {
		bin->GetIntAuto(pos);  //fid
		if (ii < LOLayer::LAYER_BASE_COUNT) {
			if(!G_baseLayer[ii]->DeSerializeForce(bin, pos)) return false ;
		}
		else {
            SDL_Log("save dat layer count is %d, but this app max is %d", count, LOLayer::LAYER_BASE_COUNT);
			bin->JumpEntity("fdat", pos);
		}
	}
	*pos = next;

	//读取后台
	if (!bin->CheckEntity("BAKE", &next, nullptr, pos)) return false;
	count = bin->GetIntAuto(pos);
	for (int ii = 0; ii < count; ii++) {
		int fid = bin->GetIntAuto(pos);
		LOLayer *lyr = LOLayer::CreateLayer(fid);
		if (!lyr->DeSerializeBak(bin, pos)) return false;
	}
	*pos = next;
    return true;
}


void LOImageModule::SerializePrintQue(BinArray *bin) {
	bin->WriteInt(backDataMaps.size());
	for (int ii = 0; ii < backDataMaps.size(); ii++) {
		backDataMaps.at(ii)->Serialize(bin);
	}
}


void LOImageModule::SerializeState(BinArray *bin) {
	int len = bin->WriteLpksEntity("imgo", 0, 1);

	spStyle.Serialize(bin);
	bin->WriteLOString(&spFontName);
	sayStyle.Serialize(bin);
	bin->WriteLOString(&sayFontName);
	//对话框
	sayWindow.Serialize(bin);
	sayState.Serialize(bin);
    //文字速度
    bin->WriteInt(G_textspeed);
	//其他
	bin->WriteLOString(&btndefStr);
	bin->WriteInt(btnOverTime);
	bin->WriteInt(btnUseSeOver);
	//all sp操作
	if (allSpList) {
		bin->WriteInt(allSpList->size());
		for (int ii = 0; ii < allSpList->size(); ii++) bin->WriteInt(allSpList->at(ii));
	}
	else bin->WriteInt(0);
	if (allSpList2) {
		bin->WriteInt(allSpList2->size());
		for (int ii = 0; ii < allSpList2->size(); ii++) bin->WriteInt(allSpList2->at(ii));
	}
	else bin->WriteInt(0);

	bin->WriteInt(bin->Length() - len, &len);
}

bool LOImageModule::DeSerializeState(BinArray *bin, int *pos) {
	int next = -1;
	if (!bin->CheckEntity("imgo", &next, nullptr, pos)) return false;
    if(!spStyle.DeSerialize(bin, pos)) return false ;
    spFontName = bin->GetLOString(pos);
    if(!sayStyle.DeSerialize(bin,pos))return false ;
    sayFontName = bin->GetLOString(pos);

	//对话框
    if(!sayWindow.DeSerialize(bin,pos)) return false ;
	if (!sayState.DeSerialize(bin, pos)) return false;
    //文字速度
    G_textspeed = bin->GetIntAuto(pos);
	//其他
	btndefStr = bin->GetLOString(pos);
	btnOverTime = bin->GetIntAuto(pos);
	btnUseSeOver = bin->GetIntAuto(pos);
	//
	int count = bin->GetIntAuto(pos);
	if (count > 0) {
		if (!allSpList) allSpList = new std::vector<int>();
		allSpList->clear();
		for (int ii = 0; ii < count; ii++) allSpList->push_back(bin->GetIntAuto(pos));
	}

	count = bin->GetIntAuto(pos);
	if (count > 0) {
		if (!allSpList2) allSpList2 = new std::vector<int>();
		allSpList2->clear();
		for (int ii = 0; ii < count; ii++) allSpList2->push_back(bin->GetIntAuto(pos));
	}
	*pos = next;
	return true;
}


void LOImageModule::LOSayWindow::Serialize(BinArray *bin) {
	int len = bin->WriteLpksEntity("winn", 0, 1);

	bin->WriteInt3(x, y, w);
	bin->WriteInt3(h, textX, textY);
	bin->WriteLOString(&winstr);

	bin->WriteInt(bin->Length() - len, &len);
}

bool LOImageModule::LOSayWindow::DeSerialize(BinArray *bin, int *pos) {
    int next = -1;
    if (!bin->CheckEntity("winn", &next, nullptr, pos)) return false;
    x = bin->GetIntAuto(pos);
    y = bin->GetIntAuto(pos);
    w = bin->GetIntAuto(pos);
    h = bin->GetIntAuto(pos);
    textX = bin->GetIntAuto(pos);
    textY = bin->GetIntAuto(pos);
    winstr = bin->GetLOString(pos);
    *pos = next;
    return true;
}

void LOImageModule::LOSayState::Serialize(BinArray *bin) {
	int len = bin->WriteLpksEntity("wins", 0, 1);
	bin->WriteLOString(&say);
	bin->WriteInt(flags);
	bin->WriteInt(pageEnd);
	//z_order和winEffect不会记录，这两个应该要跟随define的时候设置

	bin->WriteInt(bin->Length() - len, &len);
}


bool LOImageModule::LOSayState::DeSerialize(BinArray *bin, int *pos) {
	int next = -1;
	if (!bin->CheckEntity("wins", &next, nullptr, pos)) return false;
	say = bin->GetLOString(pos);
	flags = bin->GetIntAuto(pos);
	pageEnd = bin->GetIntAuto(pos);
	return true;
}

