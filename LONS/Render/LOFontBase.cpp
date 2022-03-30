#include "../Scripter/FuncInterface.h"
#include "LOFontBase.h"
#include "../etc/SDL_mem.h"


LOFontWindow::LOFontWindow() {
	font = nullptr;
	Reset();
}

LOFontWindow::~LOFontWindow() {
	Closefont();
}

//清除字体的设置
void LOFontWindow::Reset() {
	topx = 0;   //文字显示位置的左上角x坐标
	topy = 0;   //文字显示位置的左上角y坐标
	xcount = 256; //横向文字数
	ycount = 256; //纵向文字数
	xsize = 0; //文字横向大小
	ysize = 0; //文字纵向大小
	xspace = 0; //横向文字间隔
	yspace = 0; //纵向文字间隔
	isbold = 0; //是否粗体
	isshaded = 0; //是否阴影

	fontColor.r = 255; fontColor.g = 255; fontColor.b = 255; fontColor.a = 255;
	outlineColor.r = 0; outlineColor.g = 0; outlineColor.b = 0; outlineColor.a = 255;
	shadowColor.r = 0;  shadowColor.g = 0; shadowColor.b = 0; shadowColor.a = 255;
	outlinePix = 0;
	style = 0;       //字体样式，由位控制
	fontName = "default.ttf";  //字体名称
	Closefont();
}

bool LOFontWindow::Openfont() {
	Closefont();
	if (fontName != "*#?") {
		if(fontFullname.length() == 0){
			fontFullname = FunctionInterface::GetReadPath(fontName.c_str()) ;
			//LOLog_i("%s",fontFullname.c_str()) ;
		}
		if (fontFullname.length() > 0) {
			font = TTF_OpenFont(fontFullname.c_str(), xsize);
		}
	}

	if (!font) {
		//LOLog_i("self font") ;
		BinArray *bin = FunctionInterface::fileModule->GetBuiltMem(FunctionInterface::BUILT_FONT);
		if (!bin) return false;
		SDL_RWops *src = SDL_RWFromMem(bin->bin, bin->Length());
		font = TTF_OpenFontRW(src, 1, xsize);
	}
	//else LOLog_i("font %s open sucess",fontName.c_str()) ;

	return font;
}

void LOFontWindow::Closefont() {
	if (font) {
		TTF_CloseFont(font);
		font = nullptr;
	}
}


bool LOFontWindow::ChangeFont(std::string *name, int size) {
	bool change = false;
	if (name != NULL && *name != fontName) change = true;
	else if (size != 0 && size != xsize) change = true;
	if (change) {
		if (name) fontName.assign(*name);
		if (size > 0) {
			xsize = size;
			ysize = size;
		}
		Closefont();
		if(!Openfont())return false;
		ChangeStyle();
	}

	return change;
}

bool LOFontWindow::ChangeStyle() {
	bool change = false;
	int cc = TTF_GetFontStyle(font);
	if ((cc & TTF_STYLE_BOLD) != (isbold % 2)) change = true;
	if (change) {
		style = 0;
		if (isbold) style |= TTF_STYLE_BOLD;
		TTF_SetFontStyle(font, style);
	}
	return change;
}


LOFontElememt::LOFontElememt() {
	su = nullptr;
}

LOFontElememt::~LOFontElememt() {
	if (su) {
		FreeSurface(su);
		su = nullptr;
	}
}

LORubyInfo::~LORubyInfo() {
	if (list) {
		list->clear(true);
		delete list;
	}
}

//依据最高的maxy计算出其他字的下沉数
void TextConitue::CaleY(int ysize, bool shadow, bool outline) {
	fixy = maxy;
	if (fixy > origin) fixy = origin;
	for (int ii = 0; ii < list.size(); ii++) {
		LOFontElememt *el = list.at(ii);
		if(el->maxy > origin) el->y = maxy - el->maxy;
		else el->y = maxy - origin;
	}
	maxh = maxy - miny;
	//最大值加上阴影
	if (shadow) maxh += (maxh / 30 + 1);
	if (maxh < ysize) maxh = ysize;
}


LOLineInfo::LOLineInfo() {
	x = y = w = h = 0;
	rubyLink = nullptr;
}
LOLineInfo::~LOLineInfo() {
	if (rubyLink) {
		for (int ii = 0; ii < rubyLink->size(); ii++) delete rubyLink->at(ii);
		delete rubyLink;
		rubyLink = nullptr;
	}
}
