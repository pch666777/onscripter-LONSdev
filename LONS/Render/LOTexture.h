/*
//����
//ͳһsurface��texture���ٸ�����
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

//��ϣ�������ܵĻ���������������SDL���������Զ�ͬһ���������͸�������ģʽ����ɫ����
//��Сд�ļ�����Ϊkey�����⻹��Ҫ����NS��͸��ģʽ�������JPG��˵ :a �� :c�Ľ���ǲ�ͬ��
//���⻹��ҪӦ�Գ�������
//�ӻ����������и�������С������
//ע�⣬��surface��ʼ��Ϊtexture��������Ⱦ�߳̽���
class MiniTexture {
public:
	MiniTexture();
	~MiniTexture();
	bool equal(SDL_Rect *rect);
	bool isRectAvailable();
	void converToGPUtex();
	//Ԥ�ڵ��ڻ��������λ��
	SDL_Rect dst;
	//ʵ���ڻ��������ϵ�λ��
	SDL_Rect srt;

	//��Ҫָ��ָ���ָ�롣����
	SDL_Texture **tex;
	SDL_Surface **su;
	//�Ƿ����ã���Ӧ��������С��������������ߴ���˵��������
	//���ڳ������� texture��surface���Ǵ��������и�������
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

	//ֻ��surface��Ч
	bool hasAlpha();
	

	MiniTexture *GetMiniTexture(SDL_Rect *rect);

	static uint16_t maxTextureW;
	static uint16_t maxTextureH;
	static SDL_Renderer *render;
	static void AvailableRect(int maxx, int maxy, SDL_Rect *re);
	//�ü�surface
	static SDL_Surface* ClipSurface(SDL_Surface *surface, SDL_Rect rect);

	//ת��ns���е�͸��
	static SDL_Surface* ConverNSalpha(SDL_Surface *surface, int cellCount);

	int ww;
	int hh;
	//ֻ�м���ʱ�����������Ч
	bool ispng;
private:
	//�Ƿ񳬴�����
	bool isbig;
	SDL_Surface *baseSurface;
	SDL_Texture *baseTexture;
	LOString Name;
	//С�����������б�
	std::vector<MiniTexture> texTureList;

	void baseNew();
};
typedef std::shared_ptr<LOtextureBase> LOShareBaseTexture;
typedef std::unique_ptr<LOtextureBase> LOUniqBaseTexture;

//========================================
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
	LOtexture(LOShareBaseTexture &base);
	//LOtexture(int w, int h, Uint32 format, SDL_TextureAccess access);
	~LOtexture();

	void SetBaseTexture(LOShareBaseTexture &base);
	bool isNull() { return baseTexture == nullptr; }
	bool isAvailable();
	bool isBig();

	//����ĳ���������������surface��ת����texture������ֻӦ������Ⱦ�̵߳���
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
	static LOShareBaseTexture& addNewEditTexture(LOString &fname, int w, int h, Uint32 format, SDL_TextureAccess access);  //ֻ�����������߳�
	static bool CopySurfaceToTextureRGBA(SDL_Surface *su, SDL_Rect *src, SDL_Texture *tex, SDL_Rect *dst);

	int baseW();
	int baseH();
	int W();
	int H();
private:
	void NewTexture();

	static std::unordered_map<std::string, LOShareBaseTexture> baseMap;

	int useflag;      //��ɫ���ӣ����ģʽ��͸��ģʽ����16λ����ʹ��������Ч������16λ��������Ļ��ģʽ
	SDL_BlendMode blendmodel;
	SDL_Color color;  //RGB��ֵ��ʾ��ɫ���ӵ�ֵ��A��ֵ��ʾ͸����

	//��������
	LOShareBaseTexture baseTexture;
	//��ǰ����
	MiniTexture *curTexture;
};

typedef std::shared_ptr<LOtexture> LOShareTexture;



#endif // !__LOTEXTURE_H__
