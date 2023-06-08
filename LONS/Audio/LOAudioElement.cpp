/*
//单个的音频资源，回调函数通常在其他线程，因此要考虑多线程的影响
*/

#include "LOAudioElement.h"

LOAudioElement::LOAudioElement() {
	SetNull();
	channel = 0;
	loopCount = 0;
	flags = 0 ;
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

void LOAudioElement::SetData2(BinArray *bin) {
	if (!bin || bin->Length() == 0) return;
	rwbin = bin;
	if (loopCount == 1) flags |= FLAGS_LOOP_ONECE;
	if (channel < 0) flags |= FLAGS_IS_BGM;
	rwpos = SDL_RWFromMem(rwbin->bin, rwbin->Length());
	if (flags & FLAGS_IS_BGM) music = Mix_LoadMUS_RW(rwpos, 0);
	else chunk = Mix_LoadWAV_RW(rwpos, 0);
}



bool LOAudioElement::isAvailable() {
	if (channel < 0 && music) return true;
	if (channel >= 0 && chunk) return true;
	return false;
}


void LOAudioElement::Serialize(BinArray *bin) {
	int len = bin->WriteLpksEntity("aude", 0, 1);
	bin->WriteInt3(channel, flags, loopCount);
	bin->WriteLOString(&buildStr);

	bin->WriteInt(bin->Length() - len, &len);
}


LOAudioElement* LOAudioElement::DeSerialize(BinArray *bin, int *pos) {
	int next = -1;
	if (!bin->CheckEntity("aude", &next, nullptr, pos)) return nullptr;
	LOAudioElement *aue = new LOAudioElement();
	aue->channel = bin->GetIntAuto(pos);
	aue->flags = bin->GetIntAuto(pos);
	aue->loopCount = bin->GetIntAuto(pos);
	aue->buildStr = bin->GetLOString(pos);
	*pos = next;
	return aue;
}