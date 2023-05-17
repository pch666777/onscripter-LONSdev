#include "LOEffect.h"
#include "LOTexture.h"
#include "../etc/LOTimer.h"

LOEffect::LOEffect(){
	masksu = nullptr;
}

LOEffect::~LOEffect(){
}

//将遮片转为8位遮片
SDL_Surface* LOEffect::Create8bitMask(SDL_Surface *su, bool isscale) {
	SDL_Surface *fsu = nullptr;
	//有缩放则缩放遮片
	if (isscale && abs(G_gameScaleX - 1.0) > 0.00001 && abs(G_gameScaleY - 1.0) > 0.00001) { //有缩放则先缩放遮片
		SDL_Surface *osu = su;
		SDL_Rect dst,src;
		dst.x = 0; dst.y = 0;
		dst.w = su->w * G_gameScaleX; dst.h = su->h * G_gameScaleY;
		src.x = 0; src.y = 0;
		src.w = su->w; src.h = su->h;
		if (su->format->BytesPerPixel == 1) osu = SDL_ConvertSurfaceFormat(su, G_Texture_format, 0);

		SDL_Surface *tmp = CreateRGBSurfaceWithFormat(0, dst.w, dst.h, 32, G_Texture_format);
		SDL_BlitScaled(osu, NULL, tmp, &dst);
		//SDL_SaveBMP(tmp, "test.bmp");
		fsu = ConverToGraySurface(tmp);
		FreeSurface(tmp);
		if (osu != su) FreeSurface(osu);
		//fsu = ConverToGraySurface(osu);
		//if (osu != su) SDL_FreeSurface(osu);
	}
	else {
		if (su->format->format != SDL_PIXELFORMAT_INDEX8) fsu = ConverToGraySurface(su);  //直接转为灰度图像
		else {
			fsu = CreateRGBSurfaceWithFormat(0, su->w, su->h, 8, SDL_PIXELFORMAT_INDEX8); //复制灰度图像
			SDL_LockSurface(fsu);
			for (int ii = 0; ii < su->h; ii++) {
				char *dst = (char*)fsu->pixels + ii * fsu->pitch;
				char *src = (char*)su->pixels + ii * su->pitch;
				memcpy(dst, src, su->pitch);
			}
		}
	}
	CreateGrayColor(fsu->format->palette);
	//SDL_SaveBMP(fsu, "test.bmp");
	return fsu;
}

//RGB图转灰度图
SDL_Surface* LOEffect::ConverToGraySurface(SDL_Surface *su) {
	SDL_Surface *tmp = CreateRGBSurfaceWithFormat(0, su->w, su->h, 8, SDL_PIXELFORMAT_INDEX8);

	char mbit[4];
	GetFormatBit(su->format, mbit);

	int val;
	SDL_LockSurface(tmp);
	for (int line = 0; line < su->h; line++) {
		Uint8 *dst = (Uint8*)tmp->pixels + line * tmp->pitch;
		Uint8 *src = (Uint8*)su->pixels + line * su->pitch;
		for (int xx = 0; xx < su->w; xx++) {
			val = 0.3*src[mbit[0]] + 0.59*src[mbit[1]] + 0.11*src[mbit[2]];
			if (val > 255) val = 255;
			dst[0] = val;
			dst++;
			src += su->format->BytesPerPixel;
		}
	}
	SDL_UnlockSurface(tmp);
	CreateGrayColor(tmp->format->palette);
	return tmp;
}

