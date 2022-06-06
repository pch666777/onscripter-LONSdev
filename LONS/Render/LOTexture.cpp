/*
//����
*/

#include "LOTexture.h"

//�ڱ��Ѿ�������
extern char G_Bit[4];
extern Uint32 G_Texture_format;

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
	//printf("size is %d\n", size);
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
	ispng = false;
	baseTexture = nullptr;
	baseSurface = nullptr;
	textTexture = nullptr;
}

LOtextureBase::~LOtextureBase() {
	//�и������������Ӧ���Զ��ͷ�
	if (baseSurface) FreeSurface(baseSurface);
	//ע�⣬ֻ�ܴ���Ⱦ�̵߳��ã���ζ��baseTextureֻ�ܴ���Ⱦ�߳��ͷ�
	if (baseTexture) DestroyTexture(baseTexture);
	if (textTexture) delete textTexture;
	baseSurface = nullptr;
	baseTexture = nullptr;
	textTexture = nullptr;
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
	ww = 0; hh = 0;
	if (baseSurface) {
		ww = baseSurface->w;
		hh = baseSurface->h;
	}
}


bool LOtextureBase::hasAlpha() {
	if (baseSurface) return baseSurface->format->Amask != 0;
	else if (baseTexture) return true;
	return false;
}

bool LOtextureBase::isBig() {
	if (!baseSurface) return false;
	if (baseSurface->w > maxTextureW || baseSurface->h > maxTextureH) return true;
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
	//LOString s = fname.toLower();
	auto iter = baseMap.find(fname);
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
	if (!isRef) {
		resetSurface();
		resetTexture();
	}
	baseTexture = base;
}


void LOtexture::NewTexture() {
	useflag = 0;
	Xfix = Yfix = 0;
	bw = bh = 0;
	expectRect = { 0,0,0,0 };
	actualRect = { 0,0,0,0 };
	isEdit = false;
	isRef = false;
	texturePtr = nullptr;
	surfacePtr = nullptr;
}


void LOtexture::resetSurface() {
	if (isRef) surfacePtr = nullptr;
	else {
		//��base��surfaceҲ�������и�������
		if (surfacePtr) FreeSurface(surfacePtr);
		surfacePtr = nullptr;
	}
}

void LOtexture::resetTexture() {
	if (isRef) texturePtr = nullptr;
	else {
		//��base��surfaceҲ�������и�������
		if (texturePtr) DestroyTexture(texturePtr);
		texturePtr = nullptr;
	}
}


LOtexture::~LOtexture() {
	if (baseTexture)notUseTextureBase(baseTexture);
	resetSurface();
	resetTexture();
}


//�п��õ�basetexture��basetext�ǿ����õ�
bool LOtexture::isAvailable() {
	if (surfacePtr || texturePtr || baseTexture || bw > 0) return true;
	else return false;
}


