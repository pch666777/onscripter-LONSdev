/*
图像队列的处理
*/
#include "../etc/LOEvent1.h"
#include "LOImageModule.h"

//在正式处理事件前必须处理帧刷新时遗留的事件
void LOImageModule::DoPreEvent(double postime) {
	for (int ii = 0; ii < preEventList.size(); ii++){
		LOShareEventHook e = preEventList[ii];

		if (e->catchFlag == PRE_EVENT_PREPRINTOK) {  //print准备完成
			LOEffect *ef = (LOEffect*)e->paramList[0]->GetPtr();
			const char *printName = e->paramList[1]->GetChars(nullptr);
			PrepareEffect(ef, printName);
			e->FinishMe();
		}
		else if (e->catchFlag == PRE_EVENT_EFFECTCONTIUE) { //继续运行
			LOEffect *ef = (LOEffect*)e->paramList[0]->GetPtr();
			const char *printName = e->paramList[1]->GetChars(nullptr);
			if (ContinueEffect(ef, printName , postime)) e->FinishMe();
			else e->closeEdit();
			//printf("main thread finish:%d\n", SDL_GetTicks());
		}
	}

	preEventList.clear();
}

void LOImageModule::DoDelayEvent(double postime) {
	/*
	//延迟事件队列只应该用于 updisplay() 里需要等待帧完成后刷新后通知的情况
	//虽然增加了事件执行的复杂度，但是我认为这是必要的，统一处理事件更容易确定程序中用了哪些通讯事件，以及外部处理中断事件
	LOEventSlot *slot = GetEventSlot(LOEvent1::EVENT_IMGMODULE_AFTER);
	
	//通过函数调用，避免暴露 LOEventSlot 的内部成员
	int ecount = slot->ForeachCall(&EventDo, postime);
	//运行时的同时事件不可能超过20个，超过该检查问题了
	if (ecount > 20) SimpleError("LonsEvent is too much! check event logic please!");
	*/
}


//print的过程无法支持异步，这会导致非常复杂的问题，特别是需要存档的话
int LOImageModule::ExportQuequ(const char *print_name, LOEffect *ef, bool iswait) {
	//考虑到需要存档
	iswait = true;

	//检查是不是有需要刷新的
	auto *map = GetPrintNameMap(print_name)->map;
	if (map->size() == 0) return 0;

	//Uint64 t1 = SDL_GetPerformanceCounter();
	//Uint64 perHtickTime = SDL_GetPerformanceFrequency() / 1000;

	//print是一个竞争过程，只有执行完成一个才能下一个
	SDL_LockMutex(doQueMutex);
	//非print1则要求抓取当前显示的图像，下一帧在继续执行
	if (ef) {
		LOEventHook::CreatePrintPreHook(printPreHook.get(), ef, print_name);
		printPreHook->ResetMe();
		//提交到等待位置
		printPreHook->waitEvent(1, -1);
		//遇到程序退出
		if (moduleState >= MODULE_STATE_EXIT) return 0;
	}
	//LOLog_i("uppos0:%f", ((double)(SDL_GetPerformanceCounter() - t1)) / perHtickTime);
	//we will add layer or delete layer and btn ,so we lock it,main thread will not render.
	SDL_LockMutex(layerQueMutex);
	//layerTestMute.lock();

	//LOLog_i("uppos1:%f", ((double)(SDL_GetPerformanceCounter() - t1)) / perHtickTime);

	//历遍图层，注意需要先处理父对象
	for (int level = 1; level <= 3; level++) {
		for (auto iter = map->begin(); iter != map->end();) {
			LOLayerData *data = iter->second;
			//检查是不是现在要处理的
			bool isnow = false;
			if (level < 3 && GetIDs(data->fullid, level) >= G_maxLayerCount[level]) isnow = true;
			else if (level >= 3) isnow = true;
			else isnow = false;

			////////
			if (isnow) {
				//指向下一个
				iter = map->erase(iter);

				LOLayer *lyr = LOLayer::FindViewLayer(data->fullid, data->isDelete() || data->isNewFile());

				if (data->isDelete()) {
					if (lyr) delete lyr;
				}
				else if (data->isNewFile()) {
					//新建图层前直接删除原有的图层，这样更干净
					if (lyr) delete lyr;
					lyr = new LOLayer(*data, true);
				}
				else {
					//只更新信息的图层
					if (lyr) {
						if (data->isUpData()) lyr->upData(data);
						if (data->isUpDataEx()) lyr->upDataEx(data);
					}
				}
			}
			else iter++;

		}
	}

	//LOLog_i("uppos2:%f", ((double)(SDL_GetPerformanceCounter() - t1)) / perHtickTime);
	//======图层已经更新完成=======

	//等待print完成才继续
	LOEventHook *ep = NULL;
	if (iswait) {
		ep = LOEventHook::CreatePrintPreHook(printHook.get(), ef, print_name);
		//提交到等待位置
		ep->ResetMe();
		//printf("script thread:%d\n", SDL_GetTicks());
	}
	SDL_UnlockMutex(layerQueMutex);
	//layerTestMute.unlock();

	if (ep) {
		//LOLog_i("uppos2-1:%f", ((double)(SDL_GetPerformanceCounter() - t1)) / perHtickTime);
		//printf("script thread wait befor:%d\n", SDL_GetTicks());
		ep->waitEvent(1, -1);
		//printf("script thread wait finish:%d\n", SDL_GetTicks());
		//LOLog_i("uppos2-2:%f", ((double)(SDL_GetPerformanceCounter() - t1)) / perHtickTime);
		//if (moduleState >= MODULE_STATE_EXIT) return 0;
	}
	SDL_UnlockMutex(doQueMutex);

	//LOLog_i("uppos3:%f", ((double)(SDL_GetPerformanceCounter() - t1)) / perHtickTime);

	return 0;
}


