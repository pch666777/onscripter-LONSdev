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
	flags = upflags = 0;
	offsetX = offsetY = centerX = centerY = 0;
	alpha = -1;
	showSrcX = showSrcY = showWidth = showHeight = 0;
	cellNum = 0;
	scaleX = scaleY = rotate = 0.0;
	showType = texType = 0;
	keyStr.reset();
	btnStr.reset();
	buildStr.reset();
	texture.reset();
	actions.reset();
}

LOLayerDataBase::~LOLayerDataBase() {

}

void LOLayerDataBase::UpData(LOLayerDataBase *base, int f) {
	upflags = f;
	if (f & UP_BTNVAL) btnval = base->btnval;
	if (f & UP_OFFX) offsetX = base->offsetX;
	if (f & UP_OFFY) offsetY = base->offsetY;
	if (f & UP_ALPHA) alpha = base->alpha;
	if (f & UP_CENX) centerX = base->centerX;
	if (f & UP_CENY) centerY = base->centerY;
	if (f & UP_SRCX) showSrcX = base->showSrcX;
	if (f & UP_SRCY) showSrcY = base->showSrcY;
	if (f & UP_SHOWW) showWidth = base->showWidth;
	if (f & UP_SHOWH) showHeight = base->showHeight;
	if (f & UP_CELLNUM) cellNum = base->cellNum;
	if (f & UP_ALPHAMODE) alphaMode = base->alphaMode;
	if (f & UP_SCALEX) scaleX = base->scaleX;
	if (f & UP_SCALEY) scaleY = base->scaleY;
	if (f & UP_ROTATE) rotate = base->rotate;
	if (f & UP_BTNSTR) btnStr = std::move(base->btnStr);
	if (f & UP_SHOWTYPE) showType = base->showType;
	if (f & UP_ACTIONS) actions = std::move(base->actions);
	if (f & UP_NEWFILE) {
		keyStr = std::move(base->keyStr);
		buildStr = std::move(base->buildStr);
		texture = base->texture;
		texType = base->texType;
	}
}

//=================================

LOLayerData::LOLayerData() {
	isinit = false;
}

LOLayerData::~LOLayerData() {
	//if (actions) for (int ii = 0; ii < actions->size(); ii++) delete actions->at(ii);
	//if (eventHooks) for (int ii = 0; ii < eventHooks->size(); ii++) delete eventHooks->at(ii);
}

void LOLayerData::GetSimpleDst(SDL_Rect *dst) {
	//dst->x = cur.offsetX; dst->y = cur.offsetY;
	//dst->w = texture->baseW(); dst->h = texture->baseH();
}

void LOLayerData::GetSimpleSrc(SDL_Rect *src) {
	//src->x = showSrcX; src->y = showSrcY;
	//src->w = showWidth; src->h = showHeight;
}

int LOLayerData::GetCellCount() {
	//if (actions) {
	//	for (int ii = 0; ii < actions->size(); ii++) {
	//		LOShareAction ac = actions->at(ii);
	//		if (ac->acType == LOAction::ANIM_NSANIM) return ((LOActionNS*)(ac.get()))->cellCount;
	//	}
	//}
	////默认是1格的
	return 1;
}


LOLayerDataBase *LOLayerData::GetBase(bool isforce) {
	if (isforce)return &cur;
	return &bak;
}


void LOLayerData::SetVisable(int v, bool isforce) {
	LOLayerDataBase *data = GetBase(isforce);
	if (v) data->flags |= FLAGS_VISIABLE;
	else data->flags &= (~FLAGS_VISIABLE);
	data->upflags |= UP_VISIABLE;
}


void LOLayerData::SetShowRect(int x, int y, int w, int h, bool isforce) {
	LOLayerDataBase *data = GetBase(isforce);

	data->showWidth = w;
	data->showHeight = h;
	data->showSrcX = x;
	data->showSrcY = y;
	data->showType |= SHOW_RECT;
	data->upflags |= (UP_SHOWW | UP_SHOWH | UP_SRCX | UP_SRCY | UP_SHOWTYPE);
}

void LOLayerData::SetShowType(int show, bool isforce) {
	LOLayerDataBase *data = GetBase(isforce);

	data->showType |= show;
	data->upflags |= UP_SHOWTYPE;
}



