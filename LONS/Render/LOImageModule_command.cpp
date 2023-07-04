/*
//图像模块命令实现
*/
#include "../etc/LOString.h"
#include "../etc/LOIO.h"
#include "LOImageModule.h"

//#if defined(__WINDOWS__) || defined(_WIN32) || defined(WIN32) || defined(_WIN64) || \
//    defined(WIN64) || defined(__WIN32__) || defined(__TOS_WIN__)
//#include <Windows.h>
//#endif


extern void FatalError(const char *fmt, ...);

int LOImageModule::lspCommand(FunctionInterface *reader) {
    //if (reader->GetCurrentLine() == 5) {
    //    int debugbreak = 1;
    //}
	bool visiable = !reader->isName("lsph");
	//
	int ids[] = { reader->GetParamInt(0),255,255 };
	int fixpos = 0;
	int fullid = GetFullID(LOLayer::LAYER_SPRINT, ids);

	ids[0] = reader->GetParamInt(0);
	if (reader->isName("lspc")) {
		ids[1] = reader->GetParamInt(1);
		fixpos = 1;
	}

	LOString tag = reader->GetParamStr(1 + fixpos);

	int pcount = reader->GetParamCount();
	int xx = 0;
	if (pcount > fixpos + 2) xx = reader->GetParamInt(2 + fixpos);
	int yy = 0;
	if (pcount > 3 + fixpos) yy = reader->GetParamInt(3 + fixpos);
	int alpha = -1;
	if (pcount > 4 + fixpos) alpha = reader->GetParamInt(4 + fixpos);
	
	reader->ExpandStr(tag);
	LeveTextDisplayMode();

	//if (ids[0] == 501) {
	//	int bbk = 0;
	//}

	//已经在队列里的需要释放
	//
	LOLayerData* info = CreateNewLayerData(fullid, reader->GetPrintName());
	loadSpCore(info, tag, xx, yy, alpha, visiable);
	return RET_CONTINUE;
}

int LOImageModule::lsp2Command(FunctionInterface *reader) {
	//if (reader->GetCurrentLine() == 577) {
	//	int bbbk = 0;
	//}

	LOString tag = reader->GetParamStr(1);
	int alpha = -1;
	int fullid = GetFullID(LOLayer::LAYER_SPRINTEX, reader->GetParamInt(0), 255, 255);
	if (reader->GetParamCount() > 7) alpha = reader->GetParamInt(7);

	LeveTextDisplayMode();

	LOLayerData *info = CreateNewLayerData(fullid, reader->GetPrintName());
	loadSpCore(info, tag, reader->GetParamInt(2), reader->GetParamInt(3), alpha, true);
	if (!info->bak.texture) return RET_CONTINUE;

	int cx = info->bak.texture->baseW() / 2;
	int cy = info->bak.texture->baseH() / 2;
	info->bak.SetPosition2(cx, cy, reader->GetParamDoublePer(4), reader->GetParamDoublePer(5));
	double rotate = (double)reader->GetParamInt(6);
	if (fabs(rotate - 0.0) > 0.001) {
		info->bak.SetRotate(rotate);
	}

	// lsp2 lsph2  lsp2add lsph2add
	const char *buf = reader->GetCmdChar() + 3;
	if (buf[0] == 'h') {
		info->bak.SetVisable(false);
		buf++;
	}

	if (buf[0] == 'a') {//lsp2add or  lsph2add
		info->bak.texture->setBlendModel(SDL_BLENDMODE_ADD);
	}
	else if (buf[0] == 's') {
		//info->texture->setBlendModel(SDL_BLENDMODE_INVALID);
		//注意并不是所有的渲染器都能正确实现
		//SDL_BlendMode bmodel = SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_SRC_COLOR, SDL_BLENDFACTOR_ONE_MINUS_DST_COLOR,
		//SDL_BLENDOPERATION_ADD, SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD);
		//info->texture->setBlendModel(bmodel);
        SDL_Log("LONS not support [lsp2sub], because some renderers do not support!");
	}
	return RET_CONTINUE;
}


int LOImageModule::printCommand(FunctionInterface *reader) {
	//if (reader->GetCurrentLine() == 39444) {
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
        SDL_Log("effect %d not find!", no);
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
	if (reader->GetCurrentLine() == 5) {
		int debugbreak = 0;
	}
	
	LeveTextDisplayMode();

	//bg命令会清除ns自带的系统立绘
	CspCore(LOLayer::LAYER_STAND, LOLayer::IDEX_LD_BASE, LOLayer::IDEX_LD_BASE + 3, reader->GetPrintName());

	int fullid = GetFullID(LOLayer::LAYER_BG, 5, 255, 255);
	LOLayerData *info = CreateNewLayerData(fullid, reader->GetPrintName());
	LOString tag = reader->GetParamStr(0);

	LOString tmp = tag.toLower();
	if (tmp == "white") tag = StringFormat(64, ":c;>%d,%d#ffffff", G_gameWidth, G_gameHeight);
	else if (tmp == "black") tag = StringFormat(64, ":c;>%d,%d#000000", G_gameWidth, G_gameHeight);
	else if (tmp.at(0) == '#') tag = StringFormat(64, ":c;>%d,%d%s", G_gameWidth, G_gameHeight, tag.c_str());
	else if (tmp.at(0) != ':') tag = ":c;" + tag;
	loadSpCore(info, tag, 0, 0, -1, true);
	if (reader->GetParamCount() > 1) {
		return printStack(reader, 1);
	}
	else {
		return RET_CONTINUE;
	}
	

	return RET_CONTINUE;
}


int LOImageModule::cspCommand(FunctionInterface *reader) {
	//if (reader->GetCurrentLine() == 419) {
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
	for (int ii = fromid; ii <= endid; ii++) {
		int fullid = GetFullID(layerType, ii, 255, 255);
		LOLayerData *data = CreateLayerBakData(fullid, print_name);
		if (data) {
			data->bak.SetDelete();
		}
	}
}