int LOImageModule::SendEventToLayer(LOEventHook *e) {
	//发往图层，看图层是否响应
	int ret = LOLayer::SENDRET_NONE;
	for (int ii = 0; ii < LOLayer::LAYER_BASE_COUNT && ret == LOLayer::SENDRET_NONE; ii++) {
		ret = G_baseLayer[ii].checkEvent(e, &waitEventQue);
	}
	return ret;
}



//捕获SDL事件，封装成指定的事件
void LOImageModule::CaptureEvents(SDL_Event *event) {
	LOShareEventHook ev(new LOEventHook());

	//包装事件
	switch (event->type){
	case SDL_MOUSEMOTION:
		//更新鼠标位置
		if (!TranzMousePos(event->motion.x, event->motion.y)) break;
		ev->catchFlag = LOLayerData::FLAGS_MOUSEMOVE;
		ev->paramList.push_back(new LOVariant(mouseXY[0]));
		ev->paramList.push_back(new LOVariant(mouseXY[1]));
		SendEventToLayer(ev.get());
		break;
	case SDL_MOUSEBUTTONUP:
		//鼠标进行了点击
		if (!TranzMousePos(event->button.x, event->button.y))break;
		//这里需要将左、右键事件先发一次hook队列，因为有些hook需要优先处理，比如print wait等点击可跳过的事件
		if (event->button.button == SDL_BUTTON_LEFT) ev->catchFlag = LOEventHook::ANSWER_LEFTCLICK;
		else if (event->button.button == SDL_BUTTON_RIGHT) ev->catchFlag = LOEventHook::ANSWER_RIGHTCLICK;
		if (ev->catchFlag) {
			ev->paramList.push_back(new LOVariant(mouseXY[0]));
			ev->paramList.push_back(new LOVariant(mouseXY[1]));
			//队列没有响应按键事件，那么就发送到图层响应
			//在多线程脚本中可能同时出现 btnwait 和 delay这种需要同时响应按键的窘境，这里btnwait的优先级都被滞后了
			if (SendEventToHooks(ev.get()) == LOEventHook::RUNFUNC_CONTINUE) {
				if (event->button.button == SDL_BUTTON_LEFT) ev->catchFlag = LOLayerData::FLAGS_LEFTCLICK;
				else if (event->button.button == SDL_BUTTON_RIGHT) ev->catchFlag = LOLayerData::FLAGS_RIGHTCLICK;
				SendEventToLayer(ev.get());
			}
		}
		break;
	}
}


void LOImageModule::HandlingEvents() {
	for (int level = LOEventQue::LEVEL_HIGH; level >= LOEventQue::LEVEL_NORMAL; level--) {
		int index = 0;
		while (true) {
			LOShareEventHook ev = waitEventQue.GetEventHook(index, level, false);
			if (!ev) break;
			//将事件传递给钩子列表处理
			SendEventToHooks(ev.get());
		}
	}

	waitEventQue.clear();
}

