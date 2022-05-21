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
	effectTex = NULL;
	maskTex = NULL;
	fpstex = NULL;
	isShowFps = true;
	fpstex = NULL;
	//screenshotSu = NULL;
	winEffect = NULL ;

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
	z_order = 499;
	trans_mode = LOLayerData::TRANS_TOPLEFT;
	effectSkipFlag = false;
	winbackMode = false;
	textbtnFlag = true;
	texecFlag = false;
	st_filelog = false;

	
	winFont.Reset();
	winFont.xsize = 20;
	winFont.ysize = 20;
	spFont.Reset();
	spFontName = "default.ttf";
	fontManager.ResetMe();

	textbtnValue = 1;
	winEraseFlag = 1;  //默认print的时候隐藏对话框
	winoff = { 0,0,0,0 };

	btndefStr.clear();
	btnOverTime = 0;
	btnUseSeOver = false;

	if (allSpList) allSpList->clear();
	if (allSpList2) allSpList2->clear();
    if (winEffect) delete winEffect;
	allSpList = NULL;
	allSpList2 = NULL;
	winEffect = NULL ;

	
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

	ResetViewPort();
	titleStr = "ONScripter-LONS " + LOString( ONS_VERSION );
	SDL_SetWindowTitle(window, titleStr.c_str());

	//初始化ttf
	if (TTF_Init() == -1) {
		SDL_LogError(0, "TTF_Init() faild!");
		return 0;
	}

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

	auto *temp = CreateTexture(render, G_Texture_format, SDL_TEXTUREACCESS_TARGET, G_viewRect.w, G_viewRect.h);
	effectTex.reset(new LOtextureBase(temp));
	maskTex.reset(new LOtextureBase());
	//FreeFps();
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
	//Uint64 lastTime = 0;
	bool minisize = false;
	moduleState = MODULE_STATE_RUNNING;

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
				DoDelayEvent(posTime);
				lastTime = hightTimeNow;
				if(moduleState >= MODULE_STATE_EXIT) break;
				else if (moduleState & MODULE_STATE_RESET) {
					ResetMe();
				}
			}
			else {
				LOLog_i("RefreshFrame faild!");
			}
		}
		
		//处理事件
		if (SDL_PollEvent(&ev)) {
			switch (ev.type) {
			case SDL_QUIT:
				loopflag = false;
				SetExitFlag(MODULE_STATE_EXIT);
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
	
	moduleState = MODULE_STATE_NOUSE;
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
	WriteLog(LOGSET_FILELOG);

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

			if (SDL_SetRenderTarget(render, effectTex->GetFullTexture()) < 0) LOLog_e("SDL_SetRenderTarget(effectTexture) faild!");
			SDL_RenderSetScale(render, G_gameScaleX, G_gameScaleY);  //跟窗口缩放保持一致
			SDL_RenderClear(render);
			UpDisplay(postime);
			SDL_SetRenderTarget(render, NULL);
			SDL_RenderCopy(render, effectTex->GetFullTexture(), NULL, NULL);
			//LOLog_i("prepare ok!") ;
			//需要在本帧刷新后通知事件已经完成，因此将一个前置事件推入渲染模块队列
			printPreHook->catchFlag = PRE_EVENT_PREPRINTOK;
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
			printHook->catchFlag = PRE_EVENT_EFFECTCONTIUE;
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
	UpDataLayer(G_baseLayer[LOLayer::LAYER_SPRINT], tickTime, 1023, z_order + 1, 0);
	UpDataLayer(G_baseLayer[LOLayer::LAYER_STAND], tickTime, 1023, 0, 0);
	if (winbackMode) {
		UpDataLayer(G_baseLayer[LOLayer::LAYER_OTHER], tickTime, 1023, 0, 0);
		UpDataLayer(G_baseLayer[LOLayer::LAYER_SPRINTEX], tickTime, 1023, 0, 0);
		UpDataLayer(G_baseLayer[LOLayer::LAYER_DIALOG], tickTime, 1023, 0, 0);
		UpDataLayer(G_baseLayer[LOLayer::LAYER_SPRINT], tickTime, z_order, 0, 0);
		UpDataLayer(G_baseLayer[LOLayer::LAYER_SELECTBAR], tickTime, 1023, 0, 0);
	}
	else {
		UpDataLayer(G_baseLayer[LOLayer::LAYER_SPRINT], tickTime, z_order, 0, 0);
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

		//在下方的对象先渲染，渲染父对象
		if (lyr->curInfo->texture) {
			//运行图层的action
			if (lyr->curInfo->actions) {
				lyr->DoAction(lyr->curInfo.get(), curTime);
			}
			//if (!lyr->curInfo->texture->isNull()) {
			if (lyr->curInfo->texture->isAvailable()) {
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

	LOString ntemp("_?_effect_?_");      //care trigraph or Digraph
	LOtexture::addNewEditTexture(ntemp, G_viewRect.w, G_viewRect.h, G_Texture_format, SDL_TEXTUREACCESS_TARGET);
	//除效果10外，其他效果均需要特殊调制
	if (ef->nseffID != 10) {
		auto *editEff = CreateTexture(render, G_Texture_format, SDL_TEXTUREACCESS_STREAMING, G_viewRect.w, G_viewRect.h);
		maskTex.reset(new LOtextureBase(editEff));
	}
	//将材质覆盖到最前面进行遮盖
	//效果层处于哪一个排列队列必须跟 ExportQuequ中的一致，不然无法立即展开队列
	LOLayerData *info = CreateNewLayerData(GetFullID(LOLayer::LAYER_NSSYS, LOLayer::IDEX_NSSYS_EFFECT, 255, 255), printName);
	ntemp = ":c;" + ntemp;
	loadSpCore(info, ntemp, 0, 0, -1);
	info->SetVisable(1);
	//至少加载一次，不然首次加载会失败
	info->texture->activeFirstTexture(); 
	//缩放模式  
	if (IsGameScale()) {
		info->SetPosition2(0, 0, 1.0 / G_gameScaleX, 1.0 / G_gameScaleY);
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
	ef->RunEffect(render, info, effectTex->GetFullTexture(), maskTex->GetFullTexture(), 0);
}

//完成了返回true, 否则返回false
bool LOImageModule::ContinueEffect(LOEffect *ef, const char *printName, double postime) {
	if (ef) { //print 2-8
		int ids[] = { LOLayer::IDEX_NSSYS_EFFECT,255,255 };
		int fullid = GetFullID(LOLayer::LAYER_NSSYS, ids);
		
		/*
		LOLayer *elyr = FindLayerInBase(fullid);
		if (elyr) {
			if (ef->RunEffect(render, elyr->curInfo.get(), effectTex->GetFullTexture(), maskTex->GetFullTexture(), postime)) {
				//前台后台数据都重置
				LOLayerData *data = GetLayerData(fullid, printName);
				if (data) data->SetDelete();
				elyr->curInfo->SetDelete();
				maskTex->FreeData();
				//effectTex经常要使用，所以不释放
				return true;
			}
			else return false;  //we will have next frame.
		}
		else return true; //maybe has some error!
		*/
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
	rect.x = fpstex[0].ww * 3 + 5; rect.y = 5;
	while (fps > 0) {
		int ii = fps % 10;

		rect.w = (int)((double)fpstex[ii].ww / G_gameScaleX);
		rect.h = (int)((double)fpstex[ii].hh / G_gameScaleX);
		//auto ittt = fpstex[ii].GetTexture(NULL);

		////LOLog_i("%d fps number!", ii) ;

		SDL_RenderCopy(render, fpstex[ii].GetFullTexture(), NULL, &rect);
		rect.x -= rect.w;
		fps = fps / 10;
	}
}

void LOImageModule::FreeFps() {
	delete[] fpstex;
	fpstex = nullptr ;
}

bool LOImageModule::InitFps() {
	LOFontWindow fpsconfig;

	fpsconfig.fontName = "*#?";
	fpsconfig.xsize = G_viewRect.h * 14 / 400;
	fpsconfig.ysize = fpsconfig.xsize;
	if (!fpsconfig.Openfont()) {
		SDL_LogError(0, "fps init faild!\n");
		return false;
	}

	fpstex = new LOtextureBase[10];
	SDL_Color cc = { 0, 255, 0 };
	for (int ii = 0; ii < 10; ii++) {
		SDL_Surface *su = LTTF_RenderGlyph_Blended(fpsconfig.font, 0x30 + ii, cc);
		//LOLog_i("fps surface is:%x",su) ;
		if (su) {
			fpstex[ii].SetSurface(su);
		}
	}
	fpsconfig.Closefont();
	return true;
}


LOtextureBase* LOImageModule::RenderText(LOLayerData *info, LOFontWindow *fontwin, LOString *s, SDL_Color *color, int cellcount) {
	//如果画面处于放大模式则缩放参数
	if (G_gameScaleX > 1.0001 || G_gameScaleY > 1.0001) ScaleTextParam(info, fontwin);

	//LOStack<LineComposition> *lines = fontManager.RenderTextCore(su, fontwin, s, color, cellcount, 0);

	//LOString out = writeDir + "/test.bmp" ;
	//SDL_SaveBMP(su->GetSurface(), out.c_str());
	//if(su) LOLog_i("surface is:%x,%d,%d",su,su->W(),su->H()) ;
	//else LOLog_i("surface is NULL!") ;

	return new LOtextureBase();
}

LOtextureBase* LOImageModule::RenderText2(LOLayerData *info, LOFontWindow *fontwin, LOString *s, int startx) {
	//如果画面处于放大模式则缩放参数
	/*
	if (G_gameScaleX > 1.0001 || G_gameScaleY > 1.0001) {
		ScaleTextParam(info, fontwin);
		startx = (int)(G_gameScaleX * startx);
	}
	double perpix = (double)fontwin->xsize / G_textspeed;  //文字每毫秒应显示的像素

	LOSurface *su = nullptr;
	LOStack<LineComposition> *lines = fontManager.RenderTextCore(su, fontwin, s, NULL, 1, startx);

	LOtextureBase *base = nullptr;
	if (su) {
		base = new LOtextureBase;
		base->SetSurface(su);
		LOAnimationText *tai = new LOAnimationText;
		tai->lineInfo = lines;
		//调整最后一行的高度，以便复制整个纹理
		LineComposition *line = lines->top();
		line->height = su->H() - line->y;

		tai->isEnble = true;
		tai->loopdelay = -1;
		tai->perpix = perpix;
		//释放掉所有的字形
		fontManager.FreeAllGlyphs(lines);
		info->ReplaceAction(tai);
		//更新控制信息
		tai->control = LOAnimation::CON_REPLACE;
	}
	*/
	return nullptr;
}

void LOImageModule::ScaleTextParam(LOLayerData *info, LOFontWindow *fontwin) {
	fontwin->xsize = G_gameScaleX * fontwin->xsize;
	fontwin->ysize = G_gameScaleX * fontwin->ysize;
	fontwin->xspace = G_gameScaleX * fontwin->xspace;
	fontwin->yspace = G_gameScaleX * fontwin->yspace;
	info->SetPosition2(0, 0, 1.0 / G_gameScaleX, 1.0 / G_gameScaleY);
}


//做一些准备工作，以便更好的加载
bool LOImageModule::ParseTag(LOLayerData *info, LOString *tag) {
	if (info->fileTextName) {
		SDL_LogError(0, "ONScripterImage::ParseTag() info->fileName not a empty value!");
		return false;
	}
	const char *buf = tag->c_str();
	int alphaMode = trans_mode;
	//LOLog_i("tag is %s",tag->c_str()) ;
	buf = ParseTrans(&alphaMode, tag->c_str());
	if (alphaMode == LOLayerData::TRANS_STRING) {
		info->SetTextureType(LOtexture::TEX_SIMPLE_STR);
		info->fileTextName.reset(new LOString(buf, tag->GetEncoder()));
	}
	else {
		buf = tag->SkipSpace(buf);
		if (buf[0] == '>') {
			buf = tag->SkipSpace(buf + 1);
			info->SetTextureType(LOtexture::TEX_COLOR_AREA);
			info->fileTextName.reset(new LOString(buf, tag->GetEncoder()));
		}
		else if (buf[0] == '*') {
			buf++;
			if (buf[0] == 'd') {
				info->SetTextureType(LOtexture::TEX_DRAW_COMMAND);
			}
			else if (buf[0] == 's') {
				info->SetTextureType(LOtexture::TEX_ACTION_STR);
				info->alphaMode = LOLayerData::TRANS_COPY;
			}
			else if (buf[0] == 'S') {
				info->SetTextureType(LOtexture::TEX_MULITY_STR);
				info->alphaMode = LOLayerData::TRANS_COPY;
			}
			else if (buf[0] == '>') {
				info->SetTextureType(LOtexture::TEX_COLOR_AREA);
				info->alphaMode = LOLayerData::TRANS_COPY;
				//LOLog_i("usecache is %c",info->usecache) ;
			}
			else if (buf[0] == 'b') {
				//"*b;50,100,内容" ;绘制一个NS样式的按钮，通常模式时只显示文字，鼠标悬停时显示变色文字和有一定透明度的灰色
				info->SetTextureType(LOtexture::TEX_NSSIMPLE_BTN);
				info->alphaMode = LOLayerData::TRANS_COPY;
			}
			else if (buf[0] == '*') {
				//** 空纹理，基本上只是为了挂载子对象
				info->SetTextureType(LOtexture::TEX_EMPTY);
			}
			buf += 2;
			info->fileTextName.reset(new LOString(buf, tag->GetEncoder()));
		}
		else {
			info->SetTextureType(LOtexture::TEX_IMG);
			info->alphaMode = alphaMode;
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
		info->SetAction(ac);

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

	if (info->fileTextName) {
		SimpleError("ONScripterImage::ParseImgSP() info->fileName not a empty value!");
		return false;
	}
	else {
		info->fileTextName.reset(new LOString(buf, tag->GetEncoder()));
		return true;
	}
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
//"**;_?_empty_?_"  空的纹理
bool LOImageModule::loadSpCore(LOLayerData *info, LOString &tag, int x, int y, int alpha) {
	return loadSpCoreWith(info, tag, x, y, alpha,0);
}

bool LOImageModule::loadSpCoreWith(LOLayerData *info, LOString &tag, int x, int y, int alpha, int eff) {
	info->SetShowType(LOLayerData::SHOW_NORMAL); //简单模式
	ParseTag(info, &tag);

	LOShareBaseTexture base = GetUseTextrue(info, nullptr, true);
	//LOLog_i("base surface is %x",base->GetSurface()) ;
	//空纹理不参与下面的设置了，以免出现问题
	info->SetNewFile(base);
	if (!base) return false;

	info->SetPosition(x, y);
	if (!info->actions) {  //没有动画则使用默认的宽高
		info->showWidth = info->texture->baseW();
		info->showHeight = info->texture->baseH();
	}
	else {
		info->FirstSNC();
	}

	info->SetAlpha(alpha);

	//记录
	//queLayerMap[info->fullid] = NULL;
	return true;
}



LOShareBaseTexture LOImageModule::GetUseTextrue(LOLayerData *info, void *data, bool addcount) {
	//LOLog_i("info is %x",info) ;
	LOShareBaseTexture base;
	if (!info->isCache()) {
		//唯一性纹理
		std::unique_ptr<LOString> tstr = std::move(info->fileTextName);
		info->fileTextName.reset(new LOString(LOString("c;") + LOString::RandomStr(30)));

		if (info->texType == LOtexture::TEX_ACTION_STR) {
			LOFontWindow ww = winFont;
			//tx = RenderText2(info, &ww, data, 0);
			//LOString::SetStr(info->textStr, data, false);
		}
		else if (info->texType == LOtexture::TEX_SIMPLE_STR) {
			base = TextureFromSimpleStr(info, tstr.get());
			//LOLog_i("TextureFromSimpleStr LOSurface is %x",tx->GetSurface()) ;
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
		return base;
	}
	else {
		//透明模式对纹理是有影响的，要将这个考虑在内
		char cc[3] = {info->alphaMode , ';', 0};
		std::unique_ptr<LOString> tstr(new LOString(cc));
		tstr->append(*info->fileTextName);
		base = LOtexture::findTextureBaseFromMap( *tstr );  // new LOTexture时会自动增加base
		//有效
		if (base) return base;
		switch (info->texType) {
		case LOtexture::TEX_IMG:
			base = TextureFromFile(info);
			break;
		case LOtexture::TEX_COLOR_AREA:
			base = TextureFromColor(info);
			break;
		case LOtexture::TEX_EMPTY:
			base = TextureFromEmpty(info);
			break;
		default:
			LOLog_e(0, "ONScripterImage::GetUseTextrue() unkown Textrue type:%d", info->texType);
			break;
		}

		info->fileTextName = std::move(tstr);
		if(base) LOtexture::addTextureBaseToMap(*info->fileTextName, base);
		return base;
	}
	return base;
}

//单行文字

LOShareBaseTexture LOImageModule::TextureFromSimpleStr(LOLayerData*info, LOString *s) {
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

	LOString text(buf);
	text.SetEncoder(s->GetEncoder());
	LOShareBaseTexture base;
	if (colorList.size() == 0) return base;
	LOTextTexture *texture = new LOTextTexture();
	
	//先创建文字描述
	int w, h;
	if (!texture->CreateTextDescribe(&text, &style, &spFontName)) {
		delete texture;
		return base;
	}

	texture->GetSurfaceSize(&w, &h);
	texture->CreateSurface(w * colorList.size(), h);

	//将文本渲染到纹理上
	for (int ii = 0; ii < colorList.size(); ii++) {
		texture->RenderTextSimple(w * ii, 0, colorList.at(ii));
	}
	//单字和文字区域已经不需要了
	texture->ClearTexts();
	texture->ClearWords();

	base.reset(new LOtextureBase());

	//SDL_SaveBMP(texture->surface, "test.bmp");
	return base;
}

//从标记中生成色块
LOShareBaseTexture LOImageModule::TextureFromColor(LOLayerData *info) {
	LOShareBaseTexture base;

	LOString *s = info->fileTextName.get();
	const char *buf = s->SkipSpace(s->c_str());

	int w = s->GetInt(buf);
	while (buf[0] == ',' || buf[0] == ' ') buf++;
	int h = s->GetInt(buf);

	while (buf[0] != '#' && buf[0] != '\0') buf++;
	if (buf[0] != '#') return base;
	buf++;
	int color = s->GetHexInt(buf, 6);

	base.reset(new LOtextureBase(CreateRGBSurfaceWithFormat(0, w, h, 8, SDL_PIXELFORMAT_INDEX8)));
	SDL_Palette *pale = base->GetSurface()->format->palette;
	SDL_Color *cc = pale->colors;
	cc->r = (color >> 16) & 0xff;
	cc->g = (color >> 8) & 0xff;
	cc->b = color & 0xff;

	return base;
}
/*
LOtextureBase* LOImageModule::TextureFromNSbtn(LOLayerInfo*info, LOString *s) {
	LOtextureBase *base = nullptr;
	return base;
}           
*/

LOShareBaseTexture LOImageModule::TextureFromEmpty(LOLayerData *info) {
	LOShareBaseTexture base(new LOtextureBase());
	base->ww = G_gameWidth;
	base->hh = G_gameHeight;
	LOtexture::addTextureBaseToMap( *(info->fileTextName.get()) , base);
	return base;
}

LOShareBaseTexture LOImageModule::TextureFromFile(LOLayerData *info) {
	bool useAlpha;
	LOShareBaseTexture base(SurfaceFromFile(info->fileTextName.get()));
	if (!base) return base;
	//转换透明格式 
	if (info->alphaMode != LOLayerData::TRANS_COPY && !base->hasAlpha()) {
		if (info->alphaMode == LOLayerData::TRANS_ALPHA && !base->ispng) {
			base->SetSurface(LOtextureBase::ConverNSalpha(base->GetSurface(), info->GetCellCount()));
			if(!base->isValid()) LOLog_i("Conver image ns alhpa faild: %s", info->fileTextName->c_str());
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

	LOString s = info->fileTextName->toLower() + "?" + std::to_string(info->alphaMode) + ";";
	LOtexture::addTextureBaseToMap(s, base);
	return base;
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
	if ( dialogDisplayMode == DISPLAY_MODE_NORMAL|| force) {
		DialogWindowSet(1, 1, 1);
		dialogDisplayMode = DISPLAY_MODE_TEXT;
		dialogTextHasChange = false;
	}
}


void LOImageModule::LeveTextDisplayMode(bool force) {
	if ( dialogDisplayMode != DISPLAY_MODE_NORMAL && (force || winEraseFlag != 0)) {
		DialogWindowSet(0, 0, 0);
		dialogDisplayMode = DISPLAY_MODE_NORMAL;
	}
}

//控制对话框和文字的显示状态，负数表示不改变，0隐藏，1显示
void LOImageModule::DialogWindowSet(int showtext, int showwin, int showbmp) {
	int fullid, index, haschange = 0;
	int ids[] = { LOLayer::IDEX_DIALOG_TEXT,255,255 };
	//文字显示
	if (showtext >= 0) {
		fullid = GetFullID(LOLayer::LAYER_DIALOG, ids);
		int ret;
		if (showtext) ret = ShowLayer(fullid, "_lons");
		else ret = HideLayer(fullid, "_lons");
		if (ret >= 0) haschange |= ret;
	}
	//对话框显示
	if (showwin >= 0) {
		ids[0] = LOLayer::IDEX_DIALOG_WINDOW;
		fullid = GetFullID(LOLayer::LAYER_DIALOG, ids);

		if (dialogTextHasChange) { //updata
			LoadDialogWin();
			haschange |= 2;
		}
		else if (showwin) { //show
			int ret = ShowLayer(fullid, "_lons");
			if (ret < 0 && winstr.length() > 0) {
				LoadDialogWin();
				haschange |= 2;
			}
			else if (ret == 1) haschange |= 2;
		}
		else { //hide
			if (HideLayer(fullid, "_lons")) haschange |= 2;
		}
	}
	//文字后的符号显示
	if (haschange > 1) DialogWindowPrint();
	else if (haschange == 1) ExportQuequ("_lons", NULL, true);
	return;
}

bool LOImageModule::LoadDialogWin() {
	/*
	int ids[] = { LOLayer::IDEX_DIALOG_WINDOW,255,255 };
	int fullid = GetFullID(LOLayer::LAYER_DIALOG, ids);
	if (winstr.length() > 0) {
		LOLayerInfo* info = GetInfoNewAndFreeOld(fullid, "_lons");
		if(fontManager.rubySize[2] == LOFontManager::RUBY_ON)  loadSpCore(info, winstr, winoff.x + fontManager.rubySize[0], winoff.y + fontManager.rubySize[1], 255);
		else loadSpCore(info, winstr, winoff.x, winoff.y, 255);
		if (info->loadType == LOtexture::TEX_COLOR_AREA) {
			if (info->texture) {
				info->texture->setAplhaModel(-1);
				info->texture->setBlendModel(SDL_BLENDMODE_MOD);
				info->texture->setColorModel(255, 255, 255);
			}
		}
	}
	else {
		LOLayerInfo* info = GetInfoNewAndFreeOld(fullid, "_lons");
		info->SetLayerDelete();
	}
	*/
	return true;
}

//隐藏图层，0表示无需刷新状态，1表示需刷新状态
int LOImageModule::HideLayer(int fullid, const char *printName) {
	/*
	int sstate;
	LOLayerInfo *cinfo = LayerInfomation(fullid, printName);
	if (cinfo && cinfo->visiable) {
		LOLayerInfo *info = GetInfoNewAndNoFreeOld(fullid, printName);
		info->SetVisable(0);
		sstate = 1;
	}
	else sstate = 0;

	if (cinfo)delete cinfo;
	*/
	//return sstate;
	return 0;
}

//显示图层，0表示无需刷新状态，1表示需刷新状态，-1表示图层不存在
int LOImageModule::ShowLayer(int fullid, const char *printName) {
	/*
	int sstate;
	LOLayerInfo *cinfo = LayerInfomation(fullid, printName);
	if (!cinfo || cinfo->visiable == 0) { //图层不存在或者隐藏的
		LOLayerInfo *info = GetInfoNewAndNoFreeOld(fullid, printName);
		info->SetVisable(1);
		sstate = 1;
	}
	else if (cinfo && cinfo->visiable) sstate = 0;
	else sstate = -1;

	if (cinfo)delete cinfo;
	return sstate;
	*/
	return 0;
}


void LOImageModule::DialogWindowPrint() {
	LOEffect *ef = nullptr;
	if (winEffect) {
		ef = new LOEffect;
		ef->CopyFrom(winEffect);
		ExportQuequ("_lons", ef, true);
		delete ef;
	}
	else ExportQuequ("_lons", nullptr, true);
}

//文字显示进入队列
bool LOImageModule::LoadDialogText(LOString *s, bool isAdd) {
	/*
	int ids[] = { LOLayer::IDEX_DIALOG_TEXT,255,255 };
	LOLayerInfo *info = GetInfoNewAndFreeOld(GetFullID(LOLayer::LAYER_DIALOG, ids), "_lons");

	LOLayer *lyr = nullptr;
	int startline = 0;
	int startpos = 0;

	if (isAdd) {
		isAdd = false;
		lyr = FindLayerInBase(LOLayer::LAYER_DIALOG, ids);
		if (lyr) {
			//确认下一行的开始位置
			//LOAnimationText *text = (LOAnimationText*)lyr->curInfo->GetAnimation(LOAnimation::ANIM_TEXT);
			//if (text) {
			//	startline = text->lineInfo->size() - 1;
			//	startpos = text->lineInfo->top()->sumx;
			//	isAdd = true;
			//}
		}
	}

	LOString tag = "*s;" + (*s);
	loadSpCore(info, tag, winFont.topx, winFont.topy + fontManager.GetRubyPreHeight(), 255);
	if (isAdd) {
		LOAnimationText *text = (LOAnimationText*)info->GetAnimation(LOAnimation::ANIM_TEXT);
		if (text) {
			text->currentLine = startline;
			text->currentPos = startpos;
			text->isadd = true;
			//SDL_SaveBMP(text->su, "test.bmp");
		}
	}
	dialogTextHasChange = true;
	*/
	return true;
}


//创建新图层，会释放原来的老的数据，lsp使用
LOLayerData* LOImageModule::CreateNewLayerData(int fullid, const char *printName) {
	LOLayer *lyr = LOLayer::CreateLayer(fullid);
	lyr->bakInfo->SetDelete();
	auto *map = GetPrintNameMap(printName)->map;
	(*map)[fullid] = lyr;
	return lyr->bakInfo.get();
}

//对已经存在的对象获取后台数据，后台为delete返回空，不可能操作已经删除的对象
//其他如果layer存在则返回layer的后台，通常是为操作图层使用
LOLayerData* LOImageModule::CreateLayerBakData(int fullid, const char *printName) {
	LOLayer *lyr = LOLayer::FindLayerInCenter(fullid);
	if (!lyr || lyr->bakInfo->isDelete()) return nullptr;
	auto *map = GetPrintNameMap(printName)->map;
	(*map)[fullid] = lyr;
	return lyr->bakInfo.get();
}

//对已经存在的对象获取前台数据，后台为delete返回空，不可能读取已删除对象的数据
//其他如果后台为newfile则返回后台，否则返回存在对象的前台，通常是为读取图层信息
LOLayerData* LOImageModule::GetLayerInfoData(int fullid, const char *printName) {
	LOLayer *lyr = LOLayer::FindLayerInCenter(fullid);
	if (!lyr || lyr->bakInfo->isDelete()) return nullptr;
	if (lyr->bakInfo->isNewFile()) return lyr->bakInfo.get();
	else return lyr->curInfo.get();
}




LOImageModule::PrintNameMap* LOImageModule::GetPrintNameMap(const char *printName) {
	for (int ii = 0; ii < backDataMaps.size(); ii++) {
		if (backDataMaps[ii]->mapName->compare(printName) == 0) return backDataMaps[ii].get();
	}
	backDataMaps.push_back(std::make_unique<PrintNameMap>(printName));
	return backDataMaps[backDataMaps.size() - 1].get();
}