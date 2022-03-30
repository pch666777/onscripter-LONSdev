/*
//存放图层的基本信息
*/

#include "LOLayerData.h"

LOLayerData::LOLayerData() {
	flags = 0;
	threadLock.store(0);
}

LOLayerData::~LOLayerData() {
	if (actions) for (int ii = 0; ii < actions->size(); ii++) delete actions->at(ii);
	if (eventHooks) for (int ii = 0; ii < eventHooks->size(); ii++) delete eventHooks->at(ii);
}

void LOLayerData::GetSimpleDst(SDL_Rect *dst) {
	dst->x = offsetX; dst->y = offsetY;
	dst->w = texture->baseW(); dst->h = texture->baseH();
}

void LOLayerData::GetSimpleSrc(SDL_Rect *src) {
	src->x = showSrcX; src->y = showSrcY;
	src->w = showWidth; src->h = showHeight;
}


void LOLayerData::SetVisable(int v) {
	flags |= FLAGS_VISIABLE;
}


void LOLayerData::SetShowRect(int x, int y, int w, int h) {
	showWidth = w;
	showHeight = h;
	showSrcX = x;
	showSrcY = y;
	showType |= SHOW_RECT;
}

void LOLayerData::SetShowType(int show) {
	showType |= show;
}



void LOLayerData::SetPosition(int ofx, int ofy) {
	offsetX = ofx;
	offsetY = ofy;
}

void LOLayerData::SetPosition2(int cx, int cy, double sx, double sy) {
	centerX = cx; centerY = cy;
	scaleX = sx; scaleY = sy;
	showType |= SHOW_SCALE;
}

void LOLayerData::SetRotate(double ro) {
	rotate = ro;
	showType |= SHOW_ROTATE;
}

void LOLayerData::SetAlpha(int alp) {
	alpha = alp & 0xff;
}


void LOLayerData::SetCell(int ce) {
	cellNum = ce;
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
}


void LOLayerData::SetAction(LOAction *action) {
	//同一种类型的action只能存在一个
	lockMe();
	if(!actions)
	for(int ii = 0; ii < )
	unLockMe();
}


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