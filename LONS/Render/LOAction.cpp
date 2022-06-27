/*
//绑定到图层的动作
*/
#include "LOAction.h"

LOAction::LOAction() {
	flags = FLAGS_ENBLE;
	acType = ANIM_NONE;
	lastTime = 0;
}

LOAction::~LOAction() {

}

void LOAction::setEnble(bool enble) {
	if (enble) flags |= FLAGS_ENBLE;
	else flags &= (~FLAGS_ENBLE);
}

void LOAction::Serialize(BinArray *bin) {
	//animi, acType, loopmode
	bin->WriteInt3(0x6D696E61, acType, loopMode);
	bin->WriteInt3(lastTime, gVal, flags);
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

void LOActionNS::Serialize(BinArray *bin) {
	LOAction::Serialize(bin);
	bin->WriteInt16(cellCount);
	bin->WriteChar((char)cellForward);
	bin->WriteChar((char)cellCurrent);
	bin->WriteInt(cellTimes.size());
	for (int ii = 0; ii < cellTimes.size(); ii++) bin->WriteInt(cellTimes.at(ii));
}

//========================

LOActionText::LOActionText() {
	acType = ANIM_TEXT;
}

LOActionText::~LOActionText() {
}

void LOActionText::Serialize(BinArray *bin) {
	//animi, acType, loopmode
	LOAction::Serialize(bin);
	bin->WriteInt16(currentPos);
	bin->WriteInt16(initPos);
	bin->WriteInt16(loopDelay);
	bin->WriteDouble(perPix);
	//hook只记录必要的内容
	//bin->WriteInt(hook->GetParam(0)->GetInt());
	bin->WriteInt(0);
}