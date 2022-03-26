/*
//����
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

//��ϣ�������ܵĻ���������������SDL���������Զ�ͬһ���������͸�������ģʽ����ɫ����
//��Сд�ļ�����Ϊkey�����⻹��Ҫ����NS��͸��ģʽ�������JPG��˵ :a �� :c�Ľ���ǲ�ͬ��
//���⻹��ҪӦ�Գ�������

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

	LOSurface *su;  //��������surface���ᱻ�ͷ�
	std::vector<intptr_t> texlist;
	std::atomic_int usecount;

	void Init();
	void NewBase();
	SDL_Texture *findTextureCache(SDL_Rect *rect);
};


//��һ��LOtextureBase,�������ò�ͬ��͸���ȣ���ɫ���ӵ�
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
	static LOtextureBase* addNewEditTexture(LOString &fname, int w, int h, Uint32 format, SDL_TextureAccess access);  //ֻ�����������߳�
	static bool CopySurfaceToTextureRGBA(SDL_Surface *su, SDL_Rect *src, SDL_Texture *tex, SDL_Rect *dst);

	int baseW();
	int baseH();
	int W() { return srcRect.w; }
	int H() { return srcRect.h; }
private:
	void NewTexture();

	int useflag;      //��ɫ���ӣ����ģʽ��͸��ģʽ����16λ����ʹ��������Ч������16λ��������Ļ��ģʽ
	SDL_BlendMode blendmodel;
	SDL_Color color;  //RGB��ֵ��ʾ��ɫ���ӵ�ֵ��A��ֵ��ʾ͸����
	LOtextureBase *baseTexture;
	SDL_Texture *tex;
	SDL_Rect srcRect;  //��ʾ��ԴĿ��
};

#endif // !__LOTEXTURE_H__
