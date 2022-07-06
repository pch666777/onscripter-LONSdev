/*
//单个的音频资源，回调函数通常在其他线程，因此要考虑多线程的影响
*/

#include "LOAudioElement.h"

LOAudioElement::LOAudioElement() {
	SetNull();
	channel = 0;
	loopCount = 0;
	volume = 100;
}

LOAudioElement::~LOAudioElement() {
	FreeData();
}

void LOAudioElement::FreeData() {
	if (music) Mix_FreeMusic(music);
	if (chunk) Mix_FreeChunk(chunk);
	if (rwpos) SDL_RWclose(rwpos);
	if (rwbin) delete rwbin;
	buildStr.clear();
	SetNull();
	channel = 0;
	loopCount = 0;
}

void LOAudioElement::SetNull() {
	rwbin = NULL;
	music = NULL;
	chunk = NULL;
	rwpos = NULL;
}


void LOAudioElement::SetData(BinArray *bin, int channel, int loops) {
	FreeData();
	if (!bin || bin->Length() == 0) return;
	rwbin = bin;
	this->channel = channel;
	this->loopCount = loops;
	if (loops == 1) flags |= FLAGS_LOOP_ONECE;
	if (channel < 0) flags |= FLAGS_IS_BGM;
	rwpos = SDL_RWFromMem(rwbin->bin, rwbin->Length());
	if (flags & FLAGS_IS_BGM) music = Mix_LoadMUS_RW(rwpos, 0);
	else chunk = Mix_LoadWAV_RW(rwpos, 0);
}

bool LOAudioElement::Play(int fade) {
	int vol = (double)volume / 100 * MIX_MAX_VOLUME;

	if (channel < 0) {
		if (music) {
			Mix_VolumeMusic(vol);
			if (fade > 0)  Mix_FadeInMusic(music, loopCount, fade);
			else Mix_PlayMusic(music, loopCount);
			return true;
		}
		else return false;
	}
	else if (channel >= 0) {
		if (chunk) {
			Mix_Volume(channel, vol);
			if (fade > 0) Mix_FadeInChannel(channel, chunk, 0, fade);
			else Mix_PlayChannel(channel, chunk, 0);
			return true;
		}
		else return false;
	}
	return false;
}

void LOAudioElement::Stop(int fade) {
	if (channel < 0) {
		if (fade > 0) Mix_FadeOutMusic(fade);
		else Mix_HaltMusic();
	}
	else {
		if(fade > 0) Mix_FadeOutChannel(channel, fade);
		else Mix_HaltChannel(channel);
	}
}


bool LOAudioElement::isAvailable() {
	if (channel < 0 && music) return true;
	if (channel >= 0 && chunk) return true;
	return false;
}


void LOAudioElement::SetVolume(int vol) {
	volume = vol;

}