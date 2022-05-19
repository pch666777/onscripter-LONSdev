/*
//文字描述
*/

#ifndef __LOTEXTDESCRIBE_H_
#define __LOTEXTDESCRIBE_H_

#include <SDL.h>
#include <vector>
#include <stdint.h>
#include "../etc/SDL_mem.h"
#include "../etc/LOString.h"
#include "LOFont.h"

//单个文字，位置是相对于文字区域描述的
struct LOWordElement{
	Uint16 unicode = 0;
	int16_t left = 0;
	int16_t top = 0;
	int minx = 0;
	int maxx = 0;
	int miny = 0;
	int maxy = 0;
	int advance = 0;
	//是否半角字符
	bool isEng = true;
	SDL_Surface *surface = nullptr;
	~LOWordElement() {
		if (surface) FreeSurface(surface);
	}
	void setSpace(int size) {
		miny = minx = 0;
		maxx = size / 2;
		maxy = size;
		advance = size / 2;
	}
};

//=======================风格样式===============
class LOTextStyle
{
public:
	enum {
		STYLE_BOLD = 1,
		STYLE_SHADOW = 2,
		STYLE_ITALICS = 4,
	};

	LOTextStyle();
	~LOTextStyle();

	void reset();

	int16_t xcount; //横向文字数
	int16_t ycount; //纵向文字数
	int16_t xsize; //文字横向大小
	int16_t ysize; //文字纵向大小
	int16_t xspace; //横向文字间隔
	int16_t yspace; //纵向文字间隔
	int16_t textIndent;  //首行缩进
	int16_t xshadow; //阴影X方向的偏移
	int16_t yshadow; //阴影y方向的偏移

	//标记，粗体、阴影、斜体等
	int flags;
	SDL_Color fontColor;
private:
};

//======================区域文字描述=================
//区域文字是Ascent、Dscent和颜色模式相同的一组描述
//RUBY是附加在区域描述上的区域描述，阴影只需要在最后排版时考虑大小即可
//位置是相对于行的
class LOTextDescribe
{
public:
	//LOTextDescribe();
	LOTextDescribe(int ascent, int dscent, int cID);
	~LOTextDescribe();

	//
	void GetSizeRect(int16_t *left, int16_t *top, int16_t *right, int16_t *bottom);
	bool CreateLocation(std::vector<LOWordElement*> *wordList, int xspace);

	//rubby
	LOTextDescribe *rubby;
	int xx;
	int yy;
	//四至范围确定宽和高，minx和miny可以小于0表示位置变化
	int16_t minx;
	int16_t maxx;
	int16_t miny;
	int16_t maxy;
	uint16_t startIndex;
	uint16_t endIndex;
	int colorID;
	int Ascent;
	int Dscent;
private:

};

//======================行描述==============
class LOLineDescribe {
public:
	LOLineDescribe();
	~LOLineDescribe();
	bool CreateLocation(std::vector<LOTextDescribe*> *textList, std::vector<LOWordElement*> *wordList, LOTextStyle *style);
	//添加阴影尺寸，只有最后一行才考虑yshadow
	void AddShadowSize(int16_t xspace, int16_t yspace, int16_t xshadow, int16_t yshadow);
	int height();

	int xx;
	int yy;
	//四至范围确定宽和高，minx和miny可以小于0表示位置变化
	int16_t minx;
	int16_t maxx;
	int16_t miny;
	int16_t maxy;
	//对齐方式
	int align;
	//连续文字描述区域索引的起点
	int16_t startIndex;
	//连续文字描述区域索引的终点
	int16_t endIndex;
};


//======================文字纹理描述=================
class LOTextTexture {
public:
	LOTextTexture();
	~LOTextTexture();
	void reset();
	void ClearLines();
	void ClearTexts();
	void ClearWords();
	bool CreateTextDescribe(LOString *s, LOTextStyle *bstyle, LOString *fName);
	void CreateSurface(int w, int h);
	void RenderTextSimple(int x, int y, SDL_Color color);

	//取得纹理的尺寸
	void GetSurfaceSize(int *width, int *height);

	//内容
	LOString text;
	//样式
	LOTextStyle style;
	//字体名称
	LOString fontName;
	//纹理
	SDL_Surface *surface;
	//位置纠正，比如<pos = -12>体现为左移12个像素
	uint16_t Xfix;
	uint16_t Yfix;
	std::vector<LOLineDescribe*> lineList;
	std::vector<LOTextDescribe*> textList;
	std::vector<LOWordElement*> wordList;
private:
	bool divideDescribe(LOFont *font);

	//分行
	void CreateLineDescribe(LOFont *font, int firstSize);
	//添加新行
	LOLineDescribe* CreateNewLine(LOTextDescribe *&des, TTF_Font *font,  LOTextStyle *style, int colorID);
	//添加新的文字描述
	LOTextDescribe* CreateNewTextDes(int ascent, int dscent, int colorID);
	void BlitToRGBA(SDL_Surface *dst, SDL_Surface *src, int x, int y, SDL_Color color);
};


#endif // !__LOTEXTDESCRIBE_H_
