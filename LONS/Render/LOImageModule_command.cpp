/*
//图像模块命令实现
*/
#include "../etc/LOString.h"
#include "LOImageModule.h"

int LOImageModule::lspCommand(FunctionInterface *reader) {
	//if (reader->GetCurrentLine() == 184968) {
	//	int debugbreak = 1;
	//}
	bool visiable = !reader->isName("lsph");
	int ids[] = { reader->GetParamInt(0),255,255 };
	int fixpos = 0;
	int fullid = GetFullID(LOLayer::LAYER_SPRINT, ids);

	ids[0] = reader->GetParamInt(0);
	if (reader->isName("lspc")) {
		ids[1] = reader->GetParamInt(1);
		fixpos = 1;
	}

	LOString tag = reader->GetParamStr(1 + fixpos);
	int xx = reader->GetParamInt(2);
	int yy = reader->GetParamInt(3);
	int alpha = -1;
	if (reader->GetParamCount() > 4 + fixpos) alpha = reader->GetParamInt(4 + fixpos);
	
	reader->ExpandStr(tag);
	LeveTextDisplayMode();

	//已经在队列里的需要释放
	//
	LOLayerData* info = CreateLayerData(fullid, reader->GetPrintName());
	loadSpCore(info, tag, xx, yy, alpha);
	info->SetVisable(visiable);
	return RET_CONTINUE;
}

int LOImageModule::lsp2Command(FunctionInterface *reader) {
	/*
	int ids[] = { reader->GetParamInt(0),255,255 };
	LOString tag = reader->GetParamStr(1);
	int alpha = -1;
	if (reader->GetParamCount() > 7) alpha = reader->GetParamInt(7);

	LeveTextDisplayMode();

	LOLayerInfo *info = GetInfoNewAndFreeOld(GetFullID(LOLayer::LAYER_SPRINTEX, ids), reader->GetPrintName());
	loadSpCore(info, tag, reader->GetParamInt(2), reader->GetParamInt(3), alpha);

	if (info->texture) {
		int w, h;
		info->centerX = info->texture->baseW() / 2;
		info->centerY = info->texture->baseH() / 2;
	}

	info->SetPosition2(info->centerX, info->centerY, reader->GetParamDoublePer(4), reader->GetParamDoublePer(5));
	
	info->rotate = reader->GetParamDoublePer(6);
	if (fabs(info->rotate - 0.0) > 0.001) {
		info->SetRotate(info->rotate);
	}

	std::string cmd(reader->GetCmdChar());
	if (cmd.length() > 3) {
		if (cmd[3] == 'h') info->visiable = false;
		std::string eflag = cmd.substr(cmd.length() - 3, 3);
		if (eflag == "add" && info->texture) {
			info->texture->setBlendModel(SDL_BLENDMODE_ADD);
		}
		else if (eflag == "sub" && info->texture) {
			//info->texture->setBlendModel(SDL_BLENDMODE_INVALID);
			//注意并不是所有的渲染器都能正确实现
			//SDL_BlendMode bmodel = SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_SRC_COLOR, SDL_BLENDFACTOR_ONE_MINUS_DST_COLOR,
				//SDL_BLENDOPERATION_ADD, SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD);
			//info->texture->setBlendModel(bmodel);
			SimpleError("LONS not support [lsp2sub], because some renderers do not support!\n");
		}
	}
	*/
	return RET_CONTINUE;
}


int LOImageModule::printCommand(FunctionInterface *reader) {
	//if (reader->GetCurrentLine() == 538) {
	//	int debugbreak = 11;
	//}
	return printStack(reader, 0);
}


int LOImageModule::printStack(FunctionInterface *reader, int fix) {

	LeveTextDisplayMode();

	int no = reader->GetParamInt(fix);
	int paramCount = reader->GetParamCount() - fix;

	LOEffect *ef = GetEffect(no);
	if (!ef && no > 1) {
		LOLog_i("effect %d not find!", no);
		ef = new LOEffect;
		ef->nseffID = 10;
		ef->time = 100;
	}
	if (ef) {
		if (paramCount > 1) ef->time = reader->GetParamInt(fix + 1);
		if (paramCount > 2) ef->mask = reader->GetParamStr(fix + 2);
	}
	ExportQuequ(reader->GetPrintName(), ef, true);
	if (ef) delete ef;
	return RET_CONTINUE;
}


int LOImageModule::bgCommand(FunctionInterface *reader) {
	//if (reader->GetCurrentLine() == 502) {
	//	int debugbreak = 0;
	//}
	/*
	LeveTextDisplayMode();

	int ids[] = { 5,255,255 };
	LOLayerInfo *info = GetInfoNewAndFreeOld(GetFullID(LOLayer::LAYER_BG, ids), reader->GetPrintName());
	LOString tag = reader->GetParamStr(0);

	LOString tmp = tag.toLower();
	if (tmp == "white") tag = StringFormat(64, ":c;>%d,%d#ffffff", G_gameWidth, G_gameHeight);
	else if (tmp == "black") tag = StringFormat(64, ":c;>%d,%d#000000", G_gameWidth, G_gameHeight);
	else if (tmp.at(0) == '#') tag = StringFormat(64, ":c;>%d,%d%s", G_gameWidth, G_gameHeight, tag.c_str());
	else if (tmp.at(0) != ':') tag = ":c;" + tag;
	loadSpCore(info, tag, 0, 0, -1);
	if (reader->GetParamCount() > 1) {
		return printStack(reader, 1);
	}
	else {
		return RET_CONTINUE;
	}
	*/

	return RET_CONTINUE;
}


