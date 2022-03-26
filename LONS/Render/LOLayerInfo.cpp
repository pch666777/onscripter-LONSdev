#include "LOLayerInfo.h"

char G_Bit[4] = {0, 1, 2, 3};  //默认材质所只用的RGBA模式位置 0-A 1-R 2-G 3-B
Uint32 G_Texture_format = SDL_PIXELFORMAT_ARGB8888;   //windows android use this format;   //默认编辑使用的材质格式
SDL_Rect G_windowRect = {0, 0, 800, 600};    //整个窗口的位置和大小
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

void GetFormatBit(SDL_PixelFormat *format, char *bit) {
	//R G B A
	Uint32 color = SDL_MapRGBA(format, 1, 2, 3, 4);
	char *cco = (char*)(&color);
	for (int ii = 0; ii < 4; ii++) {
		for (int kk = 0; kk < 4; kk++) {
			if (cco[kk] == ii + 1) bit[ii] = kk;
		}
	}
}

void GetFormatBit(Uint32 format, char *bit) {
	SDL_PixelFormat *fm = SDL_AllocFormat(format);
	GetFormatBit(fm, bit);
	SDL_FreeFormat(fm);
}

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
	if(type) *type = (fullid >> 26) & 0x3F;
    if(ids){
        ids[0] = (fullid >> 16) & 0x3FF;
        ids[1] = (fullid >> 8) & 0xFF;
        ids[2] = fullid & 0xFF;
    }
}

//================ class ==================


LOLayerInfo::LOLayerInfo()
{
	Reset();
	actions = nullptr;
	texture = nullptr;
	fileName = nullptr;
	maskName = nullptr;
	textStr = nullptr;
	btnStr = nullptr;
}

LOLayerInfo::~LOLayerInfo(){
	if (fileName) delete fileName;
	if (maskName) delete maskName;
	if (btnStr) delete btnStr;
	if (textStr) delete textStr;
}

void LOLayerInfo::Reset() {
	offsetX = 0;
	offsetY = 0;    //显示目标左上角位置
	alpha = -1;
	//alphaMode = 0;  //透明模式
	centerX = 0;    //锚点，缩放旋转时有用
	centerY = 0;
	showSrcX = 0;  //显示区域的X偏移
	showSrcY = 0;  //显示区域的Y偏移
	showWidth = 0;  //显示区域的宽度
	showHeight = 0;  //显示区域的高度
	showType = 0;   //0是简单模式，对应lsp，1为简单+旋转  2为简单+旋转+缩放
	visiable = 1;
	tWidth = 0;
	tHeight = 0;
	scaleX = 1.0f;   //X方向的缩放
	scaleY = 1.0f;	//y方向的缩放
	rotate = 0;	//旋转，角度

	loadType = LOtexture::TEX_NONE ;
	usecache = 0;

	layerControl = CON_NONE;
	fullid = 0;
}


void LOLayerInfo::ResetOther() {
	if (texture) delete texture;
	if (actions) LOAnimation::FreeAnimaList(&actions);
	if (fileName) delete fileName;
	if (maskName) delete maskName;
	if (btnStr) delete btnStr;
	if (textStr) delete textStr;
	fileName = NULL;
	maskName = NULL;
	btnStr = NULL;
	textStr = NULL;
	actions = NULL;
	texture = NULL;
}

void LOLayerInfo::CopyConWordFrom(LOLayerInfo* info, int keyword, bool ismove) {
	if (keyword & CON_UPPOS) {
		offsetX = info->offsetX;
		offsetY = info->offsetY;
	}
	if (keyword & CON_UPPOS2) {
		centerX = info->centerX;
		centerY = info->centerY;
		scaleX = info->scaleX;
		scaleY = info->scaleY;
		rotate = info->rotate;
	}
	if (keyword & CON_UPVISIABLE) visiable = info->visiable;
	if (keyword & CON_UPAPLHA)alpha = info->alpha;
	if (keyword & CON_UPSHOWRECT) {
		showSrcX = info->showSrcX;  //显示区域的X偏移
		showSrcY = info->showSrcY;  //显示区域的Y偏移
		showWidth = info->showWidth;  //显示区域的宽度
		showHeight = info->showHeight;  //显示区域的高度
	}
	if (keyword & CON_UPSHOWTYPE) showType = info->showType;
	if (keyword & CON_UPFILE) {
		tWidth = info->tWidth;
		tHeight = info->tHeight;
		texture = info->texture;
		if(ismove)info->texture = nullptr;  //材质转移

		LOString::SetStr(fileName, info->fileName, ismove);
		LOString::SetStr(maskName, info->maskName, ismove);
		LOString::SetStr(textStr, info->textStr, ismove);
	}
	if (keyword & CON_UPBTN) {
		btnValue = info->btnValue;
		LOString::SetStr(btnStr, info->btnStr, ismove);
	}
	if (keyword & CON_UPCELL) {
		cellNum = info->cellNum;
	}

	if (keyword == -1)fullid = info->fullid;
}


