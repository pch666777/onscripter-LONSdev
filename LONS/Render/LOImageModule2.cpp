/*
图像队列的处理
*/
#include "../etc/LOEvent1.h"
#include "LOImageModule.h"


//全局的添加处理事件，注意这个函数不是线程安全的，只应该从渲染线程调用
void AddPreEvent(LOShareEventHook &e) {
	if (FunctionInterface::imgeModule) {
		LOImageModule *img = (LOImageModule*)FunctionInterface::imgeModule;
		img->preEventList.push_back(e);
	}
}


//在正式处理事件前必须处理帧刷新时遗留的事件
void LOImageModule::DoPreEvent(double postime) {
	for (int ii = 0; ii < preEventList.size(); ii++){
		LOShareEventHook e = preEventList[ii];

		if (e->param2 == LOEventHook::FUN_PRE_EFFECT) {  //print准备完成
			LOEffect *ef = (LOEffect*)e->GetParam(0)->GetPtr();
			const char *printName = e->GetParam(1)->GetChars(nullptr);
			PrepareEffect(ef, printName);
			e->FinishMe();
		}
		else if (e->param2 == LOEventHook::FUN_CONTINUE_EFF) { //继续运行
			//printf("%d\n", e->catchFlag);
			LOEffect *ef = (LOEffect*)e->GetParam(0)->GetPtr();
			const char *printName = e->GetParam(1)->GetChars(nullptr);
			if (ContinueEffect(ef, printName , postime)) e->FinishMe();
			else e->closeEdit();
			//printf("state is:%d\n", e->GetState());
		}
		else if (e->param2 == LOEventHook::LOEventHook::FUN_SCREENSHOT) {
			ScreenShotCountinue(e.get());
			e->FinishMe();
		}
		else if (e->param2 == LOEventHook::FUN_TEXT_ACTION) {
			//文字处理完成
			e->FinishMe();
		}
	}

	preEventList.clear();
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
		printPreHook->InvalidMe();
		//遇到程序退出
		if (isModuleExit()) return 0;
	}
	//LOLog_i("uppos0:%f", ((double)(SDL_GetPerformanceCounter() - t1)) / perHtickTime);
	//we will add layer or delete layer and btn ,so we lock it,main thread will not render.
	SDL_LockMutex(layerQueMutex);
	//layerTestMute.lock();

	//LOLog_i("uppos1:%f", ((double)(SDL_GetPerformanceCounter() - t1)) / perHtickTime);

	//历遍图层，注意需要先处理父对象
	for (int level = 1; level <= 3; level++) {
		for (auto iter = map->begin(); iter != map->end();) {
			LOLayer *lyr = iter->second;
			//检查是不是现在要处理的
			bool isnow = false;
			if (level < 3 && lyr->id[level] >= G_maxLayerCount[level]) isnow = true;
			else if (level >= 3) isnow = true;
			else isnow = false;

			////////
			if (isnow) {
				if(lyr->data->bak.isDelete()) LOLayer::NoUseLayer(lyr);
				else lyr->UpDataToForce();
				//指向下一个
				iter = map->erase(iter);
			}
			else iter++;

		}
	}

	//LOLog_i("uppos2:%f", ((double)(SDL_GetPerformanceCounter() - t1)) / perHtickTime);
	//======图层已经更新完成=======

	//等待print完成才继续
	LOEventHook *ep = NULL;
	if (iswait) {
		ep = LOEventHook::CreatePrintHook(printHook.get(), ef, print_name);
		//提交到等待位置
		ep->ResetMe();
		//printf("script thread:%d\n", SDL_GetTicks());
	}
	SDL_UnlockMutex(layerQueMutex);
	if (ep) {
		//print2-18会响应点击事件
		if (ef) G_hookQue.push_back(printHook, LOEventQue::LEVEL_HIGH);
		ep->waitEvent(1, -1);
		//无效化，这样放入其他队列后才有机会整理
		ep->InvalidMe();
	}
	SDL_UnlockMutex(doQueMutex);
	return 0;
}


