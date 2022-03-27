/*
图像队列的处理
*/
#include "../etc/LOEvent1.h"
#include "LOImageModule.h"


uint64_t LOImageModule::GetIndexKey(int fullid, const char* print_name) {
	return GetIndexKey(fullid, LOString::HashStr(print_name));
}

uint64_t LOImageModule::GetIndexKey(int fullid, int hash) {
	uint64_t abc = (uint64_t)hash;
	abc = abc << 32;
	return abc | (uint64_t)fullid;
}

LOLayerInfo *LOImageModule::GetInfoNewAndFreeOld(int fullid, const char* print_name) {
	LOLayerInfo *info = GetInfoNewAndNoFreeOld(fullid, print_name);
	FreeLayerInfoData(info);
	info->Reset();
	info->fullid = fullid;
	return info;
}

LOLayerInfo *LOImageModule::GetInfoNewAndNoFreeOld(int fullid, const char* print_name) {
	LOLayerInfoCacheIndex *minfo;
	uint64_t key = GetIndexKey(fullid, print_name);
	SDL_LockMutex(poolMutex);
	auto iter = queLayerinfoMap.find(key);
	if (iter == queLayerinfoMap.end()) {
		minfo = GetCacheIndexFromPool(fullid, print_name);
		queLayerinfoMap[key] = minfo;
	}
	else minfo = iter->second;
	SDL_UnlockMutex(poolMutex);
	return &minfo->info;
}

std::vector<LOLayerInfo*> LOImageModule::BatchInfoNew(const char* print_name, std::vector<int> *list, bool isfree) {
	std::vector<LOLayerInfo*> dest;

	SDL_LockMutex(poolMutex);
	for (auto iter = list->begin(); iter != list->end(); iter++) {
		uint64_t key = GetIndexKey(*iter, print_name);
		auto cache = queLayerinfoMap.find(key);
		if (cache != queLayerinfoMap.end()) {
			LOLayerInfo *info = &cache->second->info;
			if (isfree) {
				FreeLayerInfoData(info);
				info->Reset();
				info->fullid = *iter;
			}
			dest.push_back(info);
		}
		else {
			LOLayerInfoCacheIndex *minfo = GetCacheIndexFromPool((*iter), print_name);
			queLayerinfoMap[key] = minfo;
			dest.push_back(&minfo->info);
		}
	}
	SDL_UnlockMutex(poolMutex);
	return dest;
}

LOLayerInfo *LOImageModule::GetInfoNew(int fullid, const char* print_name) {
	uint64_t key = GetIndexKey(fullid, print_name);
	SDL_LockMutex(poolMutex);
	LOLayerInfoCacheIndex *minfo = GetCacheIndexFromPool(fullid, print_name);
	queLayerinfoMap[key] = minfo;
	SDL_UnlockMutex(poolMutex);
	return &minfo->info;
}

void LOImageModule::NotUseInfo(LOLayerInfoCacheIndex *minfo) {
	LOLayerInfo *info = &minfo->info;
	info->Reset();
	info->ResetOther();
	minfo->iswork = false;
}

//LOLayerInfoCacheIndex * LOImageModule::GetCacheIndexFromName(int fullid, const char *print_name, int *mapindex) {
//	*mapindex = GetInfoCacheFromName(print_name);
//	auto map = mapList.at(*mapindex);
//	auto iter = map->find(fullid);
//	if (iter != map->end()) return iter->second;
//	else return NULL;
//}

std::map<uint64_t, LOLayerInfoCacheIndex*>::iterator LOImageModule::queLayerinfoMapStart(const char *print_name) {
	uint64_t key = GetIndexKey(0, print_name);
	//有序map，这是一个非常有用的特性
	auto iter = queLayerinfoMap.find(key);
	if (iter == queLayerinfoMap.end()) {
		queLayerinfoMap[key] = nullptr;
		iter = queLayerinfoMap.find(key);
		prinNameList.push_back(LOString::HashStr(print_name));
	}
	return iter;
}

//根据提供的控制类型提取出列表
std::vector<LOLayerInfoCacheIndex*> LOImageModule::FilterCacheQue(const char *print_name, int layertype, int conid, bool isremove) {
	std::vector<LOLayerInfoCacheIndex*> list;
	auto iter = queLayerinfoMapStart(print_name);

	iter++;
	while (iter != queLayerinfoMap.end()) {
		//nullptr表示已经到达下一个符号点
		if (!iter->second || strcmp(print_name, iter->second->name) != 0) break;
		LOLayerInfo *info = &iter->second->info;
		if (layertype >= 0 && GetIDs(info->fullid, IDS_LAYER_TYPE) != layertype) { //filter type
			iter++;
			continue;
		}
		if (info->GetLayerControl() & conid) list.push_back(iter->second);
		if (isremove) iter = queLayerinfoMap.erase(iter);
		else iter++;
	}
	return list;
}