//LOLayerData只提供PrintTextureEdit的图层
bool LOEffect::RunEffect2(SDL_Texture *edit, double pos) {
	//pos = 10;

	//最后一帧不用执行，比如透明类，最后一帧实际上图像是不可见的
	if (postime + pos >= time|| postime < 0) {
		postime = this->time + 1.0;
		//释放15和18特效的遮片，如果有的话
		masksu.reset();
		return true;
	}

	postime += pos;
	//增加一点速度，因为实际执行的过程中会有一点延迟
	postime += 0.2;
	if (postime > time) postime = time;

	switch (nseffID)
	{
	case 2:
		BlindsEffect(edit, pos, DIRECTION_LEFT);
		break;
	case 3:
		BlindsEffect(edit, pos, DIRECTION_RIGHT);
		break;
	case 4:
		BlindsEffect(edit, pos, DIRECTION_TOP);
		break;
	case 5:
		BlindsEffect(edit, pos, DIRECTION_BOTTOM);
		break;
	case 6:
		CurtainEffect(edit, pos, DIRECTION_LEFT);
		break;
	case 7:
		CurtainEffect(edit, pos, DIRECTION_RIGHT);
		break;
	case 8:
		CurtainEffect(edit, pos, DIRECTION_TOP);
		break;
	case 9:
		CurtainEffect(edit, pos, DIRECTION_BOTTOM);
		break;
	case 10:
		//透明度的计算不需要在这里
		//info->cur.texture->setBlendModel(SDL_BLENDMODE_BLEND);
		//FadeOut(alpha, pos);
		break;
	case 11:
		RollEffect(edit, pos, DIRECTION_LEFT);
		//TestEffect(edit, pos, DIRECTION_LEFT);
		break;
	case 12:
		RollEffect(edit, pos, DIRECTION_RIGHT);
		break;
	case 13:
		RollEffect(edit, pos, DIRECTION_TOP);
		break;
	case 14:
		RollEffect(edit, pos, DIRECTION_BOTTOM);
		break;
	case 15:
		MaskEffectCore(edit, pos, false);
		break;
	case 16:
		//if (!masksu) CreateSmallPic(ren, info, effectTex);
		//MosaicEffect(ren, info, maskTex, pos, true);
		break;
	case 17:

		break;
	case 18:
		MaskEffectCore(edit, pos, true);
		break;
	case EFFECT_ID_QUAKE:

		//晃动edit为黑色背景，cut表现为移动
		//SDL_SetRenderTarget(ren, info->cur.texture->GetTexture());
		//QuakeEffect(ren, efstex, maskTex, postime);
		break;
	default:
		break;
	}
	return false;
}

//淡入淡出
void LOEffect::FadeOut(int *alpha, double pos) {
	int per = (time - postime) / time * 255;
	if (per > 255) per = 255;
	//注意透明度界限
	if (per < 0) per = 0;
	*alpha = per;
}

//遮片
void LOEffect::MaskEffectCore(SDL_Texture *edit, double pos, bool isalpha) {
		//计算出每一个灰度色阶在当前时刻的实际透明度
		unsigned char *gray = new unsigned char[256];
		for (int ii = 0; ii < 256; ii++) {
			double pertime = time / 255 * ii; //当前灰度色阶需要多长实际变为透明
			if (postime > pertime) gray[ii] = 0; //超过时间的都为透明
			else if (isalpha) {
				gray[ii] = (pertime - postime) / pertime * 255; //没超过的计算透明度
			}
			else gray[ii] = 255; //超过的就透明，不超过就不透明
		}

		//开始生成mask
		int w, h,pitch;
		void *pixbuf;
		SDL_QueryTexture(edit, NULL, NULL, &w, &h);
		SDL_LockTexture(edit, NULL, &pixbuf, &pitch);

		//重置遮片为全白，表示图片全部显示
		for (int yy = 0; yy < h; yy++) {
			char *dstbuf = (char*)pixbuf + yy * pitch;
			memset(dstbuf, 255, pitch);
		}

		//改写每个像素的透明度
		//将遮片覆盖上去，尺寸不足的时候按平铺处理
		SDL_Surface *tsu = masksu->GetSurface();
		int mbyte = tsu->format->BytesPerPixel;

		//逐行覆盖
		for (int y = 0; y < h; y ++) {
			char *dst = (char*)pixbuf + y * pitch;
			char *src = nullptr;
			//逐像素计算
			for (int x = 0; x < w; x++) {
				if(x % tsu->w == 0) src = (char*)tsu->pixels + (y % tsu->h) * tsu->pitch;
				dst[G_Bit[3]] = gray[(Uint8)(src[0])];
				dst += 4;
				src++;
			}
		}
		delete []gray;
		SDL_UnlockTexture(edit);
		//SDL_SetTextureBlendMode(maskTex, SDL_BLENDMODE_BLEND);
}



