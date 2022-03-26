/*
//对SDL_Surface的封装，也为了更好的检查是否存在内存泄漏
*/

#ifndef __LOSURFACE_H__
#define __LOSURFACE_H__

#include <SDL.h>
#include <SDL_image.h>
#include<cstring>

#include "../etc/SDL_mem.h"

class LOSurface {
public:

	LOSurface(SDL_Surface *su);
	LOSurface(int w, int h, int depth, Uint32 format);
	LOSurface(void *mem, int size);
	~LOSurface();

	int W();
	int H();
	bool isNull();
	bool isPng();
	bool hasAlpha();
	void AvailableRect(SDL_Rect *rect);
	bool checkColor(SDL_Color &color,int allow);
	LOSurface* ClipSurface(SDL_Rect rect);
	LOSurface* ConverNSalpha(int cellCount);
	SDL_Surface* GetSurface() { return surface; }
	void setAlphaColor(SDL_Color alphacolor);
	SDL_Color getPositionColor(int x, int y);

	static void GetFormatBit(SDL_PixelFormat *format, char *bit);
private:
	SDL_Surface *surface;
	bool ispng;    //只有从LOSurface(void *mem, int size)中生成的surface此变量才有效
};


#endif // !__LOSURFACE_H__

