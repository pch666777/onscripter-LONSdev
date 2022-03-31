#include "LOLayer.h"

LOLayer G_baseLayer[LOLayer::LAYER_BASE_COUNT];


int G_maxLayerCount[3] = { 1024, 255,255 };  //对应的层级编号必须比这个数字小

void LOLayer::BaseNew(SysLayerType lyrType) {
	id[0] = -1;
	id[1] = -1;
	id[2] = -1;
	parent = nullptr;   //父对象
	childs = nullptr;
	layerType = lyrType;
	rootLyr = &G_baseLayer[layerType];
}

LOLayer::LOLayer() {
	BaseNew(LAYER_CC_USE);
}

LOLayer::LOLayer(SysLayerType lyrType) {
	BaseNew(lyrType);
}

LOLayer::LOLayer(LOLayerData &data) {
	int lyrType;
	GetTypeAndIds(&lyrType, id, data.fullid);
	BaseNew((SysLayerType)lyrType);
	curInfo.reset(new LOLayerData(data));
	//挂到父对象的层级
	if (!rootLyr->InserChild(this)) LOLog_e("new LOLayer() no father:%d,%d,%d", id[0], id[1], id[2]);
}

LOLayer::~LOLayer() {
	//FreeData();
		//释放子对象
	if (childs) {
		for (auto iter = childs->begin(); iter != childs->end(); iter++) {
			LOLayer *layer = iter->second;
			delete layer;
		}
		childs->clear();
	}
}

/*
LOLayer* LOLayer::GetExpectFather(int lyrType, int *ids) {
	LOLayer *father = &G_baseLayer[lyrType];
	for (int ii = 0; ii < 2; ii++) {
		if (!father) return father;
		if (ids[ii + 1] >= G_maxLayerCount[ii + 1]) return father;
		//向下一级
		father = father->FindChild(ids[ii + 1]);
	}
	return father;
}
*/


bool LOLayer::isMaxBorder(int index, int val) {
	return val >= G_maxLayerCount[index];
}

//注意这个函数只应该 layer->Root->InserChild()这样调用
bool LOLayer::InserChild(LOLayer *layer) {
	int ids[3];
	int index;

	GetTypeAndIds(nullptr, ids, layer->curInfo->fullid);
	LOLayer *father = DescentFather(this, &index, ids);
	if (!father) return false;
	return father->InserChild(ids[index], layer);
}

//插入子对象，注意，直接根据id插入
bool LOLayer::InserChild(int cid, LOLayer *layer) {
	if(!childs) childs = new std::map<int, LOLayer*>();
	auto iter = childs->find(cid);
	if (iter != childs->end()) return false;
	(*childs)[cid] = layer;
	layer->parent = this;
	return true;
}

//递归下降父对象
LOLayer* LOLayer::DescentFather(LOLayer *father, int *index, const int *ids) {
	*index = 0;
	//有子参数，下降一层
	if (!isMaxBorder(1, ids[1])) {
		*index = 1;
		father = father->FindChild(ids[0]);
		//有孙子参数，下降一层
		if (father && !isMaxBorder(2, ids[2])) {
			*index = 2;
			father = father->FindChild(ids[1]);
		}
	}
	return father;
}

//删除不在这里，这个方法只应该从 layer->Root调用
LOLayer* LOLayer::RemodeChild(int *cids) {
	int index;
	LOLayer *father = DescentFather(this, &index, cids);
	if (father && father->childs) return RemodeChild(cids[index]);
	return nullptr;
}

LOLayer* LOLayer::RemodeChild(int cid) {
	if (childs) {
		auto iter = childs->find(cid);
		if (iter != childs->end()) {
			LOLayer *lyr = iter->second;
			childs->erase(iter);
			return lyr;
		}
	}
	return nullptr;
}

//这个方法只应该从根对象调用
LOLayer *LOLayer::FindChild(const int *cids) {
	int index;
	LOLayer *father = DescentFather(this, &index, cids);
	if (!father) return nullptr;
	return father->FindChild(cids[index]);
}