int LOImageModule::mspCommand(FunctionInterface *reader) {
	//if (reader->GetCurrentLine() == 39442) {
	//	int debugbreak = 1;
	//}
	LOLayer::SysLayerType sptype = LOLayer::LAYER_SPRINT;
	if (reader->isName("msp2") || reader->isName("amsp2")) sptype = LOLayer::LAYER_SPRINTEX;
	bool addmode = false;
	if (reader->isName("msp") || reader->isName("msp2")) addmode = true;
	int fullid = GetFullID(sptype, reader->GetParamInt(0), 255, 255);

	LeveTextDisplayMode();

	LOLayerData *info = CreateLayerBakData(fullid, reader->GetPrintName());
	if (!info) return RET_CONTINUE;

	if (sptype == LOLayer::LAYER_SPRINT) {
		int bx, by, ba = 0;
		bx = reader->GetParamInt(1);
		by = reader->GetParamInt(2);
		if (reader->GetParamCount() > 3) ba = reader->GetParamInt(3);

		if (addmode) {
			bx += info->GetOffsetX();
			by += info->GetOffsetY();
			ba += info->GetAlpha();
		}

		//ba不能超限，不然会造成错误
		if (ba < 0) ba = 0;
		else if (ba > 255) ba = 255;

		info->bak.SetPosition(bx, by);
		if (reader->GetParamCount() > 3) info->bak.SetAlpha(ba);
	}
	else {
		int ox, oy, ra, ba = 0;
		double sx, sy;
		ox = reader->GetParamInt(1);
		oy = reader->GetParamInt(2);
		sx = (double)reader->GetParamInt(3) / 100;
		sy = (double)reader->GetParamInt(4) / 100;
		ra = reader->GetParamInt(5) ;  //rotate
		if (reader->GetParamCount() > 6) ba = reader->GetParamInt(6); //alpha

		if (addmode) {
			//ex中，offset实际就是指中心点的位置
			ox += info->GetOffsetX();
			oy += info->GetOffsetY();
			sx += info->GetScaleX();
			sy += info->GetScaleY();
			ra += info->GetRotate();
			ba += info->GetAlpha();
		}

        //ba不能超限，不然会造成错误
        if (ba < 0) ba = 0;
        else if (ba > 255) ba = 255;

		info->bak.SetPosition2(info->GetCenterX(), info->GetCenterY(), sx, sy);
		info->bak.SetPosition(ox, oy);
		info->bak.SetRotate(ra);
		if (reader->GetParamCount() > 6) info->bak.SetAlpha(ba);
	}
	return RET_CONTINUE;
}


int LOImageModule::cellCommand(FunctionInterface *reader) {
	int fullid = GetFullID(LOLayer::LAYER_SPRINT, reader->GetParamInt(0), 255, 255);
	LOLayerData *info = CreateLayerBakData(fullid, reader->GetPrintName());
	if (info) {
		info->bak.SetCellNum(reader->GetParamInt(1));
	}
	return RET_CONTINUE;
}

int LOImageModule::humanzCommand(FunctionInterface *reader) {
	int no = reader->GetParamInt(0);
	if (no < 1 || no > 999) {
        SDL_Log("[humanz] command is just for 1~999, buf it's %d!", no);
		return RET_WARNING;
	}
	else {
		sayState.z_order = no;
		return RET_CONTINUE;
	}
}

int LOImageModule::strspCommand(FunctionInterface *reader) {

	int ids[] = { reader->GetParamInt(0), 255, 255 };
    LOString tag = "*S;" +  reader->GetParamStr(1);
    reader->ExpandStr(tag) ;

    strspStyle = spStyle ;
    //2-xx,3-yy
    strspStyle.xcount = reader->GetParamInt(4);
    strspStyle.ycount = reader->GetParamInt(5);
    strspStyle.xsize = reader->GetParamInt(6);
    strspStyle.ysize = reader->GetParamInt(7) ;
    strspStyle.xspace = reader->GetParamInt(8);
    strspStyle.yspace = reader->GetParamInt(9);
    if(reader->GetParamInt(10)) strspStyle.SetFlags(LOTextStyle::STYLE_BOLD) ;
    else strspStyle.UnSetFlags(LOTextStyle::STYLE_BOLD);
    if(reader->GetParamInt(11)) strspStyle.SetFlags(LOTextStyle::STYLE_SHADOW);
    else strspStyle.UnSetFlags(LOTextStyle::STYLE_SHADOW);

	int colorCount = reader->GetParamCount() - 12;
	LeveTextDisplayMode();

    LOLayerData *info = CreateNewLayerData(GetFullID(LOLayer::LAYER_SPRINT, ids), reader->GetPrintName());
    //有颜色获取颜色，否则默认为白色
    if(colorCount > 0){
        info->bak.btnStr.reset(new LOString()) ;
        for (int ii = 0; ii < colorCount; ii++) info->bak.btnStr->append(reader->GetParamStr(12 + ii)) ;
    }
    else{
        info->bak.btnStr.reset(new LOString("#ffffff")) ;
    }

    loadSpCore(info, tag, reader->GetParamInt(2), reader->GetParamInt(3), -1, reader->isName("strsp"));

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
		spFontName = sayFontName;
		spStyle = sayStyle;
		spStyle.xcount = 1024;
		spStyle.ycount = 1024;
	}
	else {
		spFontName = reader->GetParamStr(0);
		spStyle.xsize = reader->GetParamInt(1);
		spStyle.ysize = reader->GetParamInt(2);
		spStyle.xspace = reader->GetParamInt(3);
		spStyle.yspace = reader->GetParamInt(4);
        if (reader->GetParamInt(5)) spStyle.SetFlags(LOTextStyle::STYLE_BOLD);
        else spStyle.UnSetFlags(LOTextStyle::STYLE_BOLD);
		if (reader->GetParamInt(6)) {
			spStyle.xshadow = -1;
			spStyle.yshadow = -1;
		}
		else {
			spStyle.xshadow = 0;
			spStyle.yshadow = 0;
		}
	}
	return RET_CONTINUE;
}

