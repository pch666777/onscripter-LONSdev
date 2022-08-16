/*
//存放图层的基本信息
*/

#include "LOLayerData.h"

char G_Bit[4] = { 0, 1, 2, 3 };  //默认材质所只用的RGBA模式位置 0-A 1-R 2-G 3-B
Uint32 G_Texture_format = SDL_PIXELFORMAT_ARGB8888;   //windows android use this format;   //默认编辑使用的材质格式
SDL_Rect G_windowRect = { 0, 0, 800, 600 };    //整个窗口的位置和大小
SDL_Rect G_viewRect = { 0, 0, 800, 600 };     //实际显示视口的位置和大小
int G_gameWidth = 800;         //游戏的实际宽度
int G_gameHeight = 600;         //游戏的实际高度
float G_gameScaleX = 1.0f;     //游戏X方向的缩放
float G_gameScaleY = 1.0f;     //游戏Y方向的缩放，鼠标事件时需要使用
int G_gameRatioW = 1;       //游戏比例，默认为1:1
int G_gameRatioH = 1;       //游戏比例，默认为1:1
int G_fullScreen = 0;       //全屏
int G_destWidth = -1;   //目标窗口宽度
int G_destHeight = -1;  //目标窗口高度
int G_fpsnum = 60;      //默认的fps速度
int G_textspeed = 20;   //默认的文字速度

//============ static and globle ================

//游戏画面是否被缩放
bool IsGameScale() {
	if (abs(G_gameWidth - G_viewRect.w) > 1 || abs(G_gameHeight - G_viewRect.h) > 1) return true;
	else return false;
}

int  GetFullID(int t, int *ids) {
	int a = ((uint32_t)ids[0]) & 0x3ff;
	int b = ((uint32_t)ids[1]) & 0xff;
	int c = ((uint32_t)ids[2]) & 0xff;
	return (t << 26) | (a << 16) | (b << 8) | c;
}

int  GetFullID(int t, int father, int child, int grandson) {
	int ids[] = { father , child, grandson };
	return GetFullID(t, ids);
}

int GetIDs(int fullid, int pos) {
	if (pos == IDS_LAYER_NORMAL) return (fullid >> 16) & 0x3FF;
	if (pos == IDS_LAYER_CHILD) return (fullid >> 8) & 0xFF;
	if (pos == IDS_LAYER_GRANDSON) return fullid & 0xFF;
	if (pos == IDS_LAYER_TYPE) return (fullid >> 26) & 0x3F;  //type
	return -1;
}

void GetTypeAndIds(int *type, int *ids, int fullid) {
	if (type) *type = (fullid >> 26) & 0x3F;
	if (ids) {
		ids[0] = (fullid >> 16) & 0x3FF;
		ids[1] = (fullid >> 8) & 0xFF;
		ids[2] = fullid & 0xFF;
	}
}


//=================================
LOLayerDataBase::LOLayerDataBase() {
	resetBase();
}

void LOLayerDataBase::resetBase() {
	flags = upflags = upaction = 0;
	offsetX = offsetY = centerX = centerY = 0;
	alpha = -1;
	showSrcX = showSrcY = showWidth = showHeight = 0;
	cellNum = 0;
	scaleX = scaleY = 1.0;
	rotate = 0.0;
	showType = texType = 0;
	keyStr.reset();
	btnStr.reset();
	buildStr.reset();
	texture.reset();
	actions.reset();
}

LOLayerDataBase::~LOLayerDataBase() {

}


LOAction* LOLayerDataBase::GetAction(int t) {
	if (actions) {
		for (int ii = 0; ii < actions->size(); ii++) {
			if (actions->at(ii)->acType == t) return actions->at(ii).get();
		}
	}
	return nullptr;
}

void LOLayerDataBase::SetAction(LOAction *ac) {
	LOShareAction sac(ac);
	SetAction(sac);
}

