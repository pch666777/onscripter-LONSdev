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
	int16_t minx = 0;
	int16_t maxx = 0;
	int16_t miny = 0;
	int16_t maxy = 0;
	int16_t advance = 0;
	int16_t size = 0;
	int16_t ascent = 0;
	int16_t dscent = 0;
	//颜色样式索引
	int16_t color = 0;
	//是否半角字符
	bool isEng = true;
	SDL_Surface *surface = nullptr;
	~LOWordElement() {
		if (surface) FreeSurface(surface);
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

	//标记，粗体、阴影、斜体等
	int flags;
	SDL_Color fontColor = { 255,255,255,255 };
private:
};

//======================区域文字描述=================
class LOTextDescribe
{
public:
	LOTextDescribe();
	~LOTextDescribe();

	//释放字列表
	void ReleaseWords(bool ischild);
	void ReleaseSurface(bool ischild);
	void FreeChildSurface();
	//历遍对象，获取尺寸，注意设置初始值
	void GetSizeRect(int *left, int *top, int *right, int *bottom);
	//位置从0,0开始
	void SetPositionZero();
	void MovePosition(int mx, int my);

	//生成文字区域描述链
	//static LOTextDescribe*  CreateTextDescribe(LOString *s, LOTextStyle *bstyle, LOFont *font);

	//父对象，子对象的位置是相对于父对象的
	LOTextDescribe *parent;
	//子对象
	std::vector<LOTextDescribe*> *childs;
	//字列表
	std::vector<LOWordElement*> *words;
	//纹理
	SDL_Surface *surface;

	int x;
	int y;
private:

};

//======================行描述==============
class LOTextLineDescribe {
public:
	LOTextLineDescribe();
	~LOTextLineDescribe();

	int left;
	int top;
	int width;
	int height;
	//对齐方式
	int align;
	//连续的区域描述，链表——层级混合结构
	std::unique_ptr<LOTextDescribe> textDes;
	std::unique_ptr<LOTextLineDescribe> next;
};


//======================区域文字描述根类=================
class LOTextTexture {
public:
	LOTextTexture();
	~LOTextTexture();
	void reset();
	bool CreateTextDescribe(LOString *s, LOTextStyle *bstyle, LOString *fName);

	//内容
	LOString text;
	//样式
	LOTextStyle style;
	//字体名称
	LOString fontName;
	//纹理
	SDL_Surface *surface;
	//
	std::unique_ptr<LOTextLineDescribe> lines;
private:
	bool divideDescribe(LOFont *font);

	//获取所有的字形列表
	void getWordElements(std::vector<LOWordElement*> *list, LOFont *font, int firstSize);
	//段落排布
	void setParagraph(std::vector<LOWordElement*> *list);

};


#endif // !__LOTEXTDESCRIBE_H_