int LOImageModule::effectskipCommand(FunctionInterface *reader) {
	effectSkipFlag = reader->GetParamInt(0);
	return RET_CONTINUE;
}

int LOImageModule::getspmodeCommand(FunctionInterface *reader) {

	ONSVariableRef *v = reader->GetParamRef(0);
	int fullid = GetFullID(LOLayer::LAYER_SPRINT, reader->GetParamInt(1), 255, 255);
	int visiable = 0;
	LOLayerData *info = CreateLayerBakData(fullid, reader->GetPrintName());
	if (info && info->GetVisiable()) visiable = 1;
	v->SetValue((double)visiable);
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
	cell = 1;
	LOLayerData *data = GetLayerData(fullid, reader->GetPrintName());
	if (data) {
		ww = data->GetShowWidth();
		hh = data->GetShowHeight();
		cell = data->GetCellCount();
	}
	v1->SetValue((double)ww);
	v2->SetValue((double)hh);
	if (v3) v3->SetValue((double)cell);
	
	return RET_CONTINUE;
}

int LOImageModule::getspposCommand(FunctionInterface *reader) {
	int fullid = GetFullID(LOLayer::LAYER_SPRINT, reader->GetParamInt(0), 255, 255);
	LOLayerData *data = CreateLayerBakData(fullid, reader->GetPrintName());
	int xx = 0;
	int yy = 0;
	ONSVariableRef *v1 = reader->GetParamRef(1);
	ONSVariableRef *v2 = reader->GetParamRef(2);
	if (data) {
		v1->SetValue((double)data->GetOffsetX());
		v2->SetValue((double)data->GetOffsetY());
	}
	else {
		v1->SetValue(0.0);
		v2->SetValue(0.0);
	}
	return RET_CONTINUE;
}

int LOImageModule::getspalphaCommand(FunctionInterface *reader) {
	int fullid = GetFullID(LOLayer::LAYER_SPRINT, reader->GetParamInt(0), 255, 255);
	LOLayerData *data = CreateLayerBakData(fullid, reader->GetPrintName());
	ONSVariableRef *v = reader->GetParamRef(1);
	double val = 0.0;
	if (data) {
		val = data->GetAlpha();
		if (val < 0.0 || val > 255.0) val = 255.0;
	}
	v->SetValue(val);
	return RET_CONTINUE;
}

int LOImageModule::getspposexCommand(FunctionInterface *reader) {
	//if (reader->GetCurrentLine() == 582) {
	//	int bbk = 1;
	//}

	ONSVariableRef *v[5];
	double val[5];
	for (int ii = 0; ii < 5; ii++) {
		if (ii < reader->GetParamCount()) v[ii] = reader->GetParamRef(ii + 1);
		else v[ii] = NULL;
		val[ii] = 0.0;
	}
	val[2] = val[3] = 100.0;

	int lyrType = LOLayer::LAYER_SPRINT;
	if (reader->isName("getspposex2")) lyrType = LOLayer::LAYER_SPRINTEX;
	int fullid = GetFullID(lyrType, reader->GetParamInt(0), 255, 255);
	LOLayerData *data = CreateLayerBakData(fullid, reader->GetPrintName());

	if (data) {
		//在ex中offset指的是中心点的位置
		val[0] = data->GetOffsetX();
		val[1] = data->GetOffsetY();
		val[2] = data->GetScaleX() * 100;
		val[3] = data->GetScaleY() * 100;
		val[4] = data->GetRotate();
	}

	for (int ii = 0; ii < 5; ii++) {
		if (v[ii]) v[ii]->SetValue(val[ii]);
	}

	return RET_CONTINUE;
}

int LOImageModule::vspCommand(FunctionInterface *reader) {
	//if (reader->GetCurrentLine() == 434) {
	//	int bbk = 0;
	//}

	LOLayer::SysLayerType sptype = LOLayer::LAYER_SPRINT;
	if (reader->isName("vsp2")) sptype = LOLayer::LAYER_SPRINTEX;

	LeveTextDisplayMode();

	//if (reader->GetParamInt(0) == 501) {
	//	int bbk = 1;
	//}

	VspCore(GetFullID(sptype, reader->GetParamInt(0), 255,255), reader->GetPrintName(), reader->GetParamInt(1));
	return RET_CONTINUE;
}

void LOImageModule::VspCore(int fullid, const char *print_name, int vals) {
	LOLayerData *data = CreateLayerBakData(fullid, print_name);
	if (data) data->bak.SetVisable(vals);
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
	sayState.setFlags(LOSayState::FLAGS_WINBACK_MODE);
	return RET_CONTINUE;
}

int LOImageModule::textCommand(FunctionInterface *reader) {
	//if (reader->GetCurrentLine() == 298263) {
	//	int debugbreak = 1;
	//}

	LOString text = reader->GetParamStr(0);
	sayState.pageEnd = reader->GetParamInt(1);
	//是不是添加模式只看原来有没有缓存的文字
	bool isadd = sayState.say.length() > 0;
	//texec标记只是决定新添加的文字是否换行
	if (sayState.isTexec() && isadd > 0) sayState.say.append("\n");
	sayState.say.append(text);
	sayState.say.SetEncoder(text.GetEncoder());
	//注意总是有文字被改变
	sayState.setFlags(LOSayState::FLAGS_TEXT_CHANGE);
	LOActionText *ac = LoadDialogText(&sayState.say, sayState.pageEnd,  isadd);
	
	//发出文字事件hook
	if (ac) {
		reader->waitEventQue.push_N_back(ac->hook);
	}

	EnterTextDisplayMode(true);  //will display here

	//after show text
	//if (fontManager->rubySize[2] == LOFontManager::RUBY_LINE) fontManager->rubySize[2] = LOFontManager::RUBY_OFF;
	return RET_CONTINUE;
}

