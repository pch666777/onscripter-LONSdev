/*
//音频部分
*/
#ifndef __LOAUDIO_H__
#define __LOAUDIO_H__



#include "../Scripter/FuncInterface.h"
#include "LOAudioElement.h"
#include <SDL.h>
#include <SDL_mixer.h>

#define DEFAULT_AUDIOBUF 4096

class LOAudioModule :public FunctionInterface{
public:
	LOAudioModule();
	~LOAudioModule();
	
	enum {
		//0~49 is se
		INDEX_WAVE = 50,
		INDEX_MUSIC = 51,
	};
	enum {
		FLAGS_SEPLAY_BGMDOWN = 1,
		//是否产生se播放完成的回调信号，有捕获事件需求时才产生回调信号
		FLAGS_SEND_PALYFINISH = 2,
		//是否所有的channel都产生回调信号，默认只有0通道才产生
		FLAGS_SEND_ALLChANNEL = 4,
	};

	LOString afterBgmName;
	int InitAudioModule();

	void SetFlags(int f) { flags |= f; }
	void UnsetFlags(int f) { flags &= (~f); }
	bool isSeCallBack() { return flags & FLAGS_SEND_PALYFINISH; }
	bool isAllChannelCallBack() { return flags & FLAGS_SEND_ALLChANNEL; }
	void ResetMe();
	void ResetMeFinish();
	void PlayAfter();
	void channelFinish_t(int channel);
	
	void SePlay(int channel, LOString s, int loopcount);

	int bgmCommand(FunctionInterface *reader);
	int bgmonceCommand(FunctionInterface *reader);
	int bgmstopCommand(FunctionInterface *reader);
	int loopbgmCommand(FunctionInterface *reader);
	int bgmfadeCommand(FunctionInterface *reader);
	int waveCommand(FunctionInterface *reader);
	int bgmdownmodeCommand(FunctionInterface *reader);
	int voicevolCommand(FunctionInterface *reader);
	int chvolCommand(FunctionInterface *reader);
	int dwaveCommand(FunctionInterface *reader);
	int dwaveloadCommand(FunctionInterface *reader);
	int dwaveplayCommand(FunctionInterface *reader);
	int dwavestopCommand(FunctionInterface *reader);
	int getvoicevolCommand(FunctionInterface *reader);
	int loopbgmstopCommand(FunctionInterface *reader);
	int stopCommand(FunctionInterface *reader);

private:
	int bgmFadeInTime;
	int bgmFadeOutTime;
	int currentChannel;
	//bool bgmDownFlag;
	int flags;

	//每次播放都新建一个LOAudioElement，因此设置，读取时必须加锁
	LOAudioElement *_audioPtr[INDEX_MUSIC + 1];  //不要直接操作
	int _audioVol[INDEX_MUSIC + 1];
	std::mutex ptrMutex;

	
	void SetAudioEl(int index, LOAudioElement *aue);
	void SetAudioElAndPlay(int index, LOAudioElement *aue);
	LOAudioElement* GetAudioEl(int index);
	void FreeAudioEl(int index);
	void BGMCore(LOString &s, int looptimes);
	void SeCore(int channel, LOString &s, int looptimes);
	void SetBGMvol(int vol, double ratio);
	void SetSevol(int channel, int vol);
	bool CheckChannel(int channel,const char* info);
};



#endif // !__LOAUDIO_H__