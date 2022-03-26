#ifndef __LOFONTINFO_H__
#define __LOFONTINFO_H__


#include "LOFontBase.h"
#include "LOSurface.h"
#include "../etc/BinArray.h"
#include "../etc/LOStack.h"
#include "../etc/LOString.h"


#include <SDL.h>
#include <SDL_ttf.h>
#include <unordered_map>

//#include "LOLayerInfo.h"



//extern LOFontWindow glo_font_cfg;   //全局的字体设置


//每个脚本线程配一个字体管理器
class LOFontManager
{
public:
	//字形参数
	struct GlyphValue{
		bool shadow = false;
		bool outline = false;
		int maxwidth = 200;
		int firstIndent = 0;  //首行缩进
	};

	//字体风格
	enum {
		STYLE_NORMAL = 0,
		STYLE_BOLD = 1,     //粗体
		STYLE_SHADOW = 2,   //阴影
		STYLE_OUTLINE = 4,  //描边
		STYLE_ITALICS = 8,  //斜体
		STYLE_UNDERLINE = 16,  //下划线
		STYLE_STRIKETHROUGH = 32,  //中划线
		STYLE_TATEYOKO = 64,     //竖行模式
		STYLE_RUBY = 128,     //该文字为注音文字
	};

	//使用哪些设置，如果没有则使用默认设置
	enum {
		USE_SIZE_FONT = 1,   //文字大小，包括横向大小和纵向大小
		USE_FONT_COUNT = 2,   //文字横向字数和纵向字数
		USE_FONT_INTERVAL = 4,    //文字横向和纵向间隔
		USE_SIZE_RUBYON = 8,  //注音文字大小
		USE_SIZE_SHADOW = 16,  //阴影大小
		USE_SIZE_OUTLINE = 32,  //描边大小
		USE_FONTNAME = 64,     //字体名称
	};

	//文字对其模式
	enum {
		ALIG_BOTTOM,  //底部对齐，默认
		ALIG_MIDDLE,  //中间对齐
		ALIG_TOP,     //顶部对齐
	};

	enum {
		RUBY_OFF,
		RUBY_ON,
		RUBY_LINE
	};

	LOFontManager();
	~LOFontManager();

	int rubySize[3];   //0:width 1:height 2:state 
	LOString rubyFontName;
	bool usehtml;   //{解释}

	int GetRubyPreHeight();
	LOSurface *CreateSurface(LOStack<LineComposition> *lines, int cellcount);
	void RenderColor(LOSurface *tex, LOStack<LineComposition> *comlist, SDL_Color *fg, int cellx,bool isshaded);
	LOStack<LineComposition>* RenderTextCore(LOSurface *&tex,LOFontWindow *fontwin, LOString *s, SDL_Color *color, int cellcount, int startx);
	void FreeAllGlyphs(LOStack<LineComposition> *lines);
	void ResetMe();
private:
	SDL_Color bgColor; 
	SDL_Color shadowColor;
	bool st_usekinsoku; //是否使用句尾禁止换行符
	LOString st_kinsoku; //句尾禁止换行符

	const char* ParseStyle(LOFontWindow *fontwin, const char *cbuf, bool *isnew,int *align);
	void ToColor(SDL_Surface *su, SDL_Color *color);
	bool BlitToRGBA(SDL_Surface *src, SDL_Rect *srcrect,SDL_Surface *dst,SDL_Rect *dstrect,SDL_Color *color,bool isshaded);
	void GetBorder(SDL_Surface *su, int *minx, int *maxx);
	LOStack<LineComposition> *GetGlyphs(LOFontWindow *fontwin, LOString *s, GlyphValue *param);
	void AlignLines(LOStack<LineComposition> *lines,int yspace,bool isshadow);
};


#endif // !__LOFONTINFO_H__