int LOImageModule::effectCommand(FunctionInterface *reader) {
	//if (NotAtDefineError("effect"))return RET_ERROR;
	int no = reader->GetParamInt(0);
	//if (reader->isName("effect")) {
	//	if (no >= 1 && no <= 18) {
	//		FatalError("[effect] command id can't use 1~18,that's ONScripter system used!");
	//		return RET_ERROR;
	//	}
	//}
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
	sayState.winEffect.reset(GetEffect(reader->GetParamInt(0)));
	if (sayState.winEffect) {
		if (reader->GetParamCount() > 1) sayState.winEffect->time = reader->GetParamInt(1);
		if (reader->GetParamCount() > 2) sayState.winEffect->mask = reader->GetParamStr(2);
	}
	return RET_CONTINUE;
}


//btntime btntime2小心的用在多线程
int LOImageModule::btnwaitCommand(FunctionInterface *reader) {
	Uint32 pos, timesnap = SDL_GetTicks();
    const char *prin_name = reader->GetPrintName();
    //btnwait和btnwait2会触发离开文字显示模式
	if (reader->isName("btnwait") || reader->isName("btnwait2")) {
		LeveTextDisplayMode();
        //prin_name = reader->GetPrintName();
	}

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

	LOLayerData *data = CreateNewLayerData(fullid, prin_name);
	LOString s("**;_?_empty_?_");
	loadSpCore(data, s, 0, 0, 255);
	if (exbtn_dStr.length() > 0) data->bak.SetBtndef(&exbtn_dStr, 0, true, true);
	else data->bak.SetBtndef(nullptr, 0, true, true);
	ExportQuequ(prin_name, nullptr, true);

	//有btntime的话我们希望能比较准确的确定时间，因此要扣除print 1花费的时间
	//要排除btnOverTime很小的情况，这可能导致无法接收到点击事件
	pos = SDL_GetTicks() - timesnap;
	if (btnOverTime > 0) {
		if(btnOverTime - pos > 5) btnOverTime -= pos;
		if (btnOverTime <= 0) btnOverTime = 1;
	}

	ONSVariableRef *v1 = reader->GetParamRef(0);
	//凡是有时间要求的事件，第一个参数都是超时时间
	LOEventHook *e = LOEventHook::CreateBtnwaitHook(btnOverTime,v1->GetTypeRefid(), reader->GetPrintName(),  btnUseSeOver ? 1 : -1 , reader->GetCmdChar());
	LOShareEventHook ev(e);
	
	G_hookQue.push_N_back(ev);
	reader->waitEventQue.push_N_back(ev);
	//是否需要播放事件
	if (btnUseSeOver) audioModule->waitEventQue.push_N_back(ev);

	return RET_CONTINUE;
}


int LOImageModule::spbtnCommand(FunctionInterface *reader) {
	int fullid = GetFullID(LOLayer::LAYER_SPRINT, reader->GetParamInt(0), 255, 255);
	LOLayerData *data = CreateLayerBakData(fullid, reader->GetPrintName());
	if (data) {
		//exbtn
		if(reader->GetParamCount() > 2) {
			LOString s = reader->GetParamStr(2);
			data->bak.SetBtndef( &s, reader->GetParamInt(1), true, false);
		}
		else data->bak.SetBtndef(nullptr, reader->GetParamInt(1), true, false);
        //设置后按钮会变为显示
        data->bak.SetVisable(1) ;
	}
	return RET_CONTINUE;
}


int LOImageModule::exbtn_dCommand(FunctionInterface *reader) {
	//int fullid = GetFullID(LOLayer::LAYER_BG, LOLayer::IDEX_BG_BTNEND, 255, 255);
	//LOLayerData *data = CreateNewLayerData(fullid, reader->GetPrintName());
	//LOString s = reader->GetParamStr(0);  //it's safe
	//data->bak.SetBtndef(&s, 0, true, true);
	exbtn_dStr = reader->GetParamStr(0);
	return RET_CONTINUE;
}


int LOImageModule::texecCommand(FunctionInterface *reader) {
//texthide/textshow 只对文字进行操作，不影响对话窗口
//texton/textoff 对整个对话窗口进行操作
//textclear 清除文字，不影响对话框
//texec '\'作为结尾符时立即清除文字，"@"作为结尾符时不清除文字，不管哪个符号都保持文字显示状态
//对话框受erasetextwindow影响，当erasetextwindow有效时，leveatextmode对话框消失

	char lineEnd = sayState.pageEnd & 0xff;
	//换页清除
	if (lineEnd == '\\') {
		ClearDialogText(lineEnd);
		DialogWindowSet(0, -1, -1);
		sayState.unSetFlags(LOSayState::FLAGS_TEXT_TEXEC);
	}
	else if (lineEnd == '@') {
		//换页不清除，但是句子的结束是 \n，那么下一行还是要换页的
		if (sayState.pageEnd & 0xff00 == 0x1000) sayState.setFlags(LOSayState::FLAGS_TEXT_TEXEC);
		else sayState.unSetFlags(LOSayState::FLAGS_TEXT_TEXEC);
	}
	else sayState.unSetFlags(LOSayState::FLAGS_TEXT_TEXEC);

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

		//dialogDisplayMode = DISPLAY_MODE_TEXT;
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
		//dialogDisplayMode = DISPLAY_MODE_NORMAL; 
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
	//文字显示的起点
	sayWindow.textX = reader->GetParamInt(0);
	sayWindow.textY = reader->GetParamInt(1);
	sayStyle.xcount = reader->GetParamInt(2);
	sayStyle.ycount = reader->GetParamInt(3);
	sayStyle.xsize = reader->GetParamInt(4);
	sayStyle.ysize = reader->GetParamInt(5);
	sayStyle.xspace = reader->GetParamInt(6);
	sayStyle.yspace = reader->GetParamInt(7);
	G_textspeed = reader->GetParamInt(8);
	//ons no bold?
	//winFont.isbold = reader->GetParamInt(9);
	//阴影
	if (reader->GetParamInt(10)) {
        sayStyle.SetFlags(LOTextStyle::STYLE_SHADOW);
		sayStyle.xshadow = sayStyle.xsize / 30 + 1;
		sayStyle.yshadow = sayStyle.ysize / 30 + 1;
		//lsp的阴影效果跟对话框的一样
		spStyle.xshadow = sayStyle.xshadow;
		spStyle.yshadow = sayStyle.yshadow;
	}
	sayWindow.winstr = reader->GetParamStr(11);
	sayState.setFlags(LOSayState::FLAGS_WINDOW_CHANGE);
	if(!reader->isName("setwindow3"))sayState.say.clear();
	sayWindow.x = reader->GetParamInt(12);
	sayWindow.y = reader->GetParamInt(13);

	if (sayWindow.winstr.length() > 0 && sayWindow.winstr.at(0) == '#') {
		sayWindow.w = reader->GetParamInt(14) - sayWindow.x + 1;
		sayWindow.h = reader->GetParamInt(15) - sayWindow.y + 1;
		if (sayWindow.w < 1) {
			SDL_LogError(0, "[setwindow] command color mode w < 1, will reset 1");
			sayWindow.w = 1;
		}
		if (sayWindow.h < 1) {
			SDL_LogError(0, "[setwindow] command color mode h < 1, will reset 1");
			sayWindow.h = 1;
		}
		//
		sayWindow.winstr = StringFormat(64, ">%d,%d,%s", sayWindow.w, sayWindow.h, sayWindow.winstr.c_str());
	}

	if (reader->isName("setwindow")) ClearDialogText('\\'); //setwindow will clear text

	return RET_CONTINUE;
}