LOLayer *LOLayer::FindChild(int cid) {
	if (!childs) return nullptr;
	auto iter = childs->find(cid);
	if (iter != childs->end()) return iter->second;
	return nullptr;
}

bool LOLayer::isAllUnable(LOAnimation *ai) {
	//for (int ii = 0; ii < LOAnimation::ANIMAS_COUNT_PTR; ii++) {
	//	if (ai->isEnble[ii]) return false;
	//}
	return true;
}

bool LOLayer::isPositionInsideMe(int x, int y) {
	//LOAnimation *ai = curInfo->anim;
	//经测试，隐藏的按钮也会被激活
	//if (layerType != LAYER_NSSYS && !isVisible()) return false;  //不可见的通常按钮不被显示
	/*
	int left = curInfo->offsetX;
	int top = curInfo->offsetY;
	int right = left + curInfo->showWidth;
	int bottom = top + curInfo->showHeight;
	if (x >= left && x <= right && y >= top && y <= bottom) {
		curInfo->visiable |= 1;
		return true;
	}
	else return false;
	*/
	return false;
}



/*
//从动画列表中获取动画
LOAnimation *LOLayer::GetAnimation(LOAnimation::AnimaType type) {
	if (!curInfo->actions) return nullptr;
	LOAnimation *ai;
	for (int ii = 0; ii < curInfo->actions->size(); ii++) {
		ai = curInfo->actions->at(ii);
		if (ai->type == type) return ai;
	}
	return nullptr;
}
*/





LOMatrix2d LOLayer::GetTranzMatrix() {
	LOMatrix2d mat,tmp;
	int cx = curInfo->centerX;
	int cy = curInfo->centerY;

	if (curInfo->isShowRotate() && abs(M_PI*curInfo->rotate) > 0.00001) {
		if (cx == -1) cx = curInfo->texture->W() / 2;
		if (cy == -1) cy = curInfo->texture->H() / 2;
		if (curInfo->isShowScale()) {
			//M = T * R * S * -T
			tmp = LOMatrix2d::GetMoveMatrix(-cx, -cy);
			mat = LOMatrix2d::GetScaleMatrix(curInfo->scaleX,curInfo->scaleY);
			mat = mat * tmp;
			tmp = LOMatrix2d::GetRotateMatrix(-curInfo->rotate);
			mat = tmp * mat;
			tmp = LOMatrix2d::GetMoveMatrix(curInfo->offsetX, curInfo->offsetY);
			mat = tmp * mat;
		}
		else {
			//M = T * R * -T
			tmp = LOMatrix2d::GetMoveMatrix(-cx, -cy);
			mat = LOMatrix2d::GetRotateMatrix(-curInfo->rotate);
			mat = mat * tmp;
			tmp = LOMatrix2d::GetMoveMatrix(curInfo->offsetX, curInfo->offsetY);
			mat = tmp * mat;
		}
	}
	else if (curInfo->isShowScale()) {
		if (cx == -1) cx = curInfo->texture->W() / 2;
		if (cy == -1) cy = curInfo->texture->H() / 2;
		//缩放
		mat.Set(0, 0, curInfo->scaleX);
		mat.Set(1, 1, curInfo->scaleY);
		mat.Set(0, 2, curInfo->offsetX - cx * curInfo->scaleX);
		mat.Set(1, 2, curInfo->offsetY - cy * curInfo->scaleY);
	}
	else {
		mat.Set(0, 2, curInfo->offsetX);
		mat.Set(1, 2, curInfo->offsetY);
	}
	return mat;
}

void LOLayer::GetInheritScale(double *sx, double *sy) {
	*sx = curInfo->scaleX;
	*sy = curInfo->scaleY;
	LOLayer *lyr = this->parent;
	while (lyr) {
		*sx *= lyr->curInfo->scaleX;
		*sy *= lyr->curInfo->scaleY;
		lyr = lyr->parent;
	}
}