//同类型的动画只存在一个
void LOLayerDataBase::SetAction(LOShareAction &ac) {
	if (!actions) actions.reset(new std::vector<LOShareAction>());
	auto iter = actions->begin();
	while (iter != actions->end()) {
		if ((*iter)->acType == ac->acType) iter = actions->erase(iter);
		else iter++;
	}
	actions->push_back(ac);
	upaction |= ac->acType;
}

int LOLayerDataBase::GetCellCount() {
	LOAction *ac = GetAction(LOAction::ANIM_NSANIM);
	if (ac) {
		LOActionNS *nac = (LOActionNS*)ac;
		return nac->cellCount;
	}
	return 1;
}

void LOLayerDataBase::SetVisable(int v) {
	if (v) flags |= FLAGS_VISIABLE;
	else flags &= (~FLAGS_VISIABLE);
	upflags |= UP_VISIABLE;
}

void LOLayerDataBase::SetShowRect(int x, int y, int w, int h) {
	showWidth = w;
	showHeight = h;
	showSrcX = x;
	showSrcY = y;
	showType |= SHOW_RECT;
	upflags |= (UP_SHOWW | UP_SHOWH | UP_SRCX | UP_SRCY | UP_SHOWTYPE);
}

void LOLayerDataBase::SetShowType(int show) {
	showType |= show;
	upflags |= UP_SHOWTYPE;
}

void LOLayerDataBase::SetPosition(int ofx, int ofy) {
	offsetX = ofx;
	offsetY = ofy;
	upflags |= UP_OFFX;
	upflags |= UP_OFFY;
}

void LOLayerDataBase::SetPosition2(int cx, int cy, double sx, double sy) {
	centerX = cx; centerY = cy;
	scaleX = sx; scaleY = sy;
	showType |= SHOW_SCALE;
	upflags |= (UP_CENX | UP_CENY | UP_SCALEX | UP_SCALEY | UP_SHOWTYPE);
}

void LOLayerDataBase::SetRotate(double ro) {
	rotate = ro;
	//位置为正方向
	//if (abs(rotate) < 0.0001) showType &= (~SHOW_ROTATE);
	//else 
	showType |= SHOW_ROTATE;
	upflags |= UP_ROTATE;
}

void LOLayerDataBase::SetAlpha(int alp) {
	alpha = alp & 0xff;
	upflags |= UP_ALPHA;
}

void LOLayerDataBase::SetAlphaMode(int mode) {
	alphaMode = mode & 0xff;
	upflags |= UP_ALPHAMODE;
}

bool LOLayerDataBase::SetCell(LOActionNS *ac, int ce) {
	upflags |= UP_CELLNUM;
	//如果有active，那么应该设置action的值
	if (!ac) ac = (LOActionNS*)GetAction(LOAction::ANIM_NSANIM);
	if (ac && texture && ce < ac->cellCount) {
		showType |= SHOW_RECT;
		int perx = texture->baseW() / ac->cellCount;
		ac->cellCurrent = ce;
		showSrcX = ac->cellCurrent * perx;
		showSrcY = 0;
		showWidth = perx;
		showHeight = texture->baseH();
		upflags |= (UP_SHOWW | UP_SHOWH | UP_SRCX | UP_SRCY | UP_SHOWTYPE);
		return true;
	}
	else {
		cellNum = 0;
		return false;
	}
}

//只设置数，不检查有效性
void LOLayerDataBase::SetCellNum(int ce) {
	cellNum = ce;
	upflags |= UP_CELLNUM;
}


void LOLayerDataBase::SetTextureType(int dt) {
	texType = dt & 0xff;
	switch (texType) {
		//使用缓存
	//case LOtexture::TEX_COLOR_AREA:
	//case LOtexture::TEX_EMPTY:
	case LOtexture::TEX_IMG:
		flags |= FLAGS_USECACHE;
		break;
	default:
		//不使用缓存
		flags &= (~FLAGS_USECACHE);
		break;
	}
}