int LOImageModule::setwindow2Command(FunctionInterface *reader) {
	sayWindow.winstr = reader->GetParamStr(0);
	if (sayWindow.winstr.length() > 0 && sayWindow.winstr.at(0) == '#') {
		if (sayWindow.w < 1) sayWindow.w = 1;
		if (sayWindow.h < 1) sayWindow.h = 1;  //safe value
		sayWindow.winstr = StringFormat(64, ">%d,%d,%s", sayWindow.w, sayWindow.h, sayWindow.winstr.c_str());
	}
	return RET_CONTINUE;
}


int LOImageModule::clickCommand(FunctionInterface *reader) {
	//click只等待左键，lrclick左键和右键都可以
	LOShareEventHook ev(LOEventHook::CreateClickHook(true, reader->isName("lrclick")));
	G_hookQue.push_N_back(ev);
	reader->waitEventQue.push_N_back(ev);
	return RET_CONTINUE;
}

int LOImageModule::erasetextwindowCommand(FunctionInterface *reader) {
	if (reader->GetParamInt(0)) sayState.setFlags(LOSayState::FLAGS_PRINT_HIDE);
	else  sayState.unSetFlags(LOSayState::FLAGS_PRINT_HIDE);
	return RET_CONTINUE;
}

int LOImageModule::getmouseposCommand(FunctionInterface *reader) {
	ONSVariableRef *v1 = reader->GetParamRef(0);
	ONSVariableRef *v2 = reader->GetParamRef(1);
	v1->SetValue((double)mouseXY[0]);
    v2->SetValue((double)mouseXY[1]);
	return RET_CONTINUE;
}


int LOImageModule::btndefCommand(FunctionInterface *reader) {
	//btndef会清除上一次的btndef定义和btn定义
	LOString tag = reader->GetParamStr(0);

	//所有的按钮定义都会被清除
	//无论如何btn的系统层都将被清除
	ClearBtndef();
	//All button related settings are cleared
	//btnwait完成后只是清除了按钮的定义，btntime和btntime2的设定并没有清除，除非显示的使用btndef
	btndefStr.clear();
	exbtn_dStr.clear();
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
	//btntime，只是单纯的等待指定时间
	//btntime2，如果已经超时，还要检查0通道是否还在播放，还在播放会等待0通道播放完成
	//btime 1000 相当于 btntime 1000， btime 1000,1 相当于btntime2 1000
	btnOverTime = reader->GetParamInt(0);
	btnUseSeOver = reader->isName("btntime2");
	if (reader->GetParamCount() > 1 && reader->GetParamInt(1) == 1) btnUseSeOver = true;
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
    v->SetValue(&sayState.say);
	return RET_CONTINUE;
}



//检查指定区域是否全部为某种颜色，允许少量容差
int LOImageModule::chkcolorCommand(FunctionInterface *reader) {
	int x, y, w, h, dw, dh;
	x = reader->GetParamInt(0);
	y = reader->GetParamInt(1);
	w = reader->GetParamInt(2);
	h = reader->GetParamInt(3);
	ONSVariableRef *v = reader->GetParamRef(4);
	LOShareTexture tex(ScreenShot(x, y, w, h, w, h));
	if (tex) {
		const char *buf = v->GetStr()->c_str();
		if (buf[0] == '#')buf++;
		int hex = v->GetStr()->GetHexInt(buf);
		SDL_Color color;
		color.r = (hex >> 16) & 0xff;
		color.g = (hex >> 8) & 0xff;
		color.b = hex & 0xff;
		if (!tex->CheckColor(&color, 1)) {
			LOString tmp = StringFormat(32, "#%02X%02X%02X", color.r, color.g, color.b);
			v->SetValue(&tmp);
		}
	}
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

	screenTex.reset(ScreenShot(src.x, src.y, src.w, src.h, dst.w, dst.h));
	return RET_CONTINUE;
}


int LOImageModule::savescreenshotCommand(FunctionInterface *reader) {
	if (screenTex && screenTex->isAvailable()) {
		LOString s = reader->GetParamStr(0);
		LOIO::GetPathForWrite(s);
		SDL_Surface *su = screenTex->getSurface();
		if (su) {
			SDL_SaveBMP(screenTex->getSurface(), s.c_str());
		}
		if (reader->isName("savescreenshot")) {
			screenTex.reset();
		}

	}
	return RET_CONTINUE;
}


