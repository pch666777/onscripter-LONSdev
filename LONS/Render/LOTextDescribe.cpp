/*
//文字描述
*/

#include "LOTextDescribe.h"
#include "../etc/LOLog.h"

//在别处已经定义了
extern char G_Bit[4];
extern Uint32 G_Texture_format;

//================================================
LOTextStyle::LOTextStyle() {
	reset();
}

LOTextStyle::~LOTextStyle() {

}


void LOTextStyle::reset() {
	xcount = ycount = 1024;
	xsize = ysize = 16;
	xspace = 0;
	yspace = 0;
	textIndent = 0;
	flags = 0;
	xshadow = yshadow = 0;
	xruby = yruby = 0;
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
		//虽然advance是应该增加的宽度，但是这个有时候不太准确，SDL的问题？
		maxx += el->advance;
		if (el->minx < 0) maxx += el->minx;

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

int LOLineDescribe::width() {
	return maxx - minx;
}


void LOLineDescribe::AddShadowSize(int16_t xspace, int16_t yspace, int16_t xshadow, int16_t yshadow) {
	maxx -= xspace;
	maxy -= yspace;
	(xspace > xshadow) ? maxx += xspace : maxx += xshadow;
	(yspace > yshadow) ? maxy += yspace : maxy += yshadow;
}