//不会释放原有的记录
bool LOLayerInfo::CopyActionFrom(LOStack<LOAnimation> *&src, bool ismove) {
	if (!src) return true;
	for (auto iter = src->begin(); iter != src->end(); iter++) {
		LOAnimation *aib = (*iter);
		if (aib->control == LOAnimation::CON_REPLACE) ReplaceAction(aib);
		else return false;
	}
	if (ismove) {
		src->clear();
		delete src;
		src = nullptr;
	}
	return true;
}


//在材质缓存中,fileName表现为 透明样式;文件名
const char* LOLayerInfo::BaseFileName() {
	if (fileName)return fileName->c_str() + 2;
	return nullptr;
}

//在材质缓存中,maskName表现为 透明样式;文件名
const char* LOLayerInfo::BaseMaskName() {
	if (maskName)return maskName->c_str() + 2;
	return nullptr;
}

void LOLayerInfo::GetSimpleDst(SDL_Rect *dst) {
	dst->x = offsetX; dst->y =offsetY;
	dst->w = texture->baseW(); dst->h = texture->baseH();
}

void LOLayerInfo::GetSimpleSrc(SDL_Rect *src) {
	src->x = showSrcX; src->y = showSrcY;
	src->w = showWidth; src->h = showHeight;
}

LOAnimation* LOLayerInfo::GetAnimation(int type) {
	if (!actions) return nullptr;
	for (int ii = 0; ii < actions->size(); ii++) {
		LOAnimation *aib = actions->at(ii);
		if ((int)aib->type == type) return aib;
	}
	return nullptr;
}

//替换动作组中的相应动作
void LOLayerInfo::ReplaceAction(LOAnimation *ai) {
	if (!actions) actions = new LOStack<LOAnimation>;
	for (int ii = 0; ii < actions->size(); ii++) {
		LOAnimation *aib = actions->at(ii);
		if (aib->type == ai->type) {
			actions->swap(ai, ii);
			delete aib;
			aib = nullptr;
			return;
		}
	}
	actions->push(ai);
}

//获取文字的结尾位置
void LOLayerInfo::GetTextEndPosition(int *xx, int *yy, bool isnewline) {
	int tx = 0;
	int ty = 0;
	if (actions) {
		LOAnimation *aib = GetAnimation(LOAnimation::ANIM_TEXT);
		if (aib) {
			LOAnimationText *ai = (LOAnimationText*)aib;
			LineComposition *line = ai->lineInfo->top();
			tx = line->x + line->sumx;
			if (isnewline) ty = line->y + line->height;
			else ty = line->y;
		}
	}
	if (xx) *xx = tx;
	if (yy) *yy = ty;
}



int LOLayerInfo::GetCellCount() {
	LOAnimation *aib = GetAnimation(LOAnimation::ANIM_NSANIM);
	if (aib) {
		LOAnimationNS *ai = (LOAnimationNS*)aib;
		return ai->cellCount;
	}
	return 1;
}

void LOLayerInfo::GetSize(int &xx, int &yy, int &cell) {
	xx = 0; yy = 0; cell = cellNum + 1;
	if (showType & SHOW_RECT) {
		xx = showWidth;
		yy = showHeight;
	}
	else if (texture) {
		xx = texture->baseW();
		yy = texture->baseH();
	}
	if (xx < 0) xx = 0;
	if (yy < 0) yy = 0;
	if (cell < 1) cell = 1;
}



//将动画的初始信息同步到layerinfo上
void LOLayerInfo::FirstSNC(LOAnimation *ai) {
	if (ai->type == LOAnimation::ANIM_NSANIM) {
		if (texture) {
			LOAnimationNS *aib = (LOAnimationNS*)ai;
			showType |= SHOW_RECT;
			cellNum = aib->cellCurrent;
			showWidth = texture->baseW() / aib->cellCount;
			showHeight = texture->baseH();
		}
	}
	else if (ai->type == LOAnimation::ANIM_MOVE) {
		LOAnimationMove *aib = (LOAnimationMove*)ai;
		offsetX = aib->fromx;
		offsetY = aib->fromy;
	}
	else if (ai->type == LOAnimation::ANIM_SCALE) {
		LOAnimationScale *aib = (LOAnimationScale*)ai;
		showType |= SHOW_SCALE;
		centerX = aib->centerX;
		centerY = aib->centerY;
		scaleX = aib->startPerX;
		scaleY = aib->startPerY;
	}
	else if (ai->type == LOAnimation::ANIM_ROTATE) {
		showType |= SHOW_ROTATE;
		rotate = ((LOAnimationRotate*)ai)->startRo;
	}
	else if (ai->type == LOAnimation::ANIM_FADE) {
		LOAnimationFade *aib = (LOAnimationFade*)ai;
		alpha = aib->startAlpha;
	}
}

