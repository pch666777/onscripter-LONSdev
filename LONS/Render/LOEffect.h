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

	LOEffect();
	~LOEffect();

	int id;        //编号
	int nseffID;   //ns效果编号
	int time;      //持续的时间
	LOString mask; //遮片名称
	
	LOUniqBaseTexture masksu;  //遮片
	char m_bit[4];
	double postime;  //已经运行的时间

	void CopyFrom(LOEffect *ef);
	bool RunEffect(SDL_Renderer*ren, LOLayerData *info, LOShareTexture &efstex , LOShareTexture &efmtex, double pos);
	void ReadyToRun() { postime = 0; }
	SDL_Surface* Create8bitMask(SDL_Surface *su,bool isscale);
	static SDL_Surface* ConverToGraySurface(SDL_Surface *su);
	static void CreateGrayColor(SDL_Palette *pale);

private:
	void FadeOut(SDL_Renderer*ren, LOLayerData *info,double pos);
	void MaskEffectCore(SDL_Renderer*ren, LOLayerData *info,  SDL_Texture *maskTex, double pos,bool isalpha);
	void BlindsEffect(SDL_Renderer*ren, LOLayerData *info, SDL_Texture *maskTex, double pos, int direction);
	void CurtainEffect(SDL_Renderer*ren, LOLayerData *info, SDL_Texture *maskTex, double pos, int direction);
	void RollEffect(SDL_Renderer*ren, LOLayerData *info, SDL_Texture *maskTex, double pos, int direction);
	void MosaicEffect(SDL_Renderer*ren, LOLayerData *info, SDL_Texture *maskTex, double pos, bool isout);
	void CreateSmallPic(SDL_Renderer*ren, LOLayerData *info, SDL_Texture *effectTex);
};


#endif // !__LOEFFECT_H__
