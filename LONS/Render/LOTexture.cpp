/*
//纹理
*/

#include "LOTexture.h"

uint16_t LOtextureBase::maxTextureW = 4096;
uint16_t LOtextureBase::maxTextureH = 4096;
SDL_Renderer *LOtextureBase::render = nullptr;  //一般来说只会有一个渲染器
std::unordered_map<std::string, LOShareBaseTexture> LOtexture::baseMap;


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

//============== class ============
MiniTexture::MiniTexture() {
	dst = { 0,0,0,0 };
	srt = dst;
	tex = nullptr;
	su = nullptr;
	isref = true;
}

MiniTexture::~MiniTexture() {
	//非引用要释放数据
	if (!isref) {
		//涉及到释放需注意只能在渲染线程才能释放
		if (tex && *tex) SDL_DestroyTexture(*tex);
		if (su && *su) SDL_FreeSurface(*su);
	}
}

bool MiniTexture::equal(SDL_Rect *rect) {
	if (rect->x == dst.x && rect->y == dst.y && rect->w == dst.w && rect->h == dst.h) return true;
	return false;
}

bool MiniTexture::isRectAvailable() {
	if (srt.x < 0 || srt.y < 0 || srt.w < 1 || srt.h < 1) return false;
	return true;
}

void MiniTexture::converToGPUtex() {
	//if (!tex && su) {
	//	tex = CreateTextureFromSurface(LOtextureBase::render, su);
	//	SDL_FreeSurface(su);
	//	su = nullptr;
	//}
}

////================================================

LOtextureBase::LOtextureBase() {
	baseNew();
}

LOtextureBase::LOtextureBase(SDL_Surface *su) {
	baseNew();
	SetSurface(su);
}


LOtextureBase::LOtextureBase(void *mem, int size) {
	SDL_RWops *src = SDL_RWFromMem(mem, size);
	SDL_Surface *su = LIMG_Load_RW(src, 0);
	SetSurface(su);
	ispng = IMG_isPNG(src);
	SDL_RWclose(src);
}


LOtextureBase::LOtextureBase(SDL_Texture *tx) {
	baseNew();
	if (tx) {
		SDL_QueryTexture(tx, NULL, NULL, &ww, &hh);
		baseTexture = tx;
	}
}


void LOtextureBase::baseNew() {
	ww = hh = 0;
	isbig = false;
	ispng = false;
	baseTexture = nullptr;
	baseSurface = nullptr;
}

LOtextureBase::~LOtextureBase() {
	//切割下来的纹理块应该自动释放
	texTureList.clear();
	if (baseSurface) SDL_FreeSurface(baseSurface);
	//注意，只能从渲染线程调用，意味着baseTexture只能从渲染线程释放
	if (baseTexture) SDL_DestroyTexture(baseTexture);
}

MiniTexture *LOtextureBase::GetMiniTexture(SDL_Rect *rect) {
	SDL_Rect temp;
	if (rect) temp = *rect;
	else temp = { 0,0,ww,hh };

	for (int ii = 0; ii < texTureList.size(); ii++) {
		MiniTexture *tex = &texTureList.at(ii);
		if (tex->equal(&temp)) return tex;
	}
	//没有找到，添加一个，添加失败则返回null
	MiniTexture *tex = new MiniTexture;
	tex->dst = temp;
	tex->srt = temp;
	AvailableRect(ww, hh, &tex->srt);
	//不可用的基础纹理
	if (!tex->isRectAvailable()) return nullptr;
	//超大纹理不引用
	tex->isref = !isbig;
	if (tex->isref) {
		if (baseTexture) tex->tex = &baseTexture;
		else if (baseSurface) tex->su = &baseSurface;
		else return nullptr;
	}
	else {
		//超大纹理只从基础纹理中切割出surface
		*(tex->su) = ClipSurface(baseSurface, tex->srt);
	}
	return tex;
}