int LOImageModule::SendEventToHooks(LOEventHook *e) {
	int state = LOEventHook::RUNFUNC_CONTINUE;
	for (int level = LOEventQue::LEVEL_HIGH; level >= LOEventQue::LEVEL_NORMAL; level--) {
		int index = 0;
		while (state == LOEventHook::RUNFUNC_CONTINUE) {
			LOShareEventHook hook = G_hookQue.GetEventHook(index, level, true);
			if (!hook) break;
			if (hook->catchFlag & e->catchFlag) state = RunFuncBase(hook.get(), e);

			if (state != LOEventHook::RUNFUNC_FINISH) hook->closeEdit();
		}
	}
	return state;
}


//转换鼠标位置，超出视口位置等于事件没有发生
bool LOImageModule::TranzMousePos(int xx, int yy) {
	xx -= G_viewRect.x;
	yy -= G_viewRect.y;
	if (xx < 0 || yy < 0) return false;
	if (xx > G_viewRect.w || yy > G_viewRect.h) return false;
	mouseXY[0] = 1.0 / G_gameScaleX * xx;
	mouseXY[1] = 1.0 / G_gameScaleY * yy;
	//LOLog_i("%d,%d,%d,%d\n", mouseXY[0], mouseXY[1],xx,yy);
	return true;
}

//立刻按钮捕获模式
//void LOImageModule::LeaveCatchBtn(LOEvent1 *msg) {
//	lastActiveLayer = nullptr;
//	//赋值交给脚本线程执行，避免出现线程竞争
//	msg->FinishMe();
//}


void LOImageModule::ClearDialogText(char flag) {
	int ids[] = { LOLayer::IDEX_DIALOG_TEXT,255,255 };
	int fullid = GetFullID(LOLayer::LAYER_DIALOG, ids);
	if (flag == '\\') {
		dialogText.clear();
		//LOLayerInfo *info = GetInfoNewAndFreeOld(fullid, "_lons");
		//info->SetLayerDelete();
	}
}


//立即完成对话文字
void LOImageModule::CutDialogueAction() {
	int ids[] = { LOLayer::IDEX_DIALOG_TEXT,255,255 };
	LOLayer *layer = FindLayerInBase(LOLayer::LAYER_DIALOG, ids);
	if (layer) {
		//LOAnimation *aib = layer->GetAnimation(LOAnimation::ANIM_TEXT);
		//if (aib) {
		//	LOAnimationText *ai = (LOAnimationText*)(aib);
		//	layer->DoTextAnima(layer->curInfo, ai, ai->lastTime + 0xfffff);
		//}
	}
}


int LOImageModule::RunFunc(LOEventHook *hook, LOEventHook *e) {
	if (hook->param2 == LOEventHook::FUN_SPSTR) return RunFuncSpstr(hook, e);
	else if (hook->param2 == LOEventHook::FUN_INVILIDE) {
		hook->InvalidMe();
		return LOEventHook::RUNFUNC_FINISH;
	}

	return LOEventHook::RUNFUNC_CONTINUE;
}

int LOImageModule::RunFuncBtnFinish(LOEventHook *hook, LOEventHook *e) {
	return LOEventHook::RUNFUNC_CONTINUE;
}


/*
LOSurface* LOImageModule::ScreenShot(SDL_Rect *srcRect, SDL_Rect *dstRect) {
	LOEvent1 *e = new LOEvent1(LOEvent1::EVENT_PREPARE_EFFECT, PARAM_SCREENSHORT);
	auto *param = new LOEventParamBtnRef;
	param->ptr1 = (intptr_t)srcRect;
	param->ptr2 = (intptr_t)dstRect;
	e->SetValuePtr(param);

	G_SendEvent(e);
	e->waitEvent(1, -1);

	LOSurface *su = (LOSurface*)param->ptr2;
	e->InvalidMe();
	return su;
}

void LOImageModule::ScreenShotCountinue(LOEvent1 *e) {
	if (!e->value) return;
	LOEventParamBtnRef *param = (LOEventParamBtnRef*)e->value;
	SDL_Rect *rect = (SDL_Rect*)param->ptr1;
	SDL_Rect *dst = (SDL_Rect*)param->ptr2;
	if (!rect || rect->w == 0 || rect->h == 0) return;
	//核算截图的位置
	int xx, yy, ww, hh;
	xx = G_gameScaleX * rect->x; yy = G_gameScaleY * rect->y;
	ww = G_gameScaleX * rect->w; hh = G_gameScaleY * rect->h;
	if (xx < 0) xx = 0;
	if (yy < 0) yy = 0;
	if (ww > G_viewRect.w || ww < 0) ww = G_viewRect.w;
	if (hh > G_viewRect.h || hh < 0) hh = G_viewRect.h;
	SDL_Rect src;
	src.x = xx; src.y = yy;
	src.w = ww; src.h = hh;

	//把画面帧缩放转移到新的buf中
	SDL_Texture *tex = CreateTexture(render, G_Texture_format, SDL_TEXTUREACCESS_TARGET, dst->w, dst->h);
	SDL_SetRenderTarget(render, tex);
	SDL_RenderClear(render);
	SDL_RenderCopy(render, effectTex, &src, dst);

	//转写到surface中，注意如果是一个比较大的图像尺寸，会比较耗时
	LOSurface *su = new LOSurface(dst->w, dst->h, 32, G_Texture_format);
	SDL_Surface *su_t = su->GetSurface();
	SDL_LockSurface(su_t);
	int cks = SDL_RenderReadPixels(render, dst, G_Texture_format, su_t->pixels, su_t->pitch);
	SDL_UnlockSurface(su_t);
	//SDL_SaveBMP(su_t, "gggg.bmp");

	//还原渲染器状态
	SDL_SetRenderTarget(render, NULL);
	DestroyTexture(tex);

	param->ptr2 = (intptr_t)su;  //替换掉dst的指针
	e->FinishMe();
}
*/



