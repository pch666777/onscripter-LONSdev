/*
图像队列的处理
*/
#include "../etc/LOEvent1.h"
#include "LOImageModule.h"

extern bool st_skipflag;

//全局的添加处理事件，注意这个函数不是线程安全的，只应该从渲染线程调用
void AddPreEvent(LOShareEventHook &e) {
	if (FunctionInterface::imgeModule) {
		LOImageModule *img = (LOImageModule*)FunctionInterface::imgeModule;
		img->preEventList.push_back(e);
	}
}

void AddRenderEvent(LOShareEventHook &e) {
	if (FunctionInterface::imgeModule) {
		LOImageModule *img = (LOImageModule*)FunctionInterface::imgeModule;
		img->waitEventQue.push_N_back(e);
	}
}


//在正式处理事件前必须处理帧刷新时遗留的事件
void LOImageModule::DoPreEvent(double postime) {
	for (int ii = 0; ii < preEventList.size(); ii++){
		LOShareEventHook e = preEventList[ii];

		if (e->param2 == LOEventHook::FUN_PRE_EFFECT) {  //print准备完成
			//废弃的
			//LOEffect *ef = (LOEffect*)e->GetParam(0)->GetPtr();
			//const char *printName = e->GetParam(1)->GetChars(nullptr);
			//PrepareEffect(ef, printName);
			//e->FinishMe();
		}
		//else if (e->param2 == LOEventHook::FUN_CONTINUE_EFF) { //继续运行
		//	//printf("%d\n", e->catchFlag);
		//	LOEffect *ef = (LOEffect*)e->GetParam(0)->GetPtr();
		//	const char *printName = e->GetParam(1)->GetChars(nullptr);
		//	if (ContinueEffect(ef, printName, postime)) e->FinishMe();
		//	else e->closeEdit();
		//}
		else if (e->param2 == LOEventHook::LOEventHook::FUN_SCREENSHOT) {
			ScreenShotCountinue(e.get());
			e->FinishMe();
		}
		else if (e->param2 == LOEventHook::FUN_TEXT_ACTION) {
			//文字处理完成
			e->FinishMe();
			//SDL_Log("text finish send!");
		}
	}

	preEventList.clear();
}


//print的过程无法支持异步，这会导致非常复杂的问题，特别是需要存档的话
int LOImageModule::ExportQuequ(const char *print_name, LOEffect *ef, bool iswait, bool isEmptyContine) {
	//考虑到需要存档
	iswait = true;
	//快进模式，要尽可能的提升帧的刷新速度
	if (st_skipflag) ef = nullptr;

	//检查是不是有需要刷新的
	auto *map = GetPrintNameMap(print_name)->map;
	if (map->size() == 0 && !isEmptyContine) return 0;

	//SDL_Log("print start") ;
	//print是一个竞争过程，只有执行完成一个才能下一个
	SDL_LockMutex(doQueMutex);

	//有特效运行的情况
	if (ef) {
		//LOEventHook::CreatePrintHook(effcetRunHook.get(), ef, print_name);
		PrepareEffect(ef);
	}
	//直接创建展开队列信号，并提交到等等
	LOEventHook *ep = LOEventHook::CreatePrintHook(printHook.get(), ef, print_name);
	ep->ResetMe();

	if (iswait) {
		ep->waitEvent(1, -1);  //等待帧刷新完成
		ep->InvalidMe();
	}

	SDL_UnlockMutex(doQueMutex);
	//SDL_Log("print finish") ;
	return 0;
}