int LOImageModule::cspCommand(FunctionInterface *reader) {
	//if (reader->GetCurrentLine() == 448) {
	//	int debugbreak = -1;
	//}

	LeveTextDisplayMode();
	int layerType = LOLayer::LAYER_SPRINT;
	if (reader->isName("csp2")) layerType = LOLayer::LAYER_SPRINTEX;
	if (reader->GetParamInt(0) < 0) {
		CspCore(layerType, 0, 1023, reader->GetPrintName());
	}
	else {
		CspCore(layerType, reader->GetParamInt(0), reader->GetParamInt(0), reader->GetPrintName());
	}
	return RET_CONTINUE;
}

//fullid为-1时清除全部print_name队列
void LOImageModule::CspCore(int layerType, int fromid, int endid, const char *print_name) {
	auto *map = GetPrintNameMap(print_name)->map;
	if (map->size() == 0) return;

	for (int ii = fromid; ii <= endid; ii++) {
		int fullid = GetFullID(layerType, ii, 255, 255);
		auto iter = map->find(fullid);
		//layerdata在print同步时才删除
		if (iter != map->end()) iter->second->SetDelete();
	}
}

int LOImageModule::mspCommand(FunctionInterface *reader) {
	//if (reader->GetCurrentLine() == 571) {
	//	int debugbreak = 1;
	//}
	/*
	LOLayer::SysLayerType sptype = LOLayer::LAYER_SPRINT;
	if (reader->isName("msp2") || reader->isName("amsp2")) sptype = LOLayer::LAYER_SPRINTEX;
	bool addmode = false;
	if (reader->isName("msp") || reader->isName("msp2")) addmode = true;
	int ids[] = { reader->GetParamInt(0),255,255 };

	LeveTextDisplayMode();
	SDL_LockMutex(layerQueMutex);
	//万恶的队列操作，麻烦
	LOLayerInfo *doinfo = GetInfoLayerAvailable(sptype, ids, reader->GetPrintName());
	if (!doinfo) {
		SDL_UnlockMutex(layerQueMutex);
		return RET_CONTINUE;
	}
	LOLayerInfo *info = NULL;
	if (addmode) info = LayerInfomation(sptype, ids, reader->GetPrintName());
	SDL_UnlockMutex(layerQueMutex);

	//位置或者中心位置
	if (addmode) {
		doinfo->SetPosition(reader->GetParamInt(1) + info->offsetX, reader->GetParamInt(2) + info->offsetY);
	}
	else {
		doinfo->SetPosition(reader->GetParamInt(1), reader->GetParamInt(2));
	}

	//sp2操作
	if (sptype == LOLayer::LAYER_SPRINTEX) {
		if (addmode) {
			//doinfo->centerX = info->centerX;
			//doinfo->centerY = info->centerY;
			//doinfo->scaleX = ((double)reader->GetParamInt(3) / 100) + info->scaleX;
			//doinfo->scaleY = ((double)reader->GetParamInt(4) / 100) + info->scaleY;
			//doinfo->rotate = info->rotate + reader->GetParamInt(5);
			doinfo->SetPosition2(info->centerX, info->centerY,
				reader->GetParamDoublePer(3) + info->scaleX,
				reader->GetParamDoublePer(4) + info->scaleY);
			doinfo->SetRotate(info->rotate + reader->GetParamInt(5));
		}
		else {
			//doinfo->scaleX = ((double)reader->GetParamInt(3) / 100);
			//doinfo->scaleY = ((double)reader->GetParamInt(4) / 100);
			doinfo->SetPosition2(doinfo->centerX, doinfo->centerY, reader->GetParamDoublePer(3), reader->GetParamDoublePer(4));
			doinfo->SetRotate(reader->GetParamInt(5));
			//doinfo->rotate = reader->GetParamInt(5);
		}
	}
	//透明度
	int fixp = 3;
	if (sptype == LOLayer::LAYER_SPRINTEX) fixp = 6;
	if (reader->GetParamCount() > fixp) {
		if (addmode) doinfo->AddAlpha(reader->GetParamInt(fixp));
		else doinfo->SetAlpha(reader->GetParamInt(fixp));
		//	doinfo->alpha = reader->GetParamInt(fixp);
		//doinfo->layerControl |= LOLayerInfo::CON_UPAPLHA;
	}

	if (info) delete info;
	*/
	return RET_CONTINUE;
}


int LOImageModule::cellCommand(FunctionInterface *reader) {
	/*
	int ids[] = { reader->GetParamInt(0),255,255 };
	LOLayerInfo *info = GetInfoLayerAvailable(LOLayer::LAYER_SPRINT, ids, reader->GetPrintName());
	if (info) {
		info->SetCell(reader->GetParamInt(1));
	}
	*/
	return RET_CONTINUE;
}

int LOImageModule::humanzCommand(FunctionInterface *reader) {
	int no = reader->GetParamInt(0);
	if (no < 1 || no > 999) {
		SDL_Log("[humanz] command is just for 1~999, buf it's %d!", no);
		return RET_WARNING;
	}
	else {
		z_order = no;
		return RET_CONTINUE;
	}
}

