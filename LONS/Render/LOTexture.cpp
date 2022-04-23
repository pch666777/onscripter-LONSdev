/*
//����
*/

#include "LOTexture.h"

uint16_t LOtextureBase::maxTextureW = 4096;
uint16_t LOtextureBase::maxTextureH = 4096;
SDL_Renderer *LOtextureBase::render = nullptr;  //һ����˵ֻ����һ����Ⱦ��
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


bool EqualRect(SDL_Rect *r1, SDL_Rect *r2) {
	//���ǿգ�����һ��Ϊ��
	if (!r1 && !r2) return true;
	else if (!r1 || !r2) return false;
	else if (r1->x == r2->x && r1->y == r2->y && r1->w == r2->w && r1->h == r2->h) return true;
	else return false;
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
	ispng = IMG_isPNG(src);
	SDL_Surface *su = LIMG_Load_RW(src, 0);
	SetSurface(su);
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
	//�и������������Ӧ���Զ��ͷ�
	if (baseSurface) FreeSurface(baseSurface);
	//ע�⣬ֻ�ܴ���Ⱦ�̵߳��ã���ζ��baseTextureֻ�ܴ���Ⱦ�߳��ͷ�
	if (baseTexture) DestroyTexture(baseTexture);
	baseSurface = nullptr;
	baseTexture = nullptr;
}


SDL_Texture *LOtextureBase::GetFullTexture() {
	if (baseTexture) return baseTexture;
	else if (baseSurface) {
		baseTexture = CreateTextureFromSurface(render, baseSurface);
		FreeSurface(baseSurface);
		baseSurface = nullptr;
		return baseTexture;
	}
	return nullptr;
}

void LOtextureBase::FreeData() {
	if (baseSurface)FreeSurface(baseSurface);
	if (baseTexture) DestroyTexture(baseTexture);
	baseSurface = nullptr;
	baseTexture = nullptr;
}



void LOtextureBase::SetSurface(SDL_Surface *su) {
	if (baseSurface) FreeSurface(baseSurface);
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

	if (cellwidth == 0 || cellHight == 0) return nullptr;  //��Ч��
	SDL_Surface *su = CreateRGBSurfaceWithFormat(0, cellwidth * cellCount, cellHight, 32, SDL_PIXELFORMAT_RGBA32);
	//��ͬ��cpu�������ص��ֽ�λ�ò�����һ���ģ�������Ҫ��ȷ����͸�����ص�λ��
	char dstbit[4];
	GetFormatBit(su->format, dstbit);

	SDL_LockSurface(surface);
	SDL_LockSurface(su);
	int bpp = surface->format->BytesPerPixel;
	//ѭ��cellcount��
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
	//�����Ѿ���ӵ�map���ˣ����������ǰ�ȫ��
	return bt;
}

//ֻ�����������߳�
LOShareBaseTexture& LOtexture::addNewEditTexture(LOString &fname, int w, int h, Uint32 format, SDL_TextureAccess access) {
	LOString s = fname.toLower();
	SDL_Texture *tx = CreateTexture(LOtextureBase::render, format, access, w, h);
	return addTextureBaseToMap(s, new LOtextureBase(tx));
}


//���������һ����������������������˵���ű��߳�����lsp����csp��
//����Ⱦ�߳�ֻ����print�Ż����ͼ�㣬�����˺�������������̰߳�ȫ��
void LOtexture::notUseTextureBase(LOShareBaseTexture &base) {
	bool isdel = false;
	//��ǰ��base 1�ݣ�map�е�base1�ݣ�����2�ݾ��������ʱ��
	if (base.use_count() == 2) isdel = true;
	if (isdel) {
		auto iter = baseMap.find(base->GetName());
		if (iter != baseMap.end() && iter->second.get() == base.get()) baseMap.erase(iter);
	}
	//����-1�����Ҫɾ���ģ�����Ӧ���Ѿ����ͷ���
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
	//�������û���������ԭ���󶨵���ϢҪʧЧ
	//�����͵Ĳ��ô�������������Զ���������Ҫ����������͵�
	if (!isRef) FreeRef();
	baseTexture = base;
}

//LOtexture::LOtexture(int w, int h, Uint32 format, SDL_TextureAccess access) {
//	NewTexture();
//	SDL_Texture *t = CreateTexture(LOtextureBase::render, format, access, w, h);
//	SetBaseTexture(new LOtextureBase(t));
//}

void LOtexture::NewTexture() {
	useflag = 0;
	texturePtr = nullptr;
	surfacePtr = nullptr;
}

void LOtexture::FreeRef() {
	if (isRef) {
		//���õĲ���Ҫ�ͷ�
		texturePtr = nullptr;
		surfacePtr = nullptr;
	}
	else {
		//ɾ���и���������Դ�� �½�������ָ��
		if (texturePtr) DestroyTexture(texturePtr);
		if (surfacePtr) FreeSurface(surfacePtr);
		texturePtr = nullptr;
		surfacePtr = nullptr;
	}
}

void LOtexture::MakeGPUTexture() {
	if (isRef) {
		texturePtr = baseTexture->GetFullTexture();
		surfacePtr = baseTexture->GetSurface();
	}
	else {
		if (!texturePtr && surfacePtr) {
			texturePtr = CreateTextureFromSurface(LOtextureBase::render, surfacePtr);
			FreeSurface(surfacePtr);
			surfacePtr = nullptr;
		}
	}
}

LOtexture::~LOtexture() {
	if (baseTexture)notUseTextureBase(baseTexture);
	FreeRef();
}


//�п��õ�basetexture��basetext�ǿ����õ�
bool LOtexture::isAvailable() {
	if (baseTexture && baseTexture->ww > 0 && baseTexture->hh > 0) return true;
	else return false;
}


//���ݸ����ķ�Χ���ȡ����
bool LOtexture::activeTexture(SDL_Rect *src, bool toGPUtex) {
	if (!baseTexture || !baseTexture->isValid()) return false;

	//�ж���ʾ��Χ�͵�ǰ�Ƿ�һ��
	if (texturePtr && EqualRect(src, &expectRect)) {
		if (toGPUtex) MakeGPUTexture();
		return true;
	}

	//���ݻ�����������һ���µ�
	//����ǰ���ͷžɵ�
	FreeRef();
	isRef = !baseTexture->isbig;
	expectRect = *src;
	actualRect = expectRect;
	LOtextureBase::AvailableRect(baseTexture->ww, baseTexture->hh, &actualRect);
	if (isRef) {
		texturePtr = baseTexture->GetTexture();
		surfacePtr = baseTexture->GetSurface();
	}
	else {
		surfacePtr = LOtextureBase::ClipSurface(baseTexture->GetSurface(), actualRect);
	}

	if (toGPUtex) MakeGPUTexture();
	return true;
}


bool LOtexture::activeFirstTexture() {
	if (!baseTexture)return false;
	texturePtr = baseTexture->GetFullTexture();
}


bool LOtexture::activeFlagControl() {
	if (!texturePtr) return false;
	
	if (useflag & USE_ALPHA_MOD) {
		SDL_SetTextureAlphaMod(texturePtr, color.a);
		SDL_SetTextureBlendMode(texturePtr, SDL_BLENDMODE_BLEND);
	}
	else SDL_SetTextureBlendMode(texturePtr, SDL_BLENDMODE_NONE);

	if (useflag & USE_BLEND_MOD) {
		int iii = SDL_SetTextureBlendMode(texturePtr, blendmodel);
		auto it = SDL_GetError();
		iii = 0;
		auto itt = (SDL_BlendMode)((useflag >> 16));
		itt = SDL_BLENDMODE_NONE;
	}

	if (useflag & USE_COLOR_MOD) SDL_SetTextureColorMod(texturePtr, color.r, color.g, color.b);
	else SDL_SetTextureColorMod(texturePtr, 0xff, 0xff, 0xff);
	
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

//��ʼ���Ի��������������ǿɱ༭��
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
	//�����Կ����Դ滹������һ����ʾ�����ݣ������Ҫ���¸�������
	void *pixdata;
	int pitch;
	SDL_LockTexture(tx, NULL, &pixdata, &pitch);
	SDL_UnlockTexture(tx);

	activeTexture(NULL);
	*/
	return true;
}


//������surface�������ݵ�������
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


//ֻ����RGBA�ĸ���
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
	return texturePtr;
}
