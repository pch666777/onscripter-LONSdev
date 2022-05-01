/*
//文字描述
*/

#include "LOTextDescribe.h"


LOTextDescribe::LOTextDescribe() {
	parent = nullptr;
	childs = nullptr;
	words = nullptr;
	surface = nullptr;
	x = y = 0;
}

LOTextDescribe::~LOTextDescribe() {
	if (words) ReleaseWords(false);
	if (surface) ReleaseSurface(false);
	if (childs) {
		for (auto iter = childs->begin(); iter != childs->end(); iter++) delete *iter;
		delete childs;
	}
}



void LOTextDescribe::ReleaseWords(bool ischild) {
	if (words) {
		for (auto iter = words->begin(); iter != words->end(); iter++) delete *iter;
		delete[] words;
		words = nullptr;
	}
	//子对象的数据一起释放
	if (ischild && childs) {
		for (auto iter = childs->begin(); iter != childs->end(); iter++) (*iter)->ReleaseWords(ischild);
	}
}


void LOTextDescribe::ReleaseSurface(bool ischild) {
	if (surface) {
		FreeSurface(surface);
		surface = nullptr;
	}
	//子对象的数据一起释放
	if (ischild && childs) {
		for (auto iter = childs->begin(); iter != childs->end(); iter++) (*iter)->ReleaseSurface(ischild);
	}
}


void LOTextDescribe::FreeChildSurface() {
	if (childs) {
		for (auto iter = childs->begin(); iter != childs->end(); iter++) (*iter)->ReleaseSurface(true);
	}
}


void LOTextDescribe::GetSizeRect(int *left, int *top, int *right, int *bottom) {
	//从最底层的对象开始
	if (childs) {
		for (auto iter = childs->begin(); iter != childs->end(); iter++) {
			(*iter)->GetSizeRect(left, top, right, bottom);
		}
	}

	//获取相对于根对象的x y
	int x1, x2, y1, y2;
	x1 = x2 = y1 = y2 = 0;

	LOTextDescribe *tdes = this;
	while (tdes->parent) {
		tdes = tdes->parent;
		x1 += tdes->x;
		y1 += tdes->y;
	}
	if (surface) {
		x2 = surface->w + x1;
		y2 = surface->h + x2;
	}

	if (left && x1 < *left) *left = x1;
	if (top  && y1 < *top) *top = y1;
	if (right && x2 > *right) *right = x2;
	if (bottom && y2 > *bottom) *bottom = y2;
}


void LOTextDescribe::SetPositionZero() {
	int left, top ;
	left = top = 0;
	GetSizeRect(&left, &top, nullptr, nullptr);
	if (left != 0 || top != 0) MovePosition(0 - left, 0 - top);
}


void LOTextDescribe::MovePosition(int mx, int my) {
	x += mx;
	y += my;
	if (childs) {
		for (auto iter = childs->begin(); iter != childs->end(); iter++) {
			(*iter)->MovePosition(mx, my);
		}
	}
}