//百叶窗
void LOEffect::BlindsEffect(SDL_Texture *edit, double pos, int direction) {
	int hideCount = time / 16;
	if (hideCount == 0) hideCount++;    //The minimum time is 1ms.
	hideCount = (int)(postime / hideCount); //how much line be hide.

	int hideFlag = 0;
	for (int ii = 0; ii < hideCount; ii++) {
		if (direction == DIRECTION_LEFT || direction == DIRECTION_TOP) hideFlag |= (1 << ii);
		else hideFlag |= (1 << (15 - ii));
	}
	
	int w, h, pitch;
	void *pixbuf;
	SDL_QueryTexture(edit, NULL, NULL, &w, &h);
	SDL_LockTexture(edit, NULL, &pixbuf, &pitch);

	if (direction == DIRECTION_TOP || direction == DIRECTION_BOTTOM) {
		for (int ii = 0; ii < h; ii++) {
			unsigned char *line = (unsigned char*)pixbuf + pitch * ii;
			if ((hideFlag & (1 << (ii % 16)))) memset(line, 0, pitch);
			else memset(line, 255, pitch);
		}
	}
	else {
		//make first line
		unsigned char *line = (unsigned char*)pixbuf;
		memset(line, 255, pitch);
		for (int ii = 0; ii < w; ii++) {
			if ((hideFlag & (1 << (ii % 16))))  memset(line + ii * 4, 0, 4);
		}
		//copy to every line
		for (int ii = 1; ii < h; ii++) {
			memcpy((unsigned char*)pixbuf + ii * pitch, line, pitch);
		}
	}

	SDL_UnlockTexture(edit);
	//SDL_SetTextureBlendMode(maskTex, SDL_BLENDMODE_BLEND);
}


//卷帘
void LOEffect::CurtainEffect(SDL_Texture *edit, double pos, int direction) {
	int blockSize = 24;
	int w, h, pitch, max;
	void *pixbuf;
	SDL_QueryTexture(edit, NULL, NULL, &w, &h);
	if (direction == DIRECTION_TOP || direction == DIRECTION_BOTTOM) max = h;
	else max = w;

	int blockCount = max / blockSize;
	if (max % blockCount) blockCount++;
	if (time < blockCount * 2) time = blockCount * 2;

	//使用不同的延迟时间
	double delayPerTime = (double)time / 2 / blockCount;  //Delay time of block
	double workPerTime = (double)time / 2 / blockSize;    //one pix finish time


	SDL_LockTexture(edit, NULL, &pixbuf, &pitch);

	for (int yy = 0; yy < max; yy += blockSize) {
		//首先根据当前的块确定有多少个像素需要被隐藏
		int blockID = yy / blockSize;
		int restTime = postime - delayPerTime * blockID;
		int hideFlag = 0;
		if (restTime > workPerTime * blockSize) hideFlag = -1;
		else if (restTime > 0) {
			for (int ii = 0; ii < restTime / workPerTime; ii++) {
				if (direction == DIRECTION_LEFT || direction == DIRECTION_TOP) hideFlag |= (1 << ii);
				else hideFlag |= (1 << (blockSize - 1 - ii));
			}
		}
		//根据隐藏标记确定状态
		for (int ii = 0; ii < blockSize; ii++) {
			int index = yy + ii;
			int copylen = pitch;
			if (index >= max) break;
			if (direction == DIRECTION_BOTTOM || direction == DIRECTION_RIGHT) index = max - 1 - index;
			if (direction == DIRECTION_LEFT || direction == DIRECTION_RIGHT) copylen = 4; //左右前进4，上下前进pitch
			unsigned char *lineBuf = (unsigned char*)pixbuf + copylen * index;
			if ((hideFlag & (1 << (ii % blockSize)))) memset(lineBuf, 0, copylen);
			else memset(lineBuf, 255, copylen);
		}
	}

	if (direction == DIRECTION_LEFT || direction == DIRECTION_RIGHT) {//copy to all screen
		for (int ii = 1; ii < h; ii++) {
			memcpy((unsigned char*)pixbuf + pitch * ii, pixbuf, pitch);
		}
	}

	SDL_UnlockTexture(edit);
	//SDL_SetTextureBlendMode(maskTex, SDL_BLENDMODE_BLEND);
}