void LOLayerData::SetPosition(int ofx, int ofy, bool isforce) {
	LOLayerDataBase *data = GetBase(isforce);

	data->offsetX = ofx;
	data->offsetY = ofy;
	data->upflags |= UP_OFFX;
	data->upflags |= UP_OFFY;
}

void LOLayerData::SetPosition2(int cx, int cy, double sx, double sy, bool isforce) {
	LOLayerDataBase *data = GetBase(isforce);

	data->centerX = cx; data->centerY = cy;
	data->scaleX = sx; data->scaleY = sy;
	data->showType |= SHOW_SCALE;
	data->upflags |= (UP_CENX | UP_CENY | UP_SCALEX | UP_SCALEY | UP_SHOWTYPE);
}

void LOLayerData::SetRotate(double ro, bool isforce) {
	LOLayerDataBase *data = GetBase(isforce);

	data->rotate = ro;
	data->showType |= SHOW_ROTATE;
	data->upflags |= UP_ROTATE;
}

void LOLayerData::SetAlpha(int alp, bool isforce) {
	LOLayerDataBase *data = GetBase(isforce);
	data->alpha = alp & 0xff;
	data->upflags |= UP_ALPHA;
}


bool LOLayerData::SetCell(LOActionNS *ac, int ce, bool isforce) {
	LOLayerDataBase *data = GetBase(isforce);
	data->upflags |= UP_CELLNUM;
	//如果有active，那么应该设置action的值
	if(!ac) ac = (LOActionNS*)GetAction(LOAction::ANIM_NSANIM);
	if (ac && data->texture && ce < ac->cellCount) {
		SetShowType(SHOW_RECT);
		int perx = data->texture->baseW() / ac->cellCount;
		ac->cellCurrent = ce;
		data->showSrcX = ac->cellCurrent * perx;
		data->showSrcY = 0;
		data->showWidth = perx;
		data->showHeight = data->texture->baseH();
		data->upflags |= (UP_SHOWW | UP_SHOWH | UP_SRCX | UP_SRCY | UP_SHOWTYPE);
		return true;
	}
	else {
		data->cellNum = 0;
		return false;
	}
}


void LOLayerData::SetTextureType(int dt, bool isforce) {
	LOLayerDataBase *data = GetBase(isforce);
	data->texType = dt & 0xff;
	switch (data->texType){
		//使用缓存
	//case LOtexture::TEX_COLOR_AREA:
	//case LOtexture::TEX_EMPTY:
	case LOtexture::TEX_IMG:
		data->flags |= FLAGS_USECACHE;
		break;
	default:
		//不使用缓存
		data->flags &= (~FLAGS_USECACHE);
		break;
	}
}

//只允许后台设置新文件
void LOLayerData::SetNewFile(LOShareTexture &tex) {
	LOLayerDataBase *data = GetBase(false);
	data->texture = tex;
	data->flags |= FLAGS_NEWFILE;
	data->upflags |= UP_NEWFILE;
	//清理删除标记
	data->flags &= (~FLAGS_DELETE);
}

//释放图层数据并打上delete标记
void LOLayerData::SetDelete(bool isforce) {
	LOLayerDataBase *data = GetBase(false);
	data->resetBase();
	data->flags |= FLAGS_DELETE;
}


//只允许后台定义按钮
void LOLayerData::SetBtndef(LOString *s, int val, bool isleft, bool isright) {
	LOLayerDataBase *data = GetBase(false);
	if (s) data->btnStr.reset(new LOString(*s));
	data->btnval = val;
	data->flags |= FLAGS_BTNDEF;
	data->flags |= FLAGS_MOUSEMOVE;
	if (isleft) data->flags |= FLAGS_LEFTCLICK;
	if (isright) data->flags |= FLAGS_RIGHTCLICK;
	data->upflags |= UP_BTNSTR;
	data->upflags |= UP_BTNVAL;
}

void LOLayerData::unSetBtndef() {
	LOLayerDataBase *data = GetBase(false);
	data->flags &= (~(FLAGS_BTNDEF| FLAGS_LEFTCLICK| FLAGS_RIGHTCLICK| FLAGS_MOUSEMOVE));
}


void LOLayerData::SetAction(LOAction *ac) {
	LOShareAction acs(ac);
	SetAction(acs);
}