//���ݸ����ķ�Χ���ȡ���󣬵�ǰtoGPUtex������
bool LOtexture::activeTexture(SDL_Rect *src, bool toGPUtex) {
	//�Ѿ������������£��ж���ʾ��Χ�͵�ǰ�Ƿ�һ��
	if (texturePtr && EqualRect(src, &expectRect)) return true;

	expectRect = *src;
	actualRect = expectRect;
	//����������ͷǸ���������
	if (!baseTexture) {
		//�Ǹ���ʽ���������ٴγ�ʼ��
		if (texturePtr) return true;
		if (!surfacePtr) return false;

		if (isEdit) {
			texturePtr = CreateTexture(LOtextureBase::render, surfacePtr->format->format, SDL_TEXTUREACCESS_STREAMING, surfacePtr->w, surfacePtr->h);
			//��������surface��texture��ͬһ�ָ�ʽ
			void *dst;
			int pitch;
			SDL_LockTexture(texturePtr, nullptr, &dst, &pitch);
			//����ģʽ���ڶ��������и��Ƶ�
			if (!isTextAction()) {
				for (int line = 0; line < surfacePtr->h; line++) {
					char *src = (char*)surfacePtr->pixels + line * surfacePtr->pitch;
					memcpy((char*)dst + line * pitch, src, pitch);
				}
			}
			SDL_UnlockTexture(texturePtr);
		}
		else texturePtr = CreateTextureFromSurface(LOtextureBase::render, surfacePtr);
		//��ȡtexturePtr����Ϣ��΢�鷳�㣬��˲���һ����¼
		bw = surfacePtr->w;
		bh = surfacePtr->h;
		if(!isTextAction()) resetSurface();
	}
	else {
		//����ʽ����
		if (!baseTexture || !baseTexture->isValid()) return false;
		resetSurface();
		resetTexture();

		//���������Ƿ�ref�ģ������Ķ���ref��
		isRef = baseTexture->isBig();
		if (isRef) {
			texturePtr = baseTexture->GetFullTexture();
			return texturePtr != nullptr;
		}
		else {
			//��ߴ����ȷ�����еķ�Χ
			LOtextureBase::AvailableRect(baseTexture->ww, baseTexture->hh, &actualRect);
			surfacePtr = LOtextureBase::ClipSurface(baseTexture->GetSurface(), actualRect);
			if (!surfacePtr) return false;
			texturePtr = CreateTextureFromSurface(LOtextureBase::render, surfacePtr);
			resetSurface();
		}
	}
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
	else if (surfacePtr) return surfacePtr->w;
	else return bw;
}
int LOtexture::baseH() {
	if (baseTexture) return baseTexture->hh;
	else if (surfacePtr) return surfacePtr->h;
	else return bh;
}


