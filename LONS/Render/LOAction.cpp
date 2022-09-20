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

int LOAction::Serialize(BinArray *bin) {
	int len = bin->WriteLpksEntity("anim", 0, 1);
	bin->WriteInt4(acType, loopMode, lastTime, gVal);
	bin->WriteInt(flags);
	//返回写入长度的位置
	return len;
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
	return 0;
}

//========================

LOActionText::LOActionText() {
	acType = ANIM_TEXT;
}

LOActionText::~LOActionText() {
}

int LOActionText::Serialize(BinArray *bin) {
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