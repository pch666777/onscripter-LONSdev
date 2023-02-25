/*
//音频部分
*/
#include "../etc/LOEvent1.h"
#include "LOAudioModule.h"
#include <mutex>

extern void musicFinished();
extern void channelFinish(int channel);
extern void FatalError(const char *fmt, ...);

LOAudioModule::LOAudioModule() {
	audioModule = this;
	memset(audioPtr, NULL, sizeof(LOAudioElement*) * (INDEX_MUSIC + 1));
	bgmFadeInTime = 0;
	bgmFadeOutTime = 0;
	currentChannel = 0;
	flags = 0;
	channelActiveFlag = 0;
	lockFlag.store(LOCK_OFF);
}

LOAudioModule::~LOAudioModule() {
	for (int ii = 0; ii < INDEX_MUSIC + 1; ii++) {
		if(audioPtr[ii]) delete audioPtr[ii];
	}
}

void LOAudioModule::lock() {
	int nf = LOCK_OFF;
	while (!lockFlag.compare_exchange_weak(nf, LOCK_ON));
}

void LOAudioModule::unlock() {
	int nf = LOCK_ON;
	while (!lockFlag.compare_exchange_weak(nf, LOCK_OFF));
}


//重置音频模块
void LOAudioModule::ResetMe() {
	lock();
	//先解除播放回调
	Mix_ChannelFinished(NULL);
	Mix_HookMusicFinished(NULL);

	//不再需要等待播放队列
	waitEventQue.clear();

	for (int ii = 0; ii < INDEX_MUSIC + 1; ii++) {
		if (audioPtr[ii]) {
			audioPtr[ii]->Stop(0);
			delete audioPtr[ii];
			audioPtr[ii] = nullptr;
			audioVol[ii] = 100;
		}
	}
	ChangeModuleState(MODULE_STATE_NOUSE);
	unlock();
}


void LOAudioModule::LoadFinish() {
	ChangeModuleState(MODULE_STATE_RUNNING);
	Mix_HookMusicFinished(musicFinished);
	Mix_ChannelFinished(channelFinish);
	for (int ii = 0; ii < INDEX_MUSIC + 1; ii++) {
		//if (ii == INDEX_MUSIC) {
		//	int bbk = 1;
		//}
		if (audioPtr[ii]) audioPtr[ii]->Play(0);
	}
}


void LOAudioModule::SetBGMvol(int vol, double ratio) {
	vol = ratio * vol / 100 * MIX_MAX_VOLUME;
	Mix_VolumeMusic(vol);
}

void LOAudioModule::SetSevol(int channel, int vol) {
	vol = (double)vol / 100 * MIX_MAX_VOLUME;
	Mix_Volume(channel, vol);
}
//
//void LOAudioModule::FreeAudioEl(int index) {
//	LOAudioElement *aue = GetAudioEl(index);
//	if (aue) {
//		if (aue->isBGM()) aue->Stop(bgmFadeOutTime);
//		else aue->Stop(0);
//		delete aue;
//	}
//}

bool LOAudioModule::CheckChannel(int channel, const char* info) {
	if (channel < 0 || channel > 49) {
		FatalError(info, channel);
		return false;
	}
	else return true;
}

int LOAudioModule::bgmCommand(FunctionInterface *reader) {
	LOString s = reader->GetParamStr(0);
	BGMCore(s, -1);
	return RET_CONTINUE;
}

int LOAudioModule::bgmonceCommand(FunctionInterface *reader) {
	LOString s = reader->GetParamStr(0);
	BGMCore(s, 0);
	
	return RET_CONTINUE;
}

void LOAudioModule::BGMCore(LOString &s, int looptimes) {
	//停止播放时会触发Mix_ChannelFinished/Mix_HookMusicFinished回调
	//先取出就不会在回调线程中再操作音频对象了
	LOAudioElement *oue = takeChannelSafe(INDEX_MUSIC);
	//加载新的音频
	LOAudioElement *nau = new LOAudioElement();
	BinArray *bin = fileModule->ReadFile(&s, false);
	nau->SetData(bin, -1, looptimes);
	nau->buildStr = s;
	inputChannelSafe(INDEX_MUSIC, nau);

	//如果
	if (oue) {
		//如果新音频无效，且有fadeout，那么相当于执行bgmstop
		if (oue->isAvailable() && !nau->isAvailable()) {
			//淡出事件
			//work it
		}
		delete oue;
	}

	if (nau->isAvailable()) {
		//SDL2的渐入是阻塞式的，而ONS是非阻塞式的，因此必须自己实现
		if (bgmFadeInTime > 0) {
			Mix_VolumeMusic(0);  //首先设置为0
			//创建音频渐入事件
			//work it
		}
		nau->Play(0);
	}
}