int LOImageModule::strspCommand(FunctionInterface *reader) {
	/*
	int ids[] = { reader->GetParamInt(0), 255, 255 };
	LOString tag = "*S;" + reader->GetParamStr(1);
	LOFontWindow ww = winFont;
	ww.topx = reader->GetParamInt(2);
	ww.topy = reader->GetParamInt(3);
	ww.xcount = reader->GetParamInt(4);
	ww.ycount = reader->GetParamInt(5);
	ww.xsize = reader->GetParamInt(6);
	ww.ysize = reader->GetParamInt(7);
	ww.xspace = reader->GetParamInt(8);
	ww.yspace = reader->GetParamInt(9);
	ww.isbold = reader->GetParamInt(10);
	ww.isshaded = reader->GetParamInt(11);

	int colorCount = reader->GetParamCount() - 12;

	LeveTextDisplayMode();

	LOLayerInfo *info = GetInfoNewAndFreeOld(GetFullID(LOLayer::LAYER_SPRINT, ids), reader->GetPrintName());
	SDL_Color *cc = new SDL_Color[3];
	info->maskName = (LOString*)(&ww);
	info->btnStr = (LOString*)cc;
	info->btnValue = colorCount;

	if (colorCount > 0) {
		for (int ii = 0; ii < colorCount; ii++) {
			int colorint = reader->GetParamColor(12 + ii);
			cc[ii].r = (colorint >> 16) & 0xff;
			cc[ii].g = (colorint >> 8) & 0xff;
			cc[ii].b = colorint & 0xff;
		}
	}
	else {
		cc[0] = spFont.fontColor;
		info->btnValue = 1;
	}

	loadSpCore(info, tag, ww.topx, ww.topy, -1);
	if (reader->isName("strsph")) {
		info->SetVisable(0);
	}

	if (cc) delete[] cc;
	*/
	return RET_CONTINUE;
}

int LOImageModule::transmodeCommand(FunctionInterface *reader) {
	LOString keyword = reader->GetParamStr(0).toLower();
	if (keyword == "leftup") trans_mode = LOLayerData::TRANS_TOPLEFT;
	else if (keyword == "copy") trans_mode = LOLayerData::TRANS_COPY;
	else if (keyword == "alpha") trans_mode = LOLayerData::TRANS_ALPHA;
	else if (keyword == "righttup")trans_mode = LOLayerData::TRANS_TOPRIGHT;
	return RET_CONTINUE;
}

int LOImageModule::bgcopyCommand(FunctionInterface *reader) {
	return RET_CONTINUE;
}

int LOImageModule::spfontCommand(FunctionInterface *reader) {
	LOString s = reader->GetParamStr(0).toLower();
	if (s == "clear") {
		spFont.Reset();
	}
	else {
		spFont.fontName = reader->GetParamStr(0);
		spFont.xsize = reader->GetParamInt(1);
		spFont.ysize = reader->GetParamInt(2);
		spFont.xspace = reader->GetParamInt(3);
		spFont.yspace = reader->GetParamInt(4);
		spFont.isbold = reader->GetParamInt(5);
		spFont.isshaded = reader->GetParamInt(6);
	}
	return RET_CONTINUE;
}

int LOImageModule::effectskipCommand(FunctionInterface *reader) {
	effectSkipFlag = reader->GetParamInt(0);
	return RET_CONTINUE;
}

int LOImageModule::getspmodeCommand(FunctionInterface *reader) {
	/*
	ONSVariableRef *v = reader->GetParamRef(0);
	int ids[] = { reader->GetParamInt(1), 255, 255 };
	int visiable = 0;

	LOLayerInfo *info = LayerInfomation(LOLayer::LAYER_SPRINT, ids, reader->GetPrintName());
	if (info) {
		visiable = info->visiable;
		delete info;
	}

	v->SetValue((double)visiable);
	*/
	return RET_CONTINUE;
}

int LOImageModule::getspsizeCommand(FunctionInterface *reader) {
	//if (reader->GetCurrentLine() == 423) {
	//	int debugbreak = 1;
	//}
	
	int fullid = GetFullID(LOLayer::LAYER_SPRINT, reader->GetParamInt(0), 255, 255);
	ONSVariableRef *v1 = reader->GetParamRef(1);
	ONSVariableRef *v2 = reader->GetParamRef(2);
	ONSVariableRef *v3 = NULL;
	if (reader->GetParamCount() > 3) v3 = reader->GetParamRef(3);

	int ww, hh, cell;
	ww = hh = 0;
	cell = 0;
	LOLayerData *data = GetLayerData(fullid, reader->GetPrintName());
	if (data) {
		if (data->texture) {

		}
	}
	v1->SetValue((double)ww);
	v2->SetValue((double)hh);
	if (v3) v3->SetValue((double)cell);
	
	return RET_CONTINUE;
}

int LOImageModule::getspposCommand(FunctionInterface *reader) {
	/*
	int ids[] = { reader->GetParamInt(0), 255, 255 };
	int xx, yy;
	xx = yy = 0;
	ONSVariableRef *v1 = reader->GetParamRef(1);
	ONSVariableRef *v2 = reader->GetParamRef(2);
	LOLayerInfo *info = NULL;
	info = LayerInfomation(LOLayer::LAYER_SPRINT, ids, reader->GetPrintName());
	if (info) {
		xx = info->offsetX;
		yy = info->offsetY;
		delete info;
	}

	v1->SetValue((double)xx);
	v2->SetValue((double)yy);
	*/
	return RET_CONTINUE;
}