//在渲染主线程上继续
int LOImageModule::ExportQuequContinue(LOEventHook *e) {
	LOEffect *ef = (LOEffect*)e->GetParam(0)->GetPtr();
	const char *print_name = (const char *)e->GetParam(1)->GetChars(nullptr);
	auto *map = GetPrintNameMap(print_name)->map;

	SDL_Texture *tmp = PrintTextureA;
	PrintTextureA = PrintTextureB;
	PrintTextureB = tmp;

	//历遍图层，注意需要先处理父对象
	for (int level = 1; level <= 3; level++) {
		for (auto iter = map->begin(); iter != map->end();) {
			LOLayer *lyr = iter->second;
			//检查是不是现在要处理的
			bool isnow = false;
			if (level < 3 && lyr->id[level] >= G_maxLayerCount[level]) isnow = true;
			else if (level >= 3) isnow = true;
			else isnow = false;

			//if (lyr->layerType == LOLayer::LAYER_NSSYS && lyr->id[0] == LOLayer::IDEX_NSSYS_BTN) {
			//	int bbk = 1;
			//}

			////////
			if (isnow) {
				//注意纹理的释放必须在渲染主线程
				if (lyr->data->bak.isDelete()) LOLayer::NoUseLayerForce(lyr);
				else {
					//if (scriptModule->GetCurrentLine() == 40148) {
					//	if(lyr->data->bak.buildStr) 
					//		SDL_Log("%d,%s\n", lyr->id[0], lyr->data->bak.buildStr->c_str());
					//	else SDL_Log("%d,%s\n", lyr->id[0], "empty");
					//	breakFlag = true;
					//}
					lyr->UpDataToForce();
				}
				//指向下一个
				iter = map->erase(iter);
			}
			else iter++;

		}
	}
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
    const char *errstr ;

	//包装事件
	switch (event->type){
		//下面是触控事件，触控事件中有touchId来标识是哪一个触控，fingerId来标识是哪一根手指
		case SDL_FINGERDOWN:  //按下触屏，要判断是否双指事件，取值只取触点1的
			if( SDL_GetNumTouchFingers(event->tfinger.touchId) == 2 && event->tfinger.fingerId == 1){
				//防止反复进入
				if(!isFingerEvent){
					fingerEvent = event->tfinger ;
					isFingerEvent = true ;
					//SDL_Log("down tid is %d, fid is %d", event->tfinger.touchId, event->tfinger.fingerId) ;
					//SDL_Log("down at %f, %f", fingerEvent.x, fingerEvent.y) ;
				}
			}
			//SDL_Log("point at %f,%f", event->tfinger.x, event->tfinger.y) ;
			break;
		case SDL_FINGERUP: //离开触屏
			if(isFingerEvent && fingerEvent.touchId == event->tfinger.touchId && fingerEvent.fingerId == event->tfinger.fingerId){
				//触点左上角是[0,0]  右下角是[1,1]
				//SDL_Log("up at %f,%f", event->tfinger.x, event->tfinger.y) ;
				if(event->tfinger.y - fingerEvent.y > 0.28){ //双指下滑
					ev->catchFlag = LOLayerDataBase::FLAGS_RIGHTCLICK;
					SendEventToLayer(ev.get());
					//SDL_Log("right click!");
				}
				else if(event->tfinger.x - fingerEvent.x > 0.25){ //快进

				}
				isFingerEvent = false ;
			}
			break;
		case SDL_FINGERMOTION: //在触屏上移动
			break;
		//=========================================
	case SDL_MOUSEMOTION:
		//更新鼠标位置
		if (!TranzMousePos(event->motion.x, event->motion.y)) break;
		ev->catchFlag = LOLayerDataBase::FLAGS_MOUSEMOVE;
		ev->PushParam(new LOVariant(mouseXY[0]));
		ev->PushParam(new LOVariant(mouseXY[1]));
		SendEventToLayer(ev.get());
		break;
	case SDL_MOUSEBUTTONUP:
		//鼠标进行了点击，因为SDL上，手指事件同时也会触发鼠标事件，因此当进入双指捕获时，不应该再接受鼠标事件
		if (!TranzMousePos(event->button.x, event->button.y) || isFingerEvent)break;
		//SDL_Log("mouse at %d, %d", event->button.x, event->button.y) ;
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
			if (SendEventToHooks(ev.get(), 0) == LOEventHook::RUNFUNC_CONTINUE) {
				if(ev->catchFlag == old) {
					if (event->button.button == SDL_BUTTON_LEFT) ev->catchFlag = LOLayerDataBase::FLAGS_LEFTCLICK;
					else if (event->button.button == SDL_BUTTON_RIGHT) ev->catchFlag = LOLayerDataBase::FLAGS_RIGHTCLICK;
					SendEventToLayer(ev.get());
				}
			}
		}
		break;
	case SDL_KEYDOWN:
		//按下ctrl键，快进
		if (event->key.keysym.sym == SDL_KeyCode::SDLK_LCTRL || event->key.keysym.sym == SDL_KeyCode::SDLK_RCTRL) {
			st_skipflag = true;
			//所有print特性都改为print 1
			//textbtnwait立即完成，文字滚动特效改为立即完成，延迟类命令无效
		}
		break;
	case SDL_KEYUP:
		//结束快进
		if (event->key.keysym.sym == SDL_KeyCode::SDLK_LCTRL || event->key.keysym.sym == SDL_KeyCode::SDLK_RCTRL) {
			st_skipflag = false;
		}
		break;
	}

}


