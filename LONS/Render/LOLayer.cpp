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
	isinit = false;
	rootLyr = &G_baseLayer[layerType];
}

LOLayer::LOLayer() {
	BaseNew(LAYER_CC_USE);
	curInfo.reset(new LOLayerData());
}

LOLayer::LOLayer(SysLayerType lyrType) {
	BaseNew(lyrType);
}

LOLayer::LOLayer(int fullid) {
	int lyrType;
	int ids[3];
	GetTypeAndIds(&lyrType, ids, fullid);
	BaseNew((SysLayerType)lyrType);
	memcpy(id, ids, 3 * 4);
	curInfo.reset(new LOLayerData());
	bakInfo.reset(new LOLayerData());
	curInfo->fullid = fullid;
	bakInfo->fullid = fullid;
}

LOLayer::LOLayer(LOLayerData &data, bool islink) {
	int lyrType = GetIDs(data.fullid, IDS_LAYER_TYPE);
	BaseNew((SysLayerType)lyrType);
	curInfo.reset(new LOLayerData(data));
	GetTypeAndIds(nullptr, id, curInfo->fullid);
	//挂到父对象的层级
	if (islink && !rootLyr->InserChild(this)) LOLog_e("new LOLayer() no father:%d,%d,%d", id[0], id[1], id[2]);
}

LOLayer::~LOLayer() {
	//释放子对象
	if (childs) {
		for (auto iter = childs->begin(); iter != childs->end(); iter++) {
			LOLayer *layer = iter->second;
			delete layer;
		}
		childs->clear();
	}
}

void LOLayer::releaseForce() {
	curInfo->SetDelete();
	if (bakInfo->isDelete()) NoUseLayer(this);
	//int vv = sizeof(LOLayerData);
}