int LOImageModule::rubyCommand(FunctionInterface *reader) {
    if(reader->isName("rubyoff")){
        sayStyle.UnSetFlags(LOTextStyle::STYLE_RUBYON);
    }
    else{
        sayStyle.xruby = sayStyle.xsize / 2;
        sayStyle.yruby = sayStyle.ysize / 2 ;
        if (reader->GetParamCount() >= 1) sayStyle.xruby = reader->GetParamInt(0);
        if (reader->GetParamCount() >= 2) sayStyle.yruby = reader->GetParamInt(1);
        if (reader->isName("rubyon")) sayStyle.SetFlags(LOTextStyle::STYLE_RUBYON);
        else if (reader->isName("rubyon2")) sayStyle.SetFlags(LOTextStyle::STYLE_RUBYLINE);
    }
	return RET_CONTINUE;
}

int LOImageModule::getcursorposCommand(FunctionInterface *reader) {
	int xx, yy, hh;
	xx = yy = hh = 0;
    LOLayer *lyr = LOLayer::GetLayer(GetFullID(LOLayer::LAYER_DIALOG, LOLayer::IDEX_DIALOG_TEXT,255,255));
    if(lyr && lyr->data && lyr->data->cur.texture){
        lyr->data->cur.texture->GetTextShowEnd(&xx, &yy, &hh) ;
        yy -= hh ;
        if(reader->isName("getcursorpos2")) xx -= sayStyle.xsize ;
        xx += sayWindow.textX ;
        yy += sayWindow.textY;
    }
    ONSVariableRef *v1 = reader->GetParamRef(0);
    ONSVariableRef *v2 = reader->GetParamRef(1);
    v1->SetValue((double)xx);
    v2->SetValue((double)yy);
	return RET_CONTINUE;
}


int LOImageModule::ispageCommand(FunctionInterface *reader) {
	ONSVariableRef *v = reader->GetParamRef(0);
	if (sayState.pageEnd == '\\') v->SetValue(1.0);
	else v->SetValue(0.0);
	return RET_CONTINUE;
}


int LOImageModule::btnCommand(FunctionInterface *reader) {
    //没有btndef字符串，则创建一个空纹理的按钮，没有显示，但是可以响应事件
    //if (btndefStr.length() == 0) return RET_CONTINUE;
	
	LOLayerData *info = CreateBtnData(reader->GetPrintName());
	if(!info) return RET_CONTINUE;
	BtndefCount++;
	//首个按钮需要一个父对象
	if (BtndefCount <= 1) {
		LOLayerData *father = CreateNewLayerData(GetFullID(LOLayer::LAYER_NSSYS, LOLayer::IDEX_NSSYS_BTN, 255, 255), reader->GetPrintName());
		LOString tmp("**;_?_empty_?_");
		loadSpCore(father, tmp, 0, 0, -1, true);
	}

	int val = reader->GetParamInt(0);
	int xx = reader->GetParamInt(1);
	int yy = reader->GetParamInt(2);

	//btn的按钮默认是不可见的
    if(btndefStr.length() != 0) loadSpCore(info, btndefStr, xx, yy, -1, false);
    else{ //使用空纹理
        LOString tmp("**;_?_empty_?_");
        loadSpCore(info, tmp, xx, yy, -1, false);
    }
	info->bak.SetBtndef(nullptr, val, true, false);
	info->bak.SetShowRect(reader->GetParamInt(5), reader->GetParamInt(6), reader->GetParamInt(3), reader->GetParamInt(4));
	return RET_CONTINUE;
}

int LOImageModule::spstrCommand(FunctionInterface *reader) {
	LOString s = reader->GetParamStr(0);
	LOShareEventHook ev(LOEventHook::CreateSpStrEvent(0, &s));
	imgeModule->waitEventQue.push_N_back(ev);
	ev->waitEvent(0.5, -1);
	return RET_CONTINUE;
}


int LOImageModule::filelogCommand(FunctionInterface *reader) {
	st_filelog = true;
	ReadLog(LOGSET_FILELOG);
	return RET_CONTINUE;
}


//将预先由脚本线程执行好参数的获取
int LOImageModule::movieCommand(FunctionInterface *reader) {
    if (reader->GetParamCount() == 1) {
        //movie stop，直接删除图层
        LOLayerData *data = CreateLayerBakData(GetFullID(LOLayer::LAYER_NSSYS, LOLayer::IDEX_NSSYS_MOVIE, 255, 255), "_lons") ;
        if(data){
            data->bak.SetDelete() ;
            ExportQuequ("_lons", nullptr, true);
        }
        return RET_CONTINUE;
	}

    int vflag = reader->GetParamInt(reader->GetParamCount() - 2) ;
    LOString fn = reader->GetParamStr(reader->GetParamCount() - 1);
    SDL_Rect dst = {0, 0, G_gameWidth, G_gameHeight} ;
	if (reader->GetParamCount() > 2) { //有pos参数
        dst.x = reader->GetParamInt(0); dst.y = reader->GetParamInt(1);
        dst.w = reader->GetParamInt(2); dst.h = reader->GetParamInt(3);
	}

    //普通的视频播放允许调用外部播放器
    if(dst.x == 0 && dst.y == 0 && dst.w == G_gameWidth && dst.h == G_gameHeight && !(vflag & MOVIE_CMD_LOOP) &&
            !(vflag & MOVIE_CMD_ASYNC)) vflag |= MOVIE_CMD_ALLOW_OUTSIDE;

    NormalPlayVideo(reader, fn, dst, vflag) ;
	return RET_CONTINUE;
}


int LOImageModule::aviCommand(FunctionInterface *reader) {
	LOString fn = reader->GetParamStr(0);
    int vflag = MOVIE_CMD_ALLOW_OUTSIDE;
    if(reader->GetParamCount() > 1 && reader->GetParamInt(1) > 0) vflag |= MOVIE_CMD_CLICK;
    //默认的情况都是铺满屏幕
    SDL_Rect dst = {0, 0, G_gameWidth, G_gameHeight};
    return  NormalPlayVideo(reader, fn, dst, vflag) ;
	return RET_CONTINUE;
}


