/*
//图像渲染模块
*/

#include "../etc/LOEvent1.h"
#include "../etc/LOString.h"
#include "LOImageModule.h"
#include <SDL_image.h>

bool LOImageModule::isShowFps = false;
bool LOImageModule::st_filelog;  //是否使用文件记录

LOImageModule::LOImageModule(){
	FunctionInterface::imgeModule = this;
	allSpList = NULL;
	allSpList2 = NULL;
	window = NULL;
	render = NULL;
	fpstex = NULL;
	isShowFps = true;
	//screenshotSu = NULL;

    ResetConfig();

	layerQueMutex = SDL_CreateMutex();
	//btnQueMutex = SDL_CreateMutex();
	//presentMutex = SDL_CreateMutex();
	layerDataMutex = SDL_CreateMutex();
	doQueMutex = SDL_CreateMutex();

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
}


int LOImageModule::InitImageModule() {
	if (FunctionInterface::scriptModule) scriptModule->GetGameInit(G_gameWidth, G_gameHeight);
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) < 0) {
		SDL_LogError(0, "Couldn't initialize SDL: %s\n", SDL_GetError());
		return 0;
	}

	//======初始化窗口==========//
	//获取显示器的分辨率
	SDL_Rect deviceSize;
	if (SDL_GetDisplayBounds(0, &deviceSize) < 0) {
		SDL_Log("SDL_GetDisplayBounds failed: %s", SDL_GetError());
		return 0;
	}

	//windows下默认使用游戏尺寸，其他系统由外部设置
	int winflag;

#ifdef WIN32
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
#endif // WIN32
#ifdef ANDROID
	deviceSize.w = G_gameWidth;
	deviceSize.h = G_gameHeight;
	G_windowRect.x = SDL_WINDOWPOS_UNDEFINED;
	G_windowRect.y = SDL_WINDOWPOS_UNDEFINED;
	G_gameRatioW = 1;
	G_gameRatioH = 1;
	winflag = SDL_WINDOW_FULLSCREEN;