bool LOImageModule::HasPrintQue(const char *print_name) {
	auto iter = queLayerinfoMapStart(print_name);
	iter++;
	if (iter == queLayerinfoMap.end() || !iter->second) return false;
	return (strcmp(iter->second->name, print_name) == 0);
}

void LOImageModule::ClearCacheMap(std::vector<LOLayerInfoCacheIndex*> *list) {
	SDL_LockMutex(poolMutex);
	for (int ii = 0; ii < list->size(); ii++) {
		list->at(ii)->Reset();
	}
	SDL_UnlockMutex(poolMutex);
	list->clear();
}

//int LOImageModule::GetInfoCacheFromName(const char *buf) {
//	for (int ii = 0; ii < nameList.size(); ii++) {
//		if (strcmp(buf, nameList[ii].c_str()) == 0) return ii;
//	}
//	nameList.push_back(std::string(buf));
//	mapList.push(new std::unordered_map<int, LOLayerInfoCacheIndex*>);
//	return mapList.size() - 1;
//}

//由外部锁定线程锁
LOLayerInfoCacheIndex *LOImageModule::GetCacheIndexFromPool(int fullid, const char* cacheN) {
	LOLayerInfoCacheIndex *minfo = NULL;
	//使用环形队列
	int count = 0;
	for (count = 0; count < poolData.size(); count++) {
		minfo = poolData.at(poolCurrent);
		poolCurrent = (poolCurrent + 1) % poolData.size();
		if (!minfo->iswork) break;
	}
	//检查是否需要增加 信息池
	if (poolData.size() - count < 10) {
		poolCurrent = poolData.size();
		for (int ii = 0; ii < 50; ii++) poolData.push(new LOLayerInfoCacheIndex);
		minfo = poolData.at(poolCurrent);
		SDL_Log("poolData's size be to %d\n", poolData.size());
	}
	if (!minfo || minfo->iswork) {
		SDL_LogError(0, "ONScripterImage::GetCacheIndexFromPool() faild!");
		return NULL;
	}
	minfo->iswork = true;
	minfo->name = cacheN;
	minfo->info.fullid = fullid;
	return minfo;
}


//优先考虑父对象，再考虑子对象的原则排列信息组
std::vector<LOLayerInfoCacheIndex*> LOImageModule::SortCacheList(std::vector<LOLayerInfoCacheIndex*> *list) {
	std::vector<LOLayerInfoCacheIndex*> dest;
	for (int ii = 0; ii < 3; ii++) { //1-根对象， 2-子对象， 3-孙子对象
		auto iter = list->begin();
		while (iter != list->end()) {
			auto ptr = (LOLayerInfoCacheIndex*)(*iter);
			if (ii < 2) { //0-根对象， 1-子对象需要过滤
				if (GetIDs(ptr->info.fullid, ii) == 255) {
					dest.push_back(*iter);
					iter = list->erase(iter);
				}
				else iter++;
			}
			else {  //剩下的孙子对象直接加入
				dest.push_back(*iter);
				iter++;
			}
		}
	}
	list->clear();
	return dest;
}


