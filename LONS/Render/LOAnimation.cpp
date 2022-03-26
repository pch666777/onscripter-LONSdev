/*
Animation 是一种图层在每一帧应进行变换的描述，动画在图层被删除或加载别的文件时被删除
*/

#include "LOAnimation.h"

LOAnimation::LOAnimation() {
	type = ANIM_NONE;
	control = CON_NONE;
	finishFree = false;     //执行完成后是否释放
	finish = false;
	lastTime = 0;  //上次运行的时间
	isEnble = false;   //释放可用
	loopMode = LOOP_ONSCE;
	gval = 0;
}

void LOAnimation::AddTo(LOStack<LOAnimation> **list, int addtype) {
	if (addtype == CON_NEW) {
		if (!(*list)) *list = new LOStack<LOAnimation>;
		(*list)->clear(true);
		(*list)->push(this);
	}
	else if (addtype == CON_APPEND) {
		if (!(*list)) *list = new LOStack<LOAnimation>;
		(*list)->push(this);
	}
	else if(addtype == CON_REPLACE){
		if (!(*list)) *list = new LOStack<LOAnimation>;
		for (int ii = 0; ii < (*list)->size(); ii++) {
			if ((*list)->at(ii)->type == this->type) {
				delete (*list)->swap(this, ii);
				return;
			}
		}
		(*list)->push(this);
	}
	else if (addtype == CON_CLEAR) {
		if (*list) {
			(*list)->clear(true);
			delete *list;
			*list = nullptr;
		}
	}
	else {
		if (*list) {
			for (auto tmp = (*list)->begin(); tmp != (*list)->end(); tmp++) {
				if ((*tmp)->type == this->type) (*list)->erase(tmp, true);
			}
		}
	}
}

void LOAnimation::Serialize(BinArray *sbin) {
	sbin->WriteString("animbase");
	sbin->WriteInt((int)type);
	sbin->WriteInt(control);
	sbin->WriteInt(loopMode);
	sbin->WriteInt(gval);
	sbin->WriteBool(finishFree);
	sbin->WriteBool(finish);
	sbin->WriteInt32((int32_t)lastTime);
	sbin->WriteBool(isEnble);
}

LOAnimation::~LOAnimation() {
}

void LOAnimation::FreeAnimaList(LOStack<LOAnimation> **list) {
	while ((*list)->size() > 0) {
		delete (*list)->pop();
	}
	delete (*list);
	*list = nullptr;
}

LOAnimationNS::LOAnimationNS() {
	type = LOAnimation::ANIM_NSANIM;
	finishFree = false;
}

LOAnimationNS::~LOAnimationNS() {
	if (perTime) delete[] perTime;
	if (bigsu) FreeSurface(bigsu);

	perTime = nullptr;
	bigsu = nullptr;
}

void LOAnimationNS::Serialize(BinArray *sbin) {
	sbin->WriteString("animNS");
	LOAnimation::Serialize(sbin);
	sbin->WriteInt(alphaMode);
	sbin->WriteInt(cellCount);
	sbin->WriteInt(cellForward);
	sbin->WriteInt(cellCurrent);
	for (int ii = 0; ii < cellCount; ii++) sbin->WriteInt(perTime[ii]);
}


LOAnimationText::LOAnimationText() {
	currentPos = 0;  //动画当前运行的位置
	currentLine = 0;
	loopdelay = -1;   //循环延时，-1表示不循环
	//su = nullptr;
	lineInfo = nullptr;  //行信息
	type = LOAnimation::ANIM_TEXT;
}

void LOAnimationText::Serialize(BinArray *sbin) {
	sbin->WriteString("animTX");
	LOAnimation::Serialize(sbin);
	sbin->WriteInt(currentPos);
	sbin->WriteInt(currentLine);
	sbin->WriteInt(loopdelay);
	sbin->WriteDouble(perpix);

}

LOAnimationText::~LOAnimationText() {
	if (lineInfo) {
		lineInfo->clear(true);
		delete lineInfo;
		lineInfo = nullptr;
	}
	//纹理由纹理管理器释放
	//if (su) {
	//	delete su;
	//	su = nullptr;
	//}
}


LOAnimationMove::LOAnimationMove() {
	type = LOAnimation::ANIM_MOVE;
}

void LOAnimationMove::Serialize(BinArray *sbin) {
	sbin->WriteString("animMove");
	LOAnimation::Serialize(sbin);
	sbin->WriteInt(fromx);
	sbin->WriteInt(fromy);
	sbin->WriteInt(destx);
	sbin->WriteInt(desty);
	sbin->WriteDouble(timer);
}


LOAnimationScale::LOAnimationScale() {
	type = LOAnimation::ANIM_SCALE;
}


void LOAnimationScale::Serialize(BinArray *sbin) {
	sbin->WriteString("animScale");
	LOAnimation::Serialize(sbin);
	sbin->WriteInt(centerX);
	sbin->WriteInt(centerY);
	sbin->WriteDouble(startPerX);
	sbin->WriteDouble(startPerY);
	sbin->WriteDouble(endPerX);
	sbin->WriteDouble(endPerY);
	sbin->WriteDouble(timer);
}

LOAnimationRotate::LOAnimationRotate() {
	type = LOAnimation::ANIM_ROTATE;
}


void LOAnimationRotate::Serialize(BinArray *sbin) {
	sbin->WriteString("animRotate");
	LOAnimation::Serialize(sbin);
	sbin->WriteInt(centerX);
	sbin->WriteInt(centerY);
	sbin->WriteInt(centerX);
	sbin->WriteInt(centerY);
	sbin->WriteInt(startRo);
	sbin->WriteInt(endRo);
	sbin->WriteDouble(timer);
}

LOAnimationFade::LOAnimationFade() {
	type = LOAnimation::ANIM_FADE;
}

void LOAnimationFade::Serialize(BinArray *sbin) {
	sbin->WriteString("animFade");
	LOAnimation::Serialize(sbin);
	sbin->WriteInt(startAlpha);
	sbin->WriteInt(endAlpha);
	sbin->WriteDouble(timer);
}
