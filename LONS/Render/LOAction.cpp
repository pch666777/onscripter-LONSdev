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

//========================

LOActionText::LOActionText() {
	acType = ANIM_TEXT;
}

LOActionText::~LOActionText() {
}