void LOLayer::GetInheritOffset(float *ox, float *oy) {
	*ox = curInfo->offsetX;
	*oy = curInfo->offsetY;
	LOLayer *lyr = this->parent;
	while (lyr) {
		*ox += lyr->curInfo->offsetX;
		*oy += lyr->curInfo->offsetY;
		lyr = lyr->parent;
	}
}

bool LOLayer::isFaterCopyEx() {
	LOLayer *lyr = this->parent;
	while (lyr) {
		if (lyr->curInfo->isShowRotate() || lyr->curInfo->isShowScale())return true;
		lyr = lyr->parent;
	}
	return false;
}

void LOLayer::GetShowSrc(SDL_Rect *srcR) {
	if (curInfo->isShowRect()) {
		curInfo->GetSimpleSrc(srcR);
	}
	else {
		srcR->x = 0; srcR->y = 0;
		srcR->w = curInfo->texture->baseW();
		srcR->h = curInfo->texture->baseH();
	}
}


bool LOLayer::isVisible() {
	if (curInfo->alpha == 0 || !curInfo->isVisiable()) return false ;
	else return true;
}

bool LOLayer::isChildVisible() {
	return curInfo->isChildVisiable();
}

void LOLayer::ShowMe(SDL_Renderer *render) {
	/*bool debugbreak = false ;*/
	//全透明或者不可见，不渲染对象
	//LONS::printInfo("alpha:%d visiable:%d", ptr[LOLayerInfo::alpha], ptr[LOLayerInfo::visiable]);

	if (!isVisible()) return;

	SDL_Rect src;
	SDL_FRect dst;
	bool is_ex = false;

	matrix.Clear();
	GetShowSrc(&src);
	if (curInfo->isShowScale() || curInfo->isShowRotate()) is_ex = true;
	if (isFaterCopyEx()) is_ex = true;

	if (!isInit) {
		curInfo->texture->activeTexture(&src);
		isInit = true;
	}

	curInfo->texture->setForceAplha(curInfo->alpha);
	curInfo->texture->activeFlagControl();


	if (is_ex) {
		//有旋转和缩放则计算出当前的变换矩阵
		matrix = GetTranzMatrix();
		matrix = parent->matrix * matrix;  //继承父对象的变换
		if(!(parent->parent)){
			LOMatrix2d tmp = LOMatrix2d::GetScaleMatrix(G_gameScaleX, G_gameScaleY) ; //继承游戏画面缩放
			matrix = tmp * matrix ;
		}
		double sx, sy;
		GetInheritScale(&sx, &sy);
		dst.x = 0.0f;
		dst.y = 0.0f;
		dst.w = src.w * sx;
		dst.h = src.h * sy;
		//计算出四个点的位置
		double dpt[8];
		//p1
		dpt[0] = 0; dpt[1] = 0;
		//p2
		dpt[2] = src.w; dpt[3] = 0;
		//p3
		dpt[4] = 0; dpt[5] = src.h;
		//p4
		dpt[6] = src.w; dpt[7] = src.h;
		matrix.TransPoint(&dpt[0], &dpt[1]);
		matrix.TransPoint(&dpt[2], &dpt[3]);
		matrix.TransPoint(&dpt[4], &dpt[5]);
		matrix.TransPoint(&dpt[6], &dpt[7]);

		float fpt[8];
		for (int ii = 0; ii < 8; ii++) fpt[ii] = (float)dpt[ii];

		//if(debugbreak) LOLog_i("dst positon fps is %f,%f,%f,%f",dpt[0],dpt[1],dpt[2],dpt[3]) ;
		//If you do not implement this function, the return value is less than 0 and will not be displayed
		//check it,please
		SDL_RenderCopyLONS(render, curInfo->texture->GetTexture(), &src, &dst, fpt);
	}
	else {
		GetInheritOffset(&dst.x, &dst.y);
		matrix.Set(0, 2, dst.x);
		matrix.Set(1, 2, dst.y);
		dst.w = src.w;
		dst.h = src.h;
		//if(debugbreak) LOLog_i("dst positon is %d,%d,%d,%d",dst.x,dst.y,dst.w,dst.h) ;
		SDL_RenderCopyF(render, curInfo->texture->GetTexture(), &src, &dst);
	}
}



