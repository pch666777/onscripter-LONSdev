#include "../Scripter/FuncInterface.h"
#include "LOFontInfo.h"
//#include "Global.h"

/*
//生成单色文字图像-->排版-->着色渲染
//每个脚本解析器需要配备一个文字渲染器，防止多线程时发生冲突
//第一步：生成所有的文字对象
//第二部：根据文字宽度确认每一行需要包含的文字对象，确定每一个文字的x坐标
//第三步：根据对其方式确认每一个文字的y坐标
*/
//LOFontWindow glo_font_cfg;
extern Uint32 G_Texture_format;
extern char G_Bit[4];

//==========================================

LOFontManager::LOFontManager(){
	ResetMe();
}

LOFontManager::~LOFontManager(){

}

void LOFontManager::ResetMe() {
	usehtml = false;
	rubySize[0] = 0;
	rubySize[1] = 0;
	rubySize[2] = RUBY_OFF;
	bgColor.r = 0; bgColor.g = 0; bgColor.b = 0; bgColor.a = 0;
	shadowColor = bgColor; shadowColor.a = 254;
	st_usekinsoku = false;
	st_kinsoku.clear();
}

void LOFontManager::FreeAllGlyphs(LOStack<LineComposition> *lines) {
	for (int count = 0; count < lines->size(); count++) lines->at(count)->cells.clear(true);
}

/*
//生成空白素材
LOSurface *LOFontManager::CreateSurface(LOStack<LineComposition> *lines, int cellcount) {
	int w = 0;
	int h = 0;
	for (int ii = 0; ii < lines->size(); ii++) {
		LineComposition *line = lines->at(ii);
		if (line->sumx > w) w = line->sumx;
		h = line->y + line->height + 4; //留出阴影的距离
	}
	w = w * cellcount;

	return new LOSurface(w, h, 32, G_Texture_format);
	SDL_Surface *su = CreateRGBSurfaceWithFormat(0, w, h, 32, G_Texture_format);
}


//将文字按排版渲染到纹理上
void LOFontManager::RenderColor(LOSurface *tex, LOStack<LineComposition> *comlist, SDL_Color *fg, int cellx, bool isshaded) {
	SDL_Rect srcrect, dstrect;
	for (int count = 0; count < comlist->size(); count++) {
		LineComposition *line = comlist->at(count);
		for (int ii = 0; ii < line->cells.size(); ii++) {
			LOFontElememt *el = line->cells.at(ii);
			if (el->su) { //SDL_BlitSurface的质量很差，改为手动拷贝
				srcrect.x = 0 ;
				srcrect.y = 0;  //基本上复制整个字形
				srcrect.w = el->maxx;
				srcrect.h = el->su->h;

				//直接复制整个字形图像
				dstrect.x = cellx + line->x + el->x;
				//y = 行y + 区域y + 字形y
				dstrect.y = line->y + el->y;
				dstrect.w = srcrect.w;
				dstrect.h = srcrect.h;

				if (isshaded) {  //渲染阴影
					int shaderpix = line->height / 30 + 1;
					//int shaderpix = 1;
					dstrect.x += shaderpix;
					dstrect.y += shaderpix;
					if(!BlitToRGBA(el->su, &srcrect, tex->GetSurface(), &dstrect, &shadowColor, true)) return;
					dstrect.x -= shaderpix;
					dstrect.y -= shaderpix;
				}
				if(!BlitToRGBA(el->su, &srcrect, tex->GetSurface(), &dstrect, fg, false)) printf("BlitToRGBA faild!");
			}
		}
	}
	//SDL_SaveBMP(tex, "test.bmp");
}
*/

//约束矩形
void ConstrainedRect(SDL_Rect *rect, int w, int h) {
	int x2 = rect->x + rect->w;
	int y2 = rect->y + rect->h;

	if (rect->x < 0) rect->x = 0;
	else if (rect->x > w) rect->x = w;
	if (rect->y < 0) rect->y = 0;
	else if (rect->y > h) rect->y = h;

	if (x2 < rect->x) x2 = rect->x;
	if (y2 < rect->y) y2 = rect->y;
	if (x2 > w) x2 = w;
	if (y2 > h) y2 = h;

	rect->w = x2 - rect->x;
	rect->h = y2 - rect->y;
}