void LOLayerData::SetAction(LOShareAction &ac, bool isforce) {
	LOLayerDataBase *data = GetBase(isforce);
	//同一种类型的action只能存在一个
	if (data->actions) {
		for (auto iter = data->actions->begin(); iter != data->actions->end(); ) {
			if ((*iter)->acType == ac->acType) {
				//计数会-1？
				iter = data->actions->erase(iter);
			}
			else iter++;
		}
	}
	else data->actions.reset(new std::vector<LOShareAction>());
	data->actions->push_back(ac);
	data->upflags |= UP_ACTIONS;
}


LOAction *LOLayerData::GetAction(LOAction::AnimaType acType, bool isforce) {
	LOLayerDataBase *data = GetBase(isforce);
	if (data->actions) {
		for(int ii = 0; ii < data->actions->size(); ii++){
			if (data->actions->at(ii)->acType == acType) return data->actions->at(ii).get();
		}
	}
	return nullptr;
}



void LOLayerData::FirstSNC() {
	LOLayerDataBase *data = GetBase(false);
	for (int ii = 0; ii < data->actions->size(); ii++) {
		LOShareAction ac = data->actions->at(ii);
		if (ac->acType == LOAction::ANIM_NSANIM) {
			if (data->texture) {
				LOActionNS *ai = (LOActionNS*)(ac.get());
				data->showType |= SHOW_RECT;
				data->cellNum = ai->cellCurrent;
				data->showWidth = data->texture->baseW() / ai->cellCount;
				data->showHeight = data->texture->baseH();
				data->upflags |= (UP_SHOWW | UP_SHOWH | UP_SRCX | UP_SRCY | UP_SHOWTYPE);
			}
		}
	}
}


void LOLayerData::GetSize(int *xx, int *yy, int *cell) {
	//if (showType & SHOW_RECT) {
	//	if (xx) *xx = showWidth;
	//	if (yy) *yy = showHeight;
	//}
	//else if (texture) {
	//	if (xx) *xx = texture->baseW();
	//	if (yy) *yy = texture->baseH();
	//}
	//else {
	//	if (xx) *xx = 0;
	//	if (yy) *yy = 0;
	//}

	//if (cell) {
	//	*cell = 1;
	//	LOActionNS *ac = (LOActionNS*)GetAction(LOAction::ANIM_NSANIM);
	//	if (ac) {
	//		*cell = ac->cellCount;
	//	}
	//}
}


//void LOLayerData::upData(LOLayerData *data) {
//	LOLayerDataBase *dst = (LOLayerDataBase*)this;
//	LOLayerDataBase *src = (LOLayerDataBase*)data;
//	*dst = *src;
//}
//
//void LOLayerData::upDataEx(LOLayerData *data) {
//	if (data->maskName) maskName.reset(new LOString(*data->maskName));
//	if (data->actions) actions.reset(new std::vector<LOShareAction>(*data->actions));
//	if (data->eventHooks) eventHooks.reset(new std::vector<LOShareEventHook>(*data->eventHooks));
//}



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


bool LOLayerData::isShowScale(bool isforce) {
	LOLayerDataBase *data = GetBase(isforce);
	return data->showType & SHOW_SCALE;
}

bool LOLayerData::isShowRotate(bool isforce) {
	LOLayerDataBase *data = GetBase(isforce);
	return data->showType & SHOW_ROTATE;
}

bool LOLayerData::isShowRect(bool isforce) {
	LOLayerDataBase *data = GetBase(isforce);
	return data->showType & SHOW_RECT;
}

bool LOLayerData::isVisiable(bool isforce) {
	LOLayerDataBase *data = GetBase(isforce);
	return data->flags & FLAGS_VISIABLE;
}

bool LOLayerData::isChildVisiable(bool isforce) {
	LOLayerDataBase *data = GetBase(isforce);
	return data->flags & FLAGS_CHILDVISIABLE;
}

bool LOLayerData::isCache(bool isforce) {
	LOLayerDataBase *data = GetBase(isforce);
	return data->flags & FLAGS_USECACHE;
}

bool LOLayerData::isDelete(bool isforce) {
	LOLayerDataBase *data = GetBase(isforce);
	return data->flags & FLAGS_DELETE;
}

bool LOLayerData::isBtndef(bool isforce) {
	LOLayerDataBase *data = GetBase(isforce);
	return data->flags & FLAGS_BTNDEF;
}

bool LOLayerData::isActive(bool isforce) {
	LOLayerDataBase *data = GetBase(isforce);
	return data->flags & FLAGS_ACTIVE;
}

//bool LOLayerData::isForce() {
//
//}