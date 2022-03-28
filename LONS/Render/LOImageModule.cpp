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
	screenshotSu = NULL;
	winEffect = NULL ;

    ResetConfig();

	layerQueMutex = SDL_CreateMutex();
	btnQueMutex = SDL_CreateMutex();
	presentMutex = SDL_CreateMutex();
	doQueMutex = SDL_CreateMutex();
	poolMutex = SDL_CreateMutex();

	int ids[] = { 0,-1,-1 };
	for (int ii = 0; ii < LOLayer::LAYER_BASE_COUNT; ii++) {
		ids[0] = ii;
		lonsLayers[ii] = new LOLayer(LOLayer::LAYER_CC_USE, ids);
	}

	memset(shaderList, 0, sizeof(int) * 20);
	queLayerinfoMapStart("_lons");
	queLayerinfoMapStart("_main");
}

void LOImageModule::ResetConfig() {
	tickTime = 0;
	z_order = 499;
	trans_mode = LOLayerInfo::TRANS_TOPLEFT;
	effectSkipFlag = false;
	winbackMode = false;
	textbtnFlag = true;
	texecFlag = false;
	exbtn_d_hasrun = false;
	st_filelog = false;

	winFont.Reset();
	winFont.xsize = 20;
	winFont.ysize = 20;
	spFont.Reset();
	fontManager.ResetMe();

	textbtnValue = 1;
	winEraseFlag = 1;  //默认print的时候隐藏对话框
	winoff = { 0,0,0,0 };

	btndefStr.clear();
	BtndefCount = 0;
	exbtn_count = 0;
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
	SDL_DestroyMutex(btnQueMutex);
	SDL_DestroyMutex(layerQueMutex);
	SDL_DestroyMutex(presentMutex);
	SDL_DestroyMutex(doQueMutex);
	SDL_DestroyMutex(poolMutex);
	for (int ii = 0; ii < LOLayer::LAYER_BASE_COUNT; ii++) {
		delete lonsLayers[ii];
	}

	FreeFps();
	if (allSpList) delete allSpList;
	if (allSpList2)delete allSpList2;
	if (screenshotSu) delete screenshotSu;
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

	if (effectTex) DestroyTexture(effectTex);
	effectTex = CreateTexture(render, G_Texture_format, SDL_TEXTUREACCESS_TARGET, G_viewRect.w, G_viewRect.h);

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
	bool minisize = false;
	moduleState = MODULE_STATE_RUNNING;

	while (loopflag) {
		hightTimeNow = SDL_GetPerformanceCounter();
		posTime = ((double)(hightTimeNow - lastTime)) / perHtickTime;

		if (!minisize && posTime + 0.1 > fpstime) {
			if (RefreshFrame(posTime) == 0) {
				//LOLog_i("frame ok!") ;
				//now do the delay event.like send finish signed.
				DoDelayEvent(posTime);
				lastTime = hightTimeNow;
				if(moduleState >= MODULE_STATE_EXIT) break;
				else if (moduleState & MODULE_STATE_RESET) {
					ResetMe();
				}
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
				CaptureEvents(&ev);
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

	int lockfalg = SDL_TryLockMutex(layerQueMutex);
	if (lockfalg == 0) {
		//必须小心处理渲染逻辑，不然很容导致程序崩溃，关键是正确处理 print 1 以外的多线程同步问题
		//print 1: UpDisplay
		//print other(first frame): get signed -> RenderTarget -> UpDisplay -> RenderCopy(effectTex) -> call thread
		//thread: get prepare finish -> uplayer -> wait effect finish
		//(second frame):enter effecting ->(if cut) -> send finish signed.

		//收到请求后，本帧会刷新到effct层上
		if (printPreHook.enterEdit()) {
			//if (premsg->loadParamInt(0) == PARAM_BGCOPY) {
			//	SDL_Texture *bgtex = SDL_CreateTexture(render, G_Texture_format, SDL_TEXTUREACCESS_TARGET, G_viewRect.w, G_viewRect.h);
			//	premsg->savePtr_t(true, bgtex, 0, LOEvent::VALUE_SDLTEXTURE_PTR);
			//	SDL_SetRenderTarget(render, bgtex);
			//}
			//else if (SDL_SetRenderTarget(render, effectTex) < 0) SimpleError("SDL_SetRenderTarget(effectTexture) faild!");

			if (SDL_SetRenderTarget(render, effectTex) < 0) LOLog_e("SDL_SetRenderTarget(effectTexture) faild!");
			SDL_RenderSetScale(render, G_gameScaleX, G_gameScaleY);  //跟窗口缩放保持一致
			SDL_RenderClear(render);
			UpDisplay(postime);
			SDL_SetRenderTarget(render, NULL);
			SDL_RenderCopy(render, effectTex, NULL, NULL);
			//LOLog_i("prepare ok!") ;
			//需要在本帧刷新后通知事件已经完成，因此将一个前置事件推入渲染模块队列
			LOEventHook ev(new LOEventHook_t());
			ev->catchFlag = PRE_EVENT_PREPRINTOK;
			preEventList.push_back(ev);
			//LOLog_i("prepare event change!") ;
		}
		else {
			SDL_RenderClear(render);
			UpDisplay(postime);
		}
		if(isShowFps) ShowFPS(postime);
		SDL_RenderPresent(render);

		SDL_UnlockMutex(layerQueMutex);

		//每完成一帧的刷新，我们检查是否有 print 事件需要处理，如果是print 1则直接通知完成
		//这里有个隐含的条件，在脚本线程展开队列时，绝对不会进入RefreshFrame刷新，所以如果有MSG_Wait_Print表示已经完成print的第一帧刷新
		//如果是print 2-18,我们将检查effect的运行情况
		if (printHook.enterEdit()) {
			LOEventHook ev(new LOEventHook_t());
			ev->catchFlag = PRE_EVENT_EFFECTCONTIUE;
			preEventList.push_back(ev);
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
	UpDataLayer(lonsLayers[LOLayer::LAYER_BG], tickTime, 1023, 0, 0);
	UpDataLayer(lonsLayers[LOLayer::LAYER_SPRINT], tickTime, 1023, z_order + 1, 0);
	UpDataLayer(lonsLayers[LOLayer::LAYER_STAND], tickTime, 1023, 0, 0);
	if (winbackMode) {
		UpDataLayer(lonsLayers[LOLayer::LAYER_OTHER], tickTime, 1023, 0, 0);
		UpDataLayer(lonsLayers[LOLayer::LAYER_SPRINTEX], tickTime, 1023, 0, 0);
		UpDataLayer(lonsLayers[LOLayer::LAYER_DIALOG], tickTime, 1023, 0, 0);
		UpDataLayer(lonsLayers[LOLayer::LAYER_SPRINT], tickTime, z_order, 0, 0);
		UpDataLayer(lonsLayers[LOLayer::LAYER_SELECTBAR], tickTime, 1023, 0, 0);
	}
	else {
		UpDataLayer(lonsLayers[LOLayer::LAYER_SPRINT], tickTime, z_order, 0, 0);
		UpDataLayer(lonsLayers[LOLayer::LAYER_SPRINTEX], tickTime, 1023, 0, 0);
		UpDataLayer(lonsLayers[LOLayer::LAYER_OTHER], tickTime, 1023, 0, 0);
		UpDataLayer(lonsLayers[LOLayer::LAYER_SELECTBAR], tickTime, 1023, 0, 0);
		UpDataLayer(lonsLayers[LOLayer::LAYER_DIALOG], tickTime, 1023, 0, 0);
	}
	//UpDataLayer(lonsLayers[LOLayer::LAYER_BUTTON], tickTime, 1023, 0);
	UpDataLayer(lonsLayers[LOLayer::LAYER_NSSYS], tickTime, 1023, 0, 0);
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
			//检查纹理是否已经被加载，没有则加载
			//LOtexture *tex = lyr->curInfo->texture;
			//运行图层的action
			if (lyr->curInfo->actions) {
				lyr->DoAnimation(lyr->curInfo, curTime);
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

	LOString ntemp("_?_effect_?_");      //care trigraph or Digraph
	LOtextureBase *base = LOtexture::addNewEditTexture(ntemp, G_viewRect.w, G_viewRect.h, G_Texture_format, SDL_TEXTUREACCESS_TARGET);
	//除效果10外，其他效果均需要特殊调制
	if (ef->nseffID != 10) {
		maskTex = CreateTexture(render, G_Texture_format, SDL_TEXTUREACCESS_STREAMING, G_viewRect.w, G_viewRect.h);
	}
	//将材质覆盖到最前面进行遮盖
	//效果层处于哪一个排列队列必须跟 ExportQuequ中的一致，不然无法立即展开队列
	LOLayerInfo *info = GetInfoNewAndFreeOld(GetFullID(LOLayer::LAYER_NSSYS, LOLayer::IDEX_NSSYS_EFFECT, 255, 255), printName);
	ntemp = ":c;" + ntemp;
	loadSpCore(info, ntemp, 0, 0, -1);
	info->texture->activeTexture(nullptr); //至少加载一次，不然首次加载会失败
	//缩放模式  
	if (IsGameScale()) {
		info->SetPosition2(0, 0, 1.0 / G_gameScaleX, 1.0 / G_gameScaleY);
	}

	//加载遮片
	if (ef->nseffID == 15 || ef->nseffID == 18) {
		LOSurface *su = SurfaceFromFile(&ef->mask);
		//遮片不存在按渐变处理
		if (!su) ef->nseffID = 10;
		else {
			SDL_Surface *tmp = ef->Create8bitMask(su->GetSurface(), true);
			ef->masksu = new LOSurface(tmp);
			delete su;
		}
	}
	ef->ReadyToRun();
	//准备好第一帧的运行
	ef->RunEffect(render, info, effectTex, maskTex, 0);
}

//完成了返回true, 否则返回false
bool LOImageModule::ContinueEffect(LOEffect *ef, double postime) {
	if (ef) { //print 2-8
		int ids[] = { LOLayer::IDEX_NSSYS_EFFECT,255,255 };
		LOLayer *elyr = FindLayerInBase(LOLayer::LAYER_NSSYS, ids);
		if (elyr) {
			if (ef->RunEffect(render, elyr->curInfo, effectTex, maskTex, postime)) {
				elyr->FreeData();
				if (maskTex) {
					DestroyTexture(maskTex);
					maskTex = nullptr;
				}
				return true;
			}
			else return false;  //we will have next frame.
		}
		else return true; //maybe has some error!
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
		auto ittt = fpstex[ii].GetTexture(NULL);

		//LOLog_i("%d fps number!", ii) ;

		SDL_RenderCopy(render, fpstex[ii].GetTexture(NULL), NULL, &rect);
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
			fpstex[ii].SetSurface(new LOSurface(su));
			fpstex[ii].GetTexture(NULL);
		}
	}
	fpsconfig.Closefont();
	return true;
}


LOtextureBase* LOImageModule::RenderText(LOLayerInfo *info, LOFontWindow *fontwin, LOString *s, SDL_Color *color, int cellcount) {
	LOSurface *su = nullptr;

	//如果画面处于放大模式则缩放参数
	if (G_gameScaleX > 1.0001 || G_gameScaleY > 1.0001) ScaleTextParam(info, fontwin);

	LOStack<LineComposition> *lines = fontManager.RenderTextCore(su, fontwin, s, color, cellcount, 0);

	//LOString out = writeDir + "/test.bmp" ;
	//SDL_SaveBMP(su->GetSurface(), out.c_str());
	//if(su) LOLog_i("surface is:%x,%d,%d",su,su->W(),su->H()) ;
	//else LOLog_i("surface is NULL!") ;

	return new LOtextureBase(su);
}

LOtextureBase* LOImageModule::RenderText2(LOLayerInfo *info, LOFontWindow *fontwin, LOString *s, int startx) {
	//如果画面处于放大模式则缩放参数
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

	return base;
}

void LOImageModule::ScaleTextParam(LOLayerInfo *info, LOFontWindow *fontwin) {
	fontwin->xsize = G_gameScaleX * fontwin->xsize;
	fontwin->ysize = G_gameScaleX * fontwin->ysize;
	fontwin->xspace = G_gameScaleX * fontwin->xspace;
	fontwin->yspace = G_gameScaleX * fontwin->yspace;
	info->SetPosition2(0, 0, 1.0 / G_gameScaleX, 1.0 / G_gameScaleY);
}


//做一些准备工作，以便更好的加载
bool LOImageModule::ParseTag(LOLayerInfo *info, LOString *tag) {
	if (info->fileName) {
		SDL_LogError(0, "ONScripterImage::ParseTag() info->fileName not a empty value!");
		return false;
	}
	const char *buf = tag->c_str();
	int alphaMode = trans_mode;
	//LOLog_i("tag is %s",tag->c_str()) ;
	buf = ParseTrans(&alphaMode, tag->c_str());
	if (alphaMode == LOLayerInfo::TRANS_STRING) {
		info->loadType = LOtexture::TEX_SIMPLE_STR;
		info->usecache = false;
		info->fileName = new LOString(buf, tag->GetEncoder());
	}
	else {
		buf = tag->SkipSpace(buf);
		if (buf[0] == '>') {
			buf = tag->SkipSpace(buf + 1);
			info->loadType = LOtexture::TEX_COLOR_AREA;
			info->usecache = true;
			info->fileName = new LOString(buf, tag->GetEncoder());
		}
		else if (buf[0] == '*') {
			buf++;
			if (buf[0] == 'd') {
				info->loadType = LOtexture::TEX_DRAW_COMMAND;
				info->usecache = false;
			}
			else if (buf[0] == 's') {
				info->loadType = LOtexture::TEX_ACTION_STR;
				info->alphaMode = LOLayerInfo::TRANS_COPY;
				info->usecache = false;
			}
			else if (buf[0] == 'S') {
				info->loadType = LOtexture::TEX_MULITY_STR;
				info->alphaMode = LOLayerInfo::TRANS_COPY;
				info->usecache = false;
			}
			else if (buf[0] == '>') {
				info->loadType = LOtexture::TEX_COLOR_AREA;
				info->alphaMode = LOLayerInfo::TRANS_COPY;
				info->usecache = true;
				//LOLog_i("usecache is %c",info->usecache) ;
			}
			else if (buf[0] == 'b') {
				//"*b;50,100,内容" ;绘制一个NS样式的按钮，通常模式时只显示文字，鼠标悬停时显示变色文字和有一定透明度的灰色
				info->loadType = LOtexture::TEX_NSSIMPLE_BTN;
				info->alphaMode = LOLayerInfo::TRANS_COPY;
				info->usecache = false;
			}
			else if (buf[0] == '*') {
				//** 空纹理，基本上只是为了挂载子对象
				info->loadType = LOtexture::TEX_EMPTY;
				info->usecache = true;
			}
			buf += 2;
			info->fileName = new LOString(buf, tag->GetEncoder());
		}
		else {
			info->loadType = LOtexture::TEX_IMG;
			info->alphaMode = alphaMode;
			info->usecache = true;
			ParseImgSP(info, tag, buf);
		}
	}
	//info->alphaMode = info->GetTextureFlag(1);
	//info->CreateSecondFlag(info->GetCellCount());
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
			*alphaMode = LOLayerInfo::TRANS_ALPHA;
			buf++;
		}
		else if (buf[0] == 'l') {  //左上角
			*alphaMode = LOLayerInfo::TRANS_TOPLEFT;
			buf++;
		}
		else if (buf[0] == 'r') {  //右上角
			*alphaMode = LOLayerInfo::TRANS_TOPRIGHT;
			buf++;
		}
		else if (buf[0] == 'c') {  //不使用透明
			*alphaMode = LOLayerInfo::TRANS_COPY;
			buf++;
		}
		else if (buf[0] == 's') {  //string
			*alphaMode = LOLayerInfo::TRANS_STRING;
			buf++;
		}
		else if (buf[0] == 'm') {  //遮片
			*alphaMode = LOLayerInfo::TRANS_MASK;
		}
		else if (buf[0] == '#') { //指定颜色
			*alphaMode = LOLayerInfo::TRANS_DIRECT;
		}
		else if (buf[0] == '!') { //画板
			*alphaMode = LOLayerInfo::TRANS_PALLETTE;
		}
		if (buf[0] == ';') buf++;
	}
	return buf;
}

bool LOImageModule::ParseImgSP(LOLayerInfo *info, LOString *tag, const char *buf) {
	buf = tag->SkipSpace(buf);
	if (buf[0] == '/') {
		LOAnimationNS *anim = new LOAnimationNS;
		buf++;
		anim->cellCount = tag->GetInt(buf);
		if (anim->cellCount == 0) {
			SDL_Log("Animation grid number cannot be 0!\n");
			delete anim;
			return false;
		}
		info->ReplaceAction(anim);

		//读取每格的时间
		anim->perTime = new int[anim->cellCount];
		//有逗号表示继续对时间进行描述
		if (buf[0] == ',') {
			buf++;
			if (buf[0] == '<') { //对每格时间进行描述 lsp 150,":a/10,<100 200 300 400 500 600 700 800 900 1000>,0;data\erk.png",69,113
				buf++;
				for (int ii = 0; ii < anim->cellCount; ii++) {
					*(anim->perTime + ii) = tag->GetInt(buf);
					buf++;
				}
			}
			else {  //只有一格数表示每格的时间是一样的
				int temp = tag->GetInt(buf);
				for (int ii = 0; ii < anim->cellCount; ii++)
					*(anim->perTime + ii) = temp;
			}
			buf++;
			anim->loopMode = buf[0] - '0'; // 3...no animation
		}
		else { //表示只划分格，不做动画
			for (int ii = 0; ii < anim->cellCount; ii++)
				*(anim->perTime + ii) = 0;
			anim->loopMode = 3;
		}
		anim->cellCurrent = 0; //总是指向马上要执行的帧
		anim->cellForward = 1; //1或者-1
		anim->isEnble = true;
		anim->control = LOAnimation::CON_REPLACE;
		while (buf[0] != ';' && buf[0] != '\0') buf++;
	}
	if (buf[0] == ';') buf++;

	if (info->fileName) {
		SimpleError("ONScripterImage::ParseImgSP() info->fileName not a empty value!");
		return false;
	}
	else {
		info->fileName = new LOString(buf, tag->GetEncoder());
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

void LOImageModule::FreeLayerInfoData(LOLayerInfo *info) {
	if (info->texture) delete info->texture;
	if (info->actions) LOAnimation::FreeAnimaList(&info->actions);
	if (info->fileName) delete info->fileName;
	if (info->maskName) delete info->maskName;
	if (info->btnStr) delete info->btnStr;
	if (info->textStr) delete info->textStr;
	info->texture = nullptr;
	info->actions = nullptr;
	info->fileName = nullptr;
	info->maskName = nullptr;
	info->btnStr = nullptr;
	info->textStr = nullptr;
}

LOLayer* LOImageModule::FindLayerInBase(LOLayer::SysLayerType type, const int *ids) {
	return lonsLayers[type]->FindChild(ids);
}

LOLayer* LOImageModule::FindLayerInBase(int fullid) {
	LOLayer::SysLayerType type;
	int ids[] = { -1,-1,-1 };
	GetTypeAndIds( (int*)(&type), ids, fullid);
	return lonsLayers[type]->FindChild(ids);
}

LOLayer* LOImageModule::GetRootLayer(int fullid) {
	int ids[3];
	LOLayer::SysLayerType type;
	GetTypeAndIds((int*)(&type), ids, fullid);
	return lonsLayers[type];
}


//移除按钮定义，-1移除所有，-2移除btn定义的
void LOImageModule::RemoveBtn(int fullid) {
	if (fullid == -1 || fullid == -2) {
		SDL_LockMutex(btnQueMutex);
		if (fullid == -1) btnMap.clear();
		else {
			auto iter = btnMap.begin();
			while (iter != btnMap.end()) {
				if (iter->second->layerType == LOLayer::LAYER_NSSYS) iter = btnMap.erase(iter);
				else iter++;
			}
		}
		SDL_UnlockMutex(btnQueMutex);
		//移除队列中所有的按钮定义
		SDL_LockMutex(poolMutex);
		for (int ii = 0; ii < poolData.size(); ii++) {
			LOLayerInfoCacheIndex *minfo = poolData.at(ii);
			bool isDest = true;
			if (fullid == -2) isDest = (GetIDs(minfo->info.fullid, IDS_LAYER_TYPE) == LOLayer::LAYER_NSSYS);
			if (minfo->iswork && isDest) minfo->info.UnsetBtn();
		}
		SDL_UnlockMutex(poolMutex);
	}
	else {
		auto iter = btnMap.find(fullid);
		if (iter != btnMap.end()) btnMap.erase(iter);
	}
}


//tag定义：
//":a/10,400,0;map\ais.png" 普通图像sp
//":s/40,40,0;#00ffff#ffff00$499" 文字sp
//":c;>50,50,#ff00ff" 色块sp
//"*d;rect,50,100" 绘图sp
//"*s;$499" 对话框文字sp，特效跟随setwindow的值
//"*S;文本" 多行sp
//"*>;50,100,#ff00ff#ffffff" 绘制一个色块，并使用正片叠底模式
bool LOImageModule::loadSpCore(LOLayerInfo *info, LOString &tag, int x, int y, int alpha) {
	return loadSpCoreWith(info,tag, x, y, alpha,0);
}

bool LOImageModule::loadSpCoreWith(LOLayerInfo *info, LOString &tag, int x, int y, int alpha, int eff) {
	info->SetShowType(LOLayerInfo::SHOW_NORMAL); //简单模式
	ParseTag(info, &tag);

	LOtextureBase *base = GetUseTextrue(info, nullptr, true);
	//LOLog_i("base surface is %x",base->GetSurface()) ;

	if (!base) { //没有纹理图层是没有存在的意义的
		FreeLayerInfoData(info);
		info->SetLayerDelete();
		return false;
	}
	info->SetNewFile(new LOtexture(base));
	info->SetPosition(x, y);
	if (!info->actions) {  //没有动画则使用默认的宽高
		info->showWidth = base->ww;
		info->showHeight = base->hh;
	}
	else {
		for (int ii = 0; ii < info->actions->size(); ii++) {
			info->FirstSNC(info->actions->at(ii));
		}
	}

	info->SetAlpha(alpha);

	//记录
	//queLayerMap[info->fullid] = NULL;
	return true;
}



LOtextureBase* LOImageModule::GetUseTextrue(LOLayerInfo *info, void *data, bool addcount) {
	//LOLog_i("info is %x",info) ;
	if (!info->usecache) { //唯一性纹理
		LOString *data = info->fileName;
		info->fileName = new LOString;
		*info->fileName = LOString("c;") + LOString::RandomStr(30);

		LOtextureBase *tx = nullptr;
		if (info->loadType == LOtexture::TEX_ACTION_STR) {
			LOFontWindow ww = winFont;
			tx = RenderText2(info, &ww, data, 0);
			LOString::SetStr(info->textStr, data, false);
		}
		else if (info->loadType == LOtexture::TEX_SIMPLE_STR) {
			tx = TextureFromSimpleStr(info, data);
			//LOLog_i("TextureFromSimpleStr LOSurface is %x",tx->GetSurface()) ;
		}
		else if (info->loadType == LOtexture::TEX_MULITY_STR) {
			tx = TextureFromSimpleStr(info, data);
		}
		else if (info->loadType == LOtexture::TEX_NSSIMPLE_BTN) {
			tx = TextureFromNSbtn(info, data);
		}
		else {
			LOString errs = StringFormat(128, "ONScripterImage::GetUseTextrue() unkown Textrue type:%d", info->loadType);
			//LOLog_e("%s",errs.c_str());
			SimpleError(errs.c_str());
		}
		delete data;
		LOtexture::addTextureBaseToMap(*info->fileName, tx);
		return tx;
	}
	else { //非唯一性纹理
		LOtextureBase *tx = LOtexture::findTextureBaseFromMap( *info->fileName, false);  // new LOTexture时会自动增加base
		if (tx) return tx;
		switch (info->loadType) {
		case LOtexture::TEX_IMG:
			tx = TextureFromFile(info);
			break;
		case LOtexture::TEX_COLOR_AREA:
			tx = TextureFromColor(info);
			break;
		case LOtexture::TEX_EMPTY:
			tx = EmptyTexture(info->fileName); //workhere
			break;
		default:
			LOLog_e(0, "ONScripterImage::GetUseTextrue() unkown Textrue type:%d", info->loadType);
			break;
		}
		return tx;
	}
	return nullptr;
}

//单行文字
LOtextureBase* LOImageModule::TextureFromSimpleStr(LOLayerInfo*info, LOString *s) {
	LOFontWindow *fontstyle = NULL;
	SDL_Color *colors = NULL;
	int cellcount = 1;
	const char *buf = s->c_str();
	bool isnew = (info->maskName == NULL);
	//样式
	if (!isnew) {
		fontstyle = (LOFontWindow*)info->maskName;
		colors = (SDL_Color*)info->btnStr;
		cellcount = info->btnValue;
		info->maskName = NULL;
		info->btnStr = NULL;
		info->btnValue = 0;
	}
	else {
		fontstyle = new LOFontWindow();
		*fontstyle = spFont;
		while (buf[0] == '/' || buf[0] == ',' || buf[0] == ' ') buf++;
		fontstyle->xsize = s->GetInt(buf);
		while (buf[0] == ',' || buf[0] == ' ') buf++;
		fontstyle->ysize = s->GetInt(buf);
		while (buf[0] == ',' || buf[0] == ' ') buf++;
		fontstyle->isshaded = s->GetInt(buf);
		if (buf[0] == ',') { //what?
			buf++;
			fontstyle->xspace = s->GetInt(buf);
		}
		if (buf[0] == ',') { //what?
			buf++;
			fontstyle->yspace = s->GetInt(buf);
		}
		while (buf[0] == ',') {  //看着老ons源码有这个，不知道是啥
			buf++;
			s->GetInt(buf);
		}

		while (buf[0] == ';' || buf[0] == ' ') buf++;
		fontstyle->xcount = 256;
		fontstyle->ycount = 256;
		//颜色及格数
		colors = new SDL_Color[3];
		for (int ii = 0; buf[0] != 0 && ii < 3 && buf[0] == '#'; ii++) {
			buf++;
			int color = s->GetHexInt(buf, 6);
			colors[ii].r = (color >> 16) & 0xff;
			colors[ii].g = (color >> 8) & 0xff;
			colors[ii].b = color & 0xff;
			cellcount = ii + 1;
		}
	}

	if (info->textStr) delete info->textStr;
	info->textStr = new LOString(buf, s->GetEncoder());
	info->textStr->SetEncoder(s->GetEncoder());

	LOtextureBase *base = RenderText(info, fontstyle, info->textStr, colors, cellcount);
	if (cellcount > 1) {
		LOAnimationNS *anim = new LOAnimationNS;
		info->ReplaceAction(anim);
		anim->cellCount = cellcount;
		anim->perTime = new int[anim->cellCount];
		for (int ii = 0; ii < anim->cellCount; ii++)
			*(anim->perTime + ii) = 0;
		anim->loopMode = 3;
		anim->cellCurrent = 0; //总是指向马上要执行的帧
		anim->cellForward = 1; //1或者-1
		anim->isEnble = true;
		anim->control = LOAnimation::CON_REPLACE;
	}

	if (isnew) {
		delete fontstyle;
		delete[] colors;
	}
	return base;
}

//从标记中生成色块
LOtextureBase* LOImageModule::TextureFromColor(LOLayerInfo *info) {
	LOString *s = info->fileName;
	const char *buf = s->SkipSpace(s->c_str());

	int w = s->GetInt(buf);
	while (buf[0] == ',' || buf[0] == ' ') buf++;
	int h = s->GetInt(buf);

	while (buf[0] != '#' && buf[0] != '\0') buf++;
	if (buf[0] != '#') return nullptr;
	buf++;
	int color = s->GetHexInt(buf, 6);
	LOSurface *surface = new LOSurface(w, h, 8, SDL_PIXELFORMAT_INDEX8 );
	SDL_Palette *pale = surface->GetSurface()->format->palette;
	SDL_Color *cc = pale->colors;
	cc->r = (color >> 16) & 0xff;
	cc->g = (color >> 8) & 0xff;
	cc->b = color & 0xff;

	return new LOtextureBase(surface);
}

LOtextureBase* LOImageModule::TextureFromNSbtn(LOLayerInfo*info, LOString *s) {
	LOtextureBase *base = nullptr;
	return base;
}           


LOtextureBase* LOImageModule::TextureFromFile(LOLayerInfo *info) {
	bool ispng, useAlpha;
	LOSurface *tmp = SurfaceFromFile(info->fileName, &ispng);
	if (!tmp) return nullptr;
	//转换透明格式 
	if (info->alphaMode != LOLayerInfo::TRANS_COPY && !tmp->hasAlpha()) {
		if (info->alphaMode == LOLayerInfo::TRANS_ALPHA && !ispng) {
			LOSurface *tmp2 = tmp->ConverNSalpha(info->GetCellCount());
			if (tmp2) {
				delete tmp;
				tmp = tmp2;
			}
			else SimpleError("Conver image alhpa faild: %s", info->fileName->c_str());
		}
		else if (info->alphaMode == LOLayerInfo::TRANS_TOPLEFT) {
			SDL_Color color = tmp->getPositionColor(0, 0);
			tmp->setAlphaColor(color);
		}
		else if (info->alphaMode == LOLayerInfo::TRANS_TOPRIGHT) {
			SDL_Color color = tmp->getPositionColor(tmp->W() - 1, 0);
			tmp->setAlphaColor(color);
		}
		else if (info->alphaMode == LOLayerInfo::TRANS_DIRECT) {
			LOLog_e("LOLayerInfo::TRANS_DIRECT is enmpty code!");
		}
	}

	LOtextureBase *base = new LOtextureBase(tmp);
	LOString s = info->fileName->toLower() + "?" + std::to_string(info->alphaMode) + ";";
	base->SetName(s);
	return base;
}

LOSurface* LOImageModule::SurfaceFromFile(LOString *filename, bool *ispng) {
	BinArray *bin = FunctionInterface::fileModule->ReadFile(filename);
	if (!bin) return nullptr;

	LOSurface *su = new LOSurface(bin->bin, bin->Length());
	delete bin;

	if (su->isNull()) {
		delete su;
		SimpleError("LOImageModule::SurfaceFromFile() unsupported file format!");
		return nullptr;
	}

	if(ispng) *ispng = su->isPng();
	return su;
}


LOtextureBase* LOImageModule::EmptyTexture(LOString *fn) {
	LOtextureBase *tx = new LOtextureBase();
	LOtexture::addTextureBaseToMap(*fn, tx);
	return tx;
}

LOLayer* LOImageModule::GetLayerOrNew(int fullid) {
	int ids[3];
	LOLayer::SysLayerType type;
	GetTypeAndIds( (int*)(&type), ids, fullid);
	LOLayer *lyr = FindLayerInBase(type, ids);
	if (!lyr)lyr = new LOLayer(type, ids);
	return lyr;
}

LOLayerInfo* LOImageModule::GetInfoLayerAvailable(int fullid, const char* cacheN) {
	//首先应该检查是否在队列组中
	uint64_t key = GetIndexKey(fullid, cacheN);
	LOLayerInfo *info = NULL;
	SDL_LockMutex(poolMutex);
	auto iter = queLayerinfoMap.find(key);
	if (iter != queLayerinfoMap.end()) info = &iter->second->info;
	else {
		LOLayer *lyr = FindLayerInBase(fullid);
		if (lyr) {
			LOLayerInfoCacheIndex *minfo = GetCacheIndexFromPool(fullid, cacheN);
			queLayerinfoMap[key] = minfo;
			info = &minfo->info;
			//一些关键信息我们想要知道
			info->CopyConWordFrom(lyr->curInfo, LOLayerInfo::CON_UPPOS | LOLayerInfo::CON_UPPOS2 |
				LOLayerInfo::CON_UPVISIABLE | LOLayerInfo::CON_UPAPLHA, false);
		}
	}
	SDL_UnlockMutex(poolMutex);
	return info;
}


//图层是否有效，依赖图层存在的操作才能决定是否执行，比如msp spbtn等
LOLayerInfo* LOImageModule::GetInfoLayerAvailable(LOLayer::SysLayerType type, int *ids, const char* cacheN) {
	int fullid = GetFullID(type, ids);
	return GetInfoLayerAvailable(fullid, cacheN);
}


LOLayerInfo* LOImageModule::GetInfoUnLayerAvailable(int fullid, const char* cacheN) {
	int stype;
	int ids[] = { 0,0,0 };
	GetTypeAndIds(&stype, ids,fullid);
	return GetInfoUnLayerAvailable((LOLayer::SysLayerType)stype, ids, cacheN);
}

//图层不在队列中，且不在显示中时新建一个
LOLayerInfo* LOImageModule::GetInfoUnLayerAvailable(LOLayer::SysLayerType type, int *ids, const char* cacheN) {
	int fullid = GetFullID(type, ids);
	uint64_t key = GetIndexKey(fullid, cacheN);
	LOLayerInfo *info = NULL;

	SDL_LockMutex(poolMutex);
	auto iter = queLayerinfoMap.find(key);
	if (iter != queLayerinfoMap.end()) info = &iter->second->info;
	SDL_UnlockMutex(poolMutex);
	if (info) return nullptr; //图层已经存在

	LOLayer *lyr = lonsLayers[type]->FindChild(ids);
	if (lyr) return nullptr;

	return GetInfoNew(fullid, cacheN);
}

void LOImageModule::ClearAllLayerInfo() {
	SDL_LockMutex(poolMutex);
	queLayerinfoMap.clear();
	for (int count = 0; count < poolData.size(); count++) {
		LOLayerInfoCacheIndex *minfo = poolData.at(count);
		if (minfo->iswork) {
			NotUseInfo(minfo);
		}
	}
	SDL_UnlockMutex(poolMutex);
}

LOLayerInfo* LOImageModule::LayerInfomation(LOLayer::SysLayerType type, int *ids, const char* cacheN) {
	int fullid = GetFullID(type, ids);
	return LayerInfomation(fullid, cacheN);
}

//获取图层信息，会叠加尚在队列中的数据，需要从自行释放数据
LOLayerInfo* LOImageModule::LayerInfomation(int fullid, const char* cacheN) {
	int index = -1;
	LOLayerInfo *info = new LOLayerInfo;
	uint64_t key = GetIndexKey(fullid, cacheN);

	SDL_LockMutex(poolMutex);
	//首先应该检查是否在队列组中
	LOLayerInfoCacheIndex *minfo = NULL;
	auto iter = queLayerinfoMap.find(key);
	if (iter != queLayerinfoMap.end()) minfo = iter->second;
	SDL_UnlockMutex(poolMutex);

	//队列中是一个新的图层，直接返回信息就可以了
	if (minfo &&  minfo->info.isNewLayer()) {
		info->CopyConWordFrom(&minfo->info, -1, false);
		//info->CopyActionFrom(minfo->info.actions, false);
		return info;
	}

	SDL_LockMutex(layerQueMutex);
	//有图层的复制图层信息
	LOLayer *lyr = FindLayerInBase(fullid);
	if (lyr) {
		info->CopyConWordFrom(lyr->curInfo, -1, false);
		//info->CopyActionFrom(lyr->curInfo->actions, false);
		//队列信息叠加进入
		if (minfo) info->CopyConWordFrom( &minfo->info, minfo->info.GetLayerControl(), false);

	}
	else {
		delete info;
		info = NULL;
	}
	SDL_UnlockMutex(layerQueMutex);
	return info;
}

//获取图层的使用情况，包括已经在队列中的和显示的，print_name null表示搜索全部队列
//搜索区域的起点由ids中的值指定
BinArray* LOImageModule::GetQueLayerUsedState(int sfullID, const char* print_name, int checkIndex) {
	int max = 255;
	if (checkIndex == IDS_LAYER_NORMAL) max = 1024;
	int sids[] = { 0, 0, 0 };
	int stype;
	GetTypeAndIds(&stype, sids, sfullID);

	BinArray *bin = new BinArray(max + 1);
	bin->SetLength(max + 1);
	SDL_LockMutex(poolMutex);
	//搜索队列
	for (int ii = 0; ii < prinNameList.size(); ii++) {
		int hash = prinNameList.at(ii);
		if (print_name) hash = LOString::HashStr(print_name);
		//从队列的起始开始
		uint64_t key = GetIndexKey(0, hash);
		auto iter = queLayerinfoMap.find(key);
		bool loop = true;
		while (iter != queLayerinfoMap.end() && loop) {
			int fullID = iter->first & 0xffffffff;
			//除checkIndex外，其他几个应该相等
			int kids[] = { 0,0,0 };
			int ktype;
			GetTypeAndIds(&ktype, kids, fullID);
			if (ktype == stype) {
				//判断是否需要的
				bool isit = true;
				for (int nn = 0; nn < 3; nn++) {
					if (nn != checkIndex && kids[nn] != sids[nn]) isit = false;
					else if (nn < checkIndex && kids[nn] > sids[nn]) { //超出范围
						loop = false;
						break;
					}
				}
				//写标记
				if (isit) bin->bin[kids[checkIndex]] = 1;

			}
			else if (ktype > stype) {//超出范围
				loop = false;
				break;
			}

			iter++;
		}
		if (print_name) break;
	}
	SDL_UnlockMutex(poolMutex);
	return bin;
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
	return true;
}

//隐藏图层，0表示无需刷新状态，1表示需刷新状态
int LOImageModule::HideLayer(int fullid, const char *printName) {
	int sstate;
	LOLayerInfo *cinfo = LayerInfomation(fullid, printName);
	if (cinfo && cinfo->visiable) {
		LOLayerInfo *info = GetInfoNewAndNoFreeOld(fullid, printName);
		info->SetVisable(0);
		sstate = 1;
	}
	else sstate = 0;

	if (cinfo)delete cinfo;
	return sstate;
}

//显示图层，0表示无需刷新状态，1表示需刷新状态，-1表示图层不存在
int LOImageModule::ShowLayer(int fullid, const char *printName) {
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
			LOAnimationText *text = (LOAnimationText*)lyr->curInfo->GetAnimation(LOAnimation::ANIM_TEXT);
			if (text) {
				startline = text->lineInfo->size() - 1;
				startpos = text->lineInfo->top()->sumx;
				isAdd = true;
			}
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
	return true;
}

//检索按钮
LOLayer* LOImageModule::FindLayerInBtnQuePosition(int x, int y) {
	for (auto iter = btnMap.begin(); iter != btnMap.end(); iter++) {
		if (iter->second->isPositionInsideMe(x, y)) return iter->second;
	}
	return nullptr;
}