void LOLayerInfo::AddAlpha(int val) {
	if (alpha < 0 || alpha > 255) alpha = 255;
	alpha += val;
	if (alpha < 0) alpha = 0;
	if (alpha > 255) alpha = 255;
}

void LOLayerInfo::SetBtn(LOString *btn_s, int btnval) {
	LOString::SetStr(btnStr, btn_s, false);
	btnValue = btnval;
	layerControl |= CON_UPBTN;
}

void LOLayerInfo::SetVisable(int v) {
	visiable = v;
	layerControl |= CON_UPVISIABLE;
}


void LOLayerInfo::SetShowRect(int x, int y, int w, int h) {
	showWidth = w;
	showHeight = h;
	showSrcX = x;
	showSrcY = y;
	showType |= SHOW_RECT;
	layerControl |= CON_UPSHOWRECT;
	layerControl |= CON_UPSHOWTYPE;
}

void LOLayerInfo::SetShowType(int show) {
	//if (isOverWrite) showType = show;
	//else 
	showType |= show;
	layerControl |= CON_UPSHOWTYPE;
}

//对应的资源应该在imageModule释放
void LOLayerInfo::SetLayerDelete() {
	layerControl |= CON_DELLAYER;
}

void LOLayerInfo::SetNewFile(LOtexture *tx) {
	texture = tx;
	layerControl |= CON_UPFILE;
	layerControl &= (~LOLayerInfo::CON_DELLAYER);  //清理delete标记
}

void LOLayerInfo::SetPosition(int ofx, int ofy) {
	offsetX = ofx;
	offsetY = ofy;
	layerControl |= CON_UPPOS;
}

void LOLayerInfo::SetPosition2(int cx, int cy, double sx, double sy) {
	centerX = cx; centerY = cy;
	scaleX = sx; scaleY = sy;
	layerControl |= CON_UPPOS2;
	showType |= SHOW_SCALE;
}

void LOLayerInfo::SetRotate(double ro) {
	rotate = ro;
	layerControl |= CON_UPPOS2;
	showType |= SHOW_ROTATE;
}

void LOLayerInfo::SetAlpha(int alp) {
	alpha = alp & 0xff;
	layerControl |= CON_UPAPLHA;
}

//移除按钮定义
void LOLayerInfo::UnsetBtn() {
	int eflag = ~(LOLayerInfo::CON_UPBTN);
	layerControl &= eflag;
	btnValue = -1;
	if (btnStr) delete btnStr;
	btnStr = nullptr;
}

//upfile或者delete视为产生了新的图层
bool LOLayerInfo::isNewLayer() {
	int eflag = LOLayerInfo::CON_UPFILE | LOLayerInfo::CON_DELLAYER;
	if (layerControl & eflag) return true;
	return false;
}

void LOLayerInfo::SetCell(int ce) {
	cellNum = ce;
	layerControl |= CON_UPCELL;
}


void LOLayerInfo::Serialize(BinArray *sbin) {
	sbin->WriteString("layerinfo");
	sbin->WriteInt(fullid);
	sbin->WriteInt(layerControl);
	sbin->WriteInt(offsetX);
	sbin->WriteInt(offsetY);
	sbin->WriteInt(alpha);
	//sbin->WriteInt(alphaMode);
	sbin->WriteInt(centerX);
	sbin->WriteInt(centerY);
	sbin->WriteInt(showSrcX);
	sbin->WriteInt(showSrcY);
	sbin->WriteInt(showWidth);
	sbin->WriteInt(showHeight);
	sbin->WriteInt(showType);
	sbin->WriteInt(visiable);
	sbin->WriteInt(tWidth);
	sbin->WriteInt(tHeight);
	sbin->WriteInt(btnValue);
	sbin->WriteDouble(scaleX);
	sbin->WriteDouble(scaleY);
	sbin->WriteDouble(rotate);
	//if (fileName) sbin->WriteString(fileName);
	//else sbin->WriteChar(0);
	//if (maskName) sbin->WriteString(maskName);
	//else sbin->WriteChar(0);
	//if (btnStr) sbin->WriteString(btnStr);
	//else sbin->WriteChar(0);
	//if (textStr) sbin->WriteString(textStr);
	//else sbin->WriteChar(0);
	//if (actions) {
	//	sbin->WriteInt(0);
	//}
	//else sbin->WriteInt(0);
}