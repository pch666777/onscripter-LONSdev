#include "LOEffect.h"
#include "LOTexture.h"

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

//true为已经到达时间界限，图层被隐藏，false表示需要继续进行
bool LOEffect::RunEffect(SDL_Renderer*ren, LOLayerInfo *info, SDL_Texture *effectTex, SDL_Texture *maskTex, double pos) {
	//debug use
	//pos = 10;
	//time = 576*10;
	//图像特效的核心在于我们不能直接操作 target 的textrue，需要构造一个 stream 的textrue
	//然后使用 SDL_BLENDMODE_MOD 让 target 的textrue叠加到 stream 的textrue上，改变每个像素的显示效果
	//SDL_BLENDMODE_MOD(dstRGB = srcRGB * dstRGB     dstA = dstA)

	//重置状态
	SDL_SetTextureBlendMode(effectTex, SDL_BLENDMODE_NONE);

	postime += pos;
	if (postime > time || postime < 0) {
		info->visiable = 0;   //not
		if (masksu) delete masksu;
		masksu = NULL;
		return true;
	}
	else {
		info->alpha = 255;
		switch (nseffID)
		{
		case 2:
			BlindsEffect(ren, info, maskTex, pos, DIRECTION_LEFT);
			break;
		case 3:
			BlindsEffect(ren, info, maskTex, pos, DIRECTION_RIGHT);
			break;
		case 4:
			BlindsEffect(ren, info, maskTex, pos, DIRECTION_TOP);
			break;
		case 5:
			BlindsEffect(ren, info, maskTex, pos, DIRECTION_BOTTOM);
			break;
		case 6:
			CurtainEffect(ren, info, maskTex, pos, DIRECTION_LEFT);
			break;
		case 7:
			CurtainEffect(ren, info, maskTex, pos, DIRECTION_RIGHT);
			break;
		case 8:
			CurtainEffect(ren, info, maskTex, pos, DIRECTION_TOP);
			break;
		case 9:
			CurtainEffect(ren, info, maskTex, pos, DIRECTION_BOTTOM);
			break;
		case 10:
			FadeOut(ren, info, pos);
			break;
		case 11:
			RollEffect(ren, info, maskTex, pos, DIRECTION_LEFT);
			break;
		case 12:
			RollEffect(ren, info, maskTex, pos, DIRECTION_RIGHT);
			break;
		case 13:
			RollEffect(ren, info, maskTex, pos, DIRECTION_TOP);
			break;
		case 14:
			RollEffect(ren, info, maskTex, pos, DIRECTION_BOTTOM);
			break;
		case 15:
			MaskEffectCore(ren, info, maskTex, pos, false);
			break;
		case 16:
			if (!masksu) CreateSmallPic(ren, info, effectTex);
			MosaicEffect(ren, info, maskTex, pos, true);
			break;
		case 18:
			MaskEffectCore(ren, info, maskTex, pos, true);
			break;
		default:
			break;
		}
	}

	if (nseffID == 16 || nseffID == 17) {
		//no thing to do.
	}
	else {
		//auto itt = info->texture->GetTexture();
		Uint8 r, g, b, a;
		SDL_GetRenderDrawColor(ren, &r, &g, &b, &a);

		SDL_SetRenderTarget(ren, info->texture->GetTexture());
		//it must be alpha mode,must clear render
		SDL_SetRenderDrawColor(ren, 0, 0, 0, 0);
		SDL_RenderClear(ren);

		if (nseffID == 10) SDL_SetTextureBlendMode(effectTex, SDL_BLENDMODE_BLEND);
		else {
			SDL_SetTextureBlendMode(effectTex, SDL_BLENDMODE_MOD);
			SDL_RenderCopy(ren, maskTex, NULL, NULL);
		}
		SDL_RenderCopy(ren, effectTex, NULL, NULL);

		//finish it,reset it
		SDL_SetRenderDrawColor(ren, r, g, b, a);
	}

	SDL_SetRenderTarget(ren, NULL);
	return false;
}

void LOEffect::FadeOut(SDL_Renderer*ren, LOLayerInfo *info, double pos) {
	int per = (time - postime) / time * 255;
	if (per > 255) per = 255;
	info->alpha = per;
}