int LOImageModule::NormalPlayVideo(FunctionInterface *reader, LOString &fn, SDL_Rect dst, int vflag) {
	LOString tmp = StringFormat(512, "*v/%d,%d,", dst.w, dst.h);
	//确定是哪种格式
	LOString suffix = fn.GetRightOfChar('.').toLower();
	if (suffix.length() > 0) tmp += suffix;
	else tmp += "mpg";
	tmp += ";" + fn;

	//准备纹理
	LOLayerData *info = CreateNewLayerData(GetFullID(LOLayer::LAYER_NSSYS, LOLayer::IDEX_NSSYS_MOVIE, 255, 255), "_lons");
	//必须检查是否已经有正在播放的对象
	if (info->cur.texture) {
		FatalError("A video is playing!");
		return RET_CONTINUE;
	}

	loadSpCore(info, tmp, dst.x, dst.y, -1, true);
	//要是否成功载入
	if (info->bak.texture) {
		//需要对动作做一些设置
		LOAction *ac = info->bak.GetAction(LOAction::ANIM_VIDEO);
		//if (vflag & MOVIE_CMD_CLICK) ac->setFlags(LOAction::FLAGS_CLICK_DEL);
		if (vflag & MOVIE_CMD_LOOP)  ac->loopMode = LOAction::LOOP_CIRCULAR;

		//添加播放事件
		LOShareEventHook ev(LOEventHook::CreateVideoPlayHook(info->fullid, vflag & MOVIE_CMD_CLICK));
		G_hookQue.push_N_back(ev);
		//非异步模式都阻塞脚本线程
		if (!(vflag & MOVIE_CMD_ASYNC)) {
			reader->waitEventQue.push_N_back(ev);

			ExportQuequ("_lons", nullptr, true);
		}
		return RET_CONTINUE;
	}
	else if (vflag & MOVIE_CMD_ALLOW_OUTSIDE) {
		//使用外部播放器
		UseOutSidePlayer(fn);
	}
	return RET_CONTINUE;
}


//调用外部播放器，注意这是一个阻塞的过程，不支持读取包里的文件
void LOImageModule::UseOutSidePlayer(LOString &s) {
	bool isok = false;
	//安卓
#ifdef ANDROID
	if (!isok) {

	}
#endif // ANDROID

	//windows
#if defined(__WINDOWS__) || defined(_WIN32) || defined(WIN32) || defined(_WIN64) || \
    defined(WIN64) || defined(__WIN32__) || defined(__TOS_WIN__)
	if (!isok) {
		system(s.c_str());
		isok = true;
	}
#endif

	//linux
#if defined(__linux__) || defined(linux) || defined(__linux) || defined(__LINUX__) || \
    defined(LINUX) || defined(_LINUX)
	if (!isok) {
		LOString cmd = G_playcmd;
		cmd.append(" ");
		cmd.append(LOIO::ioReadDir);
		if (LOIO::ioReadDir.length() > 0)cmd.append("/");
        cmd.append(s);
		//执行
		system(cmd.c_str());
		isok = true;
	}
#endif
}



int LOImageModule::speventCommand(FunctionInterface *reader) {
	LOLayerData *data = CreateLayerBakData(GetFullID(LOLayer::LAYER_SPRINT, reader->GetParamInt(0), 255, 255), reader->GetPrintName());
	if (data) {
		int eint = reader->GetParamInt(1);  //1-表示click左键/点击，2表示press长按
		if (eint & 1) { //含有左键事件

		}
	}


	return RET_CONTINUE;
}



int LOImageModule::quakeCommand(FunctionInterface *reader) {
	LOEffect ef;
	ef.nseffID = LOEffect::EFFECT_ID_QUAKE;
	ef.time = reader->GetParamInt(1);
	ef.id = 0;
	int val = abs(reader->GetParamInt(0)) & 0xffff;
	if (ef.time < val * 4) ef.time = val * 4;
	//借用一下ef的ID编号
	if (reader->isName("quakex")) ef.id |= val;
	else if (reader->isName("quakey")) ef.id |= val << 16;
	else {
		ef.id |= val;
		ef.id |= val << 16;
	}
	//摇晃命令必须等到执行完成才会执行下一个命令
	ExportQuequ("_lons", &ef, true, true);

	return RET_CONTINUE;
}


int LOImageModule::ldCommand(FunctionInterface *reader) {
	LOString spos = reader->GetParamStr(0).toLower();
	LOString tag = reader->GetParamStr(1);
	int xx, id;
	if (spos == "r") { 
		id = standLD[0].index; xx = G_gameWidth * 3 / 4;
	}
	else if (spos == "c") {
		id = standLD[1].index; xx = G_gameWidth / 2;
	}
	else if(spos == "l"){
		id = standLD[2].index; xx = G_gameWidth / 4;
	}
	else {
		FatalError("[ld] command first parameter error!");
		return RET_ERROR;
	}

	reader->ExpandStr(tag);
	LeveTextDisplayMode();

	LOLayerData* info = CreateNewLayerData(GetFullID(LOLayer::LAYER_STAND, id, 255, 255), reader->GetPrintName());
	loadSpCore(info, tag, xx, 0, -1, true);
	//修正x,y的位置
	if (info->bak.texture) {
		info->bak.SetPosition(xx - info->bak.texture->baseW() / 2, G_gameHeight - info->bak.texture->baseH());
	}
	//是否需要直接print
	if (reader->GetParamCount() > 2) printStack(reader, 2);
	return RET_CONTINUE;
}


