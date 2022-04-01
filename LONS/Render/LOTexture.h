/*
//纹理
//统一surface和texture减少复杂性
*/

#ifndef __LOTEXTURE_H__
#define __LOTEXTURE_H__

#include <SDL.h>
#include <stdint.h>
#include <unordered_map>
#include <vector>
#include <memory>

#include "../etc/LOString.h"
#include "../etc/SDL_mem.h"
#include "../Scripter/FuncInterface.h"

extern void GetFormatBit(SDL_PixelFormat *format, char *bit);
extern void GetFormatBit(Uint32 format, char *bit);

extern void SimpleError(const char *format, ...);

//我希望尽可能的缓存纹理，可以利用SDL的纹理特性对同一个纹理进行透明、混合模式、颜色叠加
//以小写文件名作为key，此外还需要加入NS的透明模式，比如对JPG来说 :a 和 :c的结果是不同的
//另外还需要应对超大纹理
//从基础纹理上切割下来的小块纹理
//注意，由surface初始化为texture必须在渲染线程进行
class MiniTexture {
public:
	MiniTexture();
	~MiniTexture();
	bool equal(SDL_Rect *rect);
	bool isRectAvailable();
	void converToGPUtex();
	//预期的在基础纹理的位置
	SDL_Rect dst;
	//实际在基础纹理上的位置
	SDL_Rect srt;

	//需要指向指针的指针。。。
	SDL_Texture **tex;
	SDL_Surface **su;
	//是否引用，对应基础纹理小于最大允许的纹理尺寸来说，是引用
	//对于超大纹理 texture和surface都是从纹理上切割下来的
	bool isref;
};

//==============================================

class LOtextureBase
{
public:

	LOtextureBase();
	LOtextureBase(void *mem, int size);
	LOtextureBase(SDL_Surface *su);
	LOtextureBase(SDL_Texture *tx);
	~LOtextureBase();

	//SDL_Texture* GetTexture(SDL_Rect *re);
	void SetSurface(SDL_Surface *su);
	SDL_Surface *GetSurface() { return baseSurface; }
	void SetName(LOString &s) { Name.assign(s); }
	LOString GetName() { return Name; }
	bool isBig() { return isbig; }
	bool isValid() { return baseSurface || baseTexture; }

	//只对surface有效
	bool hasAlpha();
	

	MiniTexture *GetMiniTexture(SDL_Rect *rect);

	static uint16_t maxTextureW;
	static uint16_t maxTextureH;
	static SDL_Renderer *render;
	static void AvailableRect(int maxx, int maxy, SDL_Rect *re);
	//裁剪surface
	static SDL_Surface* ClipSurface(SDL_Surface *surface, SDL_Rect rect);

	//转换ns特有的透明
	static SDL_Surface* ConverNSalpha(SDL_Surface *surface, int cellCount);

	int ww;
	int hh;
	//只有加载时这个参数才有效
	bool ispng;
private:
	//是否超大纹理
	bool isbig;
	SDL_Surface *baseSurface;
	SDL_Texture *baseTexture;
	LOString Name;
	//小块纹理区域列表
	std::vector<MiniTexture> texTureList;

	void baseNew();
};
typedef std::shared_ptr<LOtextureBase> LOShareBaseTexture;
typedef std::unique_ptr<LOtextureBase> LOUniqBaseTexture;

//========================================
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
	LOtexture(LOShareBaseTexture &base);
	//LOtexture(int w, int h, Uint32 format, SDL_TextureAccess access);
	~LOtexture();

	void SetBaseTexture(LOShareBaseTexture &base);
	bool isNull() { return baseTexture == nullptr; }
	bool isAvailable();
	bool isBig();

	//激活某个纹理，并且如果是surface则转换成texture，所以只应该在渲染线程调用
	MiniTexture* activeTexture(SDL_Rect *src, bool toGPUtex);

	bool activeActionTxtTexture();

	bool activeFlagControl();
	void setBlendModel(SDL_BlendMode model);
	void setColorModel(Uint8 r, Uint8 g, Uint8 b);
	void setAplhaModel(int alpha);
	void setForceAplha(int alpha);
	bool rollTxtTexture(SDL_Rect *src, SDL_Rect *dst);
	void SaveSurface(LOString *fname) ;  //debug use
	SDL_Surface *getSurface();

	SDL_Texture *GetTexture();
	//SDL_Rect *GetSrcRect() { return &srcRect; }

	static LOShareBaseTexture findTextureBaseFromMap(LOString &fname);
	static LOShareBaseTexture& addTextureBaseToMap(LOString &fname, LOtextureBase *base);
	static void addTextureBaseToMap(LOString &fname, LOShareBaseTexture &base);
	static void notUseTextureBase(LOShareBaseTexture &base);
	static LOShareBaseTexture& addNewEditTexture(LOString &fname, int w, int h, Uint32 format, SDL_TextureAccess access);  //只能运行在主线程
	static bool CopySurfaceToTextureRGBA(SDL_Surface *su, SDL_Rect *src, SDL_Texture *tex, SDL_Rect *dst);

	int baseW();
	int baseH();
	int W();
	int H();
private:
	void NewTexture();

	static std::unordered_map<std::string, LOShareBaseTexture> baseMap;

	int useflag;      //颜色叠加，混合模式，透明模式，低16位表面使用了那种效果，高16位表明具体的混合模式
	SDL_BlendMode blendmodel;
	SDL_Color color;  //RGB的值表示颜色叠加的值，A的值表示透明度

	//基础纹理
	LOShareBaseTexture baseTexture;
	//当前纹理
	MiniTexture *curTexture;
};

typedef std::shared_ptr<LOtexture> LOShareTexture;



#endif // !__LOTEXTURE_H__
