/*
//存放图层的基本信息
*/

#include "LOLayerData.h"


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
	texture = obj.texture;
	if (obj.actions) actions.reset(new std::vector<LOShareAction>(*obj.actions));
	if (obj.eventHooks) eventHooks.reset(new std::vector<LOShareEventHook>(*obj.eventHooks));
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


void LOLayerData::SetCell(int ce) {
	cellNum = ce;
	flags |= FLAGS_UPDATA;
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
	//事件钩子可能还在别的地方，直接失效
	if (eventHooks) {
		for (int ii = 0; ii < eventHooks->size(); ii++) {
			LOShareEventHook e = eventHooks->at(ii);
			e->InvalidMe();
		}
		eventHooks->clear();
	}
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