void LOImageModule::HandlingEvents(bool justMustDo) {
	std::vector<LOShareEventHook> backEvList;
	while (true) {
		LOShareEventHook ev = waitEventQue.TakeOutEvent();
		if (!ev) break;
		//首先考虑事件是不是Render_do
		if (ev->catchFlag & LOEventHook::ANSWER_RENDER_DO) RunRenderDo(nullptr, ev.get());
		else{
			//存档前的立即处理会设置justMustDo，将事件传递给钩子列表处理
			if (!justMustDo) SendEventToHooks(ev.get(), 0);

		}
		////某些需要持久运行的事件
		if (ev->isFinishTakeOut() && !ev->isStateAfterFinish()) backEvList.push_back(ev);
	}

	if (backEvList.size() > 0) waitEventQue.push_N_back(backEvList);
}


//单独立即处理某个事件
int LOImageModule::SendEventToHooks(LOEventHook *e, int flags) {
	int state = LOEventHook::RUNFUNC_CONTINUE;

	auto iter = G_hookQue.begin();
	while (state == LOEventHook::RUNFUNC_CONTINUE) {
		LOShareEventHook hook = G_hookQue.GetEventHook(iter, true);
		if (!hook) break;
		if(hook->catchFlag & e->catchFlag) state = RunFuncBase(hook.get(), e);
		//判断事件是否还需要继续传递
		if (state != LOEventHook::RUNFUNC_FINISH) hook->closeEdit();
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
	case LOEventHook::FUN_BTNFINISH:
		return RunFuncBtnFinish(hook, e);
	case LOEventHook::FUN_INVILIDE:
		hook->InvalidMe();
		return LOEventHook::RUNFUNC_FINISH;
	case LOEventHook::FUN_TEXT_ACTION:
		return RunFuncText(hook, e);
	case LOEventHook::FUN_CONTINUE_EFF:
		return CutPrintEffect(hook, e);
	case LOEventHook::FUN_VIDEO_FINISH:
		return RunFuncVideoFinish(hook, e);
	case LOEventHook::FUN_Video_Finish_After:
		return RunFuncVideoFinishAfter(hook);
	default:
		break;
	}

	return LOEventHook::RUNFUNC_CONTINUE;
}


int LOImageModule::RunRenderDo(LOEventHook *hook, LOEventHook *e) {
	int ret = LOEventHook::RUNFUNC_FINISH;
	//对于render_do来说，事件的param2才是处理的函数
	switch (e->param2){
	case LOEventHook::FUN_SPSTR:
		ret = RunFuncSpstr(e);
		break;
	case LOEventHook::FUN_BtnClear:
		ret = RunFuncBtnClear(e);
		break;
	case LOEventHook::FUN_AudioFade:
		ret = audioModule->RunFunc(hook, e);
		break;
	case LOEventHook::FUN_SCREENSHOT:
		//截图
		ret = ScreenShotCountinue(e);
		break;
	default:
		break;
	}
	//hook是长久有效的
	if(hook) hook->closeEdit();
	return LOEventHook::RUNFUNC_FINISH;
}


//这个函数只能从渲染线程调用
int LOImageModule::RunFuncBtnClear(LOEventHook *e) {
	//能调用到这个函数说明肯定不在刷新图层，以及向图层传递事件，所以是线程安全的
	//直接断开[btn]的图层关系
	G_baseLayer[LOLayer::LAYER_NSSYS]->RemodeChild(LOLayer::IDEX_NSSYS_BTN);

	//删除所有[btn]按钮，并且无效化所有的spbtn
	int start = GetFullID(LOLayer::LAYER_NSSYS, LOLayer::IDEX_NSSYS_BTN, 0, 0);
	int end   = GetFullID(LOLayer::LAYER_NSSYS, LOLayer::IDEX_NSSYS_BTN, 255, 255);
	for (auto iter = LOLayer::layerCenter.begin(); iter != LOLayer::layerCenter.end();) {
		if (iter->first >= start && iter->first <= end) {
			//移除图层关系
			iter->second->parent = nullptr;
			if (iter->second->childs) iter->second->childs->clear();
			delete iter->second;
			iter = LOLayer::layerCenter.erase(iter);
		}
		else {
			iter->second->data->bak.unSetBtndef();
			iter->second->data->cur.unSetBtndef();
			iter++;
		}
	}

	//清理队列
	for (int ii = 0; ii < backDataMaps.size(); ii++) {
		auto *map = backDataMaps[ii]->map;
		for (auto iter = map->begin(); iter != map->end();) {
			if (iter->first >= start && iter->first <= end) iter = map->erase(iter);
			else iter++;
		}
	}
	//重置[btn]按钮数量
	BtndefCount = 0;

	//队列清理由脚本线程继续完成
	if(e) e->InvalidMe();
	return LOEventHook::RUNFUNC_FINISH;
}