int LOImageModule::clCommand(FunctionInterface *reader) {
	LOString spos = reader->GetParamStr(0).toLower();
	int ids[] = { 0,0,0 };
	if (spos == "c") ids[0] = standLD[1].index;
	else if (spos == "l") ids[0] = standLD[2].index;
	else if (spos == "r") ids[0] = standLD[0].index;
	else if (spos == "a") {
		ids[0] = standLD[0].index; ids[1] = standLD[1].index; ids[2] = standLD[2].index;
	}
	else {
		FatalError("[cl] command first parameter error!");
		return RET_ERROR;
	}
	//
	for (int ii = 0; ii < 3; ii++) {
		CspCore(LOLayer::LAYER_STAND, ids[ii], ids[ii], reader->GetPrintName());
	}
	if (reader->GetParamCount() > 1) printStack(reader, 1);
	return RET_CONTINUE;
}


int LOImageModule::humanorderCommand(FunctionInterface *reader) {
	LOString tag = reader->GetParamStr(0);
	//tag乱写会导致编号有问题
	for (int ii = 0; ii < tag.length(); ii++) {
		if (tag[ii] == 'r') standLD[0].index = ii + LOLayer::IDEX_LD_BASE;
		else if(tag[ii] == 'c')  standLD[1].index = ii + LOLayer::IDEX_LD_BASE;
		else if (tag[ii] == 'l')  standLD[2].index = ii + LOLayer::IDEX_LD_BASE;
	}
	return RET_CONTINUE;
}

int LOImageModule::captionCommand(FunctionInterface *reader) {
	LOString s = reader->GetParamStr(0);
	std::string utf8s = s.GetEncoder()->GetUtf8String(&s);
	titleStr = utf8s + "  [ONScripter-LONS " + LOString(ONS_VERSION) + "]";
	SDL_SetWindowTitle(window, titleStr.c_str());
	return RET_CONTINUE;
}


int LOImageModule::textspeedCommand(FunctionInterface *reader) {
	G_textspeed = reader->GetParamInt(0);
	return RET_CONTINUE;
}

int LOImageModule::actionCommand(FunctionInterface *reader){
    int fid = GetFullID(LOLayer::LAYER_SPRINT, reader->GetParamInt(0), 255, 255 );
    //先确定sp是否已经载入，未载入action无效
    LOLayerData *data = CreateLayerBakData(fid, reader->GetPrintName());
    if(!data) return RET_CONTINUE;

    LOString key = reader->GetParamStr(1).toLower();
    int fix = 0 ;
    LOAction *sac = nullptr;
    if(key == "move"){
        if(reader->GetParamCount() < 7){
            FatalError("[action] command [move] action param error!") ;
            return  RET_ERROR ;
        }
        LOActionMove *ac = new LOActionMove();
        ac->startPt.x = reader->GetParamInt(2); ac->startPt.y = reader->GetParamInt(3);
        ac->endPt.x = reader->GetParamInt(4); ac->endPt.y = reader->GetParamInt(5);
        ac->duration = reader->GetParamInt(6) ;
        fix = 7 ;
        sac = ac ;
    }
    else if(key == "fade"){
    }
    else if(key == "scale"){
        if(reader->GetParamCount() < 7){
            FatalError("[action] command [scale] action param error!") ;
            return  RET_ERROR ;
        }
        LOActionScale *ac = new LOActionScale();
        ac->startSc.x = reader->GetParamInt(2); ac->startSc.y = reader->GetParamInt(3);
        ac->endSc.x = reader->GetParamInt(4); ac->endSc.y = reader->GetParamInt(5);
        ac->duration = reader->GetParamInt(6) ;
        fix = 7 ;
        sac = ac ;
    }
    else if(key == "rotate"){

    }
    else{
        FatalError("[action] command unknow action [%s] param error!", key.c_str()) ;
        return  RET_ERROR ;
    }


    //有效
    if(sac){
        if(reader->GetParamCount() > fix) sac->gVal = reader->GetParamInt(fix) ;
        data->bak.SetAction(sac) ;
    }


    return RET_CONTINUE;
}


int LOImageModule::actionloopCommand(FunctionInterface *reader){
    int fid = GetFullID(LOLayer::LAYER_SPRINT, reader->GetParamInt(0), 255, 255 );
    //先确定sp是否已经载入，未载入action无效
    LOLayerData *data = CreateLayerBakData(fid, reader->GetPrintName());
    if(!data) return RET_CONTINUE;

    LOString key = reader->GetParamStr(1).toLower();
    int actype = LOAction::ANIM_NONE;
    if(key == "move") actype = LOAction::ANIM_MOVE;
    else if(key == "fade");
    else if(key == "scale") actype = LOAction::ANIM_SCALE;
    else if(key == "rotate");

    LOAction *ac = data->bak.GetAction(actype);
    if(ac) ac->loopMode = (LOAction::AnimaLoop)reader->GetParamInt(2);


    return RET_CONTINUE;
}


int LOImageModule::monocroCommand(FunctionInterface *reader) {
	LOString s = reader->GetParamStr(0);
	if (s.length() > 0) {
		int color = 0;
		int fid = GetFullID(LOLayer::LAYER_BG, LOLayer::IDEX_FLAGS_SYNC, 255, 255);
		if (s[0] == '#') color = reader->GetParamColor(0);
		//模拟图层加载，这样状态技能跟随保存，又能符合print刷新的要求
		//LONS随时可以介入画面模式改变，为了与ONS保持一致采用这种方法
		LOLayerData *data = CreateLayerBakData(fid, reader->GetPrintName());
		if (!data) data = CreateNewLayerData(fid, reader->GetPrintName());
		data->bak.SetMonoInfo(color);
	}
	return RET_CONTINUE;
}


int LOImageModule::negaCommand(FunctionInterface *reader) {
	int fid = GetFullID(LOLayer::LAYER_BG, LOLayer::IDEX_FLAGS_SYNC, 255, 255);
	LOLayerData *data = CreateLayerBakData(fid, reader->GetPrintName());
	if (!data) data = CreateNewLayerData(fid, reader->GetPrintName());
	data->bak.SetNegaInfo(reader->GetParamInt(0));
	return RET_CONTINUE;
}