void LOLayer::Serialize(BinArray *sbin) {
	//fullid--当前信息--childs数量--child
	/*
	sbin->WriteString("layer");
	sbin->WriteInt(layerType);
	sbin->WriteInt(id[0]);
	sbin->WriteInt(id[1]);
	sbin->WriteInt(id[2]);
	curInfo->Serialize(sbin);
	sbin->WriteInt(childs->size());
	for (auto iter = childs->begin(); iter != childs->end(); iter++) {
		iter->second->Serialize(sbin);
	}
	*/
}

void LOLayer::DoAnimation(LOLayerInfo* info, Uint32 curTime) {
	/*
	if (!curInfo->actions) return;
	for (auto iter = curInfo->actions->begin(); iter != curInfo->actions->end(); iter++) {
		LOAnimation *aib = (*iter);
		if (aib->isEnble) {
			switch (aib->type)
			{
			case LOAnimation::ANIM_NSANIM:
				DoNsAnima(info, (LOAnimationNS*)(aib), curTime);
				break;
			case LOAnimation::ANIM_TEXT:
				DoTextAnima(info, (LOAnimationText*)(aib), curTime);
				break;
			case LOAnimation::ANIM_MOVE:
				DoMoveAnima(info, (LOAnimationMove*)(aib), curTime);
				break;
			case LOAnimation::ANIM_SCALE:
				DoScaleAnima(info, (LOAnimationScale*)(aib), curTime);
				break;
			case LOAnimation::ANIM_ROTATE:
				DoRotateAnima(info, (LOAnimationRotate*)(aib), curTime);
				break;
			case LOAnimation::ANIM_FADE:
				DoFadeAnima(info, (LOAnimationFade*)(aib), curTime);
				break;
			default:
				break;
			}
		}
	}
	//执行完成后立即检查哪些需要删除了
	for (auto iter = curInfo->actions->begin(); iter != curInfo->actions->end();) {
		LOAnimation *aib = (*iter);
		if (aib->finish && aib->finishFree) 
			iter = curInfo->actions->erase(iter, true); //已经自动指向下一个元素了
		else iter++;
	}
	if (curInfo->actions->size() == 0) {
		delete curInfo->actions;
		curInfo->actions = nullptr;
	}
	*/
}

void LOLayer::DoNsAnima(LOLayerInfo *info, LOAnimationNS *ai, Uint32 curTime) {
	//延迟时间够了则执行
	if (curTime - ai->lastTime > ai->perTime[ai->cellCurrent]) {
		ai->lastTime = curTime;
		//显示模式改变
		info->SetShowType(LOLayerInfo::SHOW_RECT);
		//定位指定格数对应的位置
		int perx = info->texture->baseW() / ai->cellCount;
		info->showSrcX = ai->cellCurrent * perx;
		info->showSrcY = 0;
		info->showWidth = perx;
		info->showHeight = info->texture->baseH();
		//执行之后检查循环模式
		if (ai->loopMode == LOAnimation::LOOP_CIRCULAR) {  //从头到尾循环模式
			ai->cellCurrent = (ai->cellCurrent + ai->cellForward) % ai->cellCount;
			//printf("cellCurrent:%d\n", ptr[LOAnimation::cellCurrent]);
		}
		else if (ai->loopMode == LOAnimation::LOOP_GOBACK) {  //往复循环模式
			if (ai->cellCurrent >= ai->cellCount - 1) ai->cellForward = -1;
			else if (ai->cellCurrent <= 0) ai->cellForward = 1;
			ai->cellCurrent = (ai->cellCurrent + ai->cellForward) % ai->cellCount;
		}
		else if (ai->loopMode == LOAnimation::LOOP_PTR) { //只运行一帧
			ai->isEnble = false;
		}
		else if (ai->loopMode == LOAnimation::LOOP_ONSCE) { //只运行一次
			if (ai->cellCurrent < ai->cellCount - 1) ai->cellCurrent = (ai->cellCurrent + ai->cellForward) % ai->cellCount;
			else ai->isEnble = false;
		}

	}

	//信息往回覆盖
	curInfo->cellNum = ai->cellCurrent;
}