//滚动
void LOEffect::RollEffect(SDL_Texture *edit, double pos, int direction) {
	int w, h, pitch, hideCount, max;
	void *pixbuf;
	SDL_QueryTexture(edit, NULL, NULL, &w, &h);
	if (direction == DIRECTION_TOP || direction == DIRECTION_BOTTOM) max = h;
	else max = w;
	hideCount = postime * max / time;

	SDL_LockTexture(edit, NULL, &pixbuf, &pitch);
	for (int ii = 0; ii < max; ii++) {
		int index = ii;
		int copylen = pitch;
		if (direction == DIRECTION_LEFT || direction == DIRECTION_RIGHT) copylen = 4; //左右前进4，上下前进pitch
		if (direction == DIRECTION_BOTTOM || direction == DIRECTION_RIGHT) index = max - 1 - ii;
		if (ii <= hideCount) memset((unsigned char*)pixbuf + copylen * index, 0, copylen);
		else memset((unsigned char*)pixbuf + copylen * index, 255, copylen);
	}
	if (direction == DIRECTION_LEFT || direction == DIRECTION_RIGHT) {//copy to all screen
		for (int ii = 1; ii < h; ii++) {
			memcpy((unsigned char*)pixbuf + pitch * ii, pixbuf, pitch);
		}
	}

	SDL_UnlockTexture(edit);
	//SDL_SetTextureBlendMode(maskTex, SDL_BLENDMODE_BLEND);
}

//测试用的函数
void LOEffect::TestEffect(SDL_Texture *edit, double pos, int direction) {
	int w, h, pitch;
	void *pixbuf;
	SDL_QueryTexture(edit, NULL, NULL, &w, &h);
	SDL_LockTexture(edit, NULL, &pixbuf, &pitch);

	//所有像素设置为白色，不透明，不产生颜色系数
	for (int ii = 0; ii < h; ii++) memset((unsigned char*)pixbuf + ii * pitch, 255, pitch);
	
	for (int ii = 0; ii < h / 2; ii++) {
		unsigned char *buf = (unsigned char*)pixbuf + ii * pitch;
		for (int xx = 0; xx < w / 2; xx++) {
			buf[G_Bit[0]] = 0; //r
			buf[G_Bit[1]] = 0; //r
			buf[G_Bit[2]] = 0; //b
			buf[G_Bit[3]] = 0; //b

			buf += 4; //下一个像素
		}
	}

	SDL_UnlockTexture(edit);
}