void LOLayer::releaseBack() {
	bakInfo->SetDelete();
	if (curInfo->isDelete()) NoUseLayer(this);
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


bool LOLayer::isPositionInsideMe(int x, int y) {
	//隐藏的按钮会激活为显示
	int left = curInfo->offsetX;
	int top = curInfo->offsetY;
	int right = left + curInfo->showWidth;
	int bottom = top + curInfo->showHeight;
	if (x >= left && x <= right && y >= top && y <= bottom) {
		curInfo->SetVisable(1);
		return true;
	}
	else return false;

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

void LOLayer::upData(LOLayerData *data) {
	LOLayerDataBase *dst = (LOLayerDataBase*)curInfo.get();
	LOLayerDataBase *src = (LOLayerDataBase*)data;
	*dst = *src;
	if (data->btnStr) curInfo->btnStr.reset(new LOString(*(data->btnStr.get())));
}

void LOLayer::upDataEx(LOLayerData *data) {
	if (data->maskName) curInfo->maskName.reset(new LOString(*data->maskName));
	else curInfo->maskName.reset();
	if (data->actions) curInfo->actions.reset(new std::vector<LOShareAction>(*data->actions));
	else curInfo->actions.reset();
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

	if (!isinit) {
		curInfo->texture->activeTexture(&src, true);
		isinit = true;
	}
	//不可渲染的错误
	if (!curInfo->texture->GetTexture()) return;

	if (curInfo->isShowScale() || curInfo->isShowRotate()) is_ex = true;
	if (isFaterCopyEx()) is_ex = true;

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


void LOLayer::DoAction(LOLayerData *data, Uint32 curTime) {
	if (!data->actions) return;
	for (int ii = 0; ii < data->actions->size(); ii++) {
		LOShareAction acb = data->actions->at(ii);
		if (acb->isEnble()) {
			switch (acb->acType) {
			case LOAction::ANIM_NSANIM:
				DoNsAction(data, (LOActionNS*)acb.get(), curTime);
				break;
			case LOAction::ANIM_TEXT:
				break;
			default:
				break;
			}
		}
	}
}


void LOLayer::DoNsAction(LOLayerData *data, LOActionNS *ai, Uint32 curTime) {
	if (curTime - ai->lastTime > ai->cellTimes[ai->cellCurrent]) {
		ai->lastTime = curTime;
		int cell = ai->cellCurrent;
		//确定下一格
		//从头到尾循环模式
		if (ai->loopMode == LOAction::LOOP_CIRCULAR) {
			cell = (ai->cellCurrent + ai->cellForward) % ai->cellCount;
		}
		else if (ai->loopMode == LOAction::LOOP_GOBACK) {
			//往复循环模式
			if (ai->cellCurrent >= ai->cellCount - 1) ai->cellForward = -1;
			else if (ai->cellCurrent <= 0) ai->cellForward = 1;
			cell = (ai->cellCurrent + ai->cellForward) % ai->cellCount;
		}
		else if (ai->loopMode == LOAction::LOOP_CELL) {
			//只运行一帧
			ai->setEnble(false);
		}
		else if (ai->loopMode == LOAction::LOOP_ONSCE) {
			//只运行一次
			if (ai->cellCurrent < ai->cellCount - 1) cell = (ai->cellCurrent + ai->cellForward) % ai->cellCount;
			else ai->setEnble(false);
		}

		curInfo->SetCell(ai, cell);
	}
}

//void LOLayer::DoAnimation(LOLayerInfo* info, Uint32 curTime) {
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
//}


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

//根据提供的id，在前台图层组中搜索图层
LOLayer* LOLayer::FindViewLayer(int fullid, bool isRemove) {
	int lyrType;
	int ids[3];
	GetTypeAndIds(&lyrType, ids, fullid);

	int index = 0;
	LOLayer *father = DescentFather(&G_baseLayer[lyrType], &index, ids);
	if (father && father->childs) {
		auto iter = father->childs->find(ids[index]);
		if (iter == father->childs->end()) return nullptr;
		LOLayer *lyr = iter->second;
		if (isRemove) father->childs->erase(iter);
		return lyr;
	}
	return nullptr;
}


//外部要相应的主要是鼠标进入图层和离开图层
bool LOLayer::SendEvent(LOEventHook *e, LOEventQue *aswerQue) {
	int xx = e->paramList[0]->GetInt();
	int yy = e->paramList[1]->GetInt();


	return false;
}


int LOLayer::checkEvent(LOEventHook *e, LOEventQue *aswerQue) {
	int ret = SENDRET_NONE;
	//子层在父层上方，因此先检查子层
	if (childs) {
		for (auto iter = childs->begin(); iter != childs->end() && ret != SENDRET_END; iter++) {
			ret = iter->second->checkEvent(e, aswerQue);
		}
	}
	//已经在子层中完成了
	if (ret == SENDRET_END) return ret;
	//没有需要响应的事件
	if (!(e->catchFlag & curInfo->flags)) return ret;
	//左键、右键、长按、悬停
	if (e->catchFlag & LOLayerData::FLAGS_RIGHTCLICK) {
		//只有按钮才相应右键事件
		LOShareEventHook ev(LOEventHook::CreateBtnClickEvent(GetFullID(layerType, id), -1, 0));
		aswerQue->push_back(ev, LOEventQue::LEVEL_NORMAL);
	}
	else {
		//左键、长按、悬停，如果是按钮均会触发按钮类操作
		if (isPositionInsideMe(e->paramList[0]->GetInt(), e->paramList[1]->GetInt())) {
			//按钮处理
			if (curInfo->isBtndef()) {
				//鼠标已经离开对象
				if (e->param1 & LOEventHook::BTN_STATE_ACTIVED) {
					setBtnShow(false);
					//LOLog_i("is:%d", id[0]);
					e->param1 |= LOEventHook::BTN_STATE_UNACTIVED;
				}
				else {
					//鼠标进入对象
					//已经激活的图层不会再次激活
					if (!curInfo->isActive()) {
						setBtnShow(true);
						//产生btnstr事件
						if (curInfo->btnStr) {
							LOShareEventHook ev(LOEventHook::CreateBtnStr(GetFullID(layerType, id), curInfo->btnStr.get()));
							aswerQue->push_back(ev, LOEventQue::LEVEL_NORMAL);
						}
					}
					//已经处理active事件
					e->param1 |= LOEventHook::BTN_STATE_ACTIVED;

					//左键和长按会进一步产生按钮响应点击事件
					if (e->catchFlag & (LOLayerData::FLAGS_LONGCLICK | LOLayerData::FLAGS_LEFTCLICK)) {
						LOShareEventHook ev(LOEventHook::CreateBtnClickEvent(GetFullID(layerType, id), curInfo->btnval, 0));
						aswerQue->push_back(ev, LOEventQue::LEVEL_NORMAL);
					}
				}
			}
			else {
				//非按钮产生图层响应事件
			}
		}
		else {
			//按钮处理
			if (curInfo->isBtndef() && curInfo->isActive()) {
				//鼠标已经离开对象
				setBtnShow(false);
				e->param1 |= LOEventHook::BTN_STATE_UNACTIVED;
			}
		}

		//激活和非激活都已经处理了
		if (e->param1 == (LOEventHook::BTN_STATE_UNACTIVED | LOEventHook::BTN_STATE_ACTIVED)) ret = SENDRET_END;
	}

	return ret;
}



void LOLayer::setBtnShow(bool isshow) {
	if (isshow) {
		setActiveCell(1);
		curInfo->SetVisable(1);
		curInfo->flags |= LOLayerData::FLAGS_ACTIVE;
	}
	else {
		if (!setActiveCell(0))curInfo->SetVisable(0);
		curInfo->flags &= (~LOLayerData::FLAGS_ACTIVE);
	}
}

bool LOLayer::setActiveCell(int cell) {
	bool ret = curInfo->SetCell(nullptr, cell);
	//下次刷新图层时自动更新
	if (ret) isinit = false;
	return ret;
}


void LOLayer::unSetBtndefAll() {
	curInfo->unSetBtndef();
	if (childs) {
		for (auto iter = childs->begin(); iter != childs->end(); iter++) iter->second->unSetBtndefAll();
	}
}


//删除图层，这里留了一个接口
void LOLayer::NoUseLayer(LOLayer *lyr) {
	delete lyr;
}



////只有被改变才返回真
//bool LOLayer::setActive(bool isactive) {
//
//}