//执行文字动画
void LOLayer::DoTextAnima(LOLayerInfo *info, LOAnimationText *ai, Uint32 curTime) {
	SDL_Rect srcR;
	LineComposition *line = ai->lineInfo->at(ai->currentLine);
	//给与首次运行的初始帧
	if (ai->lastTime == 0) {
		ai->lastTime = curTime;
		if(!ai->isadd) ai->currentPos = line->x;

		info->texture->activeActionTxtTexture();

		//bool hasfinish = false;
		if (layerType == LAYER_DIALOG) {
			//正常的对话文字对话事件是可以取到事件的，没有取到事件说明已经在单击事件中完成了
			//直接跳过
			//LOEvent1 *e = G_GetEvent(LOEvent1::EVENT_TEXT_ACTION);
			//if (e) e->closeEdit();
			//else { //跳过，确保不会在首帧返回，而是继续执行
			//	ai->lastTime = 1;
			//	ai->currentLine = 0;
			//	ai->currentPos = 0;
			//	ai->isadd = true;
			//}
		}

		if(!ai->isadd)return;  //普通模式的直接返回
	}

	int pos, sumpix;
	bool allFinish = false;

	if (!ai->isadd) {
		pos = curTime - ai->lastTime;
		sumpix = pos * ai->perpix;    //总共应该前进这么多
		//sumpix = 5;
		if (sumpix < 5) return;           //至少要有5个像素
	}
	else {
		ai->isadd = false;
		sumpix = 0;
	}

	
	while (sumpix > 0) {
		line = ai->lineInfo->at(ai->currentLine);
		if (sumpix + ai->currentPos < line->sumx) {
			ai->currentPos += sumpix;
			break;
		}
		else if(ai->currentLine >= ai->lineInfo->size() - 1){  //已经到了最后一行
			allFinish = true;
			break;
		}
		else {
			sumpix -= (line->sumx - ai->currentPos);
			ai->currentLine++;
			ai->currentPos = 0;
		}
	}
	//计算出要复制的行
	srcR.x = 0;
	srcR.y = 0;
	for (int ii = 0; ii < ai->lineInfo->size() ; ii++) {
		line = ai->lineInfo->at(ii);
		if (line->finish != LineComposition::LINE_FINISH_COPY) break;
		srcR.y = line->y + line->height;
	}
	if (ai->currentLine > 0) {
		line = ai->lineInfo->at(ai->currentLine);
		srcR.w = info->texture->W();
		srcR.h = line->y - srcR.y;
		//LONS::printInfo("line must be show:%d \n",srcR.h);
		if (srcR.h > 0) info->texture->rollTxtTexture(&srcR, &srcR);
	}
	//复制当前区域
	line = ai->lineInfo->at(ai->currentLine);
	srcR.x = 0; srcR.w = ai->currentPos;
	srcR.y = line->y; srcR.h = line->height;
	if (allFinish) srcR.w = info->texture->W();
	if(srcR.w > 0) info->texture->rollTxtTexture(&srcR, &srcR);

	//改写已经完成的行
	for (int ii = 0; ii < ai->currentLine; ii++) {
		ai->lineInfo->at(ii)->finish = LineComposition::LINE_FINISH_COPY;
	}

	if (allFinish) {
		ai->lineInfo->at(ai->currentLine)->finish = LineComposition::LINE_FINISH_COPY;
		ai->isEnble = false;
		ai->finish = true;
		if (layerType == LAYER_DIALOG) {
			//LOEvent1 *e = G_GetEvent(LOEvent1::EVENT_TEXT_ACTION);
			////LOEvent1 *e = G_GetSelfEventIsParam(LOEvent1::EVENT_TEXT_ACTION, FunctionInterface::LAYER_TEXT_WORKING);
			//if (e) {
			//	e->FinishMe();
			//	//printf("text finish!!\n");
			//}
		}
	}

	ai->lastTime = curTime;
}

