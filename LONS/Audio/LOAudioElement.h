/*
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

	LOString buildStr;
	
	bool isBGM() { flags & FLAGS_IS_BGM; }
	bool isAvailable();
	void FreeData();
	void SetData(BinArray *bin, int channel, int loops);  //chid < 0为music，chid >= 0为se
	void SetLoop(int l) { loopCount = l; }
	bool Play(int fade);
	void Stop(int fade);
private:
	void SetNull();
	BinArray *rwbin;
	SDL_RWops *rwpos;
	Mix_Music *music;
	Mix_Chunk *chunk;
	int flags;
	int channel;
	int loopCount;
};



#endif // !__LOAUDIOELEMENT_H__