void LOEffect::CreateSmallPic(SDL_Renderer*ren, LOLayerData *info, SDL_Texture *effectTex) {
	int w, h;
	Uint32 iformat;
	SDL_QueryTexture(effectTex, &iformat, NULL, &w, &h);
	if (!masksu) {
		SDL_Rect dst;
		dst.x = 0; dst.y = 0;
		dst.w = w / 5;  dst.h = h / 5;

		SDL_Surface *tsu = CreateRGBSurfaceWithFormat(0, dst.w, dst.h, 32, iformat);
		//masksu = new LOSurface(tsu);

		SDL_SetRenderTarget(ren, info->cur.texture->GetTexture());
		SDL_RenderClear(ren);  //must clear data
		SDL_SetTextureBlendMode(effectTex, SDL_BLENDMODE_NONE);
		SDL_RenderCopy(ren, effectTex, NULL, &dst);
		//SDL_Delay(1);
		Uint64 t1 = LOTimer::GetHighTimer();

		SDL_LockSurface(tsu);
		SDL_RenderReadPixels(ren, &dst, 0, tsu->pixels, tsu->pitch);
		SDL_UnlockSurface(tsu);

		SDL_Log("SDL_RenderReadPixels() use %d ms\n", (int)(LOTimer::GetHighTimeDiff(t1)));
		//SDL_SaveBMP(masksu, "test.bmp");
	}
}

void LOEffect::MosaicEffect(SDL_Renderer*ren, LOLayerData *info, SDL_Texture *maskTex, double pos, bool isout) {
	//5 to 155  30 levels in total



}


//画面摇晃特效
void LOEffect::QuakeEffect(SDL_Renderer *ren, LOShareTexture &efstex, SDL_Texture *maskTex, double postime) {
	int dx = id & 0xffff;
	int dy = (id >> 16) & 0xffff;
	double v1, v2;
	SDL_Texture *effectTex = efstex->GetTexture();
	SDL_Rect dst = { 0,0, efstex->baseW(), efstex->baseH() };

	if (dx == 0) { //quakey
		v1 = sin(M_PI * 2.0 * dy * postime / this->time);
		//v2 = (double)12 * G_gameRatioW / G_gameRatioH * dy * (this->time - postime);
		v2 = (double)12 * dy * (this->time - postime);
		dst.y = (int)(v1 * v2 / this->time);
		SDL_RenderCopy(ren, effectTex, NULL, &dst);
		//移动的反向用黑色填充
		if (dst.y > 0) {
			dst.h = dst.y; dst.y = 0;
		}
		else {
			dst.h = 0 - dst.y;
			dst.y = efstex->baseH() + dst.y;
		}
		SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
		SDL_RenderFillRect(ren, &dst);
	}
	else if (dy == 0) { //quakex
		v1 = sin(M_PI * 2.0 * dx * postime / this->time);
		//v2 = (double)12 * G_gameRatioW / G_gameRatioH * dx * (this->time - postime);
		v2 = (double)12 * dx * (this->time - postime);
		dst.x = (int)(v1 * v2 / this->time);
		SDL_RenderCopy(ren, effectTex, NULL, &dst);
		//移动的反向用黑色填充
		if (dst.x > 0) {
			dst.w = dst.x; dst.x = 0;
		}
		else {
			dst.w = 0 - dst.x;
			dst.x = efstex->baseW() + dst.x;
		}
		SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
		SDL_RenderFillRect(ren, &dst);
	}
	else { //quake
		dst.x = dx * ((int)(3.0*rand() / (RAND_MAX + 1.0)) - 1) * 2;
		dst.y = dx * ((int)(3.0*rand() / (RAND_MAX + 1.0)) - 1) * 2;
		//dst.x = 40; dst.y = 30;
		SDL_RenderCopy(ren, effectTex, NULL, &dst);
		SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
		//横向纵向均需要黑色覆盖
		if (dst.x != 0) {
			if (dst.x > 0) {
				dst.w = dst.x; dst.x = 0;
			}
			else {
				dst.w = 0 - dst.x;
				dst.x = efstex->baseW() + dst.x;
			}
			SDL_RenderFillRect(ren, &dst);
		}
		if (dst.y != 0) {
			//重置已经被修改的X轴
			dst.w = efstex->baseW();
			dst.x = 0;
			if (dst.y > 0) {
				dst.h = dst.y; dst.y = 0;
			}
			else {
				dst.h = 0 - dst.y;
				dst.y = efstex->baseH() + dst.y;
			}
			SDL_RenderFillRect(ren, &dst);
		}
	}
}