void LOtextureBase::SetSurface(SDL_Surface *su) {
	texTureList.clear();
	if (baseSurface) SDL_FreeSurface(baseSurface);
	baseSurface = su;
	ww = 0; hh = 0; isbig = false;
	if (baseSurface) {
		ww = baseSurface->w;
		hh = baseSurface->h;
		if (ww > maxTextureW || hh > maxTextureH) isbig = true;
	}
}


bool LOtextureBase::hasAlpha() {
	if (baseSurface) return baseSurface->format->Amask != 0;
	else if (baseTexture) return true;
	return false;
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

SDL_Surface* LOtextureBase::ClipSurface(SDL_Surface *surface, SDL_Rect rect) {
	if (!surface) return nullptr;
	AvailableRect(surface->w,surface->h, &rect);

	SDL_Surface *su = CreateRGBSurfaceWithFormat(0,
		rect.w, rect.h, surface->format->BitsPerPixel, surface->format->format);

	int bytes = su->format->BytesPerPixel;

	SDL_LockSurface(su);
	SDL_LockSurface(surface);
	for (int ii = rect.y; ii < rect.y + rect.h; ii++) {
		unsigned char *src = (unsigned char*)surface->pixels + surface->pitch * ii + rect.x * bytes;
		unsigned char *dst = (unsigned char*)su->pixels + su->pitch * (ii - rect.y);
		memcpy(dst, src, rect.w * bytes);
	}
	SDL_UnlockSurface(su);
	SDL_UnlockSurface(surface);
	return su;
}


SDL_Surface* LOtextureBase::ConverNSalpha(SDL_Surface *surface, int cellCount) {
	int cellwidth = surface->w / (cellCount * 2);
	int cellHight = surface->h;

	if (cellwidth == 0 || cellHight == 0) return nullptr;  //无效的
	SDL_Surface *su = CreateRGBSurfaceWithFormat(0, cellwidth * cellCount, cellHight, 32, SDL_PIXELFORMAT_RGBA32);
	//不同的cpu构架像素的字节位置并不是一定的，所以需要先确定好透明像素的位置
	char dstbit[4];
	GetFormatBit(su->format, dstbit);

	SDL_LockSurface(surface);
	SDL_LockSurface(su);
	int bpp = surface->format->BytesPerPixel;
	//循环cellcount次
	for (int ii = 0; ii < cellCount; ii++) {
		for (int line = 0; line < cellHight; line++) {
			int srcX = ii * cellwidth * 2;
			int aphX = srcX + cellwidth;
			int dstX = ii * cellwidth;

			unsigned char *scrdate = (unsigned char *)surface->pixels + line * surface->pitch + srcX * bpp;
			unsigned char *aphdate = (unsigned char *)surface->pixels + line * surface->pitch + aphX * bpp;
			unsigned char *dstdate = (unsigned char *)su->pixels + line * su->pitch + dstX * 4; //RGBA32
			for (int nn = 0; nn < cellwidth; nn++) {
				*(Uint32*)dstdate = *(Uint32*)scrdate;
				dstdate[dstbit[3]] = 255 - aphdate[0]; //just use 
				dstdate += 4; scrdate += bpp; aphdate += bpp;
			}
		}
	}
	SDL_UnlockSurface(surface);
	SDL_UnlockSurface(su);

	return su;
}


//=================================================

LOShareBaseTexture LOtexture::findTextureBaseFromMap(LOString &fname) {
	LOString s = fname.toLower();
	auto iter = baseMap.find(s);
	if (iter != baseMap.end()) {
		return iter->second;
	}
	LOShareBaseTexture bt;
	return bt;
}

void LOtexture::addTextureBaseToMap(LOString &fname, LOShareBaseTexture &base) {
	LOString s = fname.toLower();
	base->SetName(s);
	baseMap[s] = base;
}


LOShareBaseTexture& LOtexture::addTextureBaseToMap(LOString &fname, LOtextureBase *base) {
	LOShareBaseTexture bt(base);
	addTextureBaseToMap(fname, bt);
	return bt;
}

//只能运行在主线程
LOShareBaseTexture& LOtexture::addNewEditTexture(LOString &fname, int w, int h, Uint32 format, SDL_TextureAccess access) {
	LOString s = fname.toLower();
	SDL_Texture *tx = CreateTexture(LOtextureBase::render, format, access, w, h);
	return addTextureBaseToMap(s, new LOtextureBase(tx));
}


//这个函数有一个隐含的条件：触发函数说明脚本线程正在lsp或者csp中
//而渲染线程只有在print才会调整图层，触发此函数，因此它是线程安全的
void LOtexture::notUseTextureBase(LOShareBaseTexture &base) {
	bool isdel = false;
	//当前的base 1份，map中的base1份，所以2份就是清理的时机
	if (base.use_count() == 2) isdel = true;
	if (isdel) {
		auto iter = baseMap.find(base->GetName());
		if (iter != baseMap.end() && iter->second.get() == base.get()) baseMap.erase(iter);
	}
	//计数-1，如果要删除的，这里应该已经被释放了
	base.reset();
}


LOtexture::LOtexture() {
	NewTexture();
}


LOtexture::LOtexture(LOShareBaseTexture &base) {
	NewTexture();
	SetBaseTexture(base);
}

void LOtexture::SetBaseTexture(LOShareBaseTexture &base) {
	baseTexture = base;
}

//LOtexture::LOtexture(int w, int h, Uint32 format, SDL_TextureAccess access) {
//	NewTexture();
//	SDL_Texture *t = CreateTexture(LOtextureBase::render, format, access, w, h);
//	SetBaseTexture(new LOtextureBase(t));
//}

void LOtexture::NewTexture() {
	useflag = 0;
	baseTexture = nullptr;
	curTexture = nullptr;
}

LOtexture::~LOtexture() {
	if (baseTexture)notUseTextureBase(baseTexture);
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


MiniTexture* LOtexture::activeTexture(SDL_Rect *src, bool toGPUtex) {
	if (!baseTexture || !baseTexture->isValid()) return nullptr;
	SDL_Rect dst;
	if (src) dst = *src;
	else dst = { 0,0,baseTexture->ww,baseTexture->hh };
	curTexture = baseTexture->GetMiniTexture(&dst);

	if (curTexture && toGPUtex) {
		if (!curTexture->tex && curTexture->su) {
			//curte CreateTextureFromSurface(LOtextureBase::render, curTexture->su);
		}
	}
	return curTexture;
}


bool LOtexture::activeFlagControl() {
	if (!curTexture) return false;
	/*
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
	*/
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
	/*
	if (!baseTexture) return false;
	LOSurface *su = baseTexture->GetSurface();
	if (!su || su->isNull()) return false;

	int w = su->W();
	int h = su->H();
	SDL_Texture *tx = CreateTexture(LOtextureBase::render, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, w, h);
	if (!tx) return false;
	//baseTexture->AddSizeTexture(0, 0, w, h, tx);
	//部分显卡的显存还留有上一次显示的数据，因此需要重新覆盖数据
	void *pixdata;
	int pitch;
	SDL_LockTexture(tx, NULL, &pixdata, &pitch);
	SDL_UnlockTexture(tx);

	activeTexture(NULL);
	*/
	return true;
}


//滚动从surface复制内容到纹理中
bool LOtexture::rollTxtTexture(SDL_Rect *src, SDL_Rect *dst) {
	/*
	LOSurface *su = baseTexture->GetSurface();
	//if (!tex) tex = baseTexture->GetTexture(NULL);
	LOtextureBase::AvailableRect(baseTexture->ww, baseTexture->hh, src);
	LOtextureBase::AvailableRect(baseTexture->ww, baseTexture->hh, dst);
	//if(src->w > 0 && src->h > 0) CopySurfaceToTextureRGBA(su->GetSurface(), src, tex, dst);
	*/
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
	/*
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
	*/
}


SDL_Surface *LOtexture::getSurface(){
	/*
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
	*/
	return nullptr;
}


int LOtexture::W() {
	return 0;
}

int LOtexture::H() {
	return 0;
}

SDL_Texture *LOtexture::GetTexture() {
	return nullptr;
}