////只用于RGBA的复制
//bool LOLayer::CopySurfaceToTexture(SDL_Surface *su, SDL_Rect *src, SDL_Texture *tex, SDL_Rect *dst) {
//	void *texdata;
//	int tpitch;
//	if(SDL_LockTexture(tex, NULL, &texdata, &tpitch) != 0 )return false;
//	SDL_LockSurface(su);
//
//	char *mdst = (char*)texdata + dst->y * tpitch + dst->x * 4;
//	char *msrc = (char*)su->pixels + src->y * su->pitch + src->x * 4;
// 	for (int yy = 0; yy < src->h; yy++) {
//		memcpy(mdst, msrc, src->w * 4);
//		mdst += tpitch;
//		msrc += su->pitch;
//	}
//	SDL_UnlockSurface(su);
//	SDL_UnlockTexture(tex);
//	return true;
//}


//执行移动动画
void LOLayer::DoMoveAnima(LOLayerInfo *info, LOAnimationMove *ai, Uint32 curTime) {
	//给与首次运行的初始帧
	if (ai->lastTime == 0) {
		ai->lastTime = curTime;
		info->offsetX = ai->fromx; info->offsetY = ai->fromy;
		return;
	}
	double postime = curTime - ai->lastTime;
	if (postime > ai->timer) {
		info->offsetX = ai->destx; info->offsetY = ai->desty;
		//运行到结尾根据循环模式确定下一步动作
		if (ai->loopMode == LOAnimation::LOOP_CIRCULAR) ai->lastTime = 0;
		else if (ai->loopMode == LOAnimation::LOOP_GOBACK) {
			int tmp = ai->destx; ai->destx = ai->fromx; ai->fromx = tmp;
			tmp = ai->desty; ai->desty = ai->fromy; ai->fromy = tmp;
			ai->lastTime = curTime;
		}
		else {
			ai->finish = true;
			ai->isEnble = false;
		}
	}
	else {
		int dx = postime / ai->timer * (ai->destx - ai->fromx);
		int dy = postime / ai->timer * (ai->desty - ai->fromy);
		info->offsetX = ai->fromx + dx; info->offsetY = ai->fromy + dy;
	}
}

//执行缩放动画
void LOLayer::DoScaleAnima(LOLayerInfo *info, LOAnimationScale *ai, Uint32 curTime) {
	//给与首次运行的初始帧
	info->SetShowType(LOLayerInfo::SHOW_SCALE);
	info->centerX = ai->centerX; info->centerY = ai->centerY;
	if (ai->lastTime == 0) {
		ai->lastTime = curTime;
		info->scaleX = ai->startPerX; info->scaleY = ai->startPerY;
		return;
	}
	//正常运行
	double postime = curTime - ai->lastTime;

	if (postime > ai->timer) {
		info->scaleX = ai->endPerX;
		info->scaleY = ai->endPerY;
		//运行到结尾根据循环模式确定下一步动作
		if (ai->loopMode == LOAnimation::LOOP_CIRCULAR) ai->lastTime = 0;
		else if (ai->loopMode == LOAnimation::LOOP_GOBACK) {
			double tmp = ai->endPerX; ai->endPerX = ai->startPerX; ai->startPerX = tmp;
			tmp = ai->endPerY; ai->endPerY = ai->startPerY; ai->startPerY = tmp;
			ai->lastTime = curTime;
		}
		else {
			ai->finish = true;
			ai->isEnble = false;
		}
	}
	else {
		double perx = (ai->endPerX - ai->startPerX) * postime / ai->timer;
		double pery = (ai->endPerY - ai->startPerY) * postime / ai->timer;
		info->scaleX = ai->startPerX + perx;
		info->scaleY = ai->startPerY + pery;
	}
}