void LOLayerDataBase::SetNewFile(LOShareTexture &tex) {
	texture = tex;
	flags |= FLAGS_NEWFILE;
	upflags |= UP_NEWFILE;
	upaction |= UP_ACTION_ALL;
	//清理删除标记
	flags &= (~FLAGS_DELETE);
}

void LOLayerDataBase::SetDelete() {
	resetBase();
	flags |= FLAGS_DELETE;
}

void LOLayerDataBase::SetBtndef(LOString *s, int val, bool isleft, bool isright) {
	if (s) btnStr.reset(new LOString(*s));
	btnval = val;
	flags |= FLAGS_BTNDEF;
	flags |= FLAGS_MOUSEMOVE;
	if (isleft) flags |= FLAGS_LEFTCLICK;
	if (isright) flags |= FLAGS_RIGHTCLICK;
	upflags |= UP_BTNSTR;
	upflags |= UP_BTNVAL;
}

void LOLayerDataBase::unSetBtndef() {
	flags &= (~(FLAGS_BTNDEF | FLAGS_LEFTCLICK | FLAGS_RIGHTCLICK | FLAGS_MOUSEMOVE));
}

void LOLayerDataBase::FirstSNC() {
	if (!actions) return;
	for (int ii = 0; ii < actions->size(); ii++) {
		LOShareAction ac = actions->at(ii);
		if (ac->acType == LOAction::ANIM_NSANIM) {
			if (texture) {
				LOActionNS *ai = (LOActionNS*)(ac.get());
				showType |= SHOW_RECT;
				cellNum = ai->cellCurrent;
				showWidth = texture->baseW() / ai->cellCount;
				showHeight = texture->baseH();
				upflags |= (UP_SHOWW | UP_SHOWH | UP_SRCX | UP_SRCY | UP_SHOWTYPE);
			}
		}
	}
}

void LOLayerDataBase::GetSimpleSrc(SDL_Rect *src) {
	src->x = showSrcX; src->y = showSrcY;
	src->w = showWidth; src->h = showHeight;
}

void LOLayerDataBase::GetSimpleDst(SDL_Rect *dst) {
	dst->x = offsetX; dst->y = offsetY;
	dst->w = texture->baseW(); dst->h = texture->baseH();
}

void LOLayerDataBase::Serialize(BinArray *bin) {
	//长度记录在标记之后 base,len,version
	int len = bin->WriteLpksEntity("base", 0, 1);
	//优先存储重生成字符串
	bin->WriteLOString(buildStr.get());

	bin->WriteInt3(flags, upflags, upaction);
	bin->WriteInt(btnval);

	float fvals[] = { offsetX, offsetY, showWidth , showHeight };
	bin->Append((char*)fvals, 4 * 4);
	int16_t vals[] = {alpha ,centerX ,centerY ,showSrcX ,showSrcY};
	bin->Append((char*)vals, 5 * 2);
	uint8_t bytes[] = { cellNum ,alphaMode ,texType ,showType };
	bin->Append((char*)bytes, 4);

	bin->WriteDouble(scaleX);
	bin->WriteDouble(scaleY);
	bin->WriteDouble(rotate);
	bin->WriteLOString(btnStr.get());
	
	//文字纹理需要存储样式
	if (texture && texture->isTextTexture()) {
		texture->textData->style.Serialize(bin);
	}
	bin->WriteInt(0);

	//action
	if (actions) {
		bin->WriteInt(actions->size());
		for (int ii = 0; ii < actions->size(); ii++) actions->at(ii)->Serialize(bin);
	}
	else bin->WriteInt(0);

	bin->WriteInt(bin->Length() - len, &len);
}


//=================================

LOLayerData::LOLayerData() {
	isinit = false;
}

LOLayerData::~LOLayerData() {
	//if (actions) for (int ii = 0; ii < actions->size(); ii++) delete actions->at(ii);
	//if (eventHooks) for (int ii = 0; ii < eventHooks->size(); ii++) delete eventHooks->at(ii);
}