int LOImageModule::getspalphaCommand(FunctionInterface *reader) {
	/*
	int ids[] = { reader->GetParamInt(0), 255, 255 };
	double val = 0.0;
	LOLayerInfo *info = LayerInfomation(LOLayer::LAYER_SPRINT, ids, reader->GetPrintName());
	if (info) {
		val = info->alpha;
		if (val < 0 || val > 255) val = 255;
		delete info;
	}

	ONSVariableRef *v = reader->GetParamRef(1);
	v->SetValue(val);
	*/
	return RET_CONTINUE;
}

int LOImageModule::getspposexCommand(FunctionInterface *reader) {
	/*
	ONSVariableRef *v[5];
	double val[5];
	int ids[] = { reader->GetParamInt(0), 255,255 };
	for (int ii = 0; ii < 5; ii++) {
		if (ii < reader->GetParamCount()) v[ii] = reader->GetParamRef(ii + 1);
		else v[ii] = NULL;
		val[ii] = 0.0;
	}

	val[2] = val[3] = 100.0;
	LOLayerInfo *info = NULL;
	if(reader->isName("getspposex")) info = LayerInfomation(LOLayer::LAYER_SPRINT, ids, reader->GetPrintName());
	else info = LayerInfomation(LOLayer::LAYER_SPRINTEX, ids, reader->GetPrintName());

	if (info) {
		val[0] = info->offsetX;
		val[1] = info->offsetY;
		val[2] = info->scaleX * 100;
		val[3] = info->scaleY * 100;
		val[4] = info->rotate;
	}

	for (int ii = 0; ii < 5; ii++) {
		if (v[ii]) v[ii]->SetValue(val[ii]);
	}
	*/
	return RET_CONTINUE;
}

int LOImageModule::vspCommand(FunctionInterface *reader) {
	int ids[] = { reader->GetParamInt(0), 255,255 };
	LOLayer::SysLayerType sptype = LOLayer::LAYER_SPRINT;
	if (reader->isName("vsp2")) sptype = LOLayer::LAYER_SPRINTEX;

	LeveTextDisplayMode();

	VspCore(sptype, ids, reader->GetPrintName(), reader->GetParamInt(1));
	return RET_CONTINUE;
}

void LOImageModule::VspCore(LOLayer::SysLayerType sptype, int *cid, const char *print_name, int vals) {
	/*
	
	LOLayerInfo *info = GetInfoLayerAvailable(GetFullID(sptype, cid), print_name);
	if (info) {
		info->SetVisable(vals);
	}
	*/
}


int LOImageModule::allspCommand(FunctionInterface *reader) {
	/*
	std::vector<int> *list_t = allSpList;
	auto sptype = LOLayer::LAYER_SPRINT;
	if (reader->isName("allsp2hide") || reader->isName("allsp2resume")) {
		sptype = LOLayer::LAYER_SPRINTEX;
		list_t = allSpList2;
	}

	if (!list_t) list_t = new std::vector<int>;
	//bool visiable = true;

	if (reader->isName("allsphide") || reader->isName("allsp2hide")) { //hide
		list_t->clear();
		//队列中的新图像，和调整显示模式的任务被置为隐藏
		std::vector<LOLayerInfoCacheIndex*> list = FilterCacheQue(reader->GetPrintName(), sptype, LOLayerInfo::CON_UPFILE | LOLayerInfo::CON_UPVISIABLE, false);
		for (auto iter = list.begin(); iter != list.end(); iter++) {
			LOLayerInfo *info = &(*iter)->info;
			info->SetVisable(0);
			list_t->push_back(info->fullid);
		}

		//显示中的图像被隐藏
		LOLayer *layer = lonsLayers[sptype];
		for (auto iter = layer->childs->begin(); iter != layer->childs->end(); iter++) {
			LOLayer *lyr = iter->second;
			LOLayerInfo *info = GetInfoNewAndNoFreeOld(lyr->GetFullid(), reader->GetPrintName());
			info->SetVisable(0);
			list_t->push_back(info->fullid);
		}
	}
	else { //show
		int ids[] = { 1023,255,255 };
		for (int ii = 0; ii < list_t->size(); ii++) {
			LOLayerInfo *info = GetInfoNewAndNoFreeOld(list_t->at(ii), reader->GetPrintName());
			info->SetVisable(1);
		}
	}

	if(list_t->size() > 0 ) ExportQuequ(reader->GetPrintName(), NULL, true);
	*/
	return RET_CONTINUE;
}

int LOImageModule::windowbackCommand(FunctionInterface *reader) {
	winbackMode = true;
	return RET_CONTINUE;
}

int LOImageModule::textCommand(FunctionInterface *reader) {
	//if (reader->GetCurrentLine() == 298258) {
	//	int debugbreak = 1;
	//}

	LOString text = reader->GetParamStr(0);
	pageEndFlag = reader->GetParamInt(1);

	if (texecFlag && dialogText.length() > 0) dialogText.append("\n");
	dialogText.append(text);
	LoadDialogText(&dialogText, true);  //文字都是由text命令控制的，不受上次的符号影响

	/*
	//结尾符号进入参数传递
	LOEvent1 *e = new LOEvent1(LOEvent1::EVENT_TEXT_ACTION, FunctionInterface::LAYER_TEXT_WORKING);
	e->value = pageEndFlag;
	
	e->enterEdit();  //lock event,cant't get it now
	G_SendEvent(e);                         //进入layer初始化
	reader->blocksEvent.SendToSlot(e);      //阻塞脚本
	e->closeEdit();
	*/
	EnterTextDisplayMode(true);  //will display here

	//after show text
	//if (fontManager->rubySize[2] == LOFontManager::RUBY_LINE) fontManager->rubySize[2] = LOFontManager::RUBY_OFF;
	return RET_CONTINUE;
}