int LOImageModule::RunFuncSpstr(LOEventHook *hook, LOEventHook *e) {
	LOString btnstr = e->paramList[1]->GetLOString();
	RunExbtnStr(&btnstr);
	//这个钩子长期有效
	hook->closeEdit();
	return LOEventHook::RUNFUNC_FINISH;
}

void LOImageModule::RunExbtnStr(LOString *s) {

	const char *obuf, *buf;
	obuf = buf = s->c_str();
	int maxlen = s->length();
	
	//注意显示类的操作都需要等到下一帧才刷新
	while (buf - obuf < maxlen) {
		int cc = toupper(buf[0]);
		if (cc == 'C') {  //隐藏
			buf++;
			int ids[] = {s->GetInt(buf) ,255,255 };
			LOLayer *lyr = FindLayerInBase(LOLayer::LAYER_SPRINT, ids);
			if (lyr)  lyr->curInfo->SetVisable(0);
		}
		else if (cc == 'P') {   //显示
			buf++;
			int ids[] = { s->GetInt(buf) ,255,255 };
			LOLayer *lyr = FindLayerInBase(LOLayer::LAYER_SPRINT, ids);
			if (lyr) lyr->curInfo->SetVisable(1); //只是显示
			if (buf[0] == ',') {  //显示指定cell
				buf++;
				int cell = s->GetInt(buf);
				if (lyr) lyr->setActiveCell(cell);
					//lyr->ShowNSanima(cell);
			}
		}
		else if (cc == 'S') {  //播放音乐
			buf++;
			int channel = s->GetInt(buf);
			if (buf[0] == ',') buf++;
			if (buf[0] == '(') buf++;
			int len = 0;
			while (buf[len] != 0 && buf[len] != ')') len++;
			LOString fn(buf, len);
			audioModule->SePlay(channel, fn, 1);
			buf += len;
			buf++;
		}
		else if (cc == 'M') {
			buf++;
			int ids[] = { s->GetInt(buf) ,255,255 };
			if (buf[0] == ',') buf++;
			int xx = s->GetInt(buf);
			if (buf[0] == ',') buf++;
			int yy = s->GetInt(buf);

			LOLayer *lyr = FindLayerInBase(LOLayer::LAYER_SPRINT, ids);
			if (lyr) {
				lyr->curInfo->offsetX = xx;
				lyr->curInfo->offsetY = yy;
			}
		}
		else {
			LOLog_e("RunExbtnStr() unknow key word:%x!", buf[0]);
			return;
		}
	}
}


void LOImageModule::PrintError(LOString *err) {
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "error", err->c_str(), window);
}


void LOImageModule::ResetMe() {
	/*
	ResetConfig();
	int ids[] = { 0,-1,-1 };
	for (int ii = 0; ii < LOLayer::LAYER_BASE_COUNT; ii++) {
		delete lonsLayers[ii];
		ids[0] = ii;
		lonsLayers[ii] = new LOLayer(LOLayer::LAYER_CC_USE, ids);
	}
	//清理按钮
	btnMap.clear();
	//blocksEvent.InvalidAll();
	ClearAllLayerInfo();
	moduleState = MODULE_STATE_NOUSE;
	//等待脚本模块重置
	while (scriptModule->moduleState != MODULE_STATE_NOUSE) SDL_Delay(1);
	moduleState = MODULE_STATE_RUNNING;
	*/
}