void LOAudioModule::SeCore(int channel, LOString &s, int looptimes) {
	StopCore(channel);
	LOAudioElement* aue = new LOAudioElement();
	BinArray *bin = fileModule->ReadFile(&s, false);
	aue->SetData(bin, channel, looptimes);
	aue->buildStr = s;

	if (isSePalyBgmDown()) {
		SetBGMvol(audioVol[INDEX_MUSIC], 0.6);
	}

	inputChannelSafe(channel, aue);
	if (aue->isAvailable()) {
		channelActiveFlag |= (1 << channel);
		aue->Play(0);
	}
	else if (isSePalyBgmDown()) {
		//播放失败应该尝试恢复bgm音量
		channelActiveFlag &= (~(1 << channel));
		RebackBgmVol();
	}
}


void LOAudioModule::StopCore(int channel) {
	//停止播放时会触发Mix_ChannelFinished/Mix_HookMusicFinished回调
	//先取出就不会在回调线程中再操作音频对象了
	LOAudioElement *aue = takeChannelSafe(channel);
	if (aue) {
		if(channel == INDEX_MUSIC) aue->Stop(bgmFadeOutTime);
		else aue->Stop(0);
		delete aue;
		//channelActiveFlag &= (~(1 << channel));
	}
}

void LOAudioModule::RebackBgmVol() {
	if (channelActiveFlag == 0) {
		SetBGMvol(audioVol[INDEX_MUSIC], 1.0);
	}
}


int LOAudioModule::bgmstopCommand(FunctionInterface *reader) {
	StopCore(INDEX_MUSIC);
	return RET_CONTINUE;
}

void musicFinished() {
	//NEVER call SDL Mixer functions, nor SDL_LockAudio, from a callback function.
	//在主脚本进程再回调LOAudioModule中的相关函数
	LOAudioModule *au = (LOAudioModule*)(FunctionInterface::audioModule);
	//loopbgm播放第二段
	if (au->afterBgmName.length() > 0) {
		//模块处于挂起状态时，播放完成事件被存储在待处理区
		LOShareEventHook ev(LOEventHook::CreateSignal(LOEventHook::MOD_SCRIPTER, LOEventHook::FUN_BGM_AFTER));
		FunctionInterface::scriptModule->waitEventQue.push_N_back(ev);
	}
}

void channelFinish(int channel) {
	LOAudioModule *au = (LOAudioModule*)(FunctionInterface::audioModule);
	au->channelFinish_t(channel);
}


void LOAudioModule::channelFinish_t(int channel) {
	//注意回调既不在脚本线程，也不在渲染线程，是另外的线程，线程安全比较重要
	StopCore(channel);
	channelActiveFlag &= (~(1 << channel));
	if (isSePalyBgmDown()) RebackBgmVol();

	//0频道绑定了按钮超时事件，频道播放完成事件
	if ((isChannelZeroEv() && channel == 0) || isChannelALLEv()) {
		//模块处于挂起状态时，播放完成事件被存储在待处理区
		LOShareEventHook ev(LOEventHook::CreateSePalyFinishEvent(channel));
		if (isModuleSuspend()) {
			//进入等待区
		}
		else {
			//发到到事件处理
		}
	}
}


int LOAudioModule::InitAudioModule() {
	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, DEFAULT_AUDIOBUF) < 0) return false;
	Mix_AllocateChannels(INDEX_MUSIC);
	//ResetMeFinish();
	return true;
}


int LOAudioModule::loopbgmCommand(FunctionInterface *reader) {
	LOString s1 = reader->GetParamStr(0);
	afterBgmName = reader->GetParamStr(1);
	BGMCore(s1, 1);
	Mix_HookMusicFinished(musicFinished);
	SetFlags(FLAGS_BGM_CALLBACK);
	return RET_CONTINUE;
}

