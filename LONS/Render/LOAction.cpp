/*
//绑定到图层的动作
*/
#include "LOAction.h"

LOAction::LOAction() {
	flags = FLAGS_ENBLE;
	acType = ANIM_NONE;
}

LOAction::~LOAction() {

}

//=========================

LOActionNS::LOActionNS() {
	acType = ANIM_NSANIM;
}

LOActionNS::~LOActionNS() {

}

//========================

LOActionText::LOActionText() {
	acType = ANIM_TEXT;
}

LOActionText::~LOActionText() {
}