int LOImageModule::effectCommand(FunctionInterface *reader) {
	//if (NotAtDefineError("effect"))return RET_ERROR;
	int no = reader->GetParamInt(0);
	if (reader->isName("effect")) {
		if (no >= 1 && no <= 18) {
			SimpleError("[effect] command id can't use 1~18,that's ONScripter system used!");
			return RET_ERROR;
		}
	}
	LOEffect *ef = new LOEffect;
	ef->id = no;
	ef->nseffID = reader->GetParamInt(1);
	if (reader->GetParamCount() > 2) ef->time = reader->GetParamInt(2);
	else ef->time = 10;   //少量的时间来避免潜在的错误
	if (reader->GetParamCount() > 3) ef->mask = reader->GetParamStr(3);


	auto iter = effectMap.find(no);
	if (iter != effectMap.end()) delete iter->second;
	effectMap[no] = ef;
	return RET_CONTINUE;
}

int LOImageModule::windoweffectCommand(FunctionInterface *reader) {
	if (winEffect) delete winEffect;
	winEffect = GetEffect(reader->GetParamInt(0));
	if (winEffect) {
		if (reader->GetParamCount() > 1) winEffect->time = reader->GetParamInt(1);
		if (reader->GetParamCount() > 2) winEffect->mask = reader->GetParamStr(2);
	}
	return RET_CONTINUE;
}


//btntime btntime2小心的用在多线程
int LOImageModule::btnwaitCommand(FunctionInterface *reader) {
	Uint32 pos, timesnap = SDL_GetTicks();

	if (reader->isName("btnwait") || reader->isName("btnwait2")) LeveTextDisplayMode();
	if (textbtnFlag && reader->isName("textbtnwait")) { //注册文字按钮
		//int ids[] = { LOLayer::IDEX_DIALOG_TEXT,255,255 };
		//LOLayerInfo *info = GetInfoLayerAvailable(LOLayer::LAYER_DIALOG, ids, reader->GetPrintName());
		//if (info) {
		//	info->SetBtn(nullptr, textbtnValue);
		//}
	}
	//btnwait对右键和没有点击任何按钮均有反应，因此准备一个空白图层位于底部，以便进行反应
	//exbtn_d实际上就设置这空白层的btnstr
	int fullid = GetFullID(LOLayer::LAYER_BG, LOLayer::IDEX_BG_BTNEND, 255, 255);
	LOLayerData *data = GetOrCreateLayerData(fullid, reader->GetPrintName());
	LOString s("**;_?_empty_?_");
	loadSpCore(data, s, 0, 0, 255);
	data->SetBtndef(nullptr, 0, true, true);
	ExportQuequ(reader->GetPrintName(), nullptr, true);

	//有btntime的话我们希望能比较准确的确定时间，因此要扣除print 1花费的时间
	pos = SDL_GetTicks() - timesnap;
	if (btnOverTime > 0) {
		btnOverTime -= pos;
		if (btnOverTime <= 0) btnOverTime = 1;
	}

	ONSVariableRef *v1 = reader->GetParamRef(0);
	//凡是有时间要求的事件，第一个参数都是超时时间
	LOEventHook *e = LOEventHook::CreateBtnwaitHook(btnOverTime,v1->GetTypeRefid(), reader->GetPrintName(), -1, reader->GetCmdChar());
	LOShareEventHook ev(e);
	
	//每次btnwait都需要设置btnovetime
	btnOverTime = 0;
	G_hookQue.push_back(ev, LOEventQue::LEVEL_NORMAL);
	reader->waitEventQue.push_back(ev, LOEventQue::LEVEL_NORMAL);
	return RET_CONTINUE;
}


int LOImageModule::spbtnCommand(FunctionInterface *reader) {
	int fullid = GetFullID(LOLayer::LAYER_SPRINT, reader->GetParamInt(0), 255, 255);
	LOLayerData *data = GetLayerData(fullid, reader->GetPrintName());
	if (data) {
		if(reader->GetParamCount() > 2) data->SetBtndef( &reader->GetParamStr(2), reader->GetParamInt(1), true, false);
		else data->SetBtndef(nullptr, reader->GetParamInt(1), true, false);
	}
	return RET_CONTINUE;
}


int LOImageModule::exbtn_dCommand(FunctionInterface *reader) {
	int fullid = GetFullID(LOLayer::LAYER_BG, LOLayer::IDEX_BG_BTNEND, 255, 255);
	LOLayerData *data = GetOrCreateLayerData(fullid, reader->GetPrintName());
	data->SetBtndef(&reader->GetParamStr(0), 0, true, true);
	return RET_CONTINUE;
}


int LOImageModule::texecCommand(FunctionInterface *reader) {
//texthide/textshow 只对文字进行操作，不影响对话窗口
//texton/textoff 对整个对话窗口进行操作
//textclear 清除文字，不影响对话框
//texec '\'作为结尾符时立即清除文字，"@"作为结尾符时不清除文字，不管哪个符号都保持文字显示状态
//对话框受erasetextwindow影响，当erasetextwindow有效时，leveatextmode对话框消失

	//texecFlag决定下一行文字是紧接着还是从下一行位置开始
	char lineEnd = pageEndFlag & 0xff;
	if (lineEnd == '\\') {  //换页清除
		ClearDialogText(lineEnd);
		DialogWindowSet(0, -1, -1);
		texecFlag = false;
		//dialogDisplayMode = DISPLAY_MODE_NORMAL;
	}
	else if (lineEnd == '@') {
		if ((pageEndFlag & 0xff00) == 0x1000)texecFlag = true;
		else texecFlag = false;
	}
	else texecFlag = false;
	return RET_CONTINUE;
}

