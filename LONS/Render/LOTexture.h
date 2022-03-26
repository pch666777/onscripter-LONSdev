/*
//纹理
*/

#ifndef __LOTEXTURE_H__
#define __LOTEXTURE_H__

#include <SDL.h>
#include <atomic>
#include <unordered_map>
#include <vector>

#include "../etc/LOString.h"
#include "../Scripter/FuncInterface.h"
#include "LOSurface.h"

extern void SimpleError(const char *format, ...);

//我希望尽可能的缓存纹理，可以利用SDL的纹理特性对同一个纹理进行透明、混合模式、颜色叠加
//以小写文件名作为key，此外还需要加入NS的透明模式，比如对JPG来说 :a 和 :c的结果是不同的
//另外还需要应对超大纹理

class LOtextureBase
{
public:

	LOtextureBase();
	LOtextureBase(LOSurface *surface);
	LOtextureBase(SDL_Texture *tx);
	~LOtextureBase();

	int AddUseCount();
	int SubUseCount();

	SDL_Texture* GetTexture(SDL_Rect *re);
	void SetSurface(LOSurface *s);
	LOSurface *GetSurface() { return su; }
	void SetName(LOString &s) { Name.assign(s); }
	LOString GetName() { return Name; }
	bool isBig() { return isbig; }
	void AddSizeTexture(int x, int y, int w, int h, SDL_Texture *tx);

	static uint16_t maxTextureW;
	static uint16_t maxTextureH;
	static SDL_Renderer *render;
	static void AvailableRect(int maxx, int maxy, SDL_Rect *re);

	int ww;
	int hh;
private:
	bool isbig;
	LOString Name;

	LOSurface *su;  //超大纹理surface不会被释放
	std::vector<intptr_t> texlist;
	std::atomic_int usecount;

	void Init();
	void NewBase();
	SDL_Texture *findTextureCache(SDL_Rect *rect);
};


//绑定一个LOtextureBase,可以设置不同的透明度，颜色叠加等
class LOtexture
{
public:
	enum {
		USE_COLOR_MOD = 1,
		USE_BLEND_MOD = 2,
		USE_ALPHA_MOD = 4,
	};

	enum {
		TEX_NONE,
		TEX_IMG,
		TEX_SIMPLE_STR,
		TEX_MULITY_STR,
		TEX_ACTION_STR,
		TEX_COLOR_AREA,
		TEX_DRAW_COMMAND,
		TEX_NSSIMPLE_BTN,
		TEX_EMPTY,
	};

	enum {
		EFF_NONE,
		EFF_MONO,
		EFF_INVERT,
	};
	LOtexture();
	LOtexture(LOtextureBase *base);
	LOtexture(int w, int h, Uint32 format, SDL_TextureAccess access);
	~LOtexture();

	void SetBaseTexture(LOtextureBase *base);
	bool isNull() { return baseTexture == nullptr; }
	bool isAvailable();
	bool isBig();
	bool activeTexture(SDL_Rect *src);
	bool activeActionTxtTexture();
	bool activeFlagControl();
	void setBlendModel(SDL_BlendMode model);
	void setColorModel(Uint8 r, Uint8 g, Uint8 b);
	void setAplhaModel(int alpha);
	void setForceAplha(int alpha);
	bool rollTxtTexture(SDL_Rect *src, SDL_Rect *dst);
	void SaveSurface(LOString *fname) ;  //debug use
	SDL_Surface *getSurface();

	SDL_Texture *GetTexture() { return tex; }
	SDL_Rect *GetSrcRect() { return &srcRect; }

	static std::unordered_map<std::string, LOtextureBase*> baseMap;
	static LOtextureBase *findTextureBaseFromMap(LOString &fname, bool adduse);
	static void addTextureBaseToMap(LOString &fname, LOtextureBase *base);
	static void notUseTextureBase(LOtextureBase *base);
	static LOtextureBase* addNewEditTexture(LOString &fname, int w, int h, Uint32 format, SDL_TextureAccess access);  //只能运行在主线程
	static bool CopySurfaceToTextureRGBA(SDL_Surface *su, SDL_Rect *src, SDL_Texture *tex, SDL_Rect *dst);

	int baseW();
	int baseH();
	int W() { return srcRect.w; }
	int H() { return srcRect.h; }
private:
	void NewTexture();

	int useflag;      //颜色叠加，混合模式，透明模式，低16位表面使用了那种效果，高16位表明具体的混合模式
	SDL_BlendMode blendmodel;
	SDL_Color color;  //RGB的值表示颜色叠加的值，A的值表示透明度
	LOtextureBase *baseTexture;
	SDL_Texture *tex;
	SDL_Rect srcRect;  //显示的源目标
};

#endif // !__LOTEXTURE_H__