//执行旋转动画
void LOLayer::DoRotateAnima(LOLayerInfo *info, LOAnimationRotate *ai, Uint32 curTime) {
	//给与首次运行的初始帧
	info->SetShowType(LOLayerInfo::SHOW_ROTATE);
	info->centerX = ai->centerX; info->centerY = ai->centerY;
	if (ai->lastTime == 0) {
		ai->lastTime = curTime;
		info->rotate = ai->startRo;
		return;
	}
	//正常运行
	double postime = curTime - ai->lastTime;

	if (postime > ai->timer) {
		info->rotate = ai->endRo;
		//运行到结尾根据循环模式确定下一步动作
		if (ai->loopMode == LOAnimation::LOOP_CIRCULAR) {
			//终点角度为360的时候，需要进行优化
			if (ai->endRo == 360) {
				while (postime > ai->timer) postime -= ai->timer;
				double perRo = postime * (ai->endRo - ai->startRo) / ai->timer;
				info->rotate = ai->startRo + perRo;
			}
			else ai->lastTime = 0;
		}
		else if (ai->loopMode == LOAnimation::LOOP_GOBACK) {
			double tmp = ai->endRo; ai->endRo = ai->startRo; ai->startRo = tmp;
			ai->lastTime = curTime;
		}
		else {
			ai->finish = true;
			ai->isEnble = false;
		}
	}
	else {
		double perRo = postime * (ai->endRo - ai->startRo) / ai->timer;
		info->rotate = ai->startRo + perRo;
	}
}


void LOLayer::DoFadeAnima(LOLayerInfo *info, LOAnimationFade *ai, Uint32 curTime) {
	if (ai->lastTime == 0) { //首次
		ai->lastTime = curTime;
		info->alpha = ai->startAlpha;
	}
	else if(curTime - ai->lastTime > ai->timer){ //超时
		info->alpha = ai->endAlpha;
		//运行到结尾根据循环模式确定下一步动作
		if (ai->loopMode == LOAnimation::LOOP_CIRCULAR) ai->lastTime = 0;
		else if (ai->loopMode == LOAnimation::LOOP_GOBACK) {
			int tmp = ai->endAlpha; ai->endAlpha = ai->startAlpha; ai->startAlpha = tmp;
			ai->lastTime = curTime;
		}
		else {
			ai->finish = true;
			ai->isEnble = false;
		}
	}
	else { //正常
		double postime = curTime - ai->lastTime;
		int perAlpha = postime * (ai->endAlpha - ai->startAlpha) / ai->timer;
		info->alpha = ai->startAlpha + perAlpha;
	}
	if (info->alpha < 0) info->alpha = 0;
	if (info->alpha > 255) info->alpha = 255;
}


bool LOLayer::GetTextEndPosition(int *xx, int *yy, int *lineH) {
	/*
	LOAnimation *aib = curInfo->GetAnimation(LOAnimation::ANIM_TEXT);
	if (aib) {
		LineComposition *line = ((LOAnimationText*)aib)->lineInfo->top();
		if (line) {
			//Consider the effect of font scaling
			double sx, sy;
			sx = 1.0;
			sy = 1.0;
			if (curInfo->isShowScale()) {
				sx = curInfo->scaleX;
				sy = curInfo->scaleY;
			}
			if (xx) *xx = sx * line->sumx;
			if (yy) *yy = sy * line->y;
			if (lineH) *lineH = sy * line->height;
			return true;
		}
	}
	*/
	return false;
}


void LOLayer::GetLayerPosition(int *xx, int *yy, int *aph) {
	if (xx) *xx = curInfo->offsetX;
	if (yy) *yy = curInfo->offsetY;
	if (aph) *aph = curInfo->alpha;
}

//这个函数应该从layer->Root调用，需要确保bin的长度足够
void LOLayer::GetLayerUsedState(char *bin, int *ids) {
	int index;
	LOLayer *father = DescentFather(this, &index, ids);
	if (father && father->childs) {
		for (auto iter = father->childs->begin(); iter != father->childs->end(); iter++) {
			bin[iter->first] = 1;
		}
	}
}



//int LOLayer::GetIDs(int fullid, int pos) {
//	if (pos < 0 || pos > 2) return -1;
//	if (pos == 0) return (fullid >> 16) & 0x3FF;
//	if (pos == 1) return (fullid >> 8) & 0xFF;
//	if (pos == 2) return fullid & 0xFF;
//}

