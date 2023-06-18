/*
//音频部分
*/
#ifndef __LOAUDIO_H__
#define __LOAUDIO_H__

//音频播放有个比较重要的问题，播放完成的事件回调是在另一个线程里的
//只有回调的通道是可靠的，不可以在回调线程里处理通道对象

#include "../Scripter/FuncInterface.h"
#include "LOAudioElement.h"
#include <atomic>
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
		//0通道播放完成事件
		FLAGS_SE_SIGNAL_ZERO = 2,
		//所有通道播放完成事件
		FLAGS_SE_SIGNAL_ALL = 4,

		//是否启用bgm播放完成事件
		FLAGS_BGM_CALLBACK = 8
	};
	enum {
		LOCK_OFF = 0,
		LOCK_ON = 1
	};

	//需要淡入淡出的事件比较少，因此将其独立出来
	struct FadeData{
		int channel;
		LOShareEventHook fadeInEvent;
		LOShareEventHook fadeOutEvent;

		void ResetMe();
	};

	LOString afterBgmName;
	void TestAudio();
	int InitAudioModule();
	int SendAudioEventToQue(int channel);
	void DoTimerEvent();

	void SetFlags(int f) { flags |= f; }
	void UnsetFlags(int f) { flags &= (~f); }
	bool isChannelZeroEv() { return flags & FLAGS_SE_SIGNAL_ZERO; }
	bool isChannelALLEv() { return flags & FLAGS_SE_SIGNAL_ALL; }
	bool isSePalyBgmDown() { return flags & FLAGS_SEPLAY_BGMDOWN; }
	bool isBgmCallback() { return flags & flags & FLAGS_BGM_CALLBACK; }

	int RunFunc(LOEventHook *hook, LOEventHook *e);
	int RunFuncAudioFade(LOEventHook *hook, LOEventHook *e);
	int RunFuncPlayFinish(LOEventHook *hook, LOEventHook *e);

	void ResetMe();
	void LoadFinish();
	void Serialize(BinArray *bin);
    void SerializeVolume(BinArray *bin);
	bool DeSerialize(BinArray *bin, int *pos, LOEventMap *evmap);
    bool DeSerializeVolume(BinArray *bin, int *pos);

	void PlayAfter();
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
	int getvoicestateCommand(FunctionInterface *reader);
	int getrealvolCommand(FunctionInterface *reader);
	int getvoicefileCommand(FunctionInterface *reader);

private:
	int bgmFadeInTime;
	int bgmFadeOutTime;
	int currentChannel;
	int flags;

	//频道激活情况
	int64_t channelActiveFlag;

	std::vector<intptr_t> fadeList;

	//锁的时间都很短，采用自旋锁
	std::atomic_int lockFlag;
	void lock();
	void unlock();

	LOAudioElement *audioPtr[INDEX_MUSIC + 1];
	//设定的音量大小0-100
	int8_t channelVol[INDEX_MUSIC + 1];
	//当前音量的系数0-100，计算方式：xs / 100 * channelVol = 实际音量大小
	//设定系数主要是因为fadein、fadeout这样的功能要实现
	int8_t channelVolXS[INDEX_MUSIC + 1];

	//取出音频，存储的指针北置为null
	LOAudioElement* takeChannelSafe(int channel);

	//置入音频，必须先取出，不会释放已有的音频，只是单纯的替换指针
	void inputChannelSafe(int channel, LOAudioElement *aue);

	
	//void SetAudioEl(int index, LOAudioElement *aue);
	//void SetAudioElAndPlay(int index, LOAudioElement *aue);
	//LOAudioElement* GetAudioEl(int index);
	//void FreeAudioEl(int index);
	void BGMCore(LOString &s, int looptimes);
	void BGMStopCore();
	void SeCore(int channel, LOString &s, int looptimes);
	void StopSeCore(int channel);
	void SetChannelVol(int channel, int vol);
	void SetChannelVol(int channel);

	bool CheckChannel(int channel,const char* info);
	FadeData *GetFadeData(int channel);
};



#endif // !__LOAUDIO_H__