//在正式处理事件前必须处理帧刷新时遗留的事件
void LOImageModule::DoPreEvent() {
	for (int ii = 0; ii < preEventList.size(); ii += 2) {
		int ev = (int)preEventList[ii];
		LOEventHook *e = preEventList[ii + 1];

		if (ev == PRE_EVENT_PREPRINTOK) {
			e->FinishMe();
		}
		else if (ev == PRE_EVENT_EFFECTCONTIUE) {

		}
	}
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
	if (!HasPrintQue(print_name)) return READER_WAIT_NONE;

	//print是一个竞争过程，只有执行完成一个才能下一个
	SDL_LockMutex(doQueMutex);
	//非print1则要求抓取当前显示的图像，下一帧在继续执行
	//这里有一个坑，抓取的图形在进行 if(ef)后才会进入 queLayerMap中，所以要在这步以后才FilterCacheQue
	if (ef) {
		auto *e = LOEventHook::CreatePrintPreHook(ef, print_name);
		//提交到等待位置
		printPreHook.store((intptr_t)e);
		e->waitEvent(1, -1);
		e->InvalidMe();
		//遇到程序退出
		if (moduleState >= MODULE_STATE_EXIT) return 0;
	}

	std::vector<LOLayerInfoCacheIndex*> unorderlist = FilterCacheQue(print_name, -1, -1, true);

	//we will add layer or delete layer and btn ,so we lock it,main thread will not render.
	SDL_LockMutex(layerQueMutex);
	SDL_LockMutex(btnQueMutex);

	LOLayer *layer, *temp;
	std::vector<LOLayerInfoCacheIndex*> list = SortCacheList(&unorderlist);
	for (int ii = 0; ii < list.size(); ii++) {
		LOLayerInfoCacheIndex *minfo = (LOLayerInfoCacheIndex*)list.at(ii);
		if (!minfo->iswork || minfo->info.GetLayerControl() == LOLayerInfo::CON_NONE) continue;
		//LONS::printError("id[0]:%d,%d,%x",idd[0],idd[1],info->fullid);
		//图层被删除，或重新载入，应删除上一个按钮
		//优先处理删除事件
		if (minfo->info.GetLayerControl() & LOLayerInfo::CON_DELLAYER) {
			LOLayer::SysLayerType ltype;
			int ids[3];
			GetTypeAndIds( (int*)(&ltype), ids, minfo->info.fullid);
			LOLayer *layer = FindLayerInBase(ltype, ids);
			if (layer) {
				layer->Root->RemodeChild(layer->id[0]);
				delete layer;
			}
			//删除按钮定义
			RemoveBtn(minfo->info.fullid);
		}
		else {
			//if (GetIDs(minfo->info.fullid, IDS_LAYER_TYPE) == LOLayer::LAYER_NSSYS &&
			//	GetIDs(minfo->info.fullid, IDS_LAYER_NORMAL) == LOLayer::IDEX_NSSYS_EFFECT) {
			//	SDL_Surface *su = minfo->info.texture->getSurface();
			//	su = nullptr;
			//}
			layer = GetLayerOrNew(minfo->info.fullid);
			if (!layer->parent) { //插入图层
				layer->Root = GetRootLayer(minfo->info.fullid);
				layer->SetFullID(minfo->info.fullid);
				layer->Root->InserChild(layer);
			}
			if (minfo->info.btnStr) exbtn_count++;     //exbtn count, count > 0 exbtn_d can use
			layer->UseControl(&minfo->info, btnMap);
		}
	}
	ClearCacheMap(&list);
	//等待print完成才继续
	LOEventHook *ep = NULL;
	if (iswait) {
		ep = LOEventHook::CreatePrintPreHook(ef, print_name);
		//提交到等待位置
		printHook.store((intptr_t)ep);
	}
	SDL_UnlockMutex(layerQueMutex);
	SDL_UnlockMutex(btnQueMutex);
	if (ep) {
		ep->waitEvent(1, -1);
		ep->InvalidMe();
		//if (moduleState >= MODULE_STATE_EXIT) return 0;
	}
	SDL_UnlockMutex(doQueMutex);
	return 0;
}


void LOImageModule::ExportQuequ2(std::unordered_map<int, LOLayerInfoCacheIndex*> *map) {
	int ii = sizeof(FunctionInterface);
}



