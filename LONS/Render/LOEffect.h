#ifndef __LOEFFECT_H__
#define __LOEFFECT_H__

#include <string>
#include <SDL.h>
#include "LOLayerData.h"
#include "../etc/LOString.h"

class LOEffect
{
public:
	enum {
		DIRECTION_LEFT,
		DIRECTION_RIGHT,
		DIRECTION_TOP,
		DIRECTION_BOTTOM
	};

	enum {
		EFFECT_ID_QUAKE = 0x7FFFFF01
	};

	enum {
		RET_NONE,
		RET_FINISH,
		RET_CONTINUE
	};

	LOEffect();
	~LOEffect();

	int id;        //编号，某些特性有特殊用途
	int nseffID;   //ns效果编号
	int time;      //持续的时间
	LOString mask; //遮片名称
	
	LOUniqBaseTexture masksu;  //遮片
	char m_bit[4];   //RGBA的位置，某些时候有特殊用途
	double postime;  //已经运行的时间

	void CopyFrom(LOEffect *ef);
	//bool RunEffect(SDL_Renderer*ren, LOLayerData *info, LOShareTexture &efstex , LOShareTexture &efmtex, double pos);
	bool RunEffect2(SDL_Texture *edit, double pos);
	//刷新特效帧
	int UpdateEffect(SDL_Renderer *ren, SDL_Texture *texA, SDL_Texture *texB, SDL_Texture *edit);
	//获取当前图像因该偏移的位置，仅对print 11-14有效
	bool UpdateDstRect(SDL_Rect *rect);
	void ReadyToRun() { postime = 0; }
	SDL_Surface* Create8bitMask(SDL_Surface *su,bool isscale);
	SDL_Surface* Create8bitMask2(SDL_Surface *su, int w, int h);
	static SDL_Surface* ConverToGraySurface(SDL_Surface *su);
	static void CreateGrayColor(SDL_Palette *pale);

private:
	void FadeOut(int *alpha, double pos);
	void MaskEffectCore(SDL_Texture *edit, double pos,bool isalpha);
	void BlindsEffect(SDL_Texture *edit, double pos, int direction);
	void CurtainEffect(SDL_Texture *edit, double pos, int direction);
	void TestEffect(SDL_Texture *edit, double pos, int direction);
	void RollEffect(SDL_Texture *edit, double pos, int direction);
	void MosaicEffect(SDL_Renderer*ren, LOLayerData *info, SDL_Texture *maskTex, double pos, bool isout);
	void QuakeEffect(SDL_Renderer *ren, SDL_Texture *texB, double postime);
	void CreateSmallPic(SDL_Renderer*ren, LOLayerData *info, SDL_Texture *effectTex);
};


#endif // !__LOEFFECT_H__