#endif

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
		LOLog_e(0, "Your device does not support the specified texture format and cannot continue!");
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
	if(winView.w != G_gameWidth || winView.h != G_gameHeight) LOLog_i("viewport set error!") ;
	if(  abs( G_gameScaleX * winView.x- G_viewRect.x) > 1 ||
	     abs( G_gameScaleY * winView.y - G_viewRect.y) > 1){
		LOLog_i("viewport not in the expected position, the parameters will be adjusted automatically.") ;
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
	Uint64 hightTimeNow, perHtickTime = SDL_GetPerformanceFrequency() / 1000;
	bool loopflag = true;

	if (G_fpsnum < 2) G_fpsnum = 2;
	if (G_fpsnum > 120) G_fpsnum = 120;
	double fpstime = 1000.01 / G_fpsnum;
	double posTime;   //从上一帧后，当前帧花费的时间
	SDL_Event ev;
	Uint64 lastTime = 0;
	bool minisize = false;
	
	ChangeRunState(MODULE_STATE_RUNNING);

	//第一帧要锁住print信号
	printHook->FinishMe();
	printPreHook->FinishMe();

	while (loopflag) {
		hightTimeNow = SDL_GetPerformanceCounter();
		posTime = ((double)(hightTimeNow - lastTime)) / perHtickTime;

		if (!minisize && posTime + 0.1 > fpstime) {
			if (RefreshFrame(posTime) == 0) {
				//LOLog_i("frame time:%f", ((double)(SDL_GetPerformanceCounter() - hightTimeNow)) / perHtickTime) ;
				//now do the delay event.like send finish signed.
				DoPreEvent(posTime);
				//在进入事件处理之前，先整理一下钩子队列
				G_hookQue.arrangeList();
				lastTime = hightTimeNow;
			}
			else {
				LOLog_i("RefreshFrame faild!");
			}
		}

		//检查模块状态变化
		if (isStateChange()) {
			if (isModuleExit()) break;
			else if (isModuleSaving()) {
				ChangeRunState(MODULE_STATE_SAVING);
				//进入存档函数
			}
			else if (isModuleLoading()) {

			}
			else if (isModuleReset()) {
				ResetMe();
			}
		}
		
		//处理事件
		if (SDL_PollEvent(&ev)) {
			switch (ev.type) {
			case SDL_QUIT:
				loopflag = false;
				//这个函数要改写
				SetExitFlag(MODULE_FLAGE_EXIT);
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
				//处理事件
				HandlingEvents();
				break;
			}
		}
		
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

	ChangeRunState(MODULE_STATE_NOUSE);

	//等待其他模块退出
	lastTime = SDL_GetPerformanceCounter();
	posTime = 0.0;
	//LOLog_i("img start wait:%d\n", SDL_GetTicks());
	while (posTime < 2000 && (scriptModule->moduleState != MODULE_STATE_NOUSE
		|| audioModule->moduleState != MODULE_STATE_NOUSE)) {
		SDL_Delay(1);
		hightTimeNow = SDL_GetPerformanceCounter();
		posTime = ((double)(hightTimeNow - lastTime)) / perHtickTime;
	}
	//
	//WriteLog(LOGSET_FILELOG);

	LOLog_i("LONS::MainLoop exit.\n");
	//LOLog_i("exit ok: %d", SDL_GetTicks());
	return -1;
}


int LOImageModule::RefreshFrame(double postime) {

	int lockfalg = 0;
	SDL_LockMutex(layerQueMutex);
	//layerTestMute.lock();
	//printf("main refresh:%d\n", SDL_GetTicks());
	if (lockfalg == 0) {
		//必须小心处理渲染逻辑，不然很容导致程序崩溃，关键是正确处理 print 1 以外的多线程同步问题
		//print 1: UpDisplay
		//print other(first frame): get signed -> RenderTarget -> UpDisplay -> RenderCopy(effectTex) -> call thread
		//thread: get prepare finish -> uplayer -> wait effect finish
		//(second frame):enter effecting ->(if cut) -> send finish signed.

		//收到请求后，本帧会刷新到effct层上
		if (printPreHook->enterEdit()) {
			//if (premsg->loadParamInt(0) == PARAM_BGCOPY) {
			//	SDL_Texture *bgtex = SDL_CreateTexture(render, G_Texture_format, SDL_TEXTUREACCESS_TARGET, G_viewRect.w, G_viewRect.h);
			//	premsg->savePtr_t(true, bgtex, 0, LOEvent::VALUE_SDLTEXTURE_PTR);
			//	SDL_SetRenderTarget(render, bgtex);
			//}
			//else if (SDL_SetRenderTarget(render, effectTex) < 0) SimpleError("SDL_SetRenderTarget(effectTexture) faild!");

			if (SDL_SetRenderTarget(render, effectTex->GetTexture()) < 0) LOLog_e("SDL_SetRenderTarget(effectTexture) faild!");
			SDL_RenderSetScale(render, G_gameScaleX, G_gameScaleY);  //跟窗口缩放保持一致
			SDL_RenderClear(render);
			UpDisplay(postime);
			SDL_SetRenderTarget(render, NULL);
			SDL_RenderCopy(render, effectTex->GetTexture(), NULL, NULL);
			//LOLog_i("prepare ok!") ;
			//需要在本帧刷新后通知事件已经完成，因此将一个前置事件推入渲染模块队列
			preEventList.push_back(printPreHook);
			//LOEventHook *ev = new LOEventHook;
			//ev->catchFlag = PRE_EVENT_PREPRINTOK;
			//preEventList.push_back(ev);
			//LOLog_i("prepare event change!") ;
		}
		else {
			//printf("main refresh1-1:%d\n", SDL_GetTicks());
			SDL_RenderClear(render);
			UpDisplay(postime);
			//printf("main refresh1-2:%d\n", SDL_GetTicks());
		}
		//注意计时的时候，第一帧会需要初始化，需要几十毫秒
		if(isShowFps) ShowFPS(postime);
		//printf("main refresh1-3:%d\n", SDL_GetTicks());
		SDL_RenderPresent(render);

		//printf("main refresh2:%d\n", SDL_GetTicks());
		SDL_UnlockMutex(layerQueMutex);
		//printf("main refresh3:%d\n", SDL_GetTicks());
		//layerTestMute.unlock();

		//每完成一帧的刷新，我们检查是否有 print 事件需要处理，如果是print 1则直接通知完成
		//这里有个隐含的条件，在脚本线程展开队列时，绝对不会进入RefreshFrame刷新，所以如果有MSG_Wait_Print表示已经完成print的第一帧刷新
		//如果是print 2-18,我们将检查effect的运行情况
		if (printHook->enterEdit()) {
			//printf("main thread:%d\n", SDL_GetTicks());
			preEventList.push_back(printHook);
		}
		return 0;
	}
	else if (lockfalg == -1) {
		LOLog_i("fps lock layer faild:%s", SDL_GetError());
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
		//if (lyr->id[0] == 9) {
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
	//简单来说就是创建一个动态遮片
	//首先需要抓取图像，抓取的图像存储在effectTex中，其次需要一张保存最终结果的纹理，_?_effect_?_
	//还需要一个可编辑的遮片 maskTex

	LOString ntemp("**;_?_effect_?_");
	//除效果10外，其他效果均需要进行遮片操作
	if (ef->nseffID != 10) {
		effmakTex.reset(new LOtexture());
		effmakTex->CreateDstTexture(G_viewRect.w, G_viewRect.h, SDL_TEXTUREACCESS_STREAMING);
	}
	//将材质覆盖到最前面进行遮盖
	//效果层处于哪一个排列队列必须跟 ExportQuequ中的一致，不然无法立即展开队列
	LOLayerData *info = CreateNewLayerData(GetFullID(LOLayer::LAYER_NSSYS, LOLayer::IDEX_NSSYS_EFFECT, 255, 255), printName);
	loadSpCore(info, ntemp, 0, 0, -1, true);
	//需要马上使用
	info->cur.texture = info->bak.texture;
	//缩放模式  
	if (IsGameScale()) {
		info->bak.SetPosition2(0, 0, 1.0 / G_gameScaleX, 1.0 / G_gameScaleY);
	}

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

	ef->ReadyToRun();
	//准备好第一帧的运行
	ef->RunEffect(render, info, effectTex, effmakTex, 0);
}

//完成了返回true, 否则返回false
bool LOImageModule::ContinueEffect(LOEffect *ef, const char *printName, double postime) {
	if (ef) { //print 2-8
		int fullid = GetFullID(LOLayer::LAYER_NSSYS, LOLayer::IDEX_NSSYS_EFFECT, 255, 255);
		LOLayer *lyr = LOLayer::FindLayerInCenter(fullid);
		//maybe has some error!
		if (!lyr) return true;
		if (ef->RunEffect(render, lyr->data.get(), effectTex, effmakTex, postime)) {
			//重置数据
			lyr->data->bak.SetDelete();
			lyr->data->cur.SetDelete();
			effmakTex.reset();
			return true;
		}
		else return false;
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
		//auto ittt = fpstex[ii].GetTexture(NULL);

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
bool LOImageModule::ParseTag(LOLayerData *info, LOString *tag) {
	const char *buf = tag->c_str();
	int alphaMode = trans_mode;
	//LOLog_i("tag is %s",tag->c_str()) ;
	buf = ParseTrans(&alphaMode, tag->c_str());
	if (alphaMode == LOLayerData::TRANS_STRING) {
		info->bak.SetTextureType(LOtexture::TEX_SIMPLE_STR);
		info->bak.keyStr.reset(new LOString(buf, tag->GetEncoder()));
	}
	else {
		buf = tag->SkipSpace(buf);
		if (buf[0] == '>') {
			buf = tag->SkipSpace(buf + 1);
			info->bak.SetTextureType(LOtexture::TEX_COLOR_AREA);
			info->bak.keyStr.reset(new LOString(buf, tag->GetEncoder()));
		}
		else if (buf[0] == '*') {
			buf++;
			if (buf[0] == 'd') {
				info->bak.SetTextureType(LOtexture::TEX_DRAW_COMMAND);
			}
			else if (buf[0] == 's') {
				info->bak.SetTextureType(LOtexture::TEX_ACTION_STR);
				info->bak.SetAlphaMode(LOLayerData::TRANS_COPY);
			}
			else if (buf[0] == 'S') {
				info->bak.SetTextureType(LOtexture::TEX_MULITY_STR);
				info->bak.SetAlphaMode(LOLayerData::TRANS_COPY);
			}
			else if (buf[0] == '>') {
				info->bak.SetTextureType(LOtexture::TEX_COLOR_AREA);
				info->bak.SetAlphaMode(LOLayerData::TRANS_COPY);
				//LOLog_i("usecache is %c",info->usecache) ;
			}
			else if (buf[0] == 'b') {
				//"*b;50,100,内容" ;绘制一个NS样式的按钮，通常模式时只显示文字，鼠标悬停时显示变色文字和有一定透明度的灰色
				info->bak.SetTextureType(LOtexture::TEX_NSSIMPLE_BTN);
				info->bak.SetAlphaMode(LOLayerData::TRANS_COPY);
			}
			else if (buf[0] == '*') {
				//操作式纹理
				//** 空纹理，基本上只是为了挂载子对象
				info->bak.SetTextureType(LOtexture::TEX_CONTROL);
			}
			buf += 2;
			info->bak.keyStr.reset(new LOString(buf, tag->GetEncoder()));
		}
		else {
			info->bak.SetTextureType(LOtexture::TEX_IMG);
			info->bak.SetAlphaMode(alphaMode);
			ParseImgSP(info, tag, buf);
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

bool LOImageModule::ParseImgSP(LOLayerData *info, LOString *tag, const char *buf) {
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
		info->bak.SetAction(ac);

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

	info->bak.keyStr.reset(new LOString(buf, tag->GetEncoder()));
}

////随机字符串
//void LOImageModule::RandomFileName(LOString *s, char alphamode) {
//	s->clear();
//	char tmp[32];
//	memset(tmp, 0, 32);
//	tmp[0] = alphamode;
//	tmp[1] = ';';
//	for (int ii = 2; ii < 31; ii++) {
//		tmp[ii] = rand() % ('Z' - 'A' + 1) + 'A';
//	}
//	s->assign(tmp);
//}


//LOLayer* LOImageModule::FindLayerInBase(LOLayer::SysLayerType type, const int *ids) {
//	return G_baseLayer[type].FindChild(ids);
//}
//
//LOLayer* LOImageModule::FindLayerInBase(int fullid) {
//	LOLayer::SysLayerType type;
//	int ids[] = { -1,-1,-1 };
//	GetTypeAndIds( (int*)(&type), ids, fullid);
//	return G_baseLayer[type].FindChild(ids);
//}


//移除按钮定义
void LOImageModule::ClearBtndef(const char *printName) {
	/*
	//前台处理
	//移去当前图层内的按钮定义
	SDL_LockMutex(layerQueMutex);
	//删除btn的按钮层
	LOLayer *lyr = G_baseLayer[LOLayer::LAYER_NSSYS].RemodeChild(LOLayer::IDEX_NSSYS_BTN);
	if (lyr) delete lyr;
	//所有定义的按钮无效化
	for (int ii = 0; ii < LOLayer::LAYER_BASE_COUNT; ii++) {
		G_baseLayer[ii].unSetBtndefAll();
	}
	SDL_UnlockMutex(layerQueMutex);

	//后台处理
	auto *list = GetPrintNameMap(printName)->map;
	SDL_LockMutex(layerDataMutex);
	//删除btn的按钮层
	CspCore(LOLayer::LAYER_NSSYS, LOLayer::IDEX_NSSYS_BTN, LOLayer::IDEX_NSSYS_BTN, printName);
	for (auto iter = list->begin(); iter != list->end(); iter++) {
		//去除按钮定义
		//iter->second->unSetBtndef();
	}
	SDL_UnlockMutex(layerDataMutex);
	*/
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
bool LOImageModule::loadSpCore(LOLayerData *info, LOString &tag, int x, int y, int alpha, bool visiable) {
	bool ret = loadSpCoreWith(info, tag, x, y, alpha, 0);
	info->bak.SetVisable(visiable);
	return ret;
}

bool LOImageModule::loadSpCoreWith(LOLayerData *info, LOString &tag, int x, int y, int alpha, int eff) {
	info->bak.buildStr.reset(new LOString(tag));

	info->bak.SetShowType(LOLayerDataBase::SHOW_NORMAL); //简单模式
	ParseTag(info, &tag);

	GetUseTextrue(info, nullptr, true);
	//空纹理不参与下面的设置了，以免出现问题
	if (!info->bak.texture) return false;

	info->bak.SetPosition(x, y);
	info->SetDefaultShowSize(false);
	info->bak.SetAlpha(alpha);

	//记录
	//queLayerMap[info->fullid] = NULL;
	return true;
}



void LOImageModule::GetUseTextrue(LOLayerData *info, void *data, bool addcount) {
	if (!info->bak.isCache()) {
		//唯一性纹理
		std::unique_ptr<LOString> tstr = std::move(info->bak.keyStr);
		switch (info->bak.texType){
		case LOtexture::TEX_SIMPLE_STR:
			TextureFromSimpleStr(info, tstr.get());
			break;
		case LOtexture::TEX_CONTROL:
			TextureFromControl(info, tstr.get());
			break;
		case LOtexture::TEX_ACTION_STR:
			TextureFromActionStr(info, tstr.get());
			break;
		case LOtexture::TEX_COLOR_AREA:
			TextureFromColor(info, tstr.get());
			break;
		default:
			LOString errs = StringFormat(128, "ONScripterImage::GetUseTextrue() unkown Textrue type:%d", info->bak.texType);
			SimpleError(errs.c_str());
			break;
		}
		/*
		if (info->texType == LOtexture::TEX_ACTION_STR) {
			//tx = RenderText2(info, &ww, data, 0);
			//LOString::SetStr(info->textStr, data, false);
		}
		else if (info->texType == LOtexture::TEX_SIMPLE_STR) {
			TextureFromSimpleStr(info, tstr.get());
		}
		else if (info->texType == LOtexture::TEX_MULITY_STR) {
			//tx = TextureFromSimpleStr(info, data);
		}
		else if (info->texType == LOtexture::TEX_NSSIMPLE_BTN) {
			//tx = TextureFromNSbtn(info, data);
		}
		else {
			LOString errs = StringFormat(128, "ONScripterImage::GetUseTextrue() unkown Textrue type:%d", info->texType);
			//LOLog_e("%s",errs.c_str());
			SimpleError(errs.c_str());
		}
		//LOtexture::addTextureBaseToMap(*info->fileName, tx);
		*/
	}
	else {
		//只有图像才使用纹理缓存
		TextureFromCache(info);
		if (!info->bak.texture) {
			if (info->bak.texType == LOtexture::TEX_IMG) {
				TextureFromFile(info);
			}
			else {
				LOLog_e(0, "ONScripterImage::GetUseTextrue() unkown Textrue type:%d", info->bak.texType);
			}
		}
	}
}

//单行文字

void LOImageModule::TextureFromSimpleStr(LOLayerData*info, LOString *s) {
	//拷贝一份样式
	LOTextStyle style = spStyle;
	const char *buf = s->c_str();

	//基础设置
	while (buf[0] == '/' || buf[0] == ',' || buf[0] == ' ') buf++;
	style.xsize = s->GetInt(buf);
	while (buf[0] == ',' || buf[0] == ' ') buf++;
	style.ysize = s->GetInt(buf);
	while (buf[0] == ',' || buf[0] == ' ') buf++;
	if (s->GetInt(buf)) style.flags |= LOTextStyle::STYLE_SHADOW;
	else style.flags &= (~LOTextStyle::STYLE_SHADOW);

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
		info->bak.SetAction(ac);
	}

	info->bak.SetNewFile(texture);
	return ;
}


void LOImageModule::TextureFromActionStr(LOLayerData*info, LOString *s) {
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
	info->bak.SetNewFile(texture);
	LOActionText *ac = new LOActionText();
	ac->setFlags(LOAction::FLAGS_INIT);
	info->bak.SetAction(ac);
}

//从标记中生成色块
void LOImageModule::TextureFromColor(LOLayerData *info, LOString *s) {
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
	info->bak.SetNewFile(texture);
	texture->CreateSimpleColor(w, h, cc);
	return ;
}
/*
LOtextureBase* LOImageModule::TextureFromNSbtn(LOLayerInfo*info, LOString *s) {
	LOtextureBase *base = nullptr;
	return base;
}           
*/


void LOImageModule::TextureFromControl(LOLayerData *info, LOString *s) {
	LOShareTexture texture(new LOtexture());
	info->bak.SetNewFile(texture);
	if (*s == "_?_empty_?_") texture->setEmpty(G_gameWidth, G_gameHeight);
	else if (*s == "_?_effect_?_") texture->CreateDstTexture(G_viewRect.w, G_viewRect.h, SDL_TEXTUREACCESS_TARGET);
}

void LOImageModule::TextureFromFile(LOLayerData *info) {
	bool useAlpha;
	LOShareBaseTexture base(SurfaceFromFile(info->bak.keyStr.get()));
	if (!base) return ;

	//转换透明格式 
	if (info->bak.alphaMode != LOLayerData::TRANS_COPY && !base->hasAlpha()) {
		if (info->bak.alphaMode == LOLayerData::TRANS_ALPHA && !base->ispng) {
			base->SetSurface(LOtextureBase::ConverNSalpha(base->GetSurface(), info->bak.GetCellCount()));
			if(!base->isValid()) LOLog_i("Conver image ns alhpa faild: %s", info->bak.keyStr->c_str());
		}
		//else if (info->alphaMode == LOLayerData::TRANS_TOPLEFT) {
		//	SDL_Color color = tmp->getPositionColor(0, 0);
		//	tmp->setAlphaColor(color);
		//}
		//else if (info->alphaMode == LOLayerInfo::TRANS_TOPRIGHT) {
		//	SDL_Color color = tmp->getPositionColor(tmp->W() - 1, 0);
		//	tmp->setAlphaColor(color);
		//}
		//else if (info->alphaMode == LOLayerInfo::TRANS_DIRECT) {
		//	LOLog_e("LOLayerInfo::TRANS_DIRECT is enmpty code!");
		//}
	}

	LOString s = info->bak.keyStr->toLower() + "?" + std::to_string(info->bak.alphaMode) + ";";
	LOtexture::addTextureBaseToMap(s, base);

	LOShareTexture texture(new LOtexture(base));
	info->bak.SetNewFile(texture);
	info->bak.keyStr.reset(new LOString(s));
	return ;
}


void LOImageModule::TextureFromCache(LOLayerData *info) {
	LOString s = info->bak.keyStr->toLower() + "?" + std::to_string(info->bak.alphaMode) + ";";
	LOShareBaseTexture base = LOtexture::findTextureBaseFromMap(s);
	if (base) {
		LOShareTexture texture(new LOtexture(base));
		info->bak.SetNewFile(texture);
		info->bak.keyStr.reset(new LOString(s));
	}
}


LOtextureBase* LOImageModule::SurfaceFromFile(LOString *filename) {
	std::unique_ptr<BinArray> bin(FunctionInterface::fileModule->ReadFile(filename));
	if (!bin) return nullptr;
	LOtextureBase *base = new LOtextureBase(bin->bin, bin->Length());
	if (!base->isValid()) {
		delete base;
		SimpleError("LOImageModule::SurfaceFromFile() unsupported file format!");
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
	if (ef->id <= 18) {
		ef->nseffID = ef->id;
		ef->time = 20; //给予一个最小的时间
	}
	else {
		auto iter = effectMap.find(ef->id);
		if (iter == effectMap.end()) {
			delete ef;
			ef = NULL;
		}
		else {
			ef->nseffID = iter->second->nseffID;
			ef->time = iter->second->time;
			ef->mask.assign(iter->second->mask);
		}
	}
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

	loadSpCore(info, tag, sayWindow.textX, sayWindow.textY + sayStyle.yruby, 255, true);
	if (!info->bak.texture) return nullptr ;

	info->bak.texture->isEdit = true;
	info->bak.texture->setFlags(LOtexture::USE_TEXTACTION_MOD);

	LOActionText *ac = (LOActionText*)info->bak.GetAction(LOAction::ANIM_TEXT);
	ac->initPos = lastPos;
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


void LOImageModule::PrintNameMap::Serialize(BinArray *bin) {
	int len = bin->Length();
	//'pque', len, version
	bin->WriteInt3(0x65757170, 0, 1);
	bin->WriteString(mapName);
	bin->WriteInt(map->size());
	//写入fullid
	for (auto iter = map->begin(); iter != map->end(); iter++) bin->WriteInt(iter->first);
	bin->WriteInt(bin->Length() - len, &len);
}

void LOImageModule::Serialize(BinArray *bin) {
	//存储图层
	LOLayer::SaveLayer(bin);
	//存储队列
	SerializePrintQue(bin);
	//存储状态
	SerializeState(bin);
}

void LOImageModule::SerializePrintQue(BinArray *bin) {
	for (int ii = 0; ii < backDataMaps.size(); ii++) {
		PrintNameMap *map = backDataMaps[ii].get();
		if (map->map->size() > 0) map->Serialize(bin);
	}
}


void LOImageModule::SerializeState(BinArray *bin) {
	int len = bin->Length() + 4;
	//imgo, len, version
	bin->WriteInt3(0x6F676D69, 0, 1);

	spStyle.Serialize(bin);
	bin->WriteLOString(&spFontName);
	sayStyle.Serialize(bin);
	bin->WriteLOString(&sayFontName);
	//对话框
	sayWindow.Serialize(bin);
	sayState.Serialize(bin);
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

void LOImageModule::LOSayWindow::Serialize(BinArray *bin) {
	int len = bin->Length() + 4;
	//winn, len, version
	bin->WriteInt3(0x6E6E6977, 0, 1);
	bin->WriteInt3(x, y, w);
	bin->WriteInt3(h, textX, textY);
	bin->WriteLOString(&winstr);

	bin->WriteInt(bin->Length() - len, &len);
}

void LOImageModule::LOSayState::Serialize(BinArray *bin) {
	int len = bin->Length() + 4;
	//wins, len, version
	bin->WriteInt3(0x736E6977, 0, 1);
	bin->WriteLOString(&say);
	bin->WriteInt(flags);
	bin->WriteInt(pageEnd);
	//z_order和winEffect不会记录，这两个应该要跟随define的时候设置

	bin->WriteInt(bin->Length() - len, &len);
}