bool LOFontManager::BlitToRGBA(SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect, SDL_Color *color, bool isshaded)
{
	if (!color) color = src->format->palette->colors + 255;
	//检查参数合理性
	if (srcrect->x < 0 || srcrect->y < 0 || srcrect->x + srcrect->w > src->w || srcrect->y + srcrect->h > src->h) {
		SDL_LogError(0, "LOFontManager::BlitToRGBA() use a invalid argument!");
		return false;
	}	

	Uint32 dstU,srcU,reA ,reB, AP;
	int apos = G_Bit[3];
	int rpos = G_Bit[0];
	int gpos = G_Bit[1];
	int bpos = G_Bit[2];

	SDL_Rect odst = *dstrect;
	ConstrainedRect(dstrect, dst->w, dst->h);
	srcrect->x += (dstrect->x - odst.x);
	srcrect->y += (dstrect->y - odst.y);
	srcrect->w = dstrect->w;
	srcrect->h = dstrect->h;

	//color->r = 0;
	SDL_LockSurface(dst);
	SDL_LockSurface(src);

	unsigned char *dstda = (unsigned char*)dst->pixels + dstrect->y * dst->pitch + dstrect->x * 4;
	unsigned char *srcda = (unsigned char*)src->pixels + srcrect->y * src->pitch + srcrect->x * 1;
	for (int yy = 0; yy < srcrect->h; yy++) {
		//按像素复制
		unsigned char *ddst = dstda;

		//ddst[0] = 24; ddst[1] = 24; ddst[2] = 24; ddst[3] = 76;
		//color->r = 255; color->g = 255; color->b = 255; color->a = 160;
		//srcda[0] = 160;

		for (int nn = 0; nn < srcrect->w; nn++) {
			if (srcda[nn] != 0) {    //只复制有效的位置，混合底部颜色
				if (ddst[apos] == 0) { //没有内容，直接复制
					ddst[rpos] = color->r;
					ddst[gpos] = color->g;
					ddst[bpos] = color->b;
					ddst[apos] = srcda[nn];
				}
				else {  //混合内容
					AP = srcda[nn];
					reA = (srcda[nn] ^ 255);  // 1 - srcAlpha
					reB = (ddst[apos] * reA / 255);

					ddst[apos] = AP + ddst[apos] * reA / 255;  //dst alpha

					ddst[rpos] = (color->r * AP) / 255 + ddst[rpos] * reB / 255;
					ddst[rpos] = ddst[rpos] * 255 / ddst[apos];

					ddst[gpos] = (color->g * AP) / 255 + ddst[gpos] * reB / 255;
					ddst[gpos] = ddst[gpos] * 255 / ddst[apos];

					ddst[bpos] = (color->b * AP) / 255 + ddst[bpos] * reB / 255;
					ddst[bpos] = ddst[bpos] * 255 / ddst[apos];

					//if (!isshaded) {
					//	reA = 0x20 + ddst[apos];
					//	if (reA > 0xff) ddst[apos] = 0xff;
					//	else ddst[apos] = reA;
					//}
					
				}
			}
			ddst += 4;
		}
		srcda += src->pitch;
		dstda += dst->pitch;
	}
	SDL_UnlockSurface(dst);
	SDL_UnlockSurface(src);
	return true;
}

//改变画板的颜色
void LOFontManager::ToColor(SDL_Surface *su, SDL_Color *color) {
	if (!color) return;
	SDL_Color *p = su->format->palette->colors;
	p->a = 0;
	for (int ii = 1; ii <= 255; ii++) {
		p = su->format->palette->colors + ii;
		p->r = color->r;
		p->g = color->g;
		p->b = color->b;
		p->a = ii;
	}
}

