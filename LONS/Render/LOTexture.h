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
	void SetName(LOString &s) { Name.assign(s); }
	LOString GetName() { return Name; }
	bool isValid() { return baseSurface || baseTexture; }
	SDL_Texture *GetFullTexture();
	void FreeData();
	SDL_Surface* GetSurface() { return baseSurface; }

	//只对surface有效
	bool hasAlpha();
	bool isBig();

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
private:
	LOString Name;
	//是否超大纹理
	SDL_Surface *baseSurface;
	SDL_Texture *baseTexture;
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
		USE_TEXTACTION_MOD = 8,
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
		TEX_VIDEO,
		TEX_LAYERMODE,
		//TEX_EMPTY,
		TEX_CONTROL,
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

	struct TextRoll{
		enum {
			FLAGS_LINE_START = 1,
			FLAGS_LINE_END = 2
		};
		int16_t index = 0;
		int16_t startPos = 0;
		int16_t endPos = 0;
		int16_t flags = 0;
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

	bool activeFlagControl();
	void setBlendModel(SDL_BlendMode model);
	void setColorModel(Uint8 r, Uint8 g, Uint8 b);
	void setAplhaModel(int alpha);
	void setForceAplha(int alpha);
	void setFlags(int f) { useflag |= f; }
	void unSetFlags(int f) { useflag &= (~f); }
	bool isTextAction() { return useflag & USE_TEXTACTION_MOD; }
	bool isTextTexture() { if (textData) return true; else return false; }
	void SaveSurface(LOString *fname) ;  //debug use

	SDL_Surface *getSurface();
	SDL_Texture *GetTexture();

	static LOShareBaseTexture findTextureBaseFromMap(LOString &fname);
	static LOShareBaseTexture& addTextureBaseToMap(LOString &fname, LOtextureBase *base);
	static void addTextureBaseToMap(LOString &fname, LOShareBaseTexture &base);
	static void notUseTextureBase(LOShareBaseTexture &base);
	static LOShareBaseTexture& addNewEditTexture(LOString &fname, int w, int h, Uint32 format, SDL_TextureAccess access);  //只能运行在主线程

	int baseW();
	int baseH();
	int W();
	int H();
	void setEmpty(int w, int h);

	//创建文字文描述，通常是创建文字纹理的前奏
	bool CreateTextDescribe(LOString *s, LOTextStyle *style, LOString *fontName);
	void CreateSurface(int w, int h);
	//取得纹理的尺寸
	void GetTextSurfaceSize(int *width, int *height);
	void RenderTextSimple(int x, int y, SDL_Color color);
	//按行渐显文字，返回是否已经到终点
	int RollTextTexture(int start, int end);
	void TranzPosition(int *lineID, int *linePos, bool *isend, int position);
	int GetTextTextureEnd();
    //获取最后一行的最后一个位置
    bool GetTextShowEnd(int *xx, int *yy, int *endLineH);

	//创建色块
	void CreateSimpleColor(int w, int h, SDL_Color color);

	//创建一个纹理
	void CreateDstTexture(int w, int h, int access);
	void CreateDstTexture2(int w, int h, Uint32 format ,int access);
	//将纹理中的数据转移到surface
	void CopyTextureToSurface(bool freeTex);
	//检查surface中是否是同一种颜色
	bool CheckColor(SDL_Color *cc, int diff);
    //根据制定颜色，或者位置设置透明
    static SDL_Surface* CreateTransAlpha(SDL_Surface *src, int position) ;
	
	//重置硬件纹理的混合模式、透明度、颜色混合
	static void ResetTextureMode(SDL_Texture *tex);

	void resetSurface();
	void resetTexture();


	//位置纠正，比如<pos = -12>体现为左移12个像素，这两个参数对文本纹理才有意义
	int16_t Xfix;
	int16_t Yfix;
	bool isEdit;
	//文本纹理的信息
	std::unique_ptr<TextData> textData;
private:
	void NewTexture();

	void CreateLineDescribe(LOString *s, LOFont *font, int firstSize);
	LOLineDescribe *CreateNewLine(LOTextDescribe *&des, TTF_Font *font, LOTextStyle *style, int colorID);
	LOTextDescribe *CreateNewTextDes(int ascent, int dscent, int colorID);
	void BlitToRGBA(SDL_Surface *dst, SDL_Surface *src, SDL_Rect *dstR, SDL_Rect *srcR, SDL_Color color);
	void BlitShadow(SDL_Surface *dst, SDL_Surface *src, SDL_Rect dstR, SDL_Rect srcR, SDL_Color color, int maxsize);
	bool CheckRect(SDL_Surface *dst, SDL_Surface *src, SDL_Rect *dstR, SDL_Rect *srcR);
	void CopySurfaceToTexture(SDL_Texture *dst, SDL_Surface *src, SDL_Rect rect);

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

	//原始宽度和高度
	int16_t bw;
	int16_t bh;

	//基础纹理
	LOShareBaseTexture baseTexture;
};

typedef std::shared_ptr<LOtexture> LOShareTexture;


#endif // !__LOTEXTURE_H__
