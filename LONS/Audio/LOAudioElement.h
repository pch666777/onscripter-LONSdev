/*
//单个的音频资源，回调函数通常在其他线程，因此要考虑多线程的影响
*/
#ifndef __LOAUDIOELEMENT_H__
#define __LOAUDIOELEMENT_H__

#include "../etc/BinArray.h"
#include <atomic>
#include <string>
#include <SDL.h>
#include <SDL_mixer.h>

class LOAudioElement
{
	//struct AudioBin{
	//	BinArray *bin = NULL;
	//	SDL_RWops *rwops = NULL;
	//};
public:
	LOAudioElement();
	~LOAudioElement();
	std::string Name;
	void FreeData();
	void SetData(BinArray *bin, int chid, int loopcount);  //chid < 0为music，chid >= 0为se
	bool Play(int fade);
	bool isAvailable();
	bool isBGM() { return channel < 0; }
	void Stop(int fade);
	int loopCount;

	static bool silenceFlag;    //静默标记，为真则所有音乐都不可播放
private:
	void SetNull();
	BinArray *rwbin;
	SDL_RWops *rwpos;
	Mix_Music *music;
	Mix_Chunk *chunk;
	int channel;
};



#endif // !__LOAUDIOELEMENT_H__