int LOImageModule::texthideCommand(FunctionInterface *reader) {
	if (reader->isName("texthide")) {
		DialogWindowSet(0, -1, -1);
	}
	else {
		DialogWindowSet(1, -1, -1);
	}
	return RET_CONTINUE;
}

int LOImageModule::textonCommand(FunctionInterface *reader) {
	if (reader->isName("texton")) {
		DialogWindowSet(1, 1, 1);
		dialogDisplayMode = DISPLAY_MODE_TEXT;
	}
	else if (reader->isName("textclear")) {
		ClearDialogText('\\');
		DialogWindowSet(0, -1, -1);
	}
	else { //textoff 整个对话框隐藏，包括文字
//		pageEndFlag[1] = pageEndFlag[0];
//		ClearDialogText(pageEndFlag[0]);
		DialogWindowSet(0, 0, 0);
		//实际上显示模式只跟对话框的存在与否有关，跟文字是否存在无关
		dialogDisplayMode = DISPLAY_MODE_NORMAL; 
	}
	return RET_CONTINUE;
}

int LOImageModule::textbtnsetCommand(FunctionInterface *reader) {
	if (reader->GetParamCount() == 1) {
		textbtnFlag = true;
		textbtnValue = reader->GetParamInt(0);
	}
	else textbtnFlag = false;
	return RET_CONTINUE;
}


int LOImageModule::setwindowCommand(FunctionInterface *reader) {
	winFont.topx = reader->GetParamInt(0);
	winFont.topy = reader->GetParamInt(1);
	winFont.xcount = reader->GetParamInt(2);
	winFont.ycount = reader->GetParamInt(3);
	winFont.xsize = reader->GetParamInt(4);
	winFont.ysize = reader->GetParamInt(5);
	winFont.xspace = reader->GetParamInt(6);
	winFont.yspace = reader->GetParamInt(7);
	G_textspeed = reader->GetParamInt(8);
	//ons no bold?
	//winFont.isbold = reader->GetParamInt(9);
	winFont.isshaded = reader->GetParamInt(10);
	winstr = reader->GetParamStr(11);
	winoff.x = reader->GetParamInt(12);
	winoff.y = reader->GetParamInt(13);

	if (winstr.length() > 0 && winstr.at(0) == '#') {
		winoff.w = reader->GetParamInt(14);
		winoff.h = reader->GetParamInt(15);
		winstr = StringFormat(64, ">%d,%d,%s", winoff.w, winoff.h, winstr.c_str());
	}

	if (reader->isName("setwindow")) ClearDialogText('\\'); //setwindow will clear text

	return RET_CONTINUE;
}

int LOImageModule::setwindow2Command(FunctionInterface *reader) {
	winstr = reader->GetParamStr(11);
	if (winstr.length() > 0 && winstr.at(0) == '#') {
		if (winoff.w < 1) winoff.w = 1;
		if (winoff.h < 1) winoff.h = 1;  //safe value
		winstr = StringFormat(64, ">%d,%d,%s", winoff.w, winoff.h, winstr.c_str() + 1);
	}
	return RET_CONTINUE;
}


int LOImageModule::clickCommand(FunctionInterface *reader) {
	//click只等待左键，lrclick左键和右键都可以
	LOShareEventHook ev(LOEventHook::CreateClickHook(true, reader->isName("lrclick")));
	G_hookQue.push_back(ev, LOEventQue::LEVEL_NORMAL);
	reader->waitEventQue.push_back(ev, LOEventQue::LEVEL_NORMAL);
	return RET_CONTINUE;
}

int LOImageModule::erasetextwindowCommand(FunctionInterface *reader) {
	winEraseFlag = reader->GetParamInt(0);
	return RET_CONTINUE;
}

int LOImageModule::getmouseposCommand(FunctionInterface *reader) {
	ONSVariableRef *v1 = reader->GetParamRef(0);
	ONSVariableRef *v2 = reader->GetParamRef(1);
	v1->SetValue((double)mouseXY[0]);
	v1->SetValue((double)mouseXY[1]);
	return RET_CONTINUE;
}


int LOImageModule::btndefCommand(FunctionInterface *reader) {
	//btndef会清除上一次的btndef定义和btn定义
	LOString tag = reader->GetParamStr(0);

	//所有的按钮定义都会被清除
	//无论如何btn的系统层都将被清除
	imgeModule->ClearBtndef(reader->GetPrintName());
	ExportQuequ("_lons", nullptr, true);
	//All button related settings are cleared
	btndefStr.clear();
	btnOverTime = 0;
	btnUseSeOver = false;

	if (tag.toLower().compare("clear") != 0) {
		if (tag[0] != ':') btndefStr = ":c;" + tag;
		else btndefStr = tag;
	}
	return RET_CONTINUE;
}


int LOImageModule::btntimeCommand(FunctionInterface *reader) {
	//btnOverTime 只有在btndef时才会被重置
	btnOverTime = reader->GetParamInt(0);
	if (reader->isName("btntime2")) btnUseSeOver = true;
	return RET_CONTINUE;
}

