/*
//纹理
*/

#include "LOTexture.h"

uint16_t LOtextureBase::maxTextureW = 4096;
uint16_t LOtextureBase::maxTextureH = 4096;
SDL_Renderer *LOtextureBase::render = nullptr;  //一般来说只会有一个渲染器
std::unordered_map<std::string, LOtextureBase*> LOtexture::baseMap;

//============== class ============
LOtextureBase::LOtextureBase() {
	NewBase();
}

LOtextureBase::LOtextureBase(LOSurface *surface) {
	NewBase();
	SetSurface(surface);
}

LOtextureBase::LOtextureBase(SDL_Texture *tx) {
	NewBase();
	if (tx) {
		SDL_QueryTexture(tx, NULL, NULL, &ww, &hh);
		AddSizeTexture(0, 0, ww, hh, tx);
	}
}

void LOtextureBase::NewBase() {
	ww = 0;
	hh = 0;
	su = nullptr;
	isbig = false;
	std::atomic_init(&usecount, 0);
}

LOtextureBase::~LOtextureBase() {
	if (su) delete su;
	su = nullptr;
	for (int ii = 0; ii < texlist.size(); ii++) {
		if (ii % 2 == 0) delete ((SDL_Rect*)texlist[ii]);
		else {
			DestroyTexture((SDL_Texture*)texlist[ii]);
		}
	}
	texlist.clear();
}

int LOtextureBase::AddUseCount() {
	usecount.fetch_add(1);
	return usecount.load();
}

int LOtextureBase::SubUseCount() {
	usecount.fetch_sub(1);
	return usecount.load();
}


//初始化的时机很重要，必须在主线程上初始化
void LOtextureBase::Init() {
	SDL_Texture* tx = CreateTextureFromSurface(render, su->GetSurface());
	AddSizeTexture(0, 0, su->W(), su->H(), tx);
	delete su;
	su = nullptr;
}

void LOtextureBase::AddSizeTexture(int x, int y, int w, int h, SDL_Texture *tx) {
	SDL_Rect *rect = new SDL_Rect;
	rect->x = x; rect->y = y;
	rect->w = w; rect->h = h;
	texlist.push_back((intptr_t)rect);
	texlist.push_back((intptr_t)tx);
}

//========================================

//注意，这个函数只能用在主线程，在子线程上可能会卡死
//非超大型纹理使用跟普通纹理一样，超大型纹理只会返回rect对应的区域,src对于的范围应置为0和纹理的长宽
SDL_Texture* LOtextureBase::GetTexture(SDL_Rect *re) {
	if (texlist.size() == 0 && su && !isbig) Init();
	//非超大纹理直接返回
	if (!isbig) return (SDL_Texture*)texlist[1] ;

	//超大纹理优先查找缓存
	AvailableRect(ww, hh, re);
	SDL_Texture *texture = findTextureCache(re);

	//没有在缓存中则创建一个新的
	if (!texture) {
		LOSurface *tsu = su->ClipSurface(*re);
		texture = CreateTextureFromSurface(render, tsu->GetSurface());
		delete tsu;
		AddSizeTexture(re->x, re->y, re->w, re->h, texture);
	}
	return texture;
}

//从缓存队列中查找
SDL_Texture *LOtextureBase::findTextureCache(SDL_Rect *re) {
	for (int ii = 0; ii < texlist.size(); ii += 2) {
		SDL_Rect *rect = (SDL_Rect*)texlist[ii];
		if (rect && re->x == rect->x && re->y == rect->y && re->w == rect->w && re->h == rect->h) {
			return (SDL_Texture*)texlist[ii + 1];
		}
	}
	return nullptr;
}

void LOtextureBase::SetSurface(LOSurface *s) {
	if (su) delete su;
	su = s;
	ww = 0; hh = 0; isbig = false;
	if (su) {
		ww = su->W();
		hh = su->H();
		if (ww > maxTextureW || hh > maxTextureH) isbig = true;
	}
}




void LOtextureBase::AvailableRect(int maxx, int maxy, SDL_Rect *rect) {
	if (rect->x < 0) rect->x = 0;
	if (rect->y < 0)  rect->y = 0;
	if (rect->x > maxx) rect->x = maxx;
	if (rect->y > maxy) rect->y = maxy;
	if (rect->w < 0) rect->w = 0;
	if (rect->h < 0) rect->h = 0;
	if (rect->w > maxx - rect->x) rect->w = maxx - rect->x;
	if (rect->h > maxy - rect->y) rect->h = maxy - rect->y;
}


