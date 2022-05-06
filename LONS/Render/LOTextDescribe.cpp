/*
//文字描述
*/

#include "LOTextDescribe.h"
#include "../etc/LOLog.h"

//================================================
LOTextStyle::LOTextStyle() {
	xcount = 1024;
	ycount = 1024;
	xsize = 16;
	ysize = 16;
	xspace = 0;
	yspace = 1;
	textIndent = 0;
	flags = 0;
}

LOTextStyle::~LOTextStyle() {

}

//================================================

LOTextDescribe::LOTextDescribe() {
	parent = nullptr;
	childs = nullptr;
	words = nullptr;
	surface = nullptr;
	x = y = 0;
}

LOTextDescribe::~LOTextDescribe() {
	if (words) ReleaseWords(false);
	if (surface) ReleaseSurface(false);
	if (childs) {
		for (auto iter = childs->begin(); iter != childs->end(); iter++) delete *iter;
		delete childs;
	}
}



void LOTextDescribe::ReleaseWords(bool ischild) {
	if (words) {
		for (auto iter = words->begin(); iter != words->end(); iter++) delete *iter;
		delete[] words;
		words = nullptr;
	}
	//子对象的数据一起释放
	if (ischild && childs) {
		for (auto iter = childs->begin(); iter != childs->end(); iter++) (*iter)->ReleaseWords(ischild);
	}
}


void LOTextDescribe::ReleaseSurface(bool ischild) {
	if (surface) {
		FreeSurface(surface);
		surface = nullptr;
	}
	//子对象的数据一起释放
	if (ischild && childs) {
		for (auto iter = childs->begin(); iter != childs->end(); iter++) (*iter)->ReleaseSurface(ischild);
	}
}


void LOTextDescribe::FreeChildSurface() {
	if (childs) {
		for (auto iter = childs->begin(); iter != childs->end(); iter++) (*iter)->ReleaseSurface(true);
	}
}


void LOTextDescribe::GetSizeRect(int *left, int *top, int *right, int *bottom) {
	//从最底层的对象开始
	if (childs) {
		for (auto iter = childs->begin(); iter != childs->end(); iter++) {
			(*iter)->GetSizeRect(left, top, right, bottom);
		}
	}

	//获取相对于根对象的x y
	int x1, x2, y1, y2;
	x1 = x2 = y1 = y2 = 0;

	LOTextDescribe *tdes = this;
	while (tdes->parent) {
		tdes = tdes->parent;
		x1 += tdes->x;
		y1 += tdes->y;
	}
	if (surface) {
		x2 = surface->w + x1;
		y2 = surface->h + x2;
	}

	if (left && x1 < *left) *left = x1;
	if (top  && y1 < *top) *top = y1;
	if (right && x2 > *right) *right = x2;
	if (bottom && y2 > *bottom) *bottom = y2;
}


void LOTextDescribe::SetPositionZero() {
	int left, top ;
	left = top = 0;
	GetSizeRect(&left, &top, nullptr, nullptr);
	if (left != 0 || top != 0) MovePosition(0 - left, 0 - top);
}


void LOTextDescribe::MovePosition(int mx, int my) {
	x += mx;
	y += my;
	if (childs) {
		for (auto iter = childs->begin(); iter != childs->end(); iter++) {
			(*iter)->MovePosition(mx, my);
		}
	}
}


//=================行描述=============
LOTextLineDescribe::LOTextLineDescribe() {
	left = top = width = height = align = 0;
}



//========================
LOTextTexture::LOTextTexture() {
	surface = nullptr;
}


LOTextTexture::~LOTextTexture(){
	reset();
}


void LOTextTexture::reset() {
	lines.reset();
	if (surface) FreeSurface(surface);
	surface = nullptr;
	text.clear();
	fontName.clear();
}


