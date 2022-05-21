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
#include "LOTextDescribe.h"

extern void GetFormatBit(SDL_PixelFormat *format, char *bit);
extern void GetFormatBit(Uint32 format, char *bit);
extern bool EqualRect(SDL_Rect *r1, SDL_Rect *r2);

extern void SimpleError(const char *format, ...);

//我希望尽可能的缓存纹理，可以利用SDL的纹理特性对同一个纹理进行透明、混合模式、颜色叠加
//以小写文件名作为key，此外还需要加入NS的透明模式，比如对JPG来说 :a 和 :c的结果是不同的
//LOtextureBase直接对应图像文件，LOtexture则是LOtextureBase要显示的范围，或者是对LOtextureBase
//的切割

//==============================================

class LOtextureBase
{
public:

	LOtextureBase();
	LOtextureBase(void *mem, int size);
	LOtextureBase(SDL_Surface *su);
	LOtextureBase(SDL_Texture *tx);
	~LOtextureBase();

	void SetSurface(SDL_Surface *su);
	SDL_Surface *GetSurface() { return baseSurface; }
	SDL_Texture *GetTexture() { return baseTexture; }
	void SetName(LOString &s) { Name.assign(s); }
	LOString GetName() { return Name; }
	bool isValid() { return baseSurface || baseTexture; }
	SDL_Texture *GetFullTexture();
	void FreeData();

	//只对surface有效
	bool hasAlpha();

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
	//纹理类型，分为普通和文字纹理
	int texType;
	//只有加载时这个参数才有效
	bool ispng;
	bool isbig;
private:
	LOString Name;
	//是否超大纹理
	SDL_Surface *baseSurface;
	SDL_Texture *baseTexture;
	LOTextTexture *textTexture;
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
	~LOtexture();

	void SetBaseTexture(LOShareBaseTexture &base);
	bool isAvailable();

	//激活某个纹理，并且如果是surface则转换成texture，所以只应该在渲染线程调用
	bool activeTexture(SDL_Rect *src, bool toGPUtex);

	//某些在渲染线程使用的过程，必须直接获取一次纹理
	bool activeFirstTexture();

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
	void FreeRef();
	void MakeGPUTexture();

	static std::unordered_map<std::string, LOShareBaseTexture> baseMap;

	//是否是对baseTexture的引用，如果不是删除的时候应该删除surface和texture
	bool isRef;
	int useflag;      //颜色叠加，混合模式，透明模式，低16位表面使用了那种效果，高16位表明具体的混合模式
	SDL_Color color;  //RGB的值表示颜色叠加的值，A的值表示透明度
	SDL_BlendMode blendmodel;

	SDL_Texture *texturePtr;
	SDL_Surface *surfacePtr;

	//预期的显示位置和大小
	SDL_Rect expectRect;

	//实际纹理的位置和大小
	SDL_Rect actualRect;

	//基础纹理
	LOShareBaseTexture baseTexture;
};

typedef std::shared_ptr<LOtexture> LOShareTexture;



#endif // !__LOTEXTURE_H__
