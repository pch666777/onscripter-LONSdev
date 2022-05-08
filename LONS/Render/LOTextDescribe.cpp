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

//LOTextDescribe::LOTextDescribe() {
//	parent = nullptr;
//	childs = nullptr;
//	words = nullptr;
//	surface = nullptr;
//	colorID = x = y = 0;
//}


LOTextDescribe::LOTextDescribe(int ascent, int dscent, int cID) {
	Ascent = ascent;
	Dscent = dscent;
	colorID = cID;
	words = nullptr;
	rubby = nullptr;
	left = top = width = height = 0;
}

LOTextDescribe::~LOTextDescribe() {
	ClearWords();
	if (rubby) delete rubby;
	rubby = nullptr;
}


void LOTextDescribe::ClearWords() {
	if (words) {
		for (auto iter = words->begin(); iter != words->end(); iter++) delete *iter;
		words = nullptr;
	}
}


bool LOTextDescribe::CreateLocation(int xspace) {
	left = top = width = height = 0;
	//空描述
	if (!words || words->size() == 0) return false;
	//首先计算出X轴
	//minx为负表示字形要提前，advance为字形的宽度
	left = words->at(0)->minx;
	//Y轴总是0，因为我需要屏幕坐标
	top = 0;
	int nextX = left;
	int topY = 0;

	for (auto iter = words->begin(); iter != words->end(); iter++) {
		LOWordElement *el = (*iter);
		el->left = nextX;
		nextX += el->advance;
		nextX += xspace;
		//字形的Y轴要特别注意
		//当maxy大于FontAscent时，surface的Y位置为 FontAscent - maxy，当maxy小于等于FontAscent时，Y位置为0
		//完成后需要根据最小Y值校正Y坐标
		if (el->maxy > Ascent) el->top = Ascent - el->maxy;
		else el->top = 0;
		//找出最小的Y值
		if (el->top < topY) topY = el->top;
	}
	//
	width = nextX - left;

	//Y值校正，同时计算出最大的内容高度
	for (auto iter = words->begin(); iter != words->end(); iter++) {
		LOWordElement *el = (*iter);
		el->top += abs(topY);
		//如果maxy > Ascent，最大高度是 maxy - miny ;
		//否则是 Ascent - miny
		int h = 0;
		if (el->maxy > Ascent) h = el->maxy - el->miny;
		else h = Ascent - el->miny;
		if (h > height) height = h;
	}

	return true;
}


/*
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
*/
/*
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

*/
void LOTextDescribe::AddElement(LOWordElement *el) {
	if (!words) words = new std::vector<LOWordElement*>();
	words->push_back(el);
}


//=================行描述=============
LOLineDescribe::LOLineDescribe() {
	left = top = width = height = align = 0;
	texts = nullptr;
}


LOLineDescribe::~LOLineDescribe() {
	ClearTexts();
}


void LOLineDescribe::ClearTexts() {
	if (texts) {
		for (auto iter = texts->begin(); iter != texts->end(); iter++) {
			delete *iter;
		}
		texts = nullptr;
	}
}

LOTextDescribe *LOLineDescribe::CreateTextDescribe(int ascent, int dscent, int cID) {
	if (!texts) texts = new std::vector<LOTextDescribe*>();
	LOTextDescribe *des = new LOTextDescribe(ascent,dscent, cID);
	des->colorID = cID;
	texts->push_back(des);
	return des;
}


//确定每一个文字区域的尺寸，然后计算出行的尺寸
void LOLineDescribe::CreateLocation(int xspace, int yspace) {
	width = height = 0;
	if (!texts) return;
	int xx = left;
	for (auto iter = texts->begin(); iter != texts->end(); iter++) {
		LOTextDescribe *des = (*iter);
		des->CreateLocation(xspace);
		xx += (des->left + des->width);
		if (des->height > height) height = des->height;
	}
	//高度加上间隔
	height += yspace;
}


//================文字纹理===========
LOTextTexture::LOTextTexture() {
	surface = nullptr;
	lines = nullptr;
}


LOTextTexture::~LOTextTexture(){
	reset();
}


void LOTextTexture::reset() {
	ClearLines();
	if (surface) FreeSurface(surface);
	surface = nullptr;
	text.clear();
	fontName.clear();
}


void LOTextTexture::ClearLines() {
	if (lines) {
		for (auto iter = lines->begin(); iter != lines->end(); iter++) delete *iter;
		lines = nullptr;
	}
}


void LOTextTexture::ClearTexts() {
	if (lines) {
		for (auto iter = lines->begin(); iter != lines->end(); iter++) (*iter)->ClearTexts();
	}
}