int LOImageModule::RunFuncBtnFinish(LOEventHook *hook, LOEventHook *e) {
	//后续的赋值处理在 LOScriptReader.cpp中
	//来自超时的事件
	LOUniqEventHook ev;
	if (!e) {
		ev.reset(new LOEventHook());
		e = ev.get();
		e->PushParam(new LOVariant(-1));
		//btnval的超时值为-2
		e->PushParam(new LOVariant(-2));
	}

	if (e->catchFlag == LOEventHook::ANSWER_SEPLAYOVER) {
		//没有满足要求
		if (e->param1 != hook->GetParam(LOEventHook::PINDS_SE_CHANNEL)->GetInt()) return LOEventHook::RUNFUNC_CONTINUE;
	}
	//要确定是否清除btndef
	if (strcmp(hook->GetParam(LOEventHook::PINDS_CMD)->GetChars(nullptr), "btnwait2") != 0) {
		if (e->GetParam(1)->GetInt() > 0)
			RunFuncBtnClear(nullptr);
	}
	//确定是否要设置变量的值，变量设置要转移到脚本线程中
	if (hook->GetParam(LOEventHook::PINDS_REFID) != nullptr) {
		e->paramListMoveTo(hook->GetParamList());
		hook->FinishMe();
	}
	else hook->InvalidMe();
	return LOEventHook::RUNFUNC_FINISH;
}


int LOImageModule::RunFuncVideoFinish(LOEventHook *hook, LOEventHook *e) {
	//播放完成事件要判断图层ID是否一致
	if ((e->catchFlag & LOEventHook::ANSWER_VIDEOFINISH) &&
		hook->GetParam(0)->GetInt() != e->GetParam(0)->GetInt()) {
		return LOEventHook::RUNFUNC_CONTINUE;
	}
	//隐藏图层
	bool issys = false;
	LOLayer *lyr = LOLayer::GetLayer(e->GetParam(0)->GetInt());
	if (lyr) {
		lyr->data->cur.SetVisable(0);
		//这里有个隐含的条件，如果是非sp和spex图层的，在有播放内容的时候不允许再次进入
		lyr->data->cur.SetDelete();
		//如果位于sys层，则要求脚本线程调用渲染模块删除图层
		if (lyr->layerType != LOLayer::LAYER_SPRINT && lyr->layerType != LOLayer::LAYER_SPRINTEX) issys = true;
	}
	if (issys) hook->InvalidMe();
	else hook->FinishMe();
	return LOEventHook::RUNFUNC_FINISH;
}

int LOImageModule::RunFuncVideoFinishAfter(LOEventHook *hook) {
	int fid = hook->GetParam(0)->GetInt();
	LOLayerData* data = CreateLayerBakData(fid, scriptModule->GetPrintName());
	if (data) data->bak.SetDelete();
	hook->InvalidMe();
	return LOEventHook::RUNFUNC_FINISH;
}

//存档前，某些在队列中的事件必须马上完成
int LOImageModule::DoMustEvent(int val) {
	HandlingEvents(true);
	return 0;
}


LOtexture* LOImageModule::ScreenShot(int x, int y, int w, int h, int dw, int dh) {
	LOShareEventHook ev(LOEventHook::CreateScreenShot(x, y, w, h, dw, dh));
	imgeModule->waitEventQue.push_N_back(ev);
	ev->waitEvent(1, -1);
	LOtexture *tex = (LOtexture*)ev->GetParam(6)->GetPtr();
	ev->ClearParam();
	return tex;
}


int LOImageModule::ScreenShotCountinue(LOEventHook *e) {
	SDL_Rect src, dst;
	src.x = e->GetParam(0)->GetInt();
	src.y = e->GetParam(1)->GetInt();
	src.w = e->GetParam(2)->GetInt();
	src.h = e->GetParam(3)->GetInt();
	dst.x = 0; dst.y = 0;
	dst.w = e->GetParam(4)->GetInt();
	dst.h = e->GetParam(5)->GetInt();

	//要截取的是渲染器的位置，因此要根据缩放位置计算
	//src.x = G_gameScaleX * src.x; src.y = G_gameScaleY * src.y;
	//src.w = G_gameScaleX * src.w; src.h = G_gameScaleY * src.w;
	if (src.w > 0 && src.h > 0 && dst.w > 0 && dst.h > 0) {
		//创建一个用于接收画面的纹理
		//FatalError("wort at ScreenShotCountinue()");
		LOtexture *tex = new LOtexture();
		tex->CreateDstTexture(dst.w, dst.h, SDL_TEXTUREACCESS_TARGET);

		////把画面帧缩放转移到缓冲纹理中
		LOtexture::ResetTextureMode(PrintTextureA);
		SDL_SetRenderDrawColor(render, 0, 0, 0, 255);
		SDL_SetRenderTarget(render, tex->GetTexture());
		SDL_RenderClear(render);
		SDL_RenderCopy(render, PrintTextureA, &src, &dst);
		//从GPU拷贝数据到内存，必须在主线程中，因为涉及到纹理锁定
		tex->CopyTextureToSurface(true);
		//LOString s("6666.bmp");
		//tex->SaveSurface(&s);
		e->PushParam(new LOVariant(tex));
	}
	else e->PushParam(new LOVariant((void*)0));

	e->FinishMe();
	return LOEventHook::RUNFUNC_FINISH;
}



int LOImageModule::RunFuncSpstr(LOEventHook *e) {
	LOString btnstr = e->GetParam(1)->GetLOString();
	RunExbtnStr(&btnstr);
	e->InvalidMe();
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

	const char *obuf, *buf;
	obuf = buf = s->c_str();
	int maxlen = s->length();
	int from, end, cell;
	
	//注意显示类的操作都需要等到下一帧才刷新
	while (buf - obuf < maxlen) {
		int cc = toupper(buf[0]);
		if (cc == 'C') {  //隐藏
			buf++;
			end = from = s->GetInt(buf);
			if (buf[0] == '-') {
				buf++;
				end = s->GetInt(buf);
			}
			for (int ii = from; ii <= end; ii++) {
				int fid = GetFullID(LOLayer::LAYER_SPRINT, ii, 255, 255);
				LOLayer *lyr = LOLayer::FindLayerInCenter(fid);
				if (lyr) lyr->data->cur.SetVisable(0);
			}
		}
		else if (cc == 'P') {   //显示
			buf++;
			end = from = s->GetInt(buf);
			if (buf[0] == '-') {
				buf++;
				end = s->GetInt(buf);
			}
			cell = -1;
			if (buf[0] == ',') {  //显示指定cell
				buf++;
				cell = s->GetInt(buf);
			}
			//
			for (int ii = from; ii <= end; ii++) {
				int fid = GetFullID(LOLayer::LAYER_SPRINT, ii, 255, 255);
				LOLayer *lyr = LOLayer::FindLayerInCenter(fid);
				if (lyr) {
					lyr->data->cur.SetVisable(1);
					if(cell >= 0)  lyr->setActiveCell(cell);
				}
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
			int fid = GetFullID(LOLayer::LAYER_SPRINT, s->GetInt(buf), 255, 255);
			if (buf[0] == ',') buf++;
			int xx = s->GetInt(buf);
			if (buf[0] == ',') buf++;
			int yy = s->GetInt(buf);

			LOLayer *lyr = LOLayer::FindLayerInCenter(fid);
			if (lyr) {
				lyr->data->cur.offsetX = xx;
				lyr->data->cur.offsetY = yy;
			}
		}
		else {
            SDL_LogError(0, "RunExbtnStr() unknow key word:%x!", buf[0]);
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


void LOImageModule::LoadReset() {
	LOLayer::ResetLayer();
	//清空等待队列
	for (int ii = 0; ii < backDataMaps.size(); ii++) backDataMaps[ii]->map->clear();
	//清除所有的事件，一些常驻事件要重新注册
	G_hookQue.invalidClear();
	SDL_RenderClear(render);
	SDL_RenderPresent(render);
}


//等待事件完成，继续执行返回真，要退出返回假
bool LOImageModule::WaitStateEvent() {
	if (isModuleExit()) return false;
	//根据对应的事件做重置
	if (isModuleLoading()) LoadReset();
	else if(isModuleReset()) ResetMe();

	//发送完成信号
	ChangeModuleState(MODULE_STATE_SUSPEND);

	//完成后有些操作需要等待其他线程完成
	if (isModuleSaving() || isModuleLoading()) {
		//等待脚本完成事件操作
		while (scriptModule->isModuleSaving() || scriptModule->isModuleLoading()) {
			//G_PrecisionDelay(0.4);
			//休眠降低CPU使用率
			SDL_Delay(1);
		}
	}

	if (scriptModule->isModuleError()) return false;
	//恢复模块状态
	ChangeModuleState(MODULE_STATE_RUNNING);
	ChangeModuleFlags(0);
	return true;
}


void LOImageModule::SimpleEvent(int e, void *data){
    switch (e) {
    case SIMPLE_CLOSE_RUBYLINE:
        sayStyle.UnSetFlags(LOTextStyle::STYLE_RUBYLINE);
        break ;
    }
}
