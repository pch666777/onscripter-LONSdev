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
	resetMe();
}

void LOLayerDataBase::resetMe() {
	fullid = -1;
	flags = 0;
	offsetX = offsetY = centerX = centerY = 0;
	alpha = -1;
	showSrcX = showSrcY = showWidth = showHeight = 0;
	cellNum = tWidth = tHeight = 0;
	scaleX = scaleY = rotate = 0.0;
	showType = texType = 0;
}

LOLayerData::LOLayerData() {
	isinit = false;
}

LOLayerData::LOLayerData(const LOLayerData &obj)
:LOLayerDataBase(obj)
{
	isinit = false;
	//首先调用基类的拷贝函数
	//接着特殊处理一下
	if (obj.fileTextName) fileTextName.reset(new LOString(*obj.fileTextName));
	if (obj.maskName) maskName.reset(new LOString(*obj.maskName));
	if (obj.btnStr) btnStr.reset(new LOString(*obj.btnStr));
	texture = obj.texture;
	if (obj.actions) actions.reset(new std::vector<LOShareAction>(*obj.actions));
}

LOLayerData::~LOLayerData() {
	//if (actions) for (int ii = 0; ii < actions->size(); ii++) delete actions->at(ii);
	//if (eventHooks) for (int ii = 0; ii < eventHooks->size(); ii++) delete eventHooks->at(ii);
}

void LOLayerData::GetSimpleDst(SDL_Rect *dst) {
	dst->x = offsetX; dst->y = offsetY;
	dst->w = texture->baseW(); dst->h = texture->baseH();
}

void LOLayerData::GetSimpleSrc(SDL_Rect *src) {
	src->x = showSrcX; src->y = showSrcY;
	src->w = showWidth; src->h = showHeight;
}

int LOLayerData::GetCellCount() {
	if (actions) {
		for (int ii = 0; ii < actions->size(); ii++) {
			LOShareAction ac = actions->at(ii);
			if (ac->acType == LOAction::ANIM_NSANIM) return ((LOActionNS*)(ac.get()))->cellCount;
		}
	}
	//默认是1格的
	return 1;
}



void LOLayerData::SetVisable(int v) {
	if(v) flags |= FLAGS_VISIABLE;
	else flags &= (~FLAGS_VISIABLE);
	flags |= FLAGS_UPDATA;
}


void LOLayerData::SetShowRect(int x, int y, int w, int h) {
	showWidth = w;
	showHeight = h;
	showSrcX = x;
	showSrcY = y;
	showType |= SHOW_RECT;
	flags |= FLAGS_UPDATA;
}

void LOLayerData::SetShowType(int show) {
	showType |= show;
	flags |= FLAGS_UPDATA;
}



void LOLayerData::SetPosition(int ofx, int ofy) {
	offsetX = ofx;
	offsetY = ofy;
	flags |= FLAGS_UPDATA;
}

void LOLayerData::SetPosition2(int cx, int cy, double sx, double sy) {
	centerX = cx; centerY = cy;
	scaleX = sx; scaleY = sy;
	showType |= SHOW_SCALE;
	flags |= FLAGS_UPDATA;
}

void LOLayerData::SetRotate(double ro) {
	rotate = ro;
	showType |= SHOW_ROTATE;
	flags |= FLAGS_UPDATA;
}

void LOLayerData::SetAlpha(int alp) {
	alpha = alp & 0xff;
	flags |= FLAGS_UPDATA;
}


bool LOLayerData::SetCell(LOActionNS *ac, int ce) {
	flags |= FLAGS_UPDATA;
	//如果有active，那么应该设置action的值
	if(!ac) ac = (LOActionNS*)GetAction(LOAction::ANIM_NSANIM);
	if (ac && texture && ce < ac->cellCount) {
		SetShowType(SHOW_RECT);
		int perx = texture->baseW() / ac->cellCount;
		ac->cellCurrent = ce;
		showSrcX = ac->cellCurrent * perx;
		showSrcY = 0;
		showWidth = perx;
		showHeight = texture->baseH();
		return true;
	}
	else {
		cellNum = 0;
		return false;
	}
}


void LOLayerData::SetTextureType(int dt) {
	texType = dt & 0xff;
	switch (texType){
		//使用缓存
	case LOtexture::TEX_COLOR_AREA:
	case LOtexture::TEX_EMPTY:
	case LOtexture::TEX_IMG:
		flags |= FLAGS_USECACHE;
		break;
	default:
		//不使用缓存
		flags &= (~FLAGS_USECACHE);
		break;
	}
	flags |= FLAGS_UPDATA;
}

void LOLayerData::SetNewFile(LOShareBaseTexture &base) {
	texture.reset(new LOtexture(base));
	flags |= FLAGS_NEWFILE;
	flags |= FLAGS_UPDATA;
	flags |= FLAGS_UPDATAEX;
	//清理删除标记
	flags &= (~FLAGS_DELETE);
}

//释放图层数据并打上delete标记
void LOLayerData::SetDelete() {
	flags |= FLAGS_DELETE;
	resetMe();
	fileTextName.reset();
	maskName.reset();
	texture.reset();
	if (actions) actions->clear();
}


void LOLayerData::SetBtndef(LOString *s, int val, bool isleft, bool isright) {
	if (s)btnStr.reset(new LOString(*s));
	btnval = val;
	flags |= FLAGS_BTNDEF;
	flags |= FLAGS_MOUSEMOVE;
	if (isleft) flags |= FLAGS_LEFTCLICK;
	if (isright) flags |= FLAGS_RIGHTCLICK;
	flags |= FLAGS_UPDATA;
}

void LOLayerData::unSetBtndef() {
	flags &= (~(FLAGS_BTNDEF| FLAGS_LEFTCLICK| FLAGS_RIGHTCLICK| FLAGS_MOUSEMOVE));
}


void LOLayerData::SetAction(LOAction *ac) {
	LOShareAction acs(ac);
	SetAction(acs);
}


void LOLayerData::SetAction(LOShareAction &ac) {
	//同一种类型的action只能存在一个
	if (actions) {
		for (auto iter = actions->begin(); iter != actions->end(); ) {
			if ((*iter)->acType == ac->acType) {
				//计数会-1？
				iter = actions->erase(iter);
			}
			else iter++;
		}
	}
	else actions.reset(new std::vector<LOShareAction>());
	actions->push_back(ac);
	flags |= FLAGS_UPDATAEX;
}


LOAction *LOLayerData::GetAction(LOAction::AnimaType acType) {
	if (actions) {
		for(int ii = 0; ii < actions->size(); ii++){
			if (actions->at(ii)->acType == acType) return actions->at(ii).get();
		}
	}
	return nullptr;
}



void LOLayerData::FirstSNC() {
	for (int ii = 0; ii < actions->size(); ii++) {
		LOShareAction ac = actions->at(ii);
		if (ac->acType == LOAction::ANIM_NSANIM) {
			if (texture) {
				LOActionNS *ai = (LOActionNS*)(ac.get());
				showType |= SHOW_RECT;
				cellNum = ai->cellCurrent;
				showWidth = texture->baseW() / ai->cellCount;
				showHeight = texture->baseH();
			}
		}
	}
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