bool LOLayerData::GetVisiable() {
	if (bak.flags & LOLayerDataBase::FLAGS_DELETE) return false;
	else if (bak.upflags & LOLayerDataBase::UP_VISIABLE) return bak.isVisiable();
	return cur.isVisiable();
}

int LOLayerData::GetOffsetX() {
	if (bak.flags & LOLayerDataBase::FLAGS_DELETE) return 0;
	if (bak.upflags & LOLayerDataBase::UP_OFFX) return bak.offsetX;
	return cur.offsetX;
}

int LOLayerData::GetOffsetY() {
	if (bak.flags & LOLayerDataBase::FLAGS_DELETE) return 0;
	if (bak.upflags & LOLayerDataBase::UP_OFFY) return bak.offsetY;
	return cur.offsetY;
}

int LOLayerData::GetCenterX() {
	if (bak.flags & LOLayerDataBase::FLAGS_DELETE) return 0;
	if (bak.upflags & LOLayerDataBase::UP_CENX) return bak.centerX;
	return cur.centerX;
}

int LOLayerData::GetCenterY() {
	if (bak.flags & LOLayerDataBase::FLAGS_DELETE) return 0;
	if (bak.upflags & LOLayerDataBase::UP_CENY) return bak.centerY;
	return cur.centerY;
}

int LOLayerData::GetAlpha() {
	if (bak.flags & LOLayerDataBase::FLAGS_DELETE) return 255;
	if (bak.upflags & LOLayerDataBase::UP_ALPHA) return bak.alpha;
	return cur.alpha;
}

int LOLayerData::GetShowWidth() {
	if (bak.flags & LOLayerDataBase::FLAGS_DELETE) return 0;
	if (bak.upflags & LOLayerDataBase::UP_SHOWW) return bak.showWidth;
	return cur.showWidth;
}

int LOLayerData::GetShowHeight() {
	if (bak.flags & LOLayerDataBase::FLAGS_DELETE) return 0;
	if (bak.upflags & LOLayerDataBase::UP_SHOWH) return bak.showHeight;
	return cur.showHeight;
}

int LOLayerData::GetCellCount() {
	if (bak.flags & LOLayerDataBase::FLAGS_DELETE) return 0;
	if (bak.upflags & LOLayerDataBase::UP_NEWFILE) return bak.GetCellCount();
	return cur.GetCellCount();
}

double LOLayerData::GetScaleX() {
	if (bak.flags & LOLayerDataBase::FLAGS_DELETE) return 0.0;
	if (bak.upflags & LOLayerDataBase::UP_SCALEX) return bak.scaleX;
	return cur.scaleX;
}

double LOLayerData::GetScaleY() {
	if (bak.flags & LOLayerDataBase::FLAGS_DELETE) return 0.0;
	if (bak.upflags & LOLayerDataBase::UP_SCALEY) return bak.scaleY;
	return cur.scaleY;
}


double LOLayerData::GetRotate() {
	if (bak.flags & LOLayerDataBase::FLAGS_DELETE) return 0.0;
	if (bak.upflags & LOLayerDataBase::UP_ROTATE) return bak.rotate;
	return cur.rotate;
}

LOLayerDataBase *LOLayerData::GetBase(bool isforce) {
	if (isforce)return &cur;
	return &bak;
}

//设置默认的显示范围，通常在loadsp后调用
void LOLayerData::SetDefaultShowSize(bool isforce) {
	LOLayerDataBase *data = GetBase(isforce);
	if (data->actions) {
		data->FirstSNC();
	}
	else {
		data->showWidth = data->texture->baseW();
		data->showHeight = data->texture->baseH();
		data->upflags |= LOLayerDataBase::UP_SHOWW;
		data->upflags |= LOLayerDataBase::UP_SHOWH;
	}
}