LOtextureBase *LOtexture::findTextureBaseFromMap(LOString &fname, bool adduse) {
	LOString s = fname.toLower();
	auto iter = baseMap.find(s);
	if (iter != baseMap.end()) {
		if (adduse) iter->second->AddUseCount();
		return iter->second;
	}
	return nullptr;
}


void LOtexture::addTextureBaseToMap(LOString &fname, LOtextureBase *base) {
	LOString s = fname.toLower();
	//auto iter = baseMap.find(s);
	//if (iter != baseMap.end) delete iter->second;
	base->SetName(s);
	baseMap[s] = base;
}

//只能运行在主线程
LOtextureBase* LOtexture::addNewEditTexture(LOString &fname, int w, int h, Uint32 format, SDL_TextureAccess access) {
	LOString s = fname.toLower();
	SDL_Texture *tx = CreateTexture(LOtextureBase::render, format, access, w, h);
	LOtextureBase *base = new LOtextureBase(tx);
	LOtexture::baseMap[s] = base;
	return base;
}


void LOtexture::notUseTextureBase(LOtextureBase *base) {
	int count = base->SubUseCount();
	if (count <= 0) {
		auto iter = baseMap.find(base->GetName());
		//必须确认为同一个
		if (iter != baseMap.end() && iter->second == base) baseMap.erase(base->GetName()); 
		delete base;
	}
}


LOtexture::LOtexture() {
	NewTexture();
}

LOtexture::LOtexture(LOtextureBase *base) {
	NewTexture();
	SetBaseTexture(base);
	baseTexture->AddUseCount();
}

LOtexture::LOtexture(int w, int h, Uint32 format, SDL_TextureAccess access) {
	NewTexture();
	SDL_Texture *t = CreateTexture(LOtextureBase::render, format, access, w, h);
	SetBaseTexture(new LOtextureBase(t));
}

void LOtexture::NewTexture() {
	useflag = 0;
	baseTexture = nullptr;
	tex = nullptr;
	srcRect = { 0,0,0,0 };
}

LOtexture::~LOtexture() {
	if (baseTexture)notUseTextureBase(baseTexture);
}

void LOtexture::SetBaseTexture(LOtextureBase *base) {
	if (baseTexture) notUseTextureBase(baseTexture);
	baseTexture = base;
}


bool LOtexture::isBig() {
	if (baseTexture) return baseTexture->isBig();
	else return false;
}

//有可用的basetexture且basetext是可以用的
bool LOtexture::isAvailable() {
	if (baseTexture && baseTexture->ww > 0 && baseTexture->hh > 0) return true;
	else return false;
}


bool LOtexture::activeTexture(SDL_Rect *src) {
	tex = NULL;
	srcRect.x = 0; srcRect.y = 0;
	srcRect.w = 0; srcRect.h = 0;

	if (!baseTexture) return false;

	if (!src) {
		srcRect.w = baseTexture->ww;
		srcRect.h = baseTexture->hh;
	}
	else srcRect = *src;

	//srcRect的值已经被改变
	tex = baseTexture->GetTexture(&srcRect);

	if (!tex) {
		srcRect.x = 0; srcRect.y = 0;
		srcRect.w = 0; srcRect.h = 0;
	}
	return true;
}


bool LOtexture::activeFlagControl() {
	if (!tex) return false;

	if (useflag & USE_ALPHA_MOD) {
		SDL_SetTextureAlphaMod(tex, color.a);
		SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
	}
	else SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_NONE);

	if (useflag & USE_BLEND_MOD) {
		int iii = SDL_SetTextureBlendMode(tex, blendmodel);
		//auto it = SDL_GetError();
		//iii = 0;
		//auto itt = (SDL_BlendMode)((useflag >> 16));
		//itt = SDL_BLENDMODE_NONE;
	}

	if (useflag & USE_COLOR_MOD) SDL_SetTextureColorMod(tex, color.r, color.g, color.b);
	else SDL_SetTextureColorMod(tex, 0xff, 0xff, 0xff);
	return true;
}


void LOtexture::setBlendModel(SDL_BlendMode model) {
	useflag &= (~USE_BLEND_MOD);
	if (model != SDL_BLENDMODE_NONE) {
		blendmodel = model;
		useflag |= USE_BLEND_MOD;
	}
}