bool LOTextTexture::CreateTextDescribe(LOString *s, LOTextStyle *bstyle, LOString *fName) {
	reset();
	if (!s || !bstyle || !fName) return false;
	text = *s;
	style = *bstyle;
	fontName = *fName;
	LOFont *font = LOFont::CreateFont(fontName);
	if (!font) return false;

	lines = new std::vector<LOLineDescribe*>();
	CreateLineDescribe(font, style.xsize);
	CreateLineLocation(style.xspace, style.yspace);
}


bool LOTextTexture::divideDescribe(LOFont *font) {
	return true;
}


LOLineDescribe* LOTextTexture::CreateNewLine(LOTextDescribe *&des, TTF_Font *font, int indent, int colorID) {
	if(!lines) lines = new std::vector<LOLineDescribe*>();
	LOLineDescribe *line = new LOLineDescribe();
	lines->push_back(line);
	line->left = indent;

	if (&des && font) {
		des = line->CreateTextDescribe(TTF_FontAscent(font), TTF_FontDescent(font), colorID);
	}

	return line;
}


//初步分割文字连续描述
void LOTextTexture::CreateLineDescribe(LOFont *font, int firstSize) {
	int colorID = 0;
	int maxWidth = style.xcount * (firstSize + style.xspace);
	const char *buf = text.c_str();
	auto encoder = text.GetEncoder();
	auto wordFont = font->GetFont(firstSize);
	SDL_Color bgColor{ 0,0,0,0 };
	SDL_Color fsColor{ 255,255,255,255 };

	LOTextDescribe *des = nullptr;
	LOLineDescribe *line = CreateNewLine(des, wordFont->font, style.textIndent, colorID);
	int currentX = line->left;

	//可以在这里增加风格变换功能
	while (buf[0] != 0) {
		//忽略的符号
		if (buf[0] == '\r') {
			buf++;
			continue;
		}
		LOWordElement *el = new LOWordElement();
		int ulen = 0;
		el->unicode = encoder->GetUtf16(buf, &ulen);
		buf += ulen;
		el->isEng = (ulen == 1);

		//风格变换开始
		if (el->unicode == '{') {

		}
		else if (el->unicode == '}') {
			//风格变换结束
		}
		else if (el->unicode == '\n') {
			delete el;
			el = nullptr;
			line = CreateNewLine(des, wordFont->font, style.textIndent, colorID);
			currentX = line->left;
		}
		else {
			if (LOString::GetCharacter(buf - ulen) != LOString::CHARACTER_SPACE && wordFont->font) {
				//字符都是基线对齐的，基线即FontAscent，但这是一个世界的XY坐标系，不利于理解，需要转换成屏幕坐标系
				//假定一条在世界坐标系(FontAscent,0)的屏幕坐标系基线，那么有：
				//当maxy大于FontAscent时，surface的Y位置为 FontAscent - maxy，当maxy小于等于FontAscent时，Y位置为0
				//X方向上，minx即为X的位置，surface的宽度 = advance - minx，即下一个符号的位置
				TTF_GlyphMetrics(wordFont->font, el->unicode, &el->minx, &el->maxx, &el->miny, &el->maxy, &el->advance);
				el->surface = LTTF_RenderGlyph_Shaded(wordFont->font, el->unicode, fsColor, bgColor);
				//检查是否需要新行了
				if (currentX + el->minx + el->advance > maxWidth) {
					line = CreateNewLine(des, wordFont->font, style.textIndent, colorID);
					currentX = line->left;
				}

				des->AddElement(el);
				//std::string s = "d:\\aaaa\\" + std::to_string(count) + ".bmp";
				//SDL_SaveBMP(ele->surface, s.c_str());
				//count++;
				currentX += (el->minx + el->advance);
			}
			else {
				//空格符号
				el->setSpace(firstSize);
				des->AddElement(el);
			}

			//间隔
			currentX += style.xspace;
		}
	}
	font->CloseAll();
}



void LOTextTexture::CreateLineLocation(int xspace, int yspace) {
	for (auto iter = lines->begin(); iter != lines->end(); iter++) {
		(*iter)->CreateLocation(xspace, yspace);
	}
}


void LOTextTexture::GetSize(int *width, int *height) {
	int left, right, top, bottom;
	left = right = top = bottom = 0;
	if (lines) {
		for (auto iter = lines->begin(); iter != lines->end(); iter++) {
			LOLineDescribe *line = (*iter);
			if (line->left < left) left = (*iter)->left;
			if (line->width + line->left > right) right = line->width + line->left;
			if (line->top < top) top = line->top;
			if (line->top + line->height > bottom) bottom = line->top + line->height;
		}
	}
	if (width) *width = right - left;
	if (height) *height = bottom - top;
}