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
	for (int ii = 0; ii < fadeList.size(); ii++) delete (FadeData*)fadeList[ii];
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
	LOAudioElement *aue = takeChannelSafe(INDEX_MUSIC);
	if (aue) {
		aue->Stop(0);
		delete aue;
	}

	aue = new LOAudioElement();
	BinArray *bin = fileModule->ReadFile(&s, false);
	aue->SetData(bin, -1, looptimes);
	aue->buildStr = s;
	inputChannelSafe(INDEX_MUSIC, aue);

	if (aue->isAvailable()) {
		//SDL2的渐入是阻塞式的，而ONS是非阻塞式的，因此必须自己实现
		if (bgmFadeInTime > 0) {
			FadeData *fade = GetFadeData(INDEX_MUSIC);
			fade->ResetMe();
			fade->fadeInEvent.reset(LOEventHook::CreateAudioFadeEvent(INDEX_MUSIC, audioVol[INDEX_MUSIC], bgmFadeInTime));
			//由静音开始
			SetBGMvol(0, 0);
			//提交到主线程
			imgeModule->waitEventQue.push_N_back(fade->fadeInEvent);
		}
		aue->Play(0);
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
		if (channel == INDEX_MUSIC) {
			if (bgmFadeOutTime > 0) {
				FadeData *fade = GetFadeData(INDEX_MUSIC);
				fade->ResetMe();
				fade->fadeOutEvent.reset(LOEventHook::CreateAudioFadeEvent(INDEX_MUSIC, 0, bgmFadeOutTime));
				//aue需要重新放回，由播放结束事件清理
				inputChannelSafe(INDEX_MUSIC, aue);
				aue = nullptr;
				//提交主线程
				imgeModule->waitEventQue.push_N_back(fade->fadeOutEvent);
			}
			else aue->Stop(0);
		}
		else aue->Stop(0);

		if(aue) delete aue;
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


void LOAudioModule::FadeData::ResetMe() {
	if (fadeInEvent) fadeInEvent->InvalidMe();
	if (fadeOutEvent) fadeOutEvent->InvalidMe();
	//注意不能使用reset()，因为可能在两个线程中同时操作，设置ptr的过程并不是线程安全的
	fadeInEvent = LOShareEventHook();
	fadeOutEvent = LOShareEventHook();
}

LOAudioModule::FadeData *LOAudioModule::GetFadeData(int channel) {
	FadeData *fade = nullptr;
	for (int ii = 0; ii < fadeList.size(); ii++) {
		fade = (FadeData*)fadeList[ii];
		if (fade->channel == channel) return fade;
	}
	fade = new FadeData();
	fadeList.push_back((intptr_t)fade);
	return fade;
}


int LOAudioModule::RunFunc(LOEventHook *hook, LOEventHook *e) {
	if (e->param1 == LOEventHook::SCRIPT_CALL_AUDIOFADE) RunFuncAudioFade(hook, e);
	return LOEventHook::RUNFUNC_FINISH;
}


int LOAudioModule::RunFuncAudioFade(LOEventHook *hook, LOEventHook *e) {
	Uint32 curTick = SDL_GetTicks();
	if(curTick - e->timeStamp < 2) return LOEventHook::RUNFUNC_FINISH;

	int channel = e->param2;
	double per = e->GetParam(0)->GetDoule();
	double curVol = e->GetParam(1)->GetDoule();
	per *= (curTick - e->timeStamp);
	if( per < 1.0) return LOEventHook::RUNFUNC_FINISH;
	curVol -= per;
	e->GetParam(1)->SetDoule(curVol);

	if (curVol < 0.0) curVol = 0.0;
	if (curVol > audioVol[channel]) curVol = (double)audioVol[channel];

	//到达音量边界
	if (curVol <= 0 || curVol >= audioVol[channel]) e->InvalidMe();

	//百分比转到SDL的音量比
	curVol = curVol / 100 *  MIX_MAX_VOLUME;
	if (channel == INDEX_MUSIC) Mix_VolumeMusic((int)curVol);
	else Mix_Volume(channel, (int)curVol);

	//fade out finish delete it
	if (curVol <= 0) {
		LOAudioElement *aue = takeChannelSafe(channel);
		if (aue) {
			aue->Stop(0);
			delete aue;
		}
	}

	return LOEventHook::RUNFUNC_FINISH;
}
