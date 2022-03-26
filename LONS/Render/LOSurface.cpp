/*
//对SDL_Surface的封装，也为了更好的检查是否存在内存泄漏
*/

#include "LOSurface.h"

LOSurface::LOSurface(SDL_Surface *su) {
	surface = su;
}

LOSurface::LOSurface(int w, int h, int depth, Uint32 format) {
	surface = nullptr;
	if (w > 0 && h > 0) {
		surface = CreateRGBSurfaceWithFormat(0, w, h, depth, format);
	}
}


LOSurface::LOSurface(void *mem, int size) {
	SDL_RWops *src = SDL_RWFromMem(mem, size);
	ispng = IMG_isPNG(src);
	surface = LIMG_Load_RW(src, 0);
	SDL_RWclose(src);
}


LOSurface::~LOSurface() {
	if (surface) FreeSurface(surface);
}

int LOSurface::W() {
	if (surface) return surface->w;
	return 0;
}

int LOSurface::H() {
	if (surface) return surface->h;
	return 0;
}

bool LOSurface::isPng() {
	return ispng;
}


bool LOSurface::isNull() {
	return surface == nullptr;
}

bool LOSurface::hasAlpha() {
	if (surface) return surface->format->Amask != 0;
	return false;
}

void LOSurface::AvailableRect(SDL_Rect *rect) {
	int maxx = W();
	int maxy = H();
	if (rect->x < 0) rect->x = 0;
	if (rect->y < 0)  rect->y = 0;
	if (rect->x > maxx) rect->x = maxx;
	if (rect->y > maxy) rect->y = maxy;
	if (rect->w < 0) rect->w = 0;
	if (rect->h < 0) rect->h = 0;
	if (rect->w > maxx - rect->x) rect->w = maxx - rect->x;
	if (rect->h > maxy - rect->y) rect->h = maxy - rect->y;
}

LOSurface* LOSurface::ClipSurface(SDL_Rect rect) {
	if (!surface) return nullptr;
	AvailableRect(&rect);

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
	return new LOSurface(su);
}

//注意，只有RGB或者RGBA模式的图像检查才是准确的
void LOSurface::GetFormatBit(SDL_PixelFormat *format, char *bit) {
	//R G B A
	Uint32 color = SDL_MapRGBA(format, 1, 2, 3, 4);
	char *cco = (char*)(&color);
	for (int ii = 0; ii < 4; ii++) {
		for (int kk = 0; kk < 4; kk++) {
			if (cco[kk] == ii + 1) bit[ii] = kk;
		}
	}
}


//由NS模式的JPG图像转换为透明图像，count表示划分的格数
LOSurface* LOSurface::ConverNSalpha(int cellCount) {
	int cellwidth = W() / (cellCount * 2);
	int cellHight = H();

	if (cellwidth == 0 || cellHight == 0) return new LOSurface(nullptr);  //无效的
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

	return new LOSurface(su);
}


bool LOSurface::checkColor(SDL_Color &color, int allow) {
	if (!surface) return false;
	char pbit[4];
	Uint8 r, g, b;
	GetFormatBit(surface->format, pbit);
	for (int line = 0; line < surface->h; line++) {
		Uint8 *buf = (Uint8*)surface->pixels + line * surface->pitch;
		for (int xx = 0; xx < surface->w; xx++) {
			r = buf[pbit[0]];
			g = buf[pbit[1]];
			b = buf[pbit[2]];
			if (abs(r - color.r) > allow || abs(g - color.g) > allow || abs(b - color.b) > allow) {
				color.r = r; color.g = g; color.b = b;
				return false;
			}
			buf += surface->format->BytesPerPixel;
		}
	}
	return true;
}

extern Uint32 G_Texture_format;

void LOSurface::setAlphaColor(SDL_Color alphacolor) {
	if (!surface) return;

	if (surface->format->BytesPerPixel != 4) {
		SDL_Surface *tmp = SDL_ConvertSurfaceFormat(surface, G_Texture_format, 0);
		if (!tmp) return;
		SDL_FreeSurface(surface);
		surface = tmp;
	}

	char pbit[4];
	Uint8 r, g, b;
	GetFormatBit(surface->format, pbit);

	for (int line = 0; line < surface->h; line++) {
		Uint8 *buf = (Uint8*)surface->pixels + line * surface->pitch;
		for (int xx = 0; xx < surface->w; xx++) {
			r = buf[pbit[0]];
			g = buf[pbit[1]];
			b = buf[pbit[2]];
			if (r == alphacolor.r && g == alphacolor.g && b == alphacolor.b) {
				buf[0] = 0;
				buf[1] = 0;
				buf[2] = 0;
				buf[3] = 0;
			}
			buf += surface->format->BytesPerPixel;
		}
	}

	//SDL_SaveBMP(surface, "d:\\test.bmp");
}


SDL_Color LOSurface::getPositionColor(int x, int y) {
	SDL_Colour color = { 0,0,0,0 };
	if (!surface || x < 0 || y < 0 || x > surface->w - 1 || y > surface->h - 1) {
		return color;
	}

	Uint8 *buf = (Uint8*)surface->pixels + y * surface->pitch;
	buf += (x * surface->format->BytesPerPixel);

	if (surface->format->BytesPerPixel == 1) { //index color
		color = surface->format->palette->colors[buf[0]];
	}
	else {
		char pbit[4];
		GetFormatBit(surface->format, pbit);
		color.r = buf[pbit[0]];
		color.g = buf[pbit[1]];
		color.b = buf[pbit[2]];
		if(surface->format->BytesPerPixel == 4) color.a = buf[pbit[3]];
		else color.a = 255;
	}
	return color;
}