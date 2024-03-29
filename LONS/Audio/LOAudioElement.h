﻿/*
//单个的音频资源，回调函数通常在其他线程，因此要考虑多线程的影响
*/
#ifndef __LOAUDIOELEMENT_H__
#define __LOAUDIOELEMENT_H__

#include "../etc/BinArray.h"
#include "../etc/LOString.h"
#include <atomic>
#include <string>
#include <SDL.h>
#include <SDL_mixer.h>

class LOAudioElement{
public:
	enum {
		//要么只播放一次，要么一直循环
		FLAGS_LOOP_ONECE = 1,
		FLAGS_IS_BGM = 2,
	};

	LOAudioElement();
	~LOAudioElement();
	void Serialize(BinArray *bin);
	static LOAudioElement* DeSerialize(BinArray *bin, int *pos);

	//loopbgm直接在'\0'后面再接一个bgm的名称
	LOString buildStr;
	Mix_Music *music;
	Mix_Chunk *chunk;
	int channel;
	int loopCount;
	
	bool isBGM() { return flags & FLAGS_IS_BGM; }
	bool isAvailable();
	bool isLoop() { return loopCount > 1 || loopCount == -1; }
	void FreeData();
	void SetData(BinArray *bin, int channel, int loops);  //chid < 0为music，chid >= 0为se
	void SetData2(BinArray *bin);  //用于load的时候单纯设置数据
	void SetLoop(int l) { loopCount = l; }
private:
	void SetNull();
	BinArray *rwbin;
	SDL_RWops *rwpos;
	int flags;
};



#endif // !__LOAUDIOELEMENT_H__