int LOAudioModule::loopbgmstopCommand(FunctionInterface *reader) {
	afterBgmName.clear();
	StopCore(INDEX_MUSIC);
	UnsetFlags(FLAGS_BGM_CALLBACK);
	return RET_CONTINUE;
}

int LOAudioModule::bgmfadeCommand(FunctionInterface *reader) {
	if (reader->isName("bgmfadein") || reader->isName("mp3fadein")) {
		bgmFadeInTime = reader->GetParamInt(0);
	}
	else {
		bgmFadeOutTime = reader->GetParamInt(0);
	}
	if (bgmFadeInTime < 0) bgmFadeInTime = 0;
	if (bgmFadeOutTime < 0) bgmFadeOutTime = 0;
	return RET_CONTINUE;
}

int LOAudioModule::waveCommand(FunctionInterface *reader) {
	if (reader->isName("wavestop")) {
		StopCore(INDEX_WAVE);
	}
	else {
		int looptime = 0;
		if (reader->isName("waveloop")) looptime = -1;
		LOString s = reader->GetParamStr(0);
		SeCore(INDEX_WAVE, s, looptime);
	}
	return RET_CONTINUE;
}


//取出音频
LOAudioElement* LOAudioModule::takeChannelSafe(int channel) {
	lock();
	LOAudioElement *aue = audioPtr[channel];
	audioPtr[channel] = nullptr;
	unlock();
	return aue;
}

//置入音频，必须先取出，不会释放已有的音频，只是单纯的替换指针
void LOAudioModule::inputChannelSafe(int channel, LOAudioElement *aue) {
	lock();
	audioPtr[channel] = aue;
	unlock();
}


int LOAudioModule::bgmdownmodeCommand(FunctionInterface *reader) {
	if( reader->GetParamInt(0)) SetFlags(FLAGS_SEPLAY_BGMDOWN) ;
	else UnsetFlags(FLAGS_SEPLAY_BGMDOWN);
	if (!isSePalyBgmDown()) {
		channelActiveFlag = 0;
		RebackBgmVol();
	}
	return RET_CONTINUE;
}



int LOAudioModule::voicevolCommand(FunctionInterface *reader) {
	int vol = reader->GetParamInt(0);
	if (vol < 0 || vol > 100) vol = 100;

	if (reader->isName("bgmvol") || reader->isName("mp3vol")) {
		audioVol[INDEX_MUSIC] = vol;
		SetBGMvol(vol, 1.0);
	}
	else if (reader->isName("voicevol")) {
		audioVol[INDEX_WAVE] = vol;
		SetSevol(INDEX_WAVE, vol);
	}
	else {
		for (int ii = 0; ii < INDEX_WAVE; ii++) {
			audioVol[ii] = vol;
			SetSevol(ii, vol);
		}
	}

	return RET_CONTINUE;
}

int LOAudioModule::chvolCommand(FunctionInterface *reader) {
	int channel = reader->GetParamInt(0);
	int vol = reader->GetParamInt(1);
	if (vol < 0 || vol > 100) vol = 100;
	if (!CheckChannel(channel, "[chvol] channel out of range:%d")) return RET_CONTINUE;

	audioVol[channel] = vol;
	SetSevol(channel, audioVol[channel]);

	return RET_CONTINUE;
}

int LOAudioModule::dwaveCommand(FunctionInterface *reader) {
	int channel = reader->GetParamInt(0);
	LOString s = reader->GetParamStr(1);

	if (!CheckChannel(channel, "[dwave] channel out of range:%d")) return RET_CONTINUE;
	
	int loopcount = 0;
	if (reader->isName("dwaveloop")) loopcount = -1;
	SeCore(channel, s, loopcount);
	currentChannel = channel;
	return RET_CONTINUE;
}

int LOAudioModule::dwaveloadCommand(FunctionInterface *reader) {
	int channel = reader->GetParamInt(0);
	LOString s = reader->GetParamStr(1);

	if (!CheckChannel(channel, "[dwaveload] channel out of range:%d")) return RET_CONTINUE;

	StopCore(channel);
	BinArray *bin = fileModule->ReadFile(&s, false);
	LOAudioElement *aue = new LOAudioElement;
	aue->SetData(bin, channel, 0);
	inputChannelSafe(channel, aue);

	return RET_CONTINUE;

}

