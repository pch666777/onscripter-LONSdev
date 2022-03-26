/*
文字排版从来都是一件很复杂的事情。。。
LONS的文字排版由行组成（横行或竖行），行由若干个连续区域描述组成，
连续区域是一组大小相同，风格相同的多个字符，
LONS应该是符号换行严格的，即结尾为标点符号，如，。！等单符号，则应该留在行尾，《书名号引号等应该进入下一行

*/

#ifndef __LOFONTBASE_H__
#define	__LOFONTBASE_H__

#include <SDL.h>
#include <SDL_ttf.h>
#include <string>
#include "../etc/LOStack.h"


//单个的字体
class LOFontElememt
{
public:
	LOFontElememt();
	~LOFontElememt();

	Uint16 unicode;     //unicode的值
	int minx;
	int miny;
	int maxx;
	int maxy;
	int advance;
	int x;
	int y;
	//int tminy;
	//int tmaxy;
	int origin;
	int torig;   //实际对齐的基线

	bool isMultibyte;

	SDL_Surface *su;
	int Width() { return maxx - minx; }
	int Height() {
		if (miny < 0) return maxy - miny;
		else return maxy;
	}
private:
};

//连续描述区域，是一群原点坐标相同的字形组
struct TextConitue {
	~TextConitue() {
		list.clear(true);
	}
	//int top = 4096; //区域顶边
	//int bottom = -4096; //区域的底边
	int origin = 0;
	int sumx = 0;
	int x = 0; //x偏移
	int y = 0; //y偏移
	int maxy = 0;  //最大Y偏移
	int miny = 10;  //最小的Y偏移
	int fixy = 0;   //修正的Y偏移
	int maxh = 0;   //最大高度
	int align = 0;  //对齐方式
	//int height() { return bottom - top; }
	void CaleY(int ysize, bool shadow, bool outline);
	LOStack<LOFontElememt> list;
};

//行排版
struct LineComposition
{
	~LineComposition() {
		cells.clear(true);
	}

	enum {
		LINE_NORMAL,
		LINE_RUBY,
		LINE_FINISH_COPY,
	};
	int x = 0;
	int y = 0;
	int sumx = 0;    //文字的尾部坐标
	int height = 0;  //本行的最大高度
	int type = LINE_NORMAL;
	int align = 0;
	int finish = 0;


	LOStack<LOFontElememt> cells;
};

//ruby结构
class LORubyInfo {
public:
	~LORubyInfo();
	int startcount;
	int endcount;
	LOStack<LOFontElememt> *list = nullptr;  //显示在字体上方的ruby内容
};

class LOFontWindow {
public:
	LOFontWindow();
	~LOFontWindow();
	int topx;   //文字显示位置的左上角x坐标
	int topy;   //文字显示位置的左上角y坐标
	int xcount; //横向文字数
	int ycount; //纵向文字数
	int xsize; //文字横向大小
	int ysize; //文字纵向大小
	int xspace; //横向文字间隔
	int yspace; //纵向文字间隔
	int isbold; //是否粗体
	int isshaded; //是否阴影

	SDL_Color fontColor;	  //默认的文字颜色
	SDL_Color outlineColor;  //描边颜色
	SDL_Color shadowColor;   //阴影颜色
	//int shadowPix;   //阴影按字体大小设置每20递增1
	int outlinePix;  //描边大小，像素
	int style;       //字体样式，由位控制
	std::string fontName;  //字体名称
	TTF_Font *font;
	std::string fontFullname; //全字体路径

	//横向换行的最大值
	int GetMaxLine() { return xcount * (xsize + xspace) - xspace; }
	void Reset();
	bool ChangeFont(std::string *name, int size);
	bool ChangeStyle();

	bool Openfont();
	void Closefont();
};

#endif // !__LOFONTBASE_H__