void LOtexture::setColorModel(Uint8 r, Uint8 g, Uint8 b) {
	useflag &= (~USE_COLOR_MOD);
	if (r != 255 && g != 255 && b != 255) {
		useflag |= USE_COLOR_MOD;
		color.r = r;
		color.g = g;
		color.b = b;
	}
}

void LOtexture::setAplhaModel(int alpha) {
	useflag &= (~USE_ALPHA_MOD);
	if (alpha >= 0 && alpha < 255) {
		useflag |= USE_ALPHA_MOD;
		color.a = alpha & 0xff;
	}
}

void LOtexture::setForceAplha(int alpha) {
	if (alpha < 0 || alpha > 255) alpha = 255;
	useflag |= USE_ALPHA_MOD;
	color.a = alpha & 0xff;
}




int LOtexture::baseW() {
	if (baseTexture) return baseTexture->ww;
	else return 0;
}
int LOtexture::baseH() {
	if (baseTexture) return baseTexture->hh;
	else return 0;
}

//初始化对话文字纹理，纹理是可编辑的
bool LOtexture::activeActionTxtTexture() {
	if (!baseTexture) return false;
	LOSurface *su = baseTexture->GetSurface();
	if (!su || su->isNull()) return false;

	int w = su->W();
	int h = su->H();
	SDL_Texture *tx = CreateTexture(LOtextureBase::render, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, w, h);
	if (!tx) return false;
	baseTexture->AddSizeTexture(0, 0, w, h, tx);
	//部分显卡的显存还留有上一次显示的数据，因此需要重新覆盖数据
	void *pixdata;
	int pitch;
	SDL_LockTexture(tx, NULL, &pixdata, &pitch);
	SDL_UnlockTexture(tx);

	activeTexture(NULL);
	return true;
}


//滚动从surface复制内容到纹理中
bool LOtexture::rollTxtTexture(SDL_Rect *src, SDL_Rect *dst) {
	LOSurface *su = baseTexture->GetSurface();
	if (!tex) tex = baseTexture->GetTexture(NULL);
	LOtextureBase::AvailableRect(baseTexture->ww, baseTexture->hh, src);
	LOtextureBase::AvailableRect(baseTexture->ww, baseTexture->hh, dst);
	if(src->w > 0 && src->h > 0) CopySurfaceToTextureRGBA(su->GetSurface(), src, tex, dst);
	return true;
}


//只用于RGBA的复制
bool LOtexture::CopySurfaceToTextureRGBA(SDL_Surface *su, SDL_Rect *src, SDL_Texture *tex, SDL_Rect *dst) {
	void *texdata;
	int tpitch;
	if (SDL_LockTexture(tex, NULL, &texdata, &tpitch) != 0)return false;
	SDL_LockSurface(su);

	char *mdst = (char*)texdata + dst->y * tpitch + dst->x * 4;
	char *msrc = (char*)su->pixels + src->y * su->pitch + src->x * 4;
	for (int yy = 0; yy < src->h; yy++) {
		memcpy(mdst, msrc, src->w * 4);
		mdst += tpitch;
		msrc += su->pitch;
	}
	SDL_UnlockSurface(su);
	SDL_UnlockTexture(tex);
	return true;
}

void LOtexture::SaveSurface(LOString *fname){
	if(baseTexture){
		LOSurface *lsu = baseTexture->GetSurface() ;
		if(lsu){
			if(lsu->GetSurface()){
				SDL_SaveBMP(lsu->GetSurface(),fname->c_str()) ;
				LOLog_i("bmp has save!") ;
			}
			else LOLog_i("LOSurface's SDL_Surface is NULL!") ;
		}
		else LOLog_i("LOSurface is NULL!") ;
	}
	else LOLog_i("baseTexture is NULL!") ;
}


SDL_Surface *LOtexture::getSurface(){
	if(baseTexture){
		LOSurface *lsu = baseTexture->GetSurface() ;
		if(lsu){
			if(lsu->GetSurface()){
				return lsu->GetSurface();
			}
			else LOLog_i("LOSurface's SDL_Surface is NULL!") ;
		}
		else LOLog_i("LOSurface is NULL!") ;
	}
	else LOLog_i("baseTexture is NULL!") ;
	return nullptr;
}