void LOEffect::MaskEffectCore(SDL_Renderer*ren, LOLayerInfo *info, SDL_Texture *maskTex, double pos, bool isalpha) {
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
		SDL_QueryTexture(maskTex, NULL, NULL, &w, &h);
		SDL_LockTexture(maskTex, NULL, &pixbuf, &pitch);

		//重置遮片为全白，表示图片全部显示
		for (int yy = 0; yy < h; yy++) {
			char *dstbuf = (char*)pixbuf + yy * pitch;
			memset(dstbuf, 255, pitch);
		}
		
		//
		int minw = masksu->W();
		int minh = masksu->H();
		if (w < minw) minw = w;
		if (h < minh) minh = h;
		

		//改写每个像素的透明度
		SDL_Surface *tsu = masksu->GetSurface();

		int mbyte = tsu->format->BytesPerPixel;
		for (int line = 0; line < h; line += tsu->h) {
			for (int yy = 0; yy < minh && line + yy < h; yy++) {
				for (int xx = 0; xx < w; xx += tsu->w) {
					char *src = (char*)tsu->pixels + yy * tsu->pitch;  //遮片应该是8位索引色的
					char *dst = (char*)pixbuf + (line + yy) * pitch + xx * 4;  //目标应该是32位色的
					for (int ii = 0; ii < minw && xx + ii < w; ii++) {
						dst[G_Bit[3]] = gray[(Uint8)(src[0])];
						dst += 4;
						src++;
					}
				}
			}
		}
		delete []gray;
		SDL_UnlockTexture(maskTex);
		SDL_SetTextureBlendMode(maskTex, SDL_BLENDMODE_BLEND);
}



void LOEffect::BlindsEffect(SDL_Renderer*ren, LOLayerInfo *info, SDL_Texture *maskTex, double pos, int direction) {
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
	SDL_QueryTexture(maskTex, NULL, NULL, &w, &h);
	SDL_LockTexture(maskTex, NULL, &pixbuf, &pitch);

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

	SDL_UnlockTexture(maskTex);
	SDL_SetTextureBlendMode(maskTex, SDL_BLENDMODE_BLEND);
}


void LOEffect::CurtainEffect(SDL_Renderer*ren, LOLayerInfo *info, SDL_Texture *maskTex, double pos, int direction) {
	int blockSize = 24;
	int w, h, pitch, max;
	void *pixbuf;
	SDL_QueryTexture(maskTex, NULL, NULL, &w, &h);
	if (direction == DIRECTION_TOP || direction == DIRECTION_BOTTOM) max = h;
	else max = w;

	int blockCount = max / blockSize;
	if (max % blockCount) blockCount++;
	if (time < blockCount * 2) time = blockCount * 2;

	//使用不同的延迟时间
	double delayPerTime = (double)time / 2 / blockCount;  //Delay time of block
	double workPerTime = (double)time / 2 / blockSize;    //one pix finish time


	SDL_LockTexture(maskTex, NULL, &pixbuf, &pitch);

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

	SDL_UnlockTexture(maskTex);
	SDL_SetTextureBlendMode(maskTex, SDL_BLENDMODE_BLEND);
}


void LOEffect::RollEffect(SDL_Renderer*ren, LOLayerInfo *info, SDL_Texture *maskTex, double pos, int direction) {
	int w, h, pitch, hideCount, max;
	void *pixbuf;
	SDL_QueryTexture(maskTex, NULL, NULL, &w, &h);
	if (direction == DIRECTION_TOP || direction == DIRECTION_BOTTOM) max = h;
	else max = w;
	hideCount = postime * max / time;

	SDL_LockTexture(maskTex, NULL, &pixbuf, &pitch);
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

	SDL_UnlockTexture(maskTex);
	SDL_SetTextureBlendMode(maskTex, SDL_BLENDMODE_BLEND);
}


void LOEffect::CreateSmallPic(SDL_Renderer*ren, LOLayerInfo *info, SDL_Texture *effectTex) {
	int w, h;
	Uint32 iformat;
	SDL_QueryTexture(effectTex, &iformat, NULL, &w, &h);
	if (!masksu) {
		SDL_Rect dst;
		dst.x = 0; dst.y = 0;
		dst.w = w / 5;  dst.h = h / 5;

		SDL_Surface *tsu = CreateRGBSurfaceWithFormat(0, dst.w, dst.h, 32, iformat);
		masksu = new LOSurface(tsu);

		SDL_SetRenderTarget(ren, info->texture->GetTexture());
		SDL_RenderClear(ren);  //must clear data
		SDL_SetTextureBlendMode(effectTex, SDL_BLENDMODE_NONE);
		SDL_RenderCopy(ren, effectTex, NULL, &dst);
		//SDL_Delay(1);
		Uint64 t1 = SDL_GetPerformanceCounter();

		SDL_LockSurface(tsu);
		SDL_RenderReadPixels(ren, &dst, 0, tsu->pixels, tsu->pitch);
		SDL_UnlockSurface(tsu);

		Uint64 t2 = SDL_GetPerformanceCounter();
		Uint64 tper = SDL_GetPerformanceFrequency() / 1000;
		SDL_Log("SDL_RenderReadPixels() use %d ms\n", (int)((t2 - t1) / tper));
		//SDL_SaveBMP(masksu, "test.bmp");
	}
}

void LOEffect::MosaicEffect(SDL_Renderer*ren, LOLayerInfo *info, SDL_Texture *maskTex, double pos, bool isout) {
	//5 to 155  30 levels in total



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