bool LOTextTexture::CreateTextDescribe(LOString *s, LOTextStyle *bstyle, LOString *fName) {
	reset();
	if (!s || !bstyle || !fName) return false;
	text = *s;
	style = *bstyle;
	fontName = *fName;
	LOFont *font = LOFont::CreateFont(fontName);
	if (!font) return false;

	//取出所有文字列表
	std::vector<LOWordElement*> list;
	getWordElements(&list, font, style.xsize);
}


bool LOTextTexture::divideDescribe(LOFont *font) {
	return true;
}


//首先获取所有的字形数据
void LOTextTexture::getWordElements(std::vector<LOWordElement*> *list, LOFont *font, int firstSize) {
	int minx, maxx, miny, maxy, advance;
	int colorID = 0;
	const char *buf = text.c_str();
	auto encoder = text.GetEncoder();
	auto wordFont = font->GetFont(firstSize);
	SDL_Color bgColor{ 0,0,0,0 };
	SDL_Color fsColor{ 255,255,255,255 };

	//int count = 0;
	
	//可以在这里增加风格变换功能
	while (buf[0] != 0) {
		LOWordElement *ele = new LOWordElement();
		int wordLen = 0;
		ele->unicode = encoder->GetUtf16(buf, &wordLen);
		ele->isEng = (wordLen == 1);
		ele->size = firstSize;
		ele->color = colorID;
		buf += wordLen;

		if (wordFont->font) {
			ele->dscent = wordFont->descent;
			ele->ascent = wordFont->ascent;
			//字符都是基线对齐的，基线即FontAscent，但这是一个世界的XY坐标系，不利于理解，需要转换成屏幕坐标系
			//假定一条在世界坐标系(FontAscent,0)的屏幕坐标系基线，那么有：
			//当maxy大于FontAscent时，surface的Y位置为 FontAscent - maxy，当maxy小于等于FontAscent时，Y位置为0
			//X方向上，minx即为X的位置，surface的宽度 = advance - minx，即下一个符号的位置
			TTF_GlyphMetrics(wordFont->font, ele->unicode, &minx, &maxx, &miny, &maxy, &advance);
			ele->surface = LTTF_RenderGlyph_Shaded(wordFont->font, ele->unicode, fsColor, bgColor);
			
			//std::string s = "d:\\aaaa\\" + std::to_string(count) + ".bmp";
			//SDL_SaveBMP(ele->surface, s.c_str());
			//count++;

		}
		else {
			//当成一个空格
			ele->minx = 0;
			ele->maxx = firstSize / 2;
			ele->miny = 0;
			ele->maxy = firstSize;
			ele->advance = firstSize / 2;
		}

		list->push_back(ele);
	}
	font->CloseAll();
}


//排布段落
void LOTextTexture::setParagraph(std::vector<LOWordElement*> *list) {
	int sectionMax = (style.xsize + style.xspace) * style.xcount;
	int currentX = style.textIndent;
	int currentY = 0;

	LOTextLineDescribe *last = nullptr;
	LOTextLineDescribe *cur = new LOTextLineDescribe;
	lines.reset(cur);

	for (auto iter = list->begin(); iter != list->end(); iter++) {
		LOWordElement *el = (*iter);

		//下一行
		if (currentX > sectionMax || el->unicode == '\n') {
			//每完成一行，需要准确计算出行高，和Y位置校正
			last = cur;

			currentY += cur->height + style.yspace;
			if (el->unicode == '\n') currentX = style.textIndent;
			else currentX = 0;
			
			cur = new LOTextLineDescribe;
			last->next.reset(cur);
		}

		//计算出每个字形的位置
		if (el->minx < 0) el->left = currentX + el->minx;
		else el->left = currentX;
		currentX += (el->advance - el->minx + el->left);

		//当maxy大于FontAscent时，surface的Y位置为 FontAscent - maxy，当maxy小于等于FontAscent时，Y位置为0
		if (el->maxy > el->ascent) el->top = el->ascent - el->maxy;
		else el->top = 0;
	}


}