﻿#include "LOLayer.h"
#include "LOLayerData.h"


//从外部向render模块添加前置处理事件
extern void AddPreEvent(LOShareEventHook &e);

//从外部向render模块添加事件
extern void AddRenderEvent(LOShareEventHook &e);

LOLayer *G_baseLayer[LOLayer::LAYER_BASE_COUNT];
int G_maxLayerCount[3] = { 1024, 255,255 };  //对应的层级编号必须比这个数字小

//所有的图层都存储在这
std::map<int, LOLayer*> LOLayer::layerCenter;

void LOLayer::BaseNew(SysLayerType lyrType) {
	id[0] = -1;
	id[1] = -1;
	id[2] = -1;
	parent = nullptr;   //父对象
	childs = nullptr;
	layerType = lyrType;
	isinit = false;
	rootLyr = G_baseLayer[layerType];
}

LOLayer::LOLayer() {
	BaseNew(LAYER_CC_USE);
	data.reset(new LOLayerData());
}

LOLayer::LOLayer(SysLayerType lyrType) {
	BaseNew(lyrType);
	data.reset(new LOLayerData());
}

LOLayer::LOLayer(int fullid) {
	int lyrType;
	int ids[3];
	GetTypeAndIds(&lyrType, ids, fullid);
	BaseNew((SysLayerType)lyrType);
	memcpy(id, ids, 3 * 4);
	data.reset(new LOLayerData());
	data->fullid = fullid;
}


LOLayer::~LOLayer() {
	//析构前应解除所有的连接关系了
	if (parent) 
        SDL_LogError(0, "LOLayer::~LOLayer() has parent now!");
	if (childs) {
		if(childs->size() > 0) 
            SDL_LogError(0,"LOLayer::~LOLayer() has childs now!");
		delete childs;
		childs = nullptr;
	}
}


//void LOLayer::releaseForce() {
//	curInfo->SetDelete();
//	if (bakInfo->isDelete()) NoUseLayer(this);
//	//int vv = sizeof(LOLayerData);
//}
//
//void LOLayer::releaseBack() {
//	bakInfo->SetDelete();
//	if (curInfo->isDelete()) NoUseLayer(this);
//}

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
//bool LOLayer::InserChild(LOLayer *layer) {
//	int ids[3];
//	int index;
//
//	GetTypeAndIds(nullptr, ids, layer->curInfo->fullid);
//	LOLayer *father = DescentFather(this, &index, ids);
//	if (!father) return false;
//	return father->InserChild(ids[index], layer);
//}