int LOImageModule::SendEventToLayer(LOEventHook *e) {
	//发往图层，看图层是否响应
	int ret = LOLayer::SENDRET_NONE;
	for (int ii = 0; ii < LOLayer::LAYER_BASE_COUNT && ret == LOLayer::SENDRET_NONE; ii++) {
		ret = G_baseLayer[ii]->checkEvent(e, &waitEventQue);
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
		ev->catchFlag = LOLayerDataBase::FLAGS_MOUSEMOVE;
		ev->PushParam(new LOVariant(mouseXY[0]));
		ev->PushParam(new LOVariant(mouseXY[1]));
		SendEventToLayer(ev.get());
		break;
	case SDL_MOUSEBUTTONUP:
		//鼠标进行了点击
		if (!TranzMousePos(event->button.x, event->button.y))break;
		//这里需要将左、右键事件先发一次hook队列，因为有些hook需要优先处理，比如print wait等点击可跳过的事件
		if (event->button.button == SDL_BUTTON_LEFT) ev->catchFlag = LOEventHook::ANSWER_LEFTCLICK;
		else if (event->button.button == SDL_BUTTON_RIGHT) ev->catchFlag = LOEventHook::ANSWER_RIGHTCLICK;
		if (ev->catchFlag) {
			ev->PushParam(new LOVariant(mouseXY[0]));
			ev->PushParam(new LOVariant(mouseXY[1]));
			//队列没有响应按键事件，那么就发送到图层响应
			//在多线程脚本中可能同时出现 btnwait 和 delay这种需要同时响应按键的窘境，这里btnwait的优先级都被滞后了
			//这里要注意检查事件类型是否被改变
			int old = ev->catchFlag;
			if (SendEventToHooks(ev.get(), LOEventQue::LEVEL_HIGH | LOEventQue::LEVEL_NORMAL) == LOEventHook::RUNFUNC_CONTINUE) {
				if(ev->catchFlag == old) {
					if (event->button.button == SDL_BUTTON_LEFT) ev->catchFlag = LOLayerDataBase::FLAGS_LEFTCLICK;
					else if (event->button.button == SDL_BUTTON_RIGHT) ev->catchFlag = LOLayerDataBase::FLAGS_RIGHTCLICK;
					SendEventToLayer(ev.get());
				}
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
			SendEventToHooks(ev.get(), LOEventQue::LEVEL_HIGH | LOEventQue::LEVEL_NORMAL);
		}
	}

	waitEventQue.clear();
}


//单独立即处理某个事件
int LOImageModule::SendEventToHooks(LOEventHook *e, int flags) {
	int state = LOEventHook::RUNFUNC_CONTINUE;
	int listID[2] = {flags & LOEventQue::LEVEL_NORMAL,flags & LOEventQue::LEVEL_HIGH };

	for (int level = 1; level >= 0; level--) {
		int index = 0;
		while (listID[level] && state == LOEventHook::RUNFUNC_CONTINUE) {
			LOShareEventHook hook = G_hookQue.GetEventHook(index, listID[level], true);
			if (!hook) break;
			//printf("%x\n", hook.get());
			if (hook->catchFlag & e->catchFlag) state = RunFuncBase(hook.get(), e);
			if (state != LOEventHook::RUNFUNC_FINISH) hook->closeEdit();
		}
	}
	return state;
}

//int LOImageModule::SendEventToHooks(LOEventHook *e) {
//	int state = LOEventHook::RUNFUNC_CONTINUE;
//	for (int level = LOEventQue::LEVEL_HIGH; level >= LOEventQue::LEVEL_NORMAL; level--) {
//		int index = 0;
//		while (state == LOEventHook::RUNFUNC_CONTINUE) {
//			LOShareEventHook hook = G_hookQue.GetEventHook(index, level, true);
//			if (!hook) break;
//			if (hook->catchFlag & e->catchFlag) state = RunFuncBase(hook.get(), e);
//
//			if (state != LOEventHook::RUNFUNC_FINISH) hook->closeEdit();
//		}
//	}
//	return state;
//}


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
	int fullid = GetFullID(LOLayer::LAYER_DIALOG, LOLayer::IDEX_DIALOG_TEXT, 255, 255);
	if (flag == '\\') {
		int fullid = GetFullID(LOLayer::LAYER_DIALOG, LOLayer::IDEX_DIALOG_TEXT, 255, 255);
		LOLayerData *info = CreateNewLayerData(fullid, "_lons");
		info->bak.SetDelete();
		sayState.say.clear();
	}
}


//立即完成对话文字
void LOImageModule::CutDialogueAction() {
	int fullid = GetFullID(LOLayer::LAYER_DIALOG, LOLayer::IDEX_DIALOG_TEXT, 255, 255);
	LOLayer *lyr = LOLayer::FindLayerInCenter(fullid);
	if (lyr && lyr->data->cur.isForce()) {
		LOActionText *ac = (LOActionText*)lyr->data->cur.GetAction(LOAction::ANIM_TEXT);
		if (ac) {
			lyr->DoTextAction(lyr->data.get(), ac, 0x7ffffff);
		}
	}
}

//立即完成print事件
int LOImageModule::CutPrintEffect(LOEventHook *hook, LOEventHook *e) {
	if (!hook) hook = printHook.get();

	if (hook->enterEdit() || hook->isState(LOEventHook::STATE_EDIT)) {
		LOEffect *ef = (LOEffect*)hook->GetParam(0)->GetPtr();
		const char *printName = hook->GetParam(1)->GetChars(nullptr);
		ContinueEffect(ef, printName, 0x7ffffff);
		hook->FinishMe();
		//转换事件类型
		if (e && e->catchFlag == LOEventHook::ANSWER_LEFTCLICK) e->catchFlag = LOEventHook::ANSWER_PRINGJMP;
		else return LOEventHook::RUNFUNC_FINISH;
	}
	return LOEventHook::RUNFUNC_CONTINUE;
}