/*
void LOLayerData::lockMe() {
	int ta = 0;
	int tb = 1;
	//锁住
	while (!threadLock.compare_exchange_strong(ta, tb)) {
		cpuDelay();
	}
}


void LOLayerData::unLockMe() {
	int ta = 1;
	int tb = 0;
	//锁住
	while (!threadLock.compare_exchange_strong(ta, tb)) {
		cpuDelay();
	}
}


void LOLayerData::cpuDelay() {
	int sum = 0;
	for (int ii = 0; ii < 200; ii++) {
		sum += rand() % 3;
	}
}
*/


void LOLayerData::UpdataToForce() {
	//首先更新action，后面有用
	if (bak.upaction == LOLayerDataBase::UP_ACTION_ALL) cur.actions = std::move(bak.actions);
	else if (bak.upaction != 0) {
		//更新单个状态
	}
	
	if (bak.upflags & LOLayerDataBase::UP_BTNVAL) cur.btnval = bak.btnval;
	if (bak.upflags & LOLayerDataBase::UP_OFFX) cur.offsetX = bak.offsetX;
	if (bak.upflags & LOLayerDataBase::UP_OFFY) cur.offsetY = bak.offsetY;
	if (bak.upflags & LOLayerDataBase::UP_ALPHA) cur.alpha = bak.alpha;
	if (bak.upflags & LOLayerDataBase::UP_CENX) cur.centerX = bak.centerX;
	if (bak.upflags & LOLayerDataBase::UP_CENY) cur.centerY = bak.centerY;
	if (bak.upflags & LOLayerDataBase::UP_SRCX) cur.showSrcX = bak.showSrcX;
	if (bak.upflags & LOLayerDataBase::UP_SRCY) cur.showSrcY = bak.showSrcY;
	if (bak.upflags & LOLayerDataBase::UP_SHOWW) cur.showWidth = bak.showWidth;
	if (bak.upflags & LOLayerDataBase::UP_SHOWH) cur.showHeight = bak.showHeight;
	if (bak.upflags & LOLayerDataBase::UP_ALPHAMODE) cur.alphaMode = bak.alphaMode;
	if (bak.upflags & LOLayerDataBase::UP_SCALEX) cur.scaleX = bak.scaleX;
	if (bak.upflags & LOLayerDataBase::UP_SCALEY) cur.scaleY = bak.scaleY;
	if (bak.upflags & LOLayerDataBase::UP_ROTATE) cur.rotate = bak.rotate;
	if (bak.upflags & LOLayerDataBase::UP_BTNSTR) cur.btnStr = std::move(bak.btnStr);
	if (bak.upflags & LOLayerDataBase::UP_SHOWTYPE) cur.showType = bak.showType;
	if (bak.upflags == LOLayerDataBase::UP_NEWFILE) {
		cur.keyStr = std::move(bak.keyStr);
		cur.buildStr = std::move(bak.buildStr);
		cur.texture = bak.texture;
		cur.texType = bak.texType;
		cur.flags = bak.flags;
	}

	//最后更新动画格数，因为涉及一些操作
	if (bak.upflags & LOLayerDataBase::UP_CELLNUM) {
		cur.cellNum = bak.cellNum;
		cur.SetCell(nullptr, cur.cellNum);
	}

	if (bak.upflags & LOLayerDataBase::UP_VISIABLE) cur.SetVisable(bak.isVisiable());
	cur.flags |= LOLayerDataBase::FLAGS_ISFORCE;
	bak.resetBase();
}


void LOLayerData::Serialize(BinArray *bin) {
	//data, len, version
	int len = bin->WriteLpksEntity("data", 0, 1);
	//fullid
	bin->WriteInt(fullid);
	if (cur.isNothing()) bin->WriteChar(0);
	else {
		bin->WriteChar(1);
		cur.Serialize(bin);
	}
	if (bak.isNothing()) bin->WriteChar(0);
	else {
		bin->WriteChar(1);
		bak.Serialize(bin);
	}
	bin->WriteInt(bin->Length() - len, &len);
}