int LOImageModule::getpixcolorCommand(FunctionInterface *reader) {
	int xx = reader->GetParamInt(1);
	int yy = reader->GetParamInt(2);
	LOString s = "000000";
	if (xx >= 0 && yy >= 0 && xx <= G_gameWidth && yy <= G_gameHeight) {
		//画面可能被放大，因此需要找到 x,y对应的放大区域的中心位置
		for (int ii = xx + 1; ii <= G_gameWidth ; ii++) {
			if (ii * G_gameScaleX != xx * G_gameScaleX) {
				xx = (ii + xx - 1) / 2;
				break;
			}
		}
		for (int ii = yy + 1; ii <= G_gameHeight; ii++) {
			if (ii * G_gameScaleY != yy * G_gameScaleY) {
				yy = (ii + yy - 1) / 2;
				break;
			}
		}
		SDL_Rect rect;
		char tmp[16];
		memset(tmp, 0, 16);
		rect.x = xx * G_gameScaleX;
		rect.y = yy * G_gameScaleY;
		rect.w = 1;
		rect.h = 1;
		
		//防止帧刷新
		SDL_LockMutex(layerQueMutex);
		SDL_RenderReadPixels(render, &rect, SDL_PIXELFORMAT_RGB888, tmp, 4);
		SDL_UnlockMutex(layerQueMutex);
		int val = *(int*)(&tmp[0]);
		sprintf(&tmp[4], "%06x", val & 0xffffff );
		s.assign(&tmp[4]);
	}
	ONSVariableRef *v = reader->GetParamRef(0);
	v->SetValue(&s);
	return RET_CONTINUE;
}


int LOImageModule::gettextCommand(FunctionInterface *reader) {
	ONSVariableRef *v = reader->GetParamRef(0);
	v->SetValue(&dialogText);
	return RET_CONTINUE;
}



//检查指定区域是否全部为某种颜色，允许少量容差
int LOImageModule::chkcolorCommand(FunctionInterface *reader) {

	SDL_Rect src, dst;
	src.x = reader->GetParamInt(0); src.y = reader->GetParamInt(1);
	src.w = reader->GetParamInt(2); src.h = reader->GetParamInt(3);
	dst.x = 0; dst.y = 0;
	dst.w = src.w; dst.h = src.h;

	ONSVariableRef *v = reader->GetParamRef(4);
	/*
	LOSurface *su = nullptr;
		//ScreenShot(&src, &dst);
	if (su && v && v->GetStr()) { //
		//if (reader->GetCurrentLine() == 644) {
		//	int debugbreak = 0;
		//	//SDL_SaveBMP(su->GetSurface(), "test.bmp");
		//}

		const char *buf = v->GetStr()->c_str();
		if (buf[0] == '#')buf++;
		int hex = v->GetStr()->GetHexInt(buf);
		SDL_Color color;
		color.r = (hex >> 16) & 0xff;
		color.g = (hex >> 8) & 0xff;
		color.b = hex & 0xff;
		if (!su->checkColor(color, 3)) {
			LOString tmp = StringFormat(32, "#%02X%02X%02X", color.r, color.g, color.b);
			v->SetValue(&tmp);
		}
		delete su;
	}
	else {
		LOString s("screenshot faild or param error!");
		v->SetValue(&s);
	}
	*/
	return RET_CONTINUE;
}


//鼠标移动并点击，0-只移动，1-左键单击  2-右键单击，后一个参数为多少ms以后执行
int LOImageModule::mouseclickCommand(FunctionInterface *reader) {
	//相对于视口的位置
	int xx = G_gameScaleX * reader->GetParamInt(0) + G_viewRect.x;
	int yy = G_gameScaleY * reader->GetParamInt(1) + G_viewRect.y;
	int click = reader->GetParamInt(2);

	SDL_Event *ev = nullptr;
	if (click == 0) { //只移动
		ev = new SDL_Event();
		ev->type = SDL_MOUSEMOTION;
		ev->motion.type = SDL_MOUSEMOTION;
		ev->motion.x = xx;
		ev->motion.y = yy;
	}
	else if (click == 1) { //左键单击
		ev = new SDL_Event();
		ev->type = SDL_MOUSEBUTTONUP;
		ev->button.x = xx; ev->button.y = yy;
		ev->button.button = SDL_BUTTON_LEFT;
	}
	else if (click == 2) {
		ev = new SDL_Event();
		ev->type = SDL_MOUSEBUTTONUP;
		ev->button.x = xx; ev->button.y = yy;
		ev->button.button = SDL_BUTTON_RIGHT;
	}
	if (ev)SDL_PushEvent(ev);

	return RET_CONTINUE;
}



int LOImageModule::getscreenshotCommand(FunctionInterface *reader) {
	SDL_Rect src, dst;
	src.x = 0; src.y = 0;
	src.w = G_gameWidth; src.h = G_gameHeight;

	dst.x = 0; dst.y = 0;
	dst.w = reader->GetParamInt(0);
	dst.h = reader->GetParamInt(1);

	//if (screenshotSu) delete screenshotSu;
	//screenshotSu =  ScreenShot(&src, &dst);
	//SDL_SaveBMP(su->GetSurface(), "test.bmp");
	return RET_CONTINUE;
}


int LOImageModule::savescreenshotCommand(FunctionInterface *reader) {
	/*
	if (screenshotSu) {
		bool delshot = true;
		ONSVariableRef *v = NULL;

		if (reader->GetParamCount() > 0) v = reader->GetParamRef(0);
		if (reader->isName("savescreenshot")) {
			LOString *s = v->GetStr();
			SDL_SaveBMP(screenshotSu->GetSurface(), s->c_str());
		}
		else if (reader->isName("savescreenshot2")) {
			LOString *s = v->GetStr();
			SDL_SaveBMP(screenshotSu->GetSurface(), s->c_str());
			delshot = false;
		}

		if (delshot) {
			delete screenshotSu;
			screenshotSu = NULL;
		}
	}
	*/
	return RET_CONTINUE;
}