//插入子对象，注意，直接根据id插入
bool LOLayer::InserChild(int cid, LOLayer *layer) {
	if(!childs) childs = new std::map<int, LOLayer*>();
	auto iter = childs->find(cid);
	if (iter != childs->end()) {
		//不是同一个对象
		if (layer != iter->second) return false;
		layer->parent = this;
	}
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
//LOLayer* LOLayer::RemodeChild(int *cids) {
//	int index;
//	LOLayer *father = DescentFather(this, &index, cids);
//	if (father && father->childs) return RemodeChild(cids[index]);
//	return nullptr;
//}

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
//LOLayer *LOLayer::FindChild(const int *cids) {
//	int index;
//	LOLayer *father = DescentFather(this, &index, cids);
//	if (!father) return nullptr;
//	return father->FindChild(cids[index]);
//}



LOLayer *LOLayer::FindChild(int cid) {
	if (!childs) return nullptr;
	auto iter = childs->find(cid);
	if (iter != childs->end()) return iter->second;
	return nullptr;
}


bool LOLayer::isPositionInsideMe(int x, int y) {
	//隐藏的按钮会激活为显示
	LOLayerDataBase *curInfo = &data->cur;
	int left = curInfo->offsetX;
	int top = curInfo->offsetY;
	int right = left + curInfo->showWidth;
	int bottom = top + curInfo->showHeight;
	if (x >= left && x <= right && y >= top && y <= bottom) {
		data->cur.SetVisable(1);
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
	LOLayerDataBase *curInfo = &data->cur;

	int cx = curInfo->centerX;
	int cy = curInfo->centerY;
	int ofx = curInfo->offsetX;
	int ofy = curInfo->offsetY;
	if (curInfo->texture) {
		ofx += curInfo->texture->Xfix;
		ofy += curInfo->texture->Yfix;
	}

	if (data->cur.isShowRotate() && abs(M_PI*curInfo->rotate) > 0.00001) {
		if (cx == -1) cx = curInfo->texture->W() / 2;
		if (cy == -1) cy = curInfo->texture->H() / 2;
		if (data->cur.isShowScale()) {
			//M = T * R * S * -T
			tmp = LOMatrix2d::GetMoveMatrix(-cx, -cy);
			mat = LOMatrix2d::GetScaleMatrix(curInfo->scaleX,curInfo->scaleY);
			mat = mat * tmp;
			tmp = LOMatrix2d::GetRotateMatrix(-curInfo->rotate);
			mat = tmp * mat;
			tmp = LOMatrix2d::GetMoveMatrix(ofx, ofy);
			mat = tmp * mat;
		}
		else {
			//M = T * R * -T
			tmp = LOMatrix2d::GetMoveMatrix(-cx, -cy);
			mat = LOMatrix2d::GetRotateMatrix(-curInfo->rotate);
			mat = mat * tmp;
			tmp = LOMatrix2d::GetMoveMatrix(ofx, ofy);
			mat = tmp * mat;
		}
	}
	else if (data->cur.isShowScale()) {
		if (cx == -1) cx = curInfo->texture->W() / 2;
		if (cy == -1) cy = curInfo->texture->H() / 2;
		//缩放
		mat.Set(0, 0, curInfo->scaleX);
		mat.Set(1, 1, curInfo->scaleY);
		mat.Set(0, 2, ofx - cx * curInfo->scaleX);
		mat.Set(1, 2, ofy - cy * curInfo->scaleY);
	}
	else {
		mat.Set(0, 2, ofx);
		mat.Set(1, 2, ofy);
	}
	return mat;
}

void LOLayer::GetInheritScale(double *sx, double *sy) {
	*sx = data->cur.scaleX;
	*sy = data->cur.scaleY;
	LOLayer *lyr = this->parent;
	while (lyr) {
		*sx *= lyr->data->cur.scaleX;
		*sy *= lyr->data->cur.scaleY;
		lyr = lyr->parent;
	}
}

void LOLayer::GetInheritOffset(float *ox, float *oy) {
	LOLayerDataBase *curInfo = &data->cur;
	*ox = curInfo->offsetX;
	*oy = curInfo->offsetY;
	if (curInfo->texture) {
		*ox += curInfo->texture->Xfix;
		*oy += curInfo->texture->Yfix;
	}
	LOLayer *lyr = this->parent;
	while (lyr) {
		*ox += lyr->data->cur.offsetX ;
		*oy += lyr->data->cur.offsetY;
		if (lyr->data->cur.texture) {
			*ox += lyr->data->cur.texture->Xfix;
			*oy += lyr->data->cur.texture->Yfix;
		}
		lyr = lyr->parent;
	}
}

bool LOLayer::isFaterCopyEx() {
	LOLayer *lyr = this->parent;
	while (lyr) {
		if (lyr->data->cur.isShowRotate() || lyr->data->cur.isShowScale())return true;
		lyr = lyr->parent;
	}
	return false;
}

void LOLayer::GetShowSrc(SDL_Rect *srcR) {
	if (data->cur.isShowRect()) {
		data->cur.GetSimpleSrc(srcR);
	}
	else {
		srcR->x = 0; srcR->y = 0;
		srcR->w = data->cur.texture->baseW();
		srcR->h = data->cur.texture->baseH();
	}
}


bool LOLayer::isVisible() {
	if (data->cur.alpha == 0 || !data->cur.isVisiable()) return false ;
	else return true;
}

bool LOLayer::isChildVisible() {
	return data->cur.isChildVisiable();
}


void LOLayer::UpDataToForce() {
	bool isnew = data->bak.isNewFile();
	data->UpdataToForce();
	if (isnew) {
		LOLayer::LinkLayerLeve(this);
		isinit = false;
	}
}


void LOLayer::ShowMe(SDL_Renderer *render) {
	/*bool debugbreak = false ;*/
	//全透明或者不可见，不渲染对象
	//LONS::printInfo("alpha:%d visiable:%d", ptr[LOLayerInfo::alpha], ptr[LOLayerInfo::visiable]);
//    if(id[0] == LOLayer::IDEX_BG_BTNEND){
//        int bbk = 1 ;
//    }

	if (!isVisible()) return;
//    if (layerType == LOLayer::LAYER_SPRINT && id[0] == 999) {
//        int bbk = 1;
//    }

	LOLayerDataBase *curInfo = &data->cur;

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
	if (!curInfo->texture->isCmdTexture() && !curInfo->texture->GetTexture()) return;

	if (data->cur.isShowScale() || data->cur.isShowRotate()) is_ex = true;
	if (isFaterCopyEx()) is_ex = true;

	//采用绘图命令的
	if (curInfo->texture->isCmdTexture()) {
        if (is_ex){
            //转换为纹理实体
            curInfo->texture->AddDrawCanvas( (int)curInfo->offsetX,  (int)curInfo->offsetY) ;
            ShowMeCmd(render, curInfo, curInfo->texture.get());
            //FatalError("CmdTexture can't be scale or rotation!");
        }
        else{
            ShowMeCmd(render, curInfo, nullptr);   //显示绘图命令
            return;
        }
	}

	curInfo->texture->setForceAplha(curInfo->alpha);
	curInfo->texture->activeFlagControl();

	SDL_Vertex vec[6];
	int bw = curInfo->texture->baseW();
	int bh = curInfo->texture->baseH();
	if (bw == 0 || bh == 0) return;

	if (is_ex) {
		//有旋转和缩放则计算出当前的变换矩阵
		matrix = GetTranzMatrix();
		matrix = parent->matrix * matrix;  //继承父对象的变换
		//if(!(parent->parent)){
		//	LOMatrix2d tmp = LOMatrix2d::GetScaleMatrix(G_gameScaleX, G_gameScaleY) ; //继承游戏画面缩放
		//	matrix = tmp * matrix ;
		//}
		double sx, sy;
		GetInheritScale(&sx, &sy);
		dst.x = 0.0f;
		dst.y = 0.0f;
		dst.w = src.w * sx;
		dst.h = src.h * sy;
		//计算出四个点的位置-->这里的规则是左上，右上，左下，右下
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

		//从SDL2.0.16版本起，SDL支持渲染任意形状了，我们不再需要特定函数
		//if(debugbreak) LOLog_i("dst positon fps is %f,%f,%f,%f",dpt[0],dpt[1],dpt[2],dpt[3]) ;
		//If you do not implement this function, the return value is less than 0 and will not be displayed
		//check it,please
		//SDL_RenderCopyLONS(render, curInfo->texture->GetTexture(), &src, &dst, fpt);
		//上面计算的结果是左上，右上，左下，右下，渲染要求逆时针
		vec[0].position = { (float)dpt[0], (float)dpt[1] };
		vec[3].position = { (float)dpt[2], (float)dpt[3] };
		vec[1].position = { (float)dpt[4], (float)dpt[5] };
		vec[2].position = { (float)dpt[6], (float)dpt[7] };
	}
	else {
		GetInheritOffset(&dst.x, &dst.y);
		matrix.Set(0, 2, dst.x);
		matrix.Set(1, 2, dst.y);
		dst.w = src.w;
		dst.h = src.h;
		//if(debugbreak) LOLog_i("dst positon is %d,%d,%d,%d",dst.x,dst.y,dst.w,dst.h) ;
		//SDL_RenderCopyF(render, curInfo->texture->GetTexture(), &src, &dst);

		vec[0].position = { dst.x, dst.y };
		vec[1].position = { dst.x, dst.y + dst.h };
		vec[2].position = { dst.x + dst.w, dst.y + dst.h };
		vec[3].position = { dst.x + dst.w, dst.y };
	}
    //如果是NS动画，可能会出现超大纹理，这时候纹理可能是被裁剪下来的，计算的宽度/高度不一样
    if(curInfo->texture->isSuperClip()){
        curInfo->texture->getSuperClipSize(&bw, &bh) ;
        src.x = src.y = 0 ;
    }
	vec[0].tex_coord = { (float)src.x / bw , (float)src.y / bh };
	vec[1].tex_coord = { (float)src.x / bw , (float)(src.y + src.h) / bh };
	vec[2].tex_coord = { (float)(src.x + src.w) / bw , (float)(src.y + src.h) / bh };
	vec[3].tex_coord = { (float)(src.x + src.w) / bw , (float)src.y / bh };

	vec[4] = vec[0];
	vec[5] = vec[2];

	RenderBaseA(render, curInfo->texture.get(), vec, 6);
}


void LOLayer::ShowMeCmd(SDL_Renderer *render, LOLayerDataBase *curInfo, LOtexture *tex) {
	Uint8 r, g, b, a;
	SDL_FRect dst;
	int alpha;

	SDL_GetRenderDrawColor(render, &r, &g, &b, &a);
    //绘制到指定纹理上
    SDL_Texture *target = SDL_GetRenderTarget(render);
    if(tex){
        SDL_SetRenderTarget(render, tex->GetTexture()) ;
        SDL_SetRenderDrawColor(render, 0,0,0,0);
        SDL_RenderClear(render);
    }

	for (int ii = 0; ii < curInfo->texture->cmdList.size(); ii++) {
		LOtexture::CmdData *cmd = &curInfo->texture->cmdList.at(ii);
		//第几格动画就是第几分组
		if (cmd->grounp == curInfo->cellNum) {
			//透明度相当于画笔的透明乘上纹理设定的透明
			alpha = curInfo->alpha;
			if (alpha < 0) alpha = 255;
			alpha = alpha * cmd->drawColor.a / 255;
			if (alpha > 255) alpha = 255;
			else if (alpha < 0) alpha = 0;

			SDL_SetRenderDrawColor(render, cmd->drawColor.r, cmd->drawColor.g, cmd->drawColor.b, alpha & 0xff);
			SDL_SetRenderDrawBlendMode(render, SDL_BLENDMODE_BLEND);

			//是否有blendmod
			int flag = curInfo->texture->GetFlag();
			if (flag & LOtexture::USE_BLEND_MOD) {
				SDL_SetRenderDrawBlendMode(render, curInfo->texture->GetBlendMode());
			}

			//是否有blendcolor，直接绘图不支持color mod

			switch (cmd->cmd){
			case LOtexture::CMD_DRAW_FILL:
				//x y 填充为实际的偏移
				//dst.x = cmd->A[0]; dst.y = cmd->A[1];
                if(tex){
                    //绘制到纹理不需要
                    dst.x = 0.0f ; dst.y = 0.0f ;
                }
                else{
                    //绘制到渲染器需要加上偏移
                    dst.x = curInfo->offsetX; dst.y = curInfo->offsetY;
                }
				dst.w = cmd->B[0]; dst.h = cmd->B[1];
				SDL_RenderFillRectF(render, &dst);
				break;
			default:
				break;
			}
		}
	}

    if(tex){
        SDL_SetRenderTarget(render, target) ;
    }
	SDL_SetRenderDrawColor(render, r, g, b, a);
}


//统一的渲染函数，要求提供从左上角起，逆时针的四个点坐标，分为两个三角形
void LOLayer::RenderBaseA(SDL_Renderer *render, LOtexture *tex, SDL_Vertex *vec, int vecCount) {
	SDL_Texture *stex = tex->GetTexture();
	//当使用SDL_RenderGeometry时，SDL_SetTextureColorMod和SDL_SetTextureAlphaMod将失效
	//因此需要一个替代
	Uint8 r, g, b, a;
	SDL_GetTextureAlphaMod(stex, &a);
	SDL_GetTextureColorMod(stex, &r, &g, &b);
	SDL_Color color = { r, g, b, a };

	for (int ii = 0; ii < vecCount; ii++) vec[ii].color = color;

	/*
	//p1
	vers[0].position = { pos[0], pos[1] };
	vers[0].color = color;
	vers[0].tex_coord = { 0.0f, 0.0f };
	//p2
	vers[1].position = { pos[2], pos[3] };
	vers[1].color = color;
	vers[1].tex_coord = { 0.0f, 1.0f };
	//p3
	vers[2].position = { pos[4], pos[5] };
	vers[2].color = color;
	vers[2].tex_coord = { 1.0f, 1.0f };
	//p4
	vers[3].position = { pos[6], pos[7] };
	vers[3].color = color;
	vers[3].tex_coord = { 1.0f, 0.0f };
	vers[4] = vers[0];
	vers[5] = vers[2];
	*/
	//SDL_RenderGeometry使用三角形列表，vertex的数量必须是3的倍数
	SDL_RenderGeometry(render, stex, vec, vecCount, nullptr, 0);
}


void LOLayer::SerializeForce(BinArray *bin) {

	//fid很重要，最先读取，以便产生新建对象
	bin->WriteInt(data->fullid);

	//if (layerType == LOLayer::LAYER_DIALOG && id[0] == LOLayer::IDEX_DIALOG_TEXT) {
	//	LOActionText *ac =  (LOActionText*)data->cur.GetAction(LOAction::ANIM_TEXT);
	//	int bbk = 1;
	//}

	int len = bin->WriteLpksEntity("fdat", 0, 1);
	if (data->cur.isNothing()) bin->WriteChar(0);
	else {
		bin->WriteChar(1);
		data->cur.Serialize(bin);
	}
	//子对象的关系
	if (childs) {
		bin->WriteInt(childs->size());
		for (auto iter = childs->begin(); iter != childs->end(); iter++) {
			iter->second->SerializeForce(bin);
		}
	}
	else bin->WriteInt(0);

	bin->WriteInt(bin->Length() - len, &len);
}


bool LOLayer::DeSerializeForce(BinArray *bin, int *pos) {
	int next = -1;
	if (!bin->CheckEntity("fdat", &next, nullptr, pos)) return false;

	if (layerType == LOLayer::LAYER_DIALOG && id[0] == LOLayer::IDEX_DIALOG_TEXT) {
		int bbk = 1;
	}

	if (bin->GetChar(pos) != 0) {
		if (!data->cur.DeSerialize(bin, pos)) return false;
	}

	//子对象，应该保证layer是刚生成的
	int count = bin->GetIntAuto(pos);
	if(count > 0 && !childs) childs = new std::map<int, LOLayer*>();
	for (int ii = 0; ii < count; ii++) {
		int fid = bin->GetIntAuto(pos);
		LOLayer *lyr = CreateLayer(fid);
		if (!lyr->DeSerializeForce(bin, pos)) return false;
		//连接父对象
		lyr->parent = this;
		(*childs)[lyr->GetSelfChildID()] = lyr;
	}

	*pos = next;
	return true;
}


void LOLayer::SerializeBak(BinArray *bin) {
	//fid很重要
	bin->WriteInt(data->fullid);
	//if (data->fullid == 0x0005ffff) {
	//	int bbk = 1;
	//}

	int len = bin->WriteLpksEntity("bdat", 0, 1);
	if (data->bak.isNothing()) bin->WriteChar(0);
	else {
		bin->WriteChar(1);
		data->bak.Serialize(bin);
	}
	//后台数据不关注子对象
	bin->WriteInt(bin->Length() - len, &len);
}


bool LOLayer::DeSerializeBak(BinArray *bin, int *pos) {
	int next = -1;
	if (!bin->CheckEntity("bdat", &next, nullptr, pos)) return false;
	if (bin->GetChar(pos) != 0) {
		//if (id[0] == 11) {
		//	int bbk = 0;
		//}
		if(!data->bak.DeSerialize(bin, pos)) return false;
	}
	*pos = next;
	return true;
}


void LOLayer::DoAction(LOLayerData *data, Uint32 curTime) {
	auto *acions = data->cur.actions.get();
	if (!acions) return;
	for (int ii = 0; ii < acions->size(); ii++) {
		LOShareAction acb = acions->at(ii);
		if (acb->isEnble()) {
			switch (acb->acType) {
			case LOAction::ANIM_NSANIM:
				DoNsAction(data, (LOActionNS*)acb.get(), curTime);
				break;
			case LOAction::ANIM_TEXT:
				DoTextAction(data, (LOActionText*)acb.get(), curTime);
				break;
			case LOAction::ANIM_VIDEO:
				DoMovieAction(data, (LOActionMovie*)acb.get(), curTime);
				break;
            case LOAction::ANIM_MOVE:
                DoMoveActon(data, (LOActionMove*)acb.get(), curTime);
                break ;
            case LOAction::ANIM_SCALE:
                DoScaleAction(data, (LOActionScale*)acb.get(), curTime);
                break ;
			default:
				break;
			}
		}
	}
}


//帧更新函数，播放过程中不断的更新frameCount
void SmpegUpdateFrame(void *data, SMPEG_Frame *frame){
	SmpegContext *context = (SmpegContext *)data;
	context->frame = frame;
	context->frameCount++;
}

void LOLayer::DoMovieAction(LOLayerData *data, LOActionMovie *ai, Uint32 curTime) {
	if (ai->isInit()) { //初始化
		//对齐?从plaympeg.c中抄来的
		int fw = (ai->info.width + 15) & ~15;
		int fh = (ai->info.height + 15) & ~15;
		int dw = data->cur.showWidth;
		int dh = data->cur.showHeight;
		data->cur.texture->CreateDstTexture2(fw, fh, SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STREAMING);
		//源显示位置
		data->cur.SetShowRect(0, 0, ai->info.width, ai->info.height);
		//目标大小和位置
		if (data->cur.showWidth == 0 || data->cur.showHeight == 0) {
			data->cur.SetPosition(0, 0);
			data->cur.showHeight = G_gameWidth; data->cur.showHeight = G_gameHeight;
		}
		//设置好缩放参数
		if (dw <= 0) dw = G_gameWidth;
		if (dh <= 0) dh = G_gameHeight;
		if (abs(dw - data->cur.showWidth) >= 0.0001 || abs(dh - data->cur.showHeight) >= 0.0001) {
			data->cur.SetPosition2(data->cur.offsetX, data->cur.offsetY, dw / data->cur.showWidth, dh / data->cur.showHeight);
		}
		ai->unSetFlags(LOAction::FLAGS_INIT);
		//开始播放
		SMPEG_setdisplay(ai->mpeg, SmpegUpdateFrame, &ai->context, ai->context.lock);
		SMPEG_play(ai->mpeg);
	}
	else {
		if (SMPEG_status(ai->mpeg) == SMPEG_PLAYING) {
			//帧还没更新
			if (ai->context.frameCount <= ai->frameID) return;
			//更新纹理
			SDL_LockMutex(ai->context.lock);
			//检查宽高是否正确，因为是直接复制的，防止越界
			
			if (ai->context.frame->image_width != data->cur.texture->baseW() || !ai->context.frame ||
				ai->context.frame->image_height != data->cur.texture->baseH()) {
				FatalError("When playing mpeg, get an error!");
				ai->setEnble(false);
				SMPEG_stop(ai->mpeg);
			}
			else {
				SDL_UpdateTexture(data->cur.texture->GetTexture(), NULL, ai->context.frame->image, 
					ai->context.frame->image_width);
			}
			SDL_UnlockMutex(ai->context.lock);
			ai->frameID = ai->context.frameCount;
		}
		else {
			//播放完成了
			ai->setEnble(false);
			SMPEG_stop(ai->mpeg);
			//播放完成事件
			LOShareEventHook ev(LOEventHook::CreateVideoFinish(data->fullid));
			AddRenderEvent(ev);
		}
	}
}

void LOLayer::DoNsAction(LOLayerData *data, LOActionNS *ai, Uint32 curTime) {
	if (curTime - ai->lastTime > ai->cellTimes[ai->cellCurrent]) {
		ai->lastTime = curTime;
		int cell = ai->cellCurrent;
//        if(cell >= 25){
//            int bbk = 1 ;
//        }

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


        //超大纹理每一帧都必须进行初始化
        if(cell != ai->cellCurrent && !data->cur.texture->isCmdTexture() && data->cur.texture->isSuperClip()){
            isinit = false ;
        }
		data->cur.SetCell(ai, cell);
	}
}


void LOLayer::DoTextAction(LOLayerData *data, LOActionText *ai, Uint32 curTime) {
	LOShareTexture texture = data->cur.texture;
	if (ai->isInit()) {
		SDL_Rect srcR = { 0,0, texture->baseW(), texture->baseH() };
		texture->activeTexture(&srcR, true);
		texture->RollTextTexture(0, ai->initPos);
		ai->currentPos = ai->initPos;
		ai->lastTime = curTime;
		ai->unSetFlags(LOAction::FLAGS_INIT);
		//初始化后才会相应左键跳过事件
		if (ai->hook) {
			ai->hook->catchFlag = LOEventHook::ANSWER_LEFTCLICK | LOEventHook::ANSWER_PRINGJMP;
			G_hookQue.push_H_back(ai->hook);
		}
	}
	else {
		int posPx = ai->perPix * (curTime - ai->lastTime);
        //如果是直接cut的，要注意不要溢出了
        if(curTime == 0x7ffffff) posPx = 0x7ffffff;

		if (posPx > 1) {
			//posPx = 1;
			int ret = texture->RollTextTexture(ai->currentPos, ai->currentPos + posPx);
			//printf("currentPos:%d\n", ai->currentPos);
			if (ret == LOtexture::RET_ROLL_END) {
				ai->setEnble(false);
				//添加帧刷新后事件
				if(ai->hook) AddPreEvent(ai->hook);
				//SDL_Log("text finish");
				ai->currentPos += posPx;
			}
			else if (ret == LOtexture::RET_ROLL_CONTINUE) ai->currentPos += posPx;
			else ai->setEnble(false);  //发送错误就不能继续了
			ai->lastTime = curTime;
		}
	}
}


void LOLayer::DoMoveActon(LOLayerData *data, LOActionMove *ai, Uint32 curTime){
    //首帧
    if(ai->lastTime == 0){
        data->cur.offsetX = ai->startPt.x ;   data->cur.offsetY = ai->startPt.y ;
        ai->lastTime = curTime ;
        return ;
    }
    Uint32 pos = curTime - ai->lastTime;

    //ai->gVal += 1 ;
    //pos = 20 * ai->gVal ;

    if(pos > ai->duration) pos = ai->duration ;
    //计算出X，Y的位置
    double perx = (double)(ai->endPt.x - ai->startPt.x) / ai->duration * pos ;
    double pery = (double)(ai->endPt.y - ai->startPt.y) / ai->duration * pos ;

    data->cur.offsetX = ai->startPt.x + (float)perx ;
    data->cur.offsetY = ai->startPt.y + (float)pery ;

    //printf("%f\n", data->cur.offsetY) ;

    if(pos >= ai->duration){
        if(ai->loopMode == LOAction::LOOP_CIRCULAR) ai->lastTime = 0 ; //重置
        else if(ai->loopMode == LOAction::LOOP_GOBACK){
            SDL_Point tmp = ai->endPt;
            ai->endPt = ai->startPt;
            ai->startPt = tmp ;
            ai->lastTime = 0 ;
        }
        else ai->setEnble(false) ;
    }

}


void LOLayer::DoScaleAction(LOLayerData *data, LOActionScale *ai, Uint32 curTime){
    data->cur.showType |= LOLayerDataBase::SHOW_SCALE;
    if(ai->lastTime == 0){ //首帧
        data->cur.scaleX = (double)ai->startSc.x / 1000 ; data->cur.scaleY = (double)ai->startSc.y / 1000 ;
        ai->lastTime = curTime ;
        return ;
    }
    Uint32 pos = curTime - ai->lastTime;
    //ai->gVal += 1 ;
    //pos = 20 * ai->gVal ;
    if(pos > ai->duration) pos = ai->duration ;
    //计算出x y应该的缩放
    int duration = (int)ai->duration ;
    int dx = (ai->endSc.x - ai->startSc.x) * (int)pos / duration ;
    int dy = (ai->endSc.y - ai->startSc.y) * (int)pos / duration ;
    data->cur.scaleX = (double)(ai->startSc.x + dx) / 1000;
    data->cur.scaleY = (double)(ai->startSc.x + dy) / 1000;
    //循环
    if(pos >= ai->duration){
        if(ai->loopMode == LOAction::LOOP_CIRCULAR) ai->lastTime = 0 ; //重置
        else if(ai->loopMode == LOAction::LOOP_GOBACK){
            SDL_Point tmp = ai->endSc;
            ai->endSc = ai->startSc;
            ai->startSc = tmp ;
            ai->lastTime = 0 ;
            //ai->gVal = 0;  //debug
        }
        else ai->setEnble(false) ;
    }
}


void LOLayer::GetLayerPosition(int *xx, int *yy, int *aph) {
	LOLayerDataBase *curInfo = &data->cur;
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
	LOLayer *father = DescentFather(G_baseLayer[lyrType], &index, ids);
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
	int xx = e->GetParam(0)->GetInt();
	int yy = e->GetParam(1)->GetInt();


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
	if (!(e->catchFlag & data->cur.flags)) return ret;
	//左键、右键、长按、悬停
	if (e->catchFlag & LOLayerDataBase::FLAGS_RIGHTCLICK) {
		//只有按钮才相应右键事件
		LOShareEventHook ev(LOEventHook::CreateBtnClickEvent(GetFullID(layerType, id), -1, 0));
		aswerQue->push_N_back(ev);
	}
	else {
		//左键、长按、悬停，如果是按钮均会触发按钮类操作
		if (isPositionInsideMe(e->GetParam(0)->GetInt(), e->GetParam(1)->GetInt())) {
			//按钮处理
			if (data->cur.isBtndef()) {
				//鼠标已经离开对象
				if (e->param1 & LOEventHook::BTN_STATE_ACTIVED) {
					setBtnShow(false);
					//LOLog_i("is:%d", id[0]);
					e->param1 |= LOEventHook::BTN_STATE_UNACTIVED;
				}
				else {
					//鼠标进入对象
					//已经激活的图层不会再次激活
					if (!data->cur.isActive()) {
						setBtnShow(true);
						//产生btnstr事件
						if (data->cur.btnStr) {
							LOShareEventHook ev(LOEventHook::CreateSpStrEvent(GetFullID(layerType, id), data->cur.btnStr.get()));
							aswerQue->push_N_back(ev);
						}
					}
					//已经处理active事件
					e->param1 |= LOEventHook::BTN_STATE_ACTIVED;

					//左键和长按会进一步产生按钮响应点击事件
					if (e->catchFlag & (LOLayerDataBase::FLAGS_LONGCLICK | LOLayerDataBase::FLAGS_LEFTCLICK)) {
						LOShareEventHook ev(LOEventHook::CreateBtnClickEvent(GetFullID(layerType, id), data->cur.btnval, 0));
						//printf("click catch at %d\n", SDL_GetTicks());
						aswerQue->push_N_back(ev);
					}
				}
			}
			else {
				//非按钮产生图层响应事件
			}
		}
		else {
			//按钮处理
			if (data->cur.isBtndef() && data->cur.isActive()) {
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
		data->cur.SetVisable(1);
		data->cur.flags |= LOLayerDataBase::FLAGS_ACTIVE;
	}
	else {
		if (!setActiveCell(0))data->cur.SetVisable(0);
		data->cur.flags &= (~LOLayerDataBase::FLAGS_ACTIVE);
	}
}

bool LOLayer::setActiveCell(int cell) {
	bool ret = data->cur.SetCell(nullptr, cell);
	//下次刷新图层时自动更新
	if (ret) isinit = false;
	return ret;
}


//删除图层，这里留了一个接口
//void LOLayer::NoUseLayer(LOLayer *lyr) {
//	int fullid = GetFullID(lyr->layerType, lyr->id);
//	layerCenter.erase(fullid);
//	delete lyr;
//}

//图层前台已经不需要了，如果后退不是新文件，则后退应该被删除
//断开图层间的连接
void LOLayer::NoUseLayerForce(LOLayer *lyr) {
	lyr->data->cur.resetBase();
	if (lyr->parent && lyr->parent->childs) lyr->parent->RemodeChild(lyr->GetSelfChildID());
	lyr->parent = nullptr;
	if (lyr->childs) {
		for (auto iter = lyr->childs->begin(); iter != lyr->childs->end(); iter++) {
			LOLayer *layer = iter->second;
			layer->parent = nullptr;
			NoUseLayerForce(layer);
		}
		lyr->childs->clear();
	}
	//如果后台不是新文件，则删除图层
	if (!lyr->data->bak.isNewFile()) {
		LOLayer::layerCenter.erase(lyr->data->fullid);
		delete lyr;
	}
}

void LOLayer::NoUseLayerForce(int fid) {
	LOLayer *lyr = LOLayer::GetLayer(fid);
	if (lyr) NoUseLayerForce(lyr);
}

LOLayer* LOLayer::CreateLayer(int fullid) {
	LOLayer *lyr = FindLayerInCenter(fullid);
	if (lyr) return lyr;
	lyr = new LOLayer(fullid);
	layerCenter[fullid] = lyr;
	return lyr;
}

LOLayer* LOLayer::FindLayerInCenter(int fullid) {
	auto iter = layerCenter.find(fullid);
	if (iter != layerCenter.end()) return iter->second;
	return nullptr;
}

LOLayer* LOLayer::GetLayer(int fullid) {
	auto iter = layerCenter.find(fullid);
	if (iter != layerCenter.end()) return iter->second;
	return nullptr;
}

//将图层挂载到图层组上
LOLayer* LOLayer::LinkLayerLeve(LOLayer *lyr) {
	int fullid = GetFullID(lyr->layerType, lyr->id);
	int index = 0;
	LOLayer *father = DescentFather(lyr->rootLyr, &index, lyr->id);
	if (!father) {
        SDL_LogError(0, "LOLayer::LinkLayerLeve() faild! no father!");
		return nullptr;
	}
	if (!father->InserChild(lyr->id[index], lyr)) {
        SDL_LogError(0, "LOLayer::InserChild() faild! There is already another object:%d", lyr->id[index]);
		return nullptr;
	}
	return father;
}


//获取自身相对于parent的ID值
int LOLayer::GetSelfChildID() {
	if (id[1] == G_maxLayerCount[1]) return id[0];
	if (id[2] == G_maxLayerCount[1]) return id[1];
	return id[2];
}


////只有被改变才返回真
//bool LOLayer::setActive(bool isactive) {
//
//}


////存储所有layerCenter中的文件
//void LOLayer::SaveLayer(BinArray *bin) {
//	//LYRS,len, version
//	int len = bin->WriteLpksEntity("LYRS", 0, 1);
//	
//	bin->WriteInt(layerCenter.size());
//	//从图层中心获取图层
//	for (auto iter = layerCenter.begin(); iter != layerCenter.end(); iter++) {
//		auto childs = iter->second->childs;
//		if (childs) {
//			for (auto iter = childs->begin(); iter != childs->end(); iter++) {
//				iter->second->Serialize(bin);
//			}
//		}
//	}
//
//	bin->WriteInt(bin->Length() - len, &len);
//}



void LOLayer::InitBaseLayer() {
	for (int ii = 0; ii < LOLayer::LAYER_BASE_COUNT; ii++) {
		int fullid = GetFullID(LOLayer::LAYER_CC_USE, ii, 255, 255);
		G_baseLayer[ii] = LOLayer::CreateLayer(fullid);
	}
}

//重置图层文件
void LOLayer::ResetLayer() {
	//根图层也已经被删除了
	for (auto iter = layerCenter.begin(); iter != layerCenter.end(); iter++) {
		iter->second->parent = nullptr;
		//删除之前先断开图层关系，防止重复删除
		if (iter->second->childs) iter->second->childs->clear();
		delete iter->second;
	}
	layerCenter.clear();
	//重新创建根图层
	InitBaseLayer();
}