int LOAudioModule::dwaveplayCommand(FunctionInterface *reader) {
	int channel = reader->GetParamInt(0);
	if (!CheckChannel(channel, "[dwaveplay] channel out of range:%d")) return RET_CONTINUE;
	int looptime = 0;
	if (reader->isName("dwaveplayloop"))  looptime = -1;

	LOAudioElement *aue = takeChannelSafe(channel);
	if (aue) {
		aue->SetLoop(looptime);
		inputChannelSafe(channel, aue);
	}
	currentChannel = channel;
	return RET_CONTINUE;
}

int LOAudioModule::dwavestopCommand(FunctionInterface *reader) {
	StopCore(currentChannel);
	return RET_CONTINUE;
}

int LOAudioModule::getvoicevolCommand(FunctionInterface *reader) {
	ONSVariableRef *v = reader->GetParamRef(0);
	int vol = audioVol[INDEX_MUSIC];
	if (reader->isName("getvoicevol")) vol = audioVol[INDEX_WAVE];
	else if (reader->isName("getsevol")) vol = audioVol[currentChannel];
	v->SetValue((double)vol);
	return RET_CONTINUE;
}

int LOAudioModule::stopCommand(FunctionInterface *reader) {
	//禁用回调函数
	Mix_ChannelFinished(NULL);
	Mix_HookMusicFinished(NULL);
	for (int ii = 0; ii <= INDEX_MUSIC; ii++) {
		StopCore(ii);
	}
	Mix_ChannelFinished(channelFinish);
	if(isBgmCallback()) Mix_HookMusicFinished(musicFinished);
	return RET_CONTINUE;
}


void LOAudioModule::SePlay(int channel, LOString s, int loopcount) {
	SeCore(channel, s, loopcount);
}

void LOAudioModule::PlayAfter() {
	BGMCore(afterBgmName, -1);
	afterBgmName.clear();
	UnsetFlags(FLAGS_BGM_CALLBACK);
	//检查bgm回调事件？
	Mix_HookMusicFinished(NULL);
}


void LOAudioModule::Serialize(BinArray *bin) {
	int len = bin->WriteLpksEntity("audo", 0, 1);
	bin->WriteInt(currentChannel);
	bin->WriteInt(flags);
	//音量
	//bin->Append((char*)audioVol, sizeof(int) * (INDEX_MUSIC + 1));
	//循环的音乐才存储，先写入一个循环的音乐数量
	int loopCountPos = bin->Length();
	int loopCount = 0;
	bin->WriteInt(loopCount);
	for (int ii = 0; ii <= INDEX_MUSIC; ii++) {
		if (audioPtr[ii]) {
			if (audioPtr[ii]->isLoop()) {
				audioPtr[ii]->Serialize(bin);
				loopCount++;
			}
		}
	}
	//回写数量
	bin->WriteInt(loopCount, &loopCountPos);

	bin->WriteLOString(&afterBgmName);
	bin->WriteInt(bin->Length() - len, &len);
}


bool LOAudioModule::DeSerialize(BinArray *bin, int *pos, LOEventMap *evmap) {
	int next = -1;
	if (!bin->CheckEntity("audo", &next, nullptr, pos)) return false;
	currentChannel = bin->GetIntAuto(pos);
	flags = bin->GetIntAuto(pos);
	int loopCount = bin->GetIntAuto(pos);
	for (int ii = 0; ii < loopCount; ii++) {
		LOAudioElement *aue = LOAudioElement::DeSerialize(bin, pos);
		if (!aue) {
			LOLog_e("LOAudioElement::DeSerialize() faild!");
			return false;
		}
		//读取文件数据
		BinArray *bin = fileModule->ReadFile(&aue->buildStr, false);
		aue->SetData2(bin);

		if(aue->GetChannel() >= 0) audioPtr[aue->GetChannel()] = aue;
		else audioPtr[INDEX_MUSIC] = aue;
	}
	afterBgmName = bin->GetLOString(pos);
	*pos = next;
	return true;
}