int LOImageModule::RunFunc(LOEventHook *hook, LOEventHook *e) {
	switch (hook->param2){
	case LOEventHook::FUN_SPSTR:
		return RunFuncSpstr(hook, e);
	case LOEventHook::FUN_INVILIDE:
		hook->InvalidMe();
		return LOEventHook::RUNFUNC_FINISH;
	case LOEventHook::FUN_TEXT_ACTION:
		return RunFuncText(hook, e);
	case LOEventHook::FUN_CONTINUE_EFF:
		return CutPrintEffect(hook, e);
	default:
		break;
	}

	return LOEventHook::RUNFUNC_CONTINUE;
}

int LOImageModule::RunFuncBtnFinish(LOEventHook *hook, LOEventHook *e) {
	return LOEventHook::RUNFUNC_CONTINUE;
}


LOtexture* LOImageModule::ScreenShot(int x, int y, int w, int h, int dw, int dh) {
	LOEventHook::CreateScreenShot(printPreHook.get(), x, y, w, h, dw, dh);
	printPreHook->ResetMe();
	printPreHook->waitEvent(1, -1);
	LOtexture *tex = (LOtexture*)printPreHook->GetParam(6)->GetPtr();
	printPreHook->ClearParam();
	return tex;
}


void LOImageModule::ScreenShotCountinue(LOEventHook *e) {
	SDL_Rect src, dst;
	src.x = e->GetParam(0)->GetInt();
	src.y = e->GetParam(1)->GetInt();
	src.w = e->GetParam(2)->GetInt();
	src.h = e->GetParam(3)->GetInt();
	dst.x = 0; dst.y = 0;
	dst.w = e->GetParam(4)->GetInt();
	dst.h = e->GetParam(5)->GetInt();

	//要截取的是渲染器的位置，因此要根据缩放位置计算
	src.x = G_gameScaleX * src.x; src.y = G_gameScaleY * src.y;
	src.w = G_gameScaleX * src.w; src.h = G_gameScaleY * src.w;
	if (src.w > 0 && src.h > 0 && dst.w > 0 && dst.h > 0) {
		//创建一个用于接收画面的纹理
		LOtexture *tex = new LOtexture();
		tex->CreateDstTexture(dst.w, dst.h, SDL_TEXTUREACCESS_TARGET);
		//把画面帧缩放转移到缓冲纹理中
		SDL_SetRenderTarget(render, tex->GetTexture());
		SDL_RenderClear(render);
		SDL_RenderCopy(render, effectTex->GetTexture(), &src, &dst);
		//从GPU拷贝数据到内存，必须在主线程中，因为涉及到纹理锁定
		tex->CopyTextureToSurface(true);

		e->PushParam(new LOVariant(tex));
	}
	else e->PushParam(new LOVariant((void*)0));

	e->FinishMe();
}



int LOImageModule::RunFuncSpstr(LOEventHook *hook, LOEventHook *e) {
	LOString btnstr = e->GetParam(1)->GetLOString();
	RunExbtnStr(&btnstr);
	//这个钩子长期有效
	hook->closeEdit();
	return LOEventHook::RUNFUNC_FINISH;
}


int LOImageModule::RunFuncText(LOEventHook *hook, LOEventHook *e) {
	if (e->catchFlag == LOEventHook::ANSWER_LEFTCLICK) {
		e->catchFlag = LOEventHook::ANSWER_PRINGJMP;
		CutDialogueAction();
		return LOEventHook::RUNFUNC_CONTINUE;
	}
	else { //LOEventHook::ANSWER_PRINGJMP
		CutDialogueAction();
	}
	return LOEventHook::RUNFUNC_FINISH;
}


void LOImageModule::RunExbtnStr(LOString *s) {
	/*
	const char *obuf, *buf;
	obuf = buf = s->c_str();
	int maxlen = s->length();
	
	//注意显示类的操作都需要等到下一帧才刷新
	while (buf - obuf < maxlen) {
		int cc = toupper(buf[0]);
		if (cc == 'C') {  //隐藏
			buf++;
			int ids[] = {s->GetInt(buf) ,255,255 };
			//LOLayer *lyr = FindLayerInBase(LOLayer::LAYER_SPRINT, ids);
			//if (lyr)  lyr->curInfo->SetVisable(0);
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
	*/
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


void LOImageModule::LoadReset() {
	LOLayer::ResetLayer();
	//清空等待队列
	for (int ii = 0; ii < backDataMaps.size(); ii++) backDataMaps[ii]->map->clear();
	//清除所有的事件，一些常驻事件要重新注册
	G_hookQue.invalidClear();
	ChangeFlagState(0);

	SDL_RenderClear(render);
	SDL_RenderPresent(render);
}
