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

//单个文字
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
	SDL_Color fontColor = { 255,255,255,255 };
private:
};

//======================区域文字描述=================
//区域文字是Ascent、Dscent和颜色模式相同的一组描述
//RUBY是附加在区域描述上的区域描述
class LOTextDescribe
{
public:
	//LOTextDescribe();
	LOTextDescribe(int ascent, int dscent, int cID);
	~LOTextDescribe();

	//历遍对象，获取尺寸，注意设置初始值
	//void GetSizeRect(int *left, int *top, int *right, int *bottom);
	//位置从0,0开始
	//void SetPositionZero();
	//void MovePosition(int mx, int my);
	void AddElement(LOWordElement *el);
	void ClearWords();
	bool CreateLocation(int xspace);

	//rubby
	LOTextDescribe *rubby;
	//字列表
	std::vector<LOWordElement*> *words;

	int left;
	int top;
	int width;
	int height;
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
	void ClearTexts();
	LOTextDescribe *CreateTextDescribe(int ascent, int dscent, int cID);
	void CreateLocation(int xspace, int yspace);

	int left;
	int top;
	int width;
	int height;
	//对齐方式
	int align;
	//连续的文字描述区域
	std::vector<LOTextDescribe*> *texts;
};


//======================文字纹理描述=================
class LOTextTexture {
public:
	LOTextTexture();
	~LOTextTexture();
	void reset();
	void ClearLines();
	void ClearTexts();
	bool CreateTextDescribe(LOString *s, LOTextStyle *bstyle, LOString *fName);

	//取得纹理的尺寸
	void GetSize(int *width, int *height);

	//内容
	LOString text;
	//样式
	LOTextStyle style;
	//字体名称
	LOString fontName;
	//纹理
	SDL_Surface *surface;
	//
	std::vector<LOLineDescribe*> *lines;
private:
	bool divideDescribe(LOFont *font);

	//分行
	void CreateLineDescribe(LOFont *font, int firstSize);
	//重建行
	void CreateLineLocation(int xspace, int yspace);

	//添加新行
	LOLineDescribe* CreateNewLine(LOTextDescribe *&des, TTF_Font *font,  int indent, int colorID);
};


#endif // !__LOTEXTDESCRIBE_H_