int LOImageModule::rubyCommand(FunctionInterface *reader) {
	if (reader->isName("rubyon")) {
		if (reader->GetParamCount() >= 1) fontManager.rubySize[0] = reader->GetParamInt(0);
		if (reader->GetParamCount() >= 2) fontManager.rubySize[1] = reader->GetParamInt(1);
		if (reader->GetParamCount() >= 3) fontManager.rubyFontName = reader->GetParamStr(2);
		fontManager.rubySize[2] = LOFontManager::RUBY_ON;
	}
	else if (reader->isName("rubyoff")) {
		fontManager.rubySize[2] = LOFontManager::RUBY_OFF;
	}
	else if (reader->isName("rubyon2")) {
		fontManager.rubySize[0] = reader->GetParamInt(0);
		fontManager.rubySize[1] = reader->GetParamInt(1);
		if (reader->GetParamCount() >= 3) fontManager.rubyFontName = reader->GetParamStr(2);
		fontManager.rubySize[2] = LOFontManager::RUBY_LINE;
	}
	return RET_CONTINUE;
}

int LOImageModule::getcursorposCommand(FunctionInterface *reader) {
	int xx, yy, hh;
	xx = yy = hh = 0;
	int ids[] = { LOLayer::IDEX_DIALOG_TEXT,255,255 };
	LOLayer *lyr = FindLayerInBase(LOLayer::LAYER_DIALOG, ids);
	if (lyr) {
		lyr->GetTextEndPosition(&xx, &yy, &hh);  //Relative position of text
		xx += winoff.x;
		yy += winoff.y;
		if (reader->isName("getcursorpos2")) {
			xx -= winFont.xsize;
		}
		else {
			//check it's end of line
			int maxx = (winFont.xsize + winFont.xspace) * winFont.xcount;
			if (maxx - xx <= winFont.xsize / 2) {
				//next line
				xx = winoff.x;
				yy += (hh + winFont.yspace);
			}
		}
	}

	ONSVariableRef *v1 = reader->GetParamRef(0);
	ONSVariableRef *v2 = reader->GetParamRef(1);

	v1->SetValue((double)xx);
	v2->SetValue((double)yy);
	return RET_CONTINUE;
}


int LOImageModule::ispageCommand(FunctionInterface *reader) {
	ONSVariableRef *v = reader->GetParamRef(0);
	if (pageEndFlag == '\\') v->SetValue(1.0);
	else v->SetValue(0.0);
	return RET_CONTINUE;
}


int LOImageModule::btnCommand(FunctionInterface *reader) {
	/*
	if (btndefStr.length() == 0) return RET_CONTINUE;

	int ids[] = { LOLayer::IDEX_NSSYS_BTN, 0, 255 };
	int fullID = GetFullID(LOLayer::LAYER_NSSYS, ids);
	//获取按钮在队列中的情况
	BinArray *bin = GetQueLayerUsedState(fullID, NULL, IDS_LAYER_CHILD);

	//获取按钮在图层中的情况
	SDL_LockMutex(layerQueMutex);
	lonsLayers[LOLayer::LAYER_SPRINT]->GetLayerUsedState(bin->bin, ids);
	SDL_UnlockMutex(layerQueMutex);

	//挑出一个空的编号进行使用
	for (ids[1] = 0; ids[1] < 255 && bin->bin[ids[1]] != 0; ids[1]++);
	if (ids[1] < 255) {
		fullID = GetFullID(LOLayer::LAYER_NSSYS, ids);
		LOLayerInfo *info = GetInfoNewAndFreeOld(fullID, reader->GetPrintName());
		loadSpCore(info, btndefStr, reader->GetParamInt(1), reader->GetParamInt(2), 255);
		info->SetShowRect(reader->GetParamInt(5), reader->GetParamInt(6), reader->GetParamInt(3), reader->GetParamInt(4));
		info->SetBtn(nullptr, reader->GetParamInt(0));
		//检查根对象是否已经载入
		ids[1] = 255;
		info = GetInfoUnLayerAvailable(LOLayer::LAYER_NSSYS, ids, reader->GetPrintName());
		if (info) {
			LOString tmp("**;_?_emptry_?_");
			loadSpCore(info, tmp, 0, 0, 255);
		}
	}
	else LOLog_e("[btn] command more than 255 buttons have been defined!");

	delete bin;
	*/
	return RET_CONTINUE;
}

int LOImageModule::spstrCommand(FunctionInterface *reader) {
	/*
	LOEventParamBtnRef *param = new LOEventParamBtnRef;
	param->ref = new ONSVariableRef(ONSVariableRef::TYPE_STRING_IM);
	param->ref->SetValue(&reader->GetParamStr(0));
	LOEvent1 *e = new LOEvent1(SCRIPTER_RUN_SPSTR, param);
	reader->blocksEvent.SendToSlot(e);
	G_SendEventMulit(e, LOEvent1::EVENT_IMGMODULE_AFTER);
	*/


	return RET_CONTINUE;
}


int LOImageModule::filelogCommand(FunctionInterface *reader) {
	st_filelog = true;
	ReadLog(LOGSET_FILELOG);
	return RET_CONTINUE;
}