//捕获SDL事件，将对指定的事件进行处理
void LOImageModule::CaptureEvents(SDL_Event *event) {
	/*
	LOEvent1 *catMsg = NULL;
	LOEventParamBtnRef *param;
	LOLayer *layer;
	bool clickf = false;

	switch (event->type)
	{
	case SDL_MOUSEMOTION:
		//更新鼠标位置
		if (!TranzMousePos(event->motion.x, event->motion.y)) break;
		//鼠标在窗口内移动事件，将在这里检查按钮
		//只选择纯正的按钮事件
		catMsg = G_GetEvent(LOEvent1::EVENT_CATCH_BTN, LOEvent1::EVENT_CATCH_BTN);

		if (catMsg) catMsg->closeEdit(); //这里只是确认有按钮事件，按钮不在这里编辑
		if (catMsg && SDL_TryLockMutex(btnQueMutex) == 0) {
			LOLayer *layer = FindLayerInBtnQuePosition(mouseXY[0], mouseXY[1]);
			//当前激活的按钮不是同一个，或者没有激活按钮，重置上一次激活的按钮状态
			if (lastActiveLayer) {
				lastActiveLayer->HideBtnMe();
				if (lastActiveLayer->isExbtn()) {
					//上一个按钮是复合按钮，并且本次按钮不是复合按钮，重置区域外符合按钮的状态
					if (!layer || !layer->isExbtn())exbtn_d_hasrun = false;
					//当前的复合按钮不同于上一个的复合按钮，重置上一个复合按钮的状态
					if (lastActiveLayer != layer) 
						lastActiveLayer->exbtnHasRun = false;
				}
				lastActiveLayer = NULL;
			}
			if (layer) {
				layer->ShowBtnMe();         //按钮自身的动作
				if (layer->isExbtn() && !layer->exbtnHasRun) { //当前按钮是复合按钮，那么区域外复合按钮的状态就被激活了
					RunExbtnStr(layer->curInfo->btnStr);
					exbtn_d_hasrun = false;
				}
				layer->exbtnHasRun = true;
				lastActiveLayer = layer;
			}

			//没有在按钮上，或者当前的按钮不是复合按钮
			if ((!layer || !layer->isExbtn()) && !exbtn_d_hasrun && exbtn_count > 0) {
				exbtn_d_hasrun = true;
				RunExbtnStr(&exbtn_dStr);
			}

			SDL_UnlockMutex(btnQueMutex);
		}

		break;

		//鼠标点击完成
	case SDL_MOUSEBUTTONUP:
		//更新鼠标位置
		if (!TranzMousePos(event->button.x, event->button.y))break;

		//鼠标左点击
		if (event->button.button == SDL_BUTTON_LEFT) {
			//响应的是文字显示事件，则完成整个显示，同时忽略点击的其他作用
			//catMsg = G_GetSelfEventIsParam(LOEvent1::EVENT_TEXT_ACTION, FunctionInterface::LAYER_TEXT_WORKING);
			catMsg = G_GetEvent(LOEvent1::EVENT_TEXT_ACTION);
			if (catMsg) {
				catMsg->closeEdit();
				CutDialogueAction();
				clickf = true;
			}

			//响应的是print事件，print的等待事件后续都转移到EVENT_IMGMODULE_AFTER槽中了
			catMsg = G_GetEvent(LOEvent1::EVENT_IMGMODULE_AFTER, LOEvent1::EVENT_WAIT_PRINT);
			if (catMsg) {
				param = (LOEventParamBtnRef*)catMsg->param;
				ContinueEffect((LOEffect *)param->ptr1, 1000.0 * 1000.0);
				catMsg->FinishMe();
				clickf = true;
			}

			if (clickf) break;

			//左键事件
			catMsg = G_GetEvent(LOEvent1::EVENT_CATCH_BTN);
			if (catMsg) {
				switch (catMsg->eventID){
				case SCRIPTER_EVENT_DALAY:
					catMsg->InvalidMe();  //延迟事件直接失效，脚本不再阻塞
					break;
				case SCRIPTER_EVENT_LEFTCLICK: case SCRIPTER_EVENT_CLICK:
					catMsg->InvalidMe(); //无需获取具体按钮的，比如click lrclick直接无效事件
					break;
				case LOEvent1::EVENT_CATCH_BTN:
					layer = FindLayerInBtnQuePosition(mouseXY[0], mouseXY[1]);
					if (layer) catMsg->value = layer->curInfo->btnValue;
					else catMsg->value = 0; //没有点中任何按钮
					LeaveCatchBtn(catMsg);
					break;
				default:
					LOLog_e("unkown left click event %d!\n", catMsg->eventID);
				}
			}
		}
		else { //鼠标右点击
			LOEvent1 *catMsg = G_GetEvent(LOEvent1::EVENT_CATCH_BTN);
			if (catMsg) {
				switch (catMsg->eventID)
				{
				case LOEvent1::EVENT_CATCH_BTN:
					catMsg->value = -1;
					LeaveCatchBtn(catMsg);
					break;
				case SCRIPTER_EVENT_LEFTCLICK:
					catMsg->closeEdit();
					break;  //右键不响应左键信息
				default:
					catMsg->InvalidMe();
					break;
				}
			}
		}
		break;
	case SDL_MOUSEBUTTONDOWN:
		break;
	default:
		break;
	}
	*/
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
		LOLayerInfo *info = GetInfoNewAndFreeOld(fullid, "_lons");
		info->SetLayerDelete();
	}
}


//立即完成对话文字
void LOImageModule::CutDialogueAction() {
	int ids[] = { LOLayer::IDEX_DIALOG_TEXT,255,255 };
	LOLayer *layer = FindLayerInBase(LOLayer::LAYER_DIALOG, ids);
	if (layer) {
		LOAnimation *aib = layer->GetAnimation(LOAnimation::ANIM_TEXT);
		if (aib) {
			LOAnimationText *ai = (LOAnimationText*)(aib);
			layer->DoTextAnima(layer->curInfo, ai, ai->lastTime + 0xfffff);
		}
	}
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
			if (lyr)  lyr->curInfo->visiable = 0;
		}
		else if (cc == 'P') {   //显示
			buf++;
			int ids[] = { s->GetInt(buf) ,255,255 };
			LOLayer *lyr = FindLayerInBase(LOLayer::LAYER_SPRINT, ids);
			if (lyr) lyr->curInfo->visiable = 1; //只是显示
			if (buf[0] == ',') {  //显示指定cell
				buf++;
				int cell = s->GetInt(buf);
				if (lyr) lyr->ShowNSanima(cell);
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
}

