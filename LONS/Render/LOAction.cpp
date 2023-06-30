/*
//绑定到图层的动作
*/
#include "LOAction.h"

LOAction::LOAction() {
	flags = FLAGS_ENBLE;
	acType = ANIM_NONE;
	loopMode = LOOP_NONE;
	lastTime = 0;
    gVal = 0 ;
}

LOAction::~LOAction() {

}

void LOAction::setEnble(bool enble) {
	if (enble) flags |= FLAGS_ENBLE;
	else flags &= (~FLAGS_ENBLE);
}

int LOAction::Serialize(BinArray *bin) {
	int len = bin->WriteLpksEntity("anim", 0, 1);
	bin->WriteInt4(acType, loopMode, lastTime, gVal);
	bin->WriteInt(flags);
	//返回写入长度的位置
	return len;
}


int  LOAction::getTypeFromStream(BinArray *bin, int pos) {
	if (!bin->CheckEntity("anim", nullptr, nullptr, &pos)) return -1;
	return bin->GetIntAuto(&pos);
}


bool LOAction::DeSerialize(BinArray *bin, int *pos) {
	if (!bin->CheckEntity("anim", nullptr, nullptr, pos)) return false;
	acType = (AnimaType)bin->GetIntAuto(pos);
	loopMode = (AnimaLoop)bin->GetIntAuto(pos);
	lastTime = bin->GetIntAuto(pos);
	gVal = bin->GetIntAuto(pos);
	flags = bin->GetIntAuto(pos);
	return true;
}


//=========================

LOActionNS::LOActionNS() {
	acType = ANIM_NSANIM;
}

LOActionNS::~LOActionNS() {

}

void LOActionNS::setSameTime(int32_t t, int count) {
	cellTimes.clear();
	for (int ii = 0; ii < count; ii++) cellTimes.push_back(t);
}

int LOActionNS::Serialize(BinArray *bin) {
	int len = LOAction::Serialize(bin);
	//只需要记住当前处于哪一帧
	bin->WriteChar((char)cellCurrent);
	bin->WriteInt(bin->Length() - len, &len);
	return 0;
}

bool LOActionNS::DeSerialize(BinArray *bin, int *pos) {
	if (!LOAction::DeSerialize(bin, pos)) return false;
	cellCurrent = bin->GetChar(pos);
	return true;
}


//============= movie ==============
SmpegContext::SmpegContext() {
	frame = nullptr;
	lock = SDL_CreateMutex();
	frameCount = 0;
}

SmpegContext::~SmpegContext() {
	frame = nullptr;
	SDL_DestroyMutex(lock);
	frameCount = 0;
}

LOActionMovie::LOActionMovie() {
	acType = ANIM_VIDEO;
	mpeg = nullptr;
	frameID = 0;
	setFlags(FLAGS_INIT);   //要求首次运行时初始化
}

LOActionMovie::~LOActionMovie() {
	if (mpeg) {
		SMPEG_stop(mpeg);
		SMPEG_delete(mpeg);
	}
	mpeg = nullptr;
}

char* LOActionMovie::InitSmpeg(const char *fn) {
	mpeg = SMPEG_new(fn, &info, 1);
	//检查是否有错误
	if (SMPEG_error(mpeg)) {
		SMPEG_delete(mpeg);
		mpeg = nullptr;
		return SMPEG_error(mpeg);
	}
	SMPEG_enableaudio(mpeg, 1);
	SMPEG_enablevideo(mpeg, 1);

	//在action首次初始化时绑定播放纹理
	return nullptr;
}

//========================

LOActionMove::LOActionMove(){
    acType = ANIM_MOVE;
    startPt = {0, 0};
    endPt = {0, 0};
    duration = 0 ;
}

LOActionMove::~LOActionMove(){

}

//=======================

LOActionText::LOActionText() {
	acType = ANIM_TEXT;
}

LOActionText::~LOActionText() {
}

int LOActionText::Serialize(BinArray *bin) {
	//animi, acType, loopmode
	int len = LOAction::Serialize(bin);
	//只需要记录当前位置
	bin->WriteInt16(currentPos);
	bin->WriteInt(bin->Length() - len, &len);
	return 0;
}


bool LOActionText::DeSerialize(BinArray *bin, int *pos) {
	if (!LOAction::DeSerialize(bin, pos)) return false;
	currentPos = bin->GetIntAuto(pos);
	return true;
}


LOAction* CreateActionFromType(int acType) {
	LOAction *acb = nullptr;
	switch (acType) {
	case LOAction::ANIM_NSANIM:
		acb = new LOActionNS();
	case LOAction::ANIM_TEXT:
		acb = new LOActionText();
	default:
		break;
	}
	return acb;
}