void LOEffect::CopyFrom(LOEffect *ef) {
	nseffID = ef->nseffID;
	time = ef->time;
	mask.assign(ef->mask.c_str());
}

void LOEffect::CreateGrayColor(SDL_Palette *pale) {
	SDL_Color *color = pale->colors;
	for (int ii = 0; ii < 256; ii++) {
		color[ii].r = ii;
		color[ii].g = ii;
		color[ii].b = ii;
	}
}


bool LOEffect::UpdateDstRect(SDL_Rect *rect) {
	//返回的是特效层的位置，因此对于实际显示图像，位置要刚好反过来
	if (postime >= 0 && postime < this->time) {  //时间范围外的无效
		if (nseffID == 11) rect->x = postime / this->time * G_gameWidth;  //图像向右运动
		else if (nseffID == 12) rect->x = 0 - (postime / this->time * G_gameWidth);  //向左运动
		else if (nseffID == 13) rect->y = postime / this->time * G_gameHeight;   //向下运动
		else if (nseffID == 14) rect->y = 0 - (postime / this->time * G_gameHeight);  //向上运动
		else return false;
		return true;
	}
	return false;
}



int LOEffect::UpdateEffect(SDL_Renderer *ren, SDL_Texture *texA, SDL_Texture *texB, SDL_Texture *edit) {
	//更新的前提是当前的图像帧已经刷新到默认画布上了
	//postimme的增加不在此函数
	if (postime < 0 || postime > this->time) return RET_NONE;

	//第一帧，直接覆盖即可
	if (postime == 0) {
		LOtexture::ResetTextureMode(texB);   //注意要重置纹理模式
		SDL_RenderCopy(ren, texB, nullptr, nullptr);
		return RET_CONTINUE;
	}


	if (nseffID == 10) {  //透明度变化类
		LOtexture::ResetTextureMode(texB);   //先重置纹理模式，确保只有透明度模式生效
		int alpha = (this->time - postime) / this->time * 255;
		if (alpha < 0) alpha = 0;
		else if (alpha > 255) alpha = 255;
		SDL_SetTextureBlendMode(texB, SDL_BLENDMODE_BLEND);
		SDL_SetTextureAlphaMod(texB, (Uint8)alpha);
		SDL_RenderCopy(ren, texB, nullptr, nullptr);
	}
	else if (nseffID >= 11 && nseffID <= 14) { //位移类
		LOtexture::ResetTextureMode(texB);  //重置纹理模式
		SDL_Rect rect = { 0,0, G_gameWidth, G_gameHeight };
		UpdateDstRect(&rect);
		SDL_RenderCopy(ren, texB, nullptr, &rect);
	}
	else {//遮片类
		LOtexture::ResetTextureMode(texA);  //重置纹理模式
		LOtexture::ResetTextureMode(texB);  //重置纹理模式
		LOtexture::ResetTextureMode(edit);  //重置纹理模式
		//先重置纹理A
		SDL_SetRenderDrawColor(ren, 0, 0, 0, 0);
		SDL_SetRenderTarget(ren, texA);
		SDL_RenderClear(ren);
		//将纹理B与动态遮片叠加，生成效果纹理A
		SDL_SetTextureBlendMode(edit, SDL_BLENDMODE_BLEND);
		SDL_SetTextureBlendMode(texB, SDL_BLENDMODE_MOD);
		SDL_RenderCopy(ren, edit, nullptr, nullptr);
		SDL_RenderCopy(ren, texB, nullptr, nullptr);

		//将纹理A渲染到画布上
		SDL_SetTextureBlendMode(texA, SDL_BLENDMODE_BLEND);
		SDL_SetRenderTarget(ren, nullptr);
		SDL_RenderCopy(ren, texA, nullptr, nullptr);
	}

	return RET_CONTINUE;
}