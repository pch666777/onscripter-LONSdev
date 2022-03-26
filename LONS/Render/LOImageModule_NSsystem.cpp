/*
//NS一写系统
*/

#include "LOImageModule.h"


//int LOImageModule::cselCommand(FunctionInterface *reader) {
//	int count = reader->GetParamCount() / 2;
//
//	int ids[] = { 1,255,255 };
//	int fullid = GetFullID(LOLayer::LAYER_SELECTBAR, ids);
//	LOLayerInfo *info = GetInfoNewAndFreeOld(fullid, "_lons");
//	
//	LOString caption = reader->GetParamStr(0);
//	LOString tag = "*b;";
//	tag.SetEncoder(caption.GetEncoder());
//	tag.append(caption);
//
//	loadSpCore(info, tag, 0,0 , 255);
//
//	return RET_CONTINUE;
//}