void LOtexture::SaveSurface(LOString *fname){
	if (surfacePtr) {
		SDL_SaveBMP(surfacePtr, fname->c_str());
	}
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

LOtexture::TextData::~TextData() {
	ClearLines();
	ClearTexts();
	ClearWords();
}

void LOtexture::TextData::ClearLines() {
	for (auto iter = lineList.begin(); iter != lineList.end(); iter++) delete *iter;
	lineList.clear();
}
void LOtexture::TextData::ClearTexts() {
	for (auto iter = textList.begin(); iter != textList.end(); iter++) delete *iter;
	textList.clear();
}
void LOtexture::TextData::ClearWords() {
	for (auto iter = wordList.begin(); iter != wordList.end(); iter++) delete *iter;
	wordList.clear();
}


bool LOtexture::CreateTextDescribe(LOString *s, LOTextStyle *style, LOString *fontName) {
	resetSurface();
	if (!s || !style || !fontName) return false;
	if (!textData) textData.reset(new TextData());
	LOFont *font = LOFont::CreateFont(*fontName);
	if (!font) return false;

	textData->style = *style;
	textData->fontName = *fontName;

	CreateLineDescribe( s, font, textData->style.xsize);
	//����λ�þ���
	Xfix = Yfix = 0;
	for (auto iter = textData->lineList.begin(); iter != textData->lineList.end(); iter++) {
		LOLineDescribe *line = (*iter);
		if (line->xx + line->minx < Xfix) Xfix = line->xx + line->minx;
		if (line->yy + line->miny < Yfix) Yfix = line->yy + line->miny;
	}
}

LOLineDescribe *LOtexture::CreateNewLine(LOTextDescribe *&des, TTF_Font *font, LOTextStyle *style, int colorID) {
	LOLineDescribe *line = new LOLineDescribe();
	textData->lineList.push_back(line);
	line->xx = style->textIndent;
	line->startIndex = textData->textList.size();
	line->endIndex = line->startIndex;

	if (&des && font) {
		des = CreateNewTextDes(TTF_FontAscent(font), TTF_FontDescent(font), colorID);
		line->endIndex++;
	}
	return line;
}

LOTextDescribe *LOtexture::CreateNewTextDes(int ascent, int dscent, int colorID) {
	LOTextDescribe *des = new LOTextDescribe(ascent, dscent, colorID);
    textData->textList.push_back(des);
	des->startIndex = textData->wordList.size();
	des->endIndex = des->startIndex;
	return des;
}


void LOtexture::CreateLineDescribe(LOString *s, LOFont *font, int firstSize) {
	int colorID = 0;
	int maxWidth = textData->style.xcount * (firstSize + textData->style.xspace);
	const char *buf = s->c_str();
	auto encoder = s->GetEncoder();
	auto wordFont = font->GetFont(firstSize);
	SDL_Color bgColor{ 0,0,0,0 };
	SDL_Color fsColor{ 255,255,255,255 };
	int Ypos = 0;

	LOTextDescribe *des = nullptr;
	//����ʱȷ����xλ��
	LOLineDescribe *line = CreateNewLine(des, wordFont->font, &textData->style, colorID);
	int currentX = line->xx;
	line->yy = Ypos;

	//�������������ӷ��任����
	while (buf[0] != 0) {
		//���Եķ���
		if (buf[0] == '\r') {
			buf++;
			continue;
		}
		LOWordElement *el = new LOWordElement();
		int ulen = 0;
		el->unicode = encoder->GetUtf16(buf, &ulen);
		if (ulen == 0) {
			LOLog_e("LOString encoder error:%s", buf);
			return;
		}

		buf += ulen;
		el->isEng = (ulen == 1);

		//���任��ʼ
		if (el->unicode == '<') {

		}
		else if (el->unicode == '>') {
			//���任����
		}
		else if (el->unicode == '\n') {
			delete el;
			el = nullptr;
			//�������һ��
			line->endIndex = textData->textList.size();
			line->CreateLocation(&textData->textList, &textData->wordList, &textData->style);
			Ypos += line->height();
			Ypos += textData->style.yspace;
			//��λ��һ��
			line = CreateNewLine(des, wordFont->font, &textData->style, colorID);
			currentX = line->xx;
			line->yy = Ypos;
		}
		else {
			if (LOString::GetCharacter(buf - ulen) != LOString::CHARACTER_SPACE && wordFont->font) {
				//�ַ����ǻ��߶���ģ����߼�FontAscent��������һ�������XY����ϵ����������⣬��Ҫת������Ļ����ϵ
				//�ٶ�һ������������ϵ(FontAscent,0)����Ļ����ϵ���ߣ���ô�У�
				//��maxy����FontAscentʱ��surface��Yλ��Ϊ FontAscent - maxy����maxyС�ڵ���FontAscentʱ��Yλ��Ϊ0
				//X�����ϣ�minx��ΪX��λ�ã�surface�Ŀ�� = advance - minx������һ�����ŵ�λ��
				TTF_GlyphMetrics(wordFont->font, el->unicode, &el->minx, &el->maxx, &el->miny, &el->maxy, &el->advance);
				el->surface = LTTF_RenderGlyph_Shaded(wordFont->font, el->unicode, fsColor, bgColor);
				//����Ƿ���Ҫ������
				if (currentX + el->minx + el->advance > maxWidth) {
					//�������һ��
					line->endIndex = textData->textList.size();
					line->CreateLocation(&textData->textList, &textData->wordList, &textData->style);
					Ypos += line->height();
					Ypos += textData->style.yspace;
					//��λ��һ��
					line = CreateNewLine(des, wordFont->font, &textData->style, colorID);
					currentX = line->xx;
					line->yy = Ypos;
				}

				textData->wordList.push_back(el);
				des->endIndex++;
				//std::string s = "d:\\aaaa\\" + std::to_string(count) + ".bmp";
				//SDL_SaveBMP(ele->surface, s.c_str());
				//count++;
				currentX += (el->minx + el->advance);
			}
			else {
				//�ո����
				el->setSpace(firstSize);
				currentX += (el->maxx - el->minx);
				textData->wordList.push_back(el);
				des->endIndex++;
			}

			//���
			currentX += textData->style.xspace;
		}
	}
	font->CloseAll();
	line->CreateLocation(&textData->textList, &textData->wordList, &textData->style);
}

void LOtexture::CreateSurface(int w, int h) {
	resetSurface();
	isRef = false;
	surfacePtr = CreateRGBSurfaceWithFormat(0, w, h, 32, G_Texture_format);
}

void LOtexture::GetTextSurfaceSize(int *width, int *height) {
	int16_t minx, maxx, miny, maxy;
	minx = maxx = miny = maxy = 0;
	for (auto iter = textData->lineList.begin(); iter != textData->lineList.end(); iter++) {
		LOLineDescribe *line = (*iter);
		if (line->xx + line->minx < minx) minx = line->xx + line->minx;
		if (line->yy + line->miny < miny) miny = line->yy + line->miny;
		if (line->xx + line->maxx > maxx) maxx = line->xx + line->maxx;
		if (line->yy + line->maxy > maxy) maxy = line->yy + line->maxy;
	}

	Xfix = minx;
	Yfix = miny;

	if (width) *width = maxx - minx + textData->style.xshadow;
	if (height) *height = maxy - miny + textData->style.yshadow;
}


void LOtexture::RenderTextSimple(int x, int y, SDL_Color color) {
	if (!surfacePtr || !textData) return;
	x += abs(Xfix);
	y += abs(Yfix);
	SDL_Rect dstR, srcR;
	SDL_Color shadowColor = { 0,0,0,255 };

	for (int lineII = 0; lineII < textData->lineList.size(); lineII++) {
		LOLineDescribe *line = textData->lineList.at(lineII);
		int lx = x + line->xx;
		int ly = y + line->yy;
		for (int textII = line->startIndex; textII < line->endIndex && textII < textData->textList.size(); textII++) {
			LOTextDescribe *des = textData->textList.at(textII);
			int dx = lx + des->xx;
			int dy = ly + des->yy;
			for (int wordII = des->startIndex; wordII < des->endIndex && wordII < textData->wordList.size(); wordII++) {
				LOWordElement *el = textData->wordList.at(wordII);
				//�ո�����
				if (!el->surface) continue;

				dstR.x = dx + el->left;
				dstR.y = dy + el->top;
				srcR.x = 0; srcR.y = 0;
				srcR.w = el->surface->w;
				srcR.h = el->surface->h;
				//��Ӱ
				if (textData->style.xshadow != 0 || textData->style.yshadow != 0) {
					BlitShadow(surfacePtr, el->surface, dstR, srcR, shadowColor, des->Ascent - des->Dscent);
				}

				BlitToRGBA(surfacePtr, el->surface, &dstR, &srcR, color);
				//printf("%d,%d\n", wx, wy);
			}
		}
	}
}


bool LOtexture::CheckRect(SDL_Surface *dst, SDL_Surface *src, SDL_Rect *dstR, SDL_Rect *srcR) {
	if (!dst || !src || !dstR || !srcR) return false;
	//���ϽǼ��
	//�����Դ
	if (srcR->x < 0) {
		dstR->x += abs(srcR->x); srcR->w -= abs(srcR->x);
		srcR->x = 0;
	}
	if (srcR->y < 0) {
		dstR->y += abs(srcR->y); srcR->h -= abs(srcR->y);
		srcR->y = 0;
	}
	//���Ŀ��
	if (dstR->x < 0) {
		srcR->x += abs(dstR->x); srcR->w -= abs(dstR->x);
		dstR->x = 0;
	}
	if (dstR->y < 0) {
		srcR->y += abs(dstR->y); srcR->h -= abs(dstR->y);
		dstR->y = 0;
	}
	if (srcR->w < 0 || srcR->h < 0) return false;
	//������½�
	//�����Դ
	if (srcR->x + srcR->w > src->w) srcR->w = src->w - srcR->x;
	if (srcR->y + srcR->h > src->h) srcR->h = src->h - srcR->y;
	//���Ŀ��
	if (dstR->x + srcR->w > dst->w) srcR->w = dst->w - dstR->x;
	if (dstR->y + srcR->h > dst->h) srcR->h = dst->h - dstR->y;
	if (srcR->w < 0 || srcR->h < 0) return false;
	return true;
}


void LOtexture::BlitToRGBA(SDL_Surface *dst, SDL_Surface *src, SDL_Rect *dstR, SDL_Rect *srcR, SDL_Color color) {
	if (!CheckRect(dst, src, dstR, srcR)) return;

	SDL_Color *ccIndex = src->format->palette->colors;
	Uint32 reA, reB, AP;
	//Ĭ�ϲ�����ֻ�õ�RGBAģʽλ�� R  G  B  A
	int rpos = G_Bit[0];
	int gpos = G_Bit[1];
	int bpos = G_Bit[2];
	int apos = G_Bit[3];

	for (int line = 0; line < srcR->h; line++) {
		//Դͼ��8bit��
		unsigned char *sbuf = (unsigned char*)src->pixels + (srcR->y + line) * src->pitch + srcR->x;
		//Ŀ��ͼƬ��32λ��
		unsigned char *dbuf = (unsigned char*)dst->pixels + (dstR->y + line) * dst->pitch + dstR->x * 4;
		for (int px = 0; px < srcR->w; px++) {
			//����ɫ������
			if (sbuf != 0) {
				//Ĭ�ϲ�����ֻ�õ�RGBAģʽλ�� R  G  B  A
				//Ŀ��ɫû������
				if (dbuf[apos] == 0) {
					dbuf[rpos] = color.r;
					dbuf[gpos] = color.g;
					dbuf[bpos] = color.b;
					dbuf[apos] = sbuf[0];
				}
				else {
					//������ݣ����Ż��ռ䣬��ʱ������
					AP = sbuf[0];
					reA = (sbuf[0] ^ 255);  // 1 - srcAlpha
					reB = (dbuf[apos] * reA / 255);

					dbuf[apos] = AP + dbuf[apos] * reA / 255;  //dst alpha

					dbuf[rpos] = (color.r * AP) / 255 + dbuf[rpos] * reB / 255;
					dbuf[rpos] = dbuf[rpos] * 255 / dbuf[apos];

					dbuf[gpos] = (color.g * AP) / 255 + dbuf[gpos] * reB / 255;
					dbuf[gpos] = dbuf[gpos] * 255 / dbuf[apos];

					dbuf[bpos] = (color.b * AP) / 255 + dbuf[bpos] * reB / 255;
					dbuf[bpos] = dbuf[bpos] * 255 / dbuf[apos];
				}
			}
			sbuf++;
			dbuf += 4;
		}
	}
}


void LOtexture::BlitShadow(SDL_Surface *dst, SDL_Surface *src, SDL_Rect dstR, SDL_Rect srcR, SDL_Color color, int maxsize) {
	int xs = textData->style.xshadow;
	int ys = textData->style.yshadow;
	if (xs < 0) xs = maxsize / 30 + 1;
	if (ys < 0) ys = maxsize / 30 + 1;
	dstR.x += xs;
	dstR.y += ys;
	BlitToRGBA(dst, src, &dstR, &srcR, color);
}


void LOtexture::setEmpty(int w, int h) {
	bw = w;
	bh = h;
}


void LOtexture::CreateSimpleColor(int w, int h, SDL_Color color) {
	resetSurface();
	isRef = false;
	surfacePtr = CreateRGBSurfaceWithFormat(0, w, h, 8, SDL_PIXELFORMAT_INDEX8);
	SDL_Palette *pale = surfacePtr->format->palette;
	pale->colors[0] = color;
	pale->colors[255] = color;
}


int LOtexture::RollTextTexture(int start, int end) {
	//�������еģ�ֱ���൱�ڵ��յ�
	if (!texturePtr || !isEdit || !textData || textData->lineList.size() == 0) return RET_ROLL_FAILD;
	if (end < start || end <= 0) return RET_ROLL_FAILD;

	int startLine, startPos, endLine, endPos;
	bool isend = false;
	TranzPosition(&startLine, &startPos, &isend, start);
	//��һЩ���Ƽ�� ��ͷ�ѳ������ƣ�ֱ�ӷ���
	if (isend) return RET_ROLL_END;
	TranzPosition(&endLine, &endPos, &isend, end);

	//ȷ��������Ҫ�޸ĵ�����
	SDL_Point p1, p2;
	for (int ii = startLine; ii <= endLine; ii++) {
		SDL_Rect re;
		LOLineDescribe *line = textData->lineList.at(ii);
		//������������λ��
		p1.x = line->xx + abs(Xfix);
		p1.y = line->yy + abs(Yfix);
		p2.x = p1.x + line->width() + textData->style.xshadow;
		p2.y = p1.y + line->height() + textData->style.yshadow;

		if (ii == startLine) p1.x += startPos;
		if (ii == endLine) p2.x = p1.x + endPos + textData->style.xshadow;

		re = { p1.x, p1.y , p2.x - p1.x, p2.y - p1.y };
		CopySurfaceToTexture(texturePtr, surfacePtr, re);
		//printf("left:%d,%d   right:%d,%d\n", p1.x, p1.y, p2.x, p2.y);
	}

	if (isend) return RET_ROLL_END;
	else return RET_ROLL_CONTINUE;
}


void LOtexture::TranzPosition(int *lineID, int *linePos, bool *isend, int position) {
	*lineID = -1;
	*linePos = 0;
	if (!textData || textData->lineList.size() == 0) return;
	for (*lineID = 0; *lineID < textData->lineList.size(); (*lineID)++) {
		LOLineDescribe *line = textData->lineList.at(*lineID);
		int lw = line->width() + textData->style.xshadow;
		if (position - lw > 0) position -= lw;
		else {
			*linePos = position;
			break;
		}
	}
	//���ֻ����β��
	if (*lineID == textData->lineList.size()) {
		*lineID = textData->lineList.size() - 1;
		*linePos = textData->lineList[*lineID]->width() + textData->style.xshadow;
		if (isend) *isend = true;
	}
}


int LOtexture::GetTextTextureEnd() {
	int sumlen = 0;
	if (textData) {
		for (int ii = 0; ii < textData->lineList.size(); ii++) {
			sumlen += textData->lineList[ii]->width() + textData->style.xshadow;
		}
	}
	return sumlen;
}


void LOtexture::CopySurfaceToTexture(SDL_Texture *dst, SDL_Surface *src, SDL_Rect rect) {
	if (!dst || !src) return;
	LOtextureBase::AvailableRect(src->w, src->h, &rect);
	int maxw, maxh;
	SDL_QueryTexture(dst, nullptr, nullptr, &maxw, &maxh);
	LOtextureBase::AvailableRect(maxw, maxh, &rect);

	if (rect.w < 0 || rect.h < 0) return;
	if (rect.w == 0 && rect.h == 0) return;

	void *pixs = nullptr;
	int pitch = 0;
	SDL_LockTexture(dst, &rect, &pixs, &pitch);
	for (int line = 0; line < rect.h; line++) {
		char *srcbuf = (char*)src->pixels + (line + rect.y) * src->pitch + rect.x * 4;
		char *dstbuf = (char*)pixs + line * pitch;
		memcpy(dstbuf, srcbuf, rect.w * 4);
	}
	SDL_UnlockTexture(dst);
}