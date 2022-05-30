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
#include "LOTextDescribe.h"

extern void GetFormatBit(SDL_PixelFormat *format, char *bit);
extern void GetFormatBit(Uint32 format, char *bit);
extern bool EqualRect(SDL_Rect *r1, SDL_Rect *r2);

extern void SimpleError(const char *format, ...);

//��ϣ�������ܵĻ���������������SDL���������Զ�ͬһ���������͸�������ģʽ����ɫ����
//��Сд�ļ�����Ϊkey�����⻹��Ҫ����NS��͸��ģʽ�������JPG��˵ :a �� :c�Ľ���ǲ�ͬ��
//LOtextureBaseֱ�Ӷ�Ӧͼ���ļ���LOtexture����LOtextureBaseҪ��ʾ�ķ�Χ�������Ƕ�LOtextureBase
//���и�

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
	void SetName(LOString &s) { Name.assign(s); }
	LOString GetName() { return Name; }
	bool isValid() { return baseSurface || baseTexture; }
	SDL_Texture *GetFullTexture();
	void FreeData();
	SDL_Surface* GetSurface() { return baseSurface; }

	//ֻ��surface��Ч
	bool hasAlpha();
	bool isBig();

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
	//�������ͣ���Ϊ��ͨ����������
	int texType;
	//ֻ�м���ʱ�����������Ч
	bool ispng;
private:
	LOString Name;
	//�Ƿ񳬴�����
	SDL_Surface *baseSurface;
	SDL_Texture *baseTexture;
	LOTextTexture *textTexture;
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

	enum {
		RET_ROLL_FAILD = -1,
		RET_ROLL_CONTINUE = 0,
		RET_ROLL_END = 1
	};

	struct TextData{
		std::vector<LOLineDescribe*> lineList;
		std::vector<LOTextDescribe*> textList;
		std::vector<LOWordElement*> wordList;
		LOTextStyle style;
		LOString fontName;
		~TextData();
		void ClearLines();
		void ClearTexts();
		void ClearWords();
	};


	LOtexture();
	LOtexture(LOShareBaseTexture &base);
	~LOtexture();

	void SetBaseTexture(LOShareBaseTexture &base);
	bool isAvailable();

	//����ĳ���������������surface��ת����texture������ֻӦ������Ⱦ�̵߳���
	bool activeTexture(SDL_Rect *src, bool toGPUtex);

	//ĳЩ����Ⱦ�߳�ʹ�õĹ��̣�����ֱ�ӻ�ȡһ������
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
	static LOShareBaseTexture& addNewEditTexture(LOString &fname, int w, int h, Uint32 format, SDL_TextureAccess access);  //ֻ�����������߳�

	int baseW();
	int baseH();
	int W();
	int H();
	void setEmpty(int w, int h);

	//����������������ͨ���Ǵ������������ǰ��
	bool CreateTextDescribe(LOString *s, LOTextStyle *style, LOString *fontName);
	void CreateSurface(int w, int h);
	//ȡ������ĳߴ�
	void GetTextSurfaceSize(int *width, int *height);
	void RenderTextSimple(int x, int y, SDL_Color color);
	//���н������֣������Ƿ��Ѿ����յ�
	int RollTextTexture(int start, int end);
	void TranzPosition(int *lineID, int *linePos, bool *isend, int position);
	bool SetSurfaceAlpha(uint8_t alp);

	//����ɫ��
	void CreateSimpleColor(int w, int h, SDL_Color color);

	//λ�þ���������<pos = -12>����Ϊ����12�����أ��������������ı������������
	int16_t Xfix;
	int16_t Yfix;
	bool isEdit;
	//�ı��������Ϣ
	std::unique_ptr<TextData> textData;
private:
	void NewTexture();
	void resetSurface();
	void resetTexture();

	void CreateLineDescribe(LOString *s, LOFont *font, int firstSize);
	LOLineDescribe *CreateNewLine(LOTextDescribe *&des, TTF_Font *font, LOTextStyle *style, int colorID);
	LOTextDescribe *CreateNewTextDes(int ascent, int dscent, int colorID);
	void BlitToRGBA(SDL_Surface *dst, SDL_Surface *src, SDL_Rect *dstR, SDL_Rect *srcR, SDL_Color color);
	void BlitShadow(SDL_Surface *dst, SDL_Surface *src, SDL_Rect dstR, SDL_Rect srcR, SDL_Color color, int maxsize);
	bool CheckRect(SDL_Surface *dst, SDL_Surface *src, SDL_Rect *dstR, SDL_Rect *srcR);

	static std::unordered_map<std::string, LOShareBaseTexture> baseMap;

	//�Ƿ��Ƕ�baseTexture�����ã��������ɾ����ʱ��Ӧ��ɾ��surface��texture
	bool isRef;
	int useflag;      //��ɫ���ӣ����ģʽ��͸��ģʽ����16λ����ʹ��������Ч������16λ��������Ļ��ģʽ
	SDL_Color color;  //RGB��ֵ��ʾ��ɫ���ӵ�ֵ��A��ֵ��ʾ͸����
	SDL_BlendMode blendmodel;

	SDL_Texture *texturePtr;
	SDL_Surface *surfacePtr;

	//Ԥ�ڵ���ʾλ�úʹ�С
	SDL_Rect expectRect;

	//ʵ�������λ�úʹ�С
	SDL_Rect actualRect;

	//ԭʼ��Ⱥ͸߶�
	int16_t bw;
	int16_t bh;

	//��������
	LOShareBaseTexture baseTexture;
};

typedef std::shared_ptr<LOtexture> LOShareTexture;



#endif // !__LOTEXTURE_H__
