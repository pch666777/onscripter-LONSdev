/*
//文字描述
*/

#include "LOTextDescribe.h"
#include "../etc/LOLog.h"

//================================================
LOTextStyle::LOTextStyle() {
	reset();
}

LOTextStyle::~LOTextStyle() {

}


void LOTextStyle::reset() {
	xcount = 1024;
	ycount = 1024;
	xsize = 16;
	ysize = 16;
	xspace = 0;
	yspace = 1;
	textIndent = 0;
	flags = 0;
	xshadow = 0;
	yshadow = 0;
	fontColor = { 255,255,255,255 };
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
	rubby = nullptr;
	xx = yy = 0;
	minx = maxx = miny = maxy;
}

LOTextDescribe::~LOTextDescribe() {
	if (rubby) delete rubby;
	rubby = nullptr;
}



//确定文字区域的四至范围，阴影只需要在最后排版时考虑大小即可
bool LOTextDescribe::CreateLocation(std::vector<LOWordElement*> *wordList, int xspace) {
	minx = maxx = miny = maxy = 0;
	if (!wordList || startIndex == endIndex || startIndex >= wordList->size() )  return false;

	//空描述
	//首先计算出X轴
	//minx为负表示字形要提前，advance为字形的宽度
	minx = wordList->at(startIndex)->minx;
	maxx = minx;
	//只需要处理字形提前
	if (minx > 0) minx = 0;
	//Y轴总是0，因为我需要屏幕坐标
	miny = maxy = 0;
	int topY = 0;

	for (int ii = startIndex; ii < wordList->size() && ii < endIndex; ii++) {
		LOWordElement *el = wordList->at(ii);
		el->left = maxx;
		maxx += el->advance;
		maxx += xspace;
		//字形的Y轴要特别注意
		//当maxy大于FontAscent时，surface的Y位置为 FontAscent - maxy，当maxy小于等于FontAscent时，Y位置为0
		//完成后需要根据最小Y值校正Y坐标
		if (el->maxy > Ascent) el->top = Ascent - el->maxy;
		else el->top = 0;
		//找出最小的Y值
		if (el->top < topY) topY = el->top;
	}

	//Y值校正，同时计算出最大的内容高度
	for (int ii = startIndex; ii < wordList->size() && ii < endIndex; ii++) {
		LOWordElement *el = wordList->at(ii);
		el->top += abs(topY);
		//如果maxy > Ascent，最大高度是 maxy - miny ;
		//否则是 Ascent - miny
		int h = 0;
		if (el->maxy > Ascent) h = el->maxy - el->miny;
		else h = Ascent - el->miny;
		if (h > maxy) maxy = h;
	}

	//忽略最后一个间隔
	maxx -= xspace;

	return true;
}


//有注音时考虑注音
void LOTextDescribe::GetSizeRect(int16_t *left, int16_t *top, int16_t *right, int16_t *bottom) {
	if (left) *left = minx;
	if (top) *top = miny;
	if (right) *right = maxx;
	if (bottom) *bottom = maxy;
}

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


//=================行描述=============
LOLineDescribe::LOLineDescribe() {
	xx = yy = 0;
	minx = maxx = miny = maxy = 0;
	align = 0;
	startIndex = endIndex = -1;
}


LOLineDescribe::~LOLineDescribe() {
}



//确定每一个文字区域的尺寸，然后计算出行的尺寸
bool LOLineDescribe::CreateLocation(std::vector<LOTextDescribe*> *textList, std::vector<LOWordElement*> *wordList, LOTextStyle *style) {
	minx = miny = 0x7fff;
	maxx = maxy = 0;
	if (!textList || startIndex == endIndex || startIndex >= textList->size())  return false;
	//计算每个文字区域，都是相对于行的位置
	int currentX = 0;
	for (int ii = startIndex; ii < endIndex && ii < textList->size(); ii++) {
		LOTextDescribe *des = textList->at(ii);
		des->CreateLocation(wordList ,style->xspace);
		des->xx = currentX;
		currentX += (des->maxx - des->minx);
		//文字区域最后一个字形没有保护x间隔
		currentX += style->xspace;
		if (minx > des->xx + des->minx) minx = des->xx + des->minx;
		if (maxx < des->xx + des->maxx) maxx = des->xx + des->maxx;
		if (miny > des->yy + des->miny) miny = des->yy + des->miny;
		if (maxy < des->yy + des->maxy) maxy = des->yy + des->maxy;
	}
	if (minx == 0x7fff) minx = miny = 0;
}


int LOLineDescribe::height() {
	return maxy - miny;
}

void LOLineDescribe::AddShadowSize(int16_t xspace, int16_t yspace, int16_t xshadow, int16_t yshadow) {
	maxx -= xspace;
	maxy -= yspace;
	(xspace > xshadow) ? maxx += xspace : maxx += xshadow;
	(yspace > yshadow) ? maxy += yspace : maxy += yshadow;
}


//================文字纹理===========
LOTextTexture::LOTextTexture() {
	surface = nullptr;
	Xfix = Yfix = 0;
}


LOTextTexture::~LOTextTexture(){
	reset();
}


void LOTextTexture::reset() {
	ClearLines();
	ClearTexts();
	ClearWords();
	if (surface) FreeSurface(surface);
	surface = nullptr;
	text.clear();
	fontName.clear();
}


void LOTextTexture::ClearLines() {
	for (auto iter = lineList.begin(); iter != lineList.end(); iter++) delete *iter;
}


void LOTextTexture::ClearTexts() {
	for (auto iter = textList.begin(); iter != textList.end(); iter++) delete *iter;
}


void LOTextTexture::ClearWords() {
	for (auto iter = wordList.begin(); iter != wordList.end(); iter++) delete *iter;
}


bool LOTextTexture::CreateTextDescribe(LOString *s, LOTextStyle *bstyle, LOString *fName) {
	reset();
	if (!s || !bstyle || !fName) return false;
	text = *s;
	style = *bstyle;
	fontName = *fName;
	LOFont *font = LOFont::CreateFont(fontName);
	if (!font) return false;

	CreateLineDescribe(font, style.xsize);
	//计算位置纠正
	Xfix = Yfix = 0;
	for (auto iter = lineList.begin(); iter != lineList.end(); iter++) {
		LOLineDescribe *line = (*iter);
		if (line->xx + line->minx < Xfix) Xfix =line->xx + line->minx;
		if (line->yy + line->miny < Yfix) Yfix = line->yy + line->miny;
	}
}


bool LOTextTexture::divideDescribe(LOFont *font) {
	return true;
}


LOLineDescribe* LOTextTexture::CreateNewLine(LOTextDescribe *&des, TTF_Font *font, LOTextStyle *style, int colorID) {
	LOLineDescribe *line = new LOLineDescribe();
	lineList.push_back(line);
	line->xx = style->textIndent;
	line->startIndex = textList.size();
	line->endIndex = line->startIndex;

	if (&des && font) {
		des = CreateNewTextDes(TTF_FontAscent(font), TTF_FontDescent(font), colorID);
		line->endIndex++;
	}
	return line;
}


LOTextDescribe* LOTextTexture::CreateNewTextDes(int ascent, int dscent, int colorID) {
	LOTextDescribe *des = new LOTextDescribe(ascent, dscent, colorID);
	textList.push_back(des);
	des->startIndex = wordList.size();
	des->endIndex = des->startIndex;
	return des;
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
	int Ypos = 0;

	LOTextDescribe *des = nullptr;
	//新行时确定了x位置
	LOLineDescribe *line = CreateNewLine(des, wordFont->font, &style, colorID);
	int currentX = line->xx;
	line->yy = Ypos;

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
		if (el->unicode == '<') {

		}
		else if (el->unicode == '>') {
			//风格变换结束
		}
		else if (el->unicode == '\n') {
			delete el;
			el = nullptr;
			//处理好上一行
			line->endIndex = textList.size();
			line->CreateLocation(&textList, &wordList, &style);
			Ypos += line->height();
			Ypos += style.yspace;
			//定位下一行
			line = CreateNewLine(des, wordFont->font, &style, colorID);
			currentX = line->xx;
			line->yy = Ypos;
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
					//处理好上一行
					line->endIndex = textList.size();
					line->CreateLocation(&textList, &wordList, &style);
					Ypos += line->height();
					Ypos += style.yspace;
					//定位下一行
					line = CreateNewLine(des, wordFont->font, &style, colorID);
					currentX = line->xx;
					line->yy = Ypos;
				}

				wordList.push_back(el);
				des->endIndex++;
				//std::string s = "d:\\aaaa\\" + std::to_string(count) + ".bmp";
				//SDL_SaveBMP(ele->surface, s.c_str());
				//count++;
				currentX += (el->minx + el->advance);
			}
			else {
				//空格符号
				el->setSpace(firstSize);
				wordList.push_back(el);
			}

			//间隔
			currentX += style.xspace;
		}
	}
	font->CloseAll();
	line->CreateLocation(&textList, &wordList, &style);
}




void LOTextTexture::GetSurfaceSize(int *width, int *height) {
	uint16_t minx, maxx, miny, maxy;
	minx = maxx = miny = maxy = 0;
	for (auto iter = lineList.begin(); iter != lineList.end(); iter++) {
		LOLineDescribe *line = (*iter);
		if (line->xx + line->minx < minx) minx = line->xx + line->minx;
		if (line->yy + line->miny < miny) miny = line->yy + line->miny;
		if (line->xx + line->maxx > maxx) maxx = line->xx + line->maxx;
		if (line->yy + line->maxy > maxy) maxy = line->yy + line->maxy;
	}

	Xfix = minx;
	Yfix = miny;

	if (width) *width = maxx - minx + style.xshadow;
	if (height) *height = maxy - miny + style.yshadow;
}