/*
LOStack<LineComposition>* LOFontManager::RenderTextCore(LOSurface *&tex, LOFontWindow *fontwin, LOString *s, SDL_Color *color, int cellcount, int startx) {
	if (s->length() == 0) return NULL;
	if (color == NULL) color = &fontwin->fontColor;

	if (!fontwin->Openfont()) {
		LOLog_i( "%s%s", "can't open font file:", fontwin->fontName.c_str());
		return NULL;
	}
	fontwin->ChangeStyle();

	//空的
	GlyphValue param;
	param.firstIndent = startx;
	param.maxwidth = fontwin->GetMaxLine();
	param.shadow = fontwin->isshaded;
	param.outline = false;
	LOStack<LineComposition> *lines = GetGlyphs(fontwin, s, &param);
	AlignLines(lines, fontwin->yspace, param.shadow);

	if (tex) delete tex;
	tex = CreateSurface(lines, cellcount);

	if (cellcount > 1) {
		for (int ii = 0; ii < cellcount; ii++) {
			RenderColor(tex, lines, color + ii, tex->W() / cellcount * ii, fontwin->isshaded);
		}
	}
	else RenderColor(tex, lines, color, 0, fontwin->isshaded);
	//SDL_SaveBMP(*tex, "test.bmp");

	fontwin->Closefont();
	return lines;
}
*/
//逐列扫描，确定左右两侧的边界
void LOFontManager::GetBorder(SDL_Surface *su, int *minx, int *maxx) {
	char *src;
	bool loopflag = true;
	for (*minx = 0; *minx < su->w && loopflag;) {
		src = (char*)su->pixels + (*minx);
		for (int yy = 0; yy < su->h && loopflag; yy++) {
			if (src[0] != 0) loopflag = false;
			src += su->pitch;
		}
		if (loopflag)(*minx)++;
	}

	loopflag = true;
	for ( *maxx = su->w - 1; *maxx > *minx  && loopflag;) {
		src = (char*)su->pixels + (*maxx);
		for (int yy = 0; yy < su->h && loopflag; yy++) {
			if (src[0] != 0) loopflag = false;
			src += su->pitch;
		}
		if (loopflag) (*maxx)--;
	}
}





const char* LOFontManager::ParseStyle(LOFontWindow *fontwin, const char *cbuf, bool *isnew, int *align) {
	char *buf = (char*)(intptr_t)(cbuf);
	*isnew = false;
	if (buf[0] == '{') buf++;

	LOString keywork, value;
	LOCodePage *encoder = (LOCodePage*)FunctionInterface::scriptModule->GetEncoder();
	while (buf[0] != 0) {
		//buf = LOCodePage::SkipSpace(buf);

		//if (buf[0] == '}') {
		//	buf++;
		//	break;
		//}

		//buf = encoder->GetWordStill(&keywork, buf, LOCodePage::CHARACTER_LETTER);
		//buf = encoder->SkipSpace(buf);
		//if (buf[0] == '=') buf = encoder->SkipSpace(buf+1);
		//buf = encoder->GetWordStill(&value, buf, LOCodePage::CHARACTER_LETTER | LOCodePage::CHARACTER_NUMBER);
		//buf = encoder->SkipSpace(buf);
		//if (keywork == "size") {
		//	int size = encoder->CharToInt(value.c_str());
		//	if (fontwin->ChangeFont(NULL, size)) *isnew = true;
		//}
		//else if (keywork == "color") {
		//	int cc = encoder->HexToInt(buf, 6);
		//	fontwin->fontColor.r = (cc >> 16) & 0xff;
		//	fontwin->fontColor.g = (cc >> 8) & 0xff;
		//	fontwin->fontColor.b = cc & 0xff;
		//}
		//else if (keywork == "font") {
		//	if (buf[0] == '.') value.append(".ttf");
		//	if (fontwin->ChangeFont(&value, 0)) *isnew = true;
		//	if (buf[0] == '.') buf = encoder->GetWordStill(&value, buf + 1, LOCodePage::CHARACTER_LETTER);
		//}
		//else if (keywork == "align") {
		//	if (value == "mid") *align = ALIG_MIDDLE;
		//	else if (value == "bot") *align = ALIG_BOTTOM;
		//	else *align = ALIG_TOP;
		//}
	}
	return buf;
}


LOStack<LineComposition> *LOFontManager::GetGlyphs(LOFontWindow *fontwin, LOString *s, GlyphValue *param) {
	const char *buf = s->c_str();
	LOStack<LineComposition> *lines = new LOStack<LineComposition>;
	lines->push(new LineComposition);
	LineComposition *line = lines->top();
	line->sumx = param->firstIndent;

	int origin = TTF_FontAscent(fontwin->font);
	int ulen;
	int lastWordLeft = 0;  //上一个字形还允许往前多少

	LOCodePage *encoder = (LOCodePage*)FunctionInterface::scriptModule->GetEncoder();

	while (buf[0] != 0) {
		if (buf[0] == '\n') {
			if (buf[1] == '\0') break;
			lines->push(new LineComposition);
			line = lines->top();
			lastWordLeft = 0;
			buf++;
		}
		else if (buf[0] == '{') {
			bool isnew;
			buf = ParseStyle(fontwin, buf, &isnew,&line->align);
			if (isnew)origin = TTF_FontAscent(fontwin->font);
		}

		LOFontElememt *ele = new LOFontElememt;
		ele->origin = origin;
		ele->unicode = encoder->GetUtf16(buf, &ulen);
		if (ulen == 1) ele->isMultibyte = false;
		else ele->isMultibyte = true;

		if (s->GetEncoder()->GetCharLen(buf) == LOCodePage::CHARACTER_SPACE) {
			ele->su = nullptr;
			ele->minx = 0; ele->maxx = fontwin->xsize / 2;
			ele->miny = 0; ele->maxy = 0;
		}
		else {
			//ele->su = LTTF_RenderGlyph_Shaded(fontwin->font, ele->unicode, fontwin->fontColor, bgColor);

			//miny maxy是相对于origin的，minx,maxx是相对于advance的，（不准确？所以需要自己检测左右边界）
			//advance为从右往左n个像素为文字开始的位置,minx负数表示文字应该迁移n个像素，正数表示应该后移n个像素
			//advance也表示建议的字形宽度
			TTF_GlyphMetrics(fontwin->font, ele->unicode, &ele->minx, &ele->maxx, &ele->miny, &ele->maxy, &ele->advance);
			//GetBorder(ele->su, &ele->minx, &ele->maxx);
			//LONS::printFormat("z%d-%d-%d-%d.bmp", ele->unicode,ele->origin,ele->maxy,ele->miny);
			//SDL_SaveBMP(ele->su,LONS::fmOutText);
			//LONS::printFormat("z%d-%d-%d-%d.bmp", ele->unicode,ele->advance,ele->maxx,ele->minx);
			//SDL_SaveBMP(ele->su,LONS::fmOutText);
			//int gksksjk = 0;
			//int gksksjk = TTF_FontAscent(fontwin->font);
			
		}

		buf += ulen;
		if (line->sumx + ele->Width() / 2 > param->maxwidth) {
			lines->push(new LineComposition);
			line = lines->top();
			lastWordLeft = 0;
		}
		ele->x = line->sumx;
		//LONS::printInfo("ele->x:%d\n", ele->x);

		line->cells.push(ele);
		//处理非可见字符
		//LONS::printInfo("%d,", ele->unicode);
		if (!ele->su) {
			if (ele->isMultibyte) line->sumx += fontwin->xsize;
			else {
				line->sumx += (fontwin->xsize * 4 / 7);
			}
			lastWordLeft = 0;
			continue;
		}

		//分为符号和文字两种对齐方式
		if (ele->isMultibyte && encoder->isFullMark(ele->unicode)) {
			ele->minx = 0;
			ele->maxx = ele->su->w;
		}
		else if (!ele->isMultibyte && encoder->isHalfMark(ele->unicode)) {
			ele->minx = 0;
			ele->maxx = ele->su->w;
		}
		else {
			ele->minx = ele->su->w - ele->advance + ele->minx;
			ele->maxx = ele->su->w - ele->advance + ele->maxx;
			if (ele->maxx > ele->su->w) ele->maxx = ele->su->w;
		}

		lastWordLeft = ele->su->w - ele->maxx;
		line->sumx += ele->maxx;
		//间距减去余量，剩余量很多则不需要增加
		//这里没有考虑英文的单词模式
		if (lastWordLeft < fontwin->xspace) line->sumx += (fontwin->xspace - lastWordLeft);
	}
	return lines;
}


void LOFontManager::AlignLines(LOStack<LineComposition> *lines, int yspace, bool isshadow) {
	int sumy = 0;
	LOStack<LOFontElememt> tmp;
	for (int count = 0; count < lines->size(); count++) {
		LineComposition *line = lines->at(count);

		//找出行中最高的基线（相当于是行中最大的字形）
		int maxorigin = 0;
		for (int ii = 0; ii < line->cells.size(); ii++) {
			LOFontElememt *ele = line->cells.at(ii);
			if (ele->origin > maxorigin) maxorigin = ele->origin;
			if (ele->maxy > maxorigin) maxorigin = ele->maxy;
		}

		//========分解行中的同高度连续区域=====
		int current = 0;
		while (current < line->cells.size()) {
			int cellnum = current;
			int origin = line->cells.at(cellnum)->origin;
			tmp.clear();
			for (; cellnum < line->cells.size() && line->cells.at(cellnum)->origin == origin; cellnum++) {
				tmp.push(line->cells.at(cellnum));
			}
			//文字对齐的方式相当麻烦，首先origin只代表字体假想按此基线对齐，实际中有可能对齐线
			//并不是这个值，因此每个对齐区域需要按最高的字形进行假想对齐

			//计算出当前字形组的最大y值和最小y值
			int miny = 1024;
			int maxy = 0;
			int torig = origin;  //部分字体不满足标准要求
			for (int kk = 0; kk < tmp.size(); kk++) {
				LOFontElememt *el = tmp.at(kk);
				if (el->miny < miny) miny = el->miny;
				if (el->maxy > maxy) maxy = el->maxy;
				//纠正合理的对齐线
				if (el->maxy > el->origin) el->torig = el->maxy;
				else el->torig = el->origin;

				if (el->maxy > torig) torig = el->maxy;

			}
			//更新假想基线
			for (int kk = 0; kk < tmp.size(); kk++) {
				LOFontElememt *el = tmp.at(kk);
				el->y = torig - el->torig;
				//LONS::printInfo("%d\n", el->y);
			}

			//根据连续区域的偏差确定文字递增下降量
			for (int kk = 0; kk < tmp.size(); kk++) {
				LOFontElememt *ele = tmp.at(kk);
				if (line->align == ALIG_MIDDLE) ele->y = ele->y + (maxorigin - origin) / 2;
				else if (line->align == ALIG_BOTTOM) ele->y = ele->y + maxorigin - origin;
				//LONS::printInfo("%d\n", ele->y);
			}
			//找出行中最大的字形高度作为高度
			//if (line->height < maxy - miny) line->height = maxy - miny;

			current = cellnum;
		}
		//==================
		//扫描一整行的y值，确保顶格
		int fmin = 1024;
		line->height = 0;
		for (int ii = 0; ii < line->cells.size(); ii++) {
			LOFontElememt *el = line->cells.at(ii);
			if (el->y < fmin) fmin = el->y;
		}
		for (int ii = 0; ii < line->cells.size(); ii++) {
			LOFontElememt *el = line->cells.at(ii);
			el->y -= fmin;
			//基线不匹配的字形
			if (el->maxy > el->origin && (el->y + el->maxy - el->miny > line->height)) line->height = el->y + el->maxy - el->miny;
			else if(el->y + el->origin - el->miny > line->height) line->height = el->y + el->origin - el->miny;
			//LONS::printInfo("%d\n", el->y);
		}

		//有阴影要加上阴影的大小
		if (isshadow) line->height += (line->height / 30 + 1);
		line->y = sumy;
		sumy = line->y + line->height + yspace;
	}

	//for (int ii = 0; ii < lines->size(); ii++) {
	//	LONS::printInfo("line %d,height:%d\n", ii, lines->at(ii)->height);
	//}
}

int LOFontManager::GetRubyPreHeight() {
	if (rubySize[2] == RUBY_ON) return rubySize[1];
	else return 0;
}