/*
//绑定到图层的动作
*/

#ifndef __H_LOACTION_
#define __H_LOACTION_

#include <stdint.h>
#include <vector>
#include <memory>
#include <SDL.h>
#include <vector>
#include "../etc/LOEvent1.h"
#include "smpeg.h"

class LOAction
{
public:
	LOAction();
	virtual ~LOAction();

	enum AnimaType { //不要修改顺序
		ANIM_NONE = 0,
		ANIM_TEXT = 1,
		ANIM_NSANIM = 2,  //传统的NS动画模式
		ANIM_FADE = 4,
		ANIM_MOVE = 8,
		ANIM_ROTATE = 16,
		ANIM_SCALE = 32,
		ANIM_VIDEO = 64,
        ANIM_EXVIDEO = 128,
	};

	enum AnimaLoop{
		LOOP_CIRCULAR,  //从头到尾再从头到尾
		LOOP_ONSCE,   //从头到尾执行一次
		LOOP_GOBACK,    //从头到尾再到头一直循环
		LOOP_CELL,     //显示某一格或帧，然后停止
		LOOP_NONE,   //不生效，表示动作不会执行
	};

	enum {
		FLAGS_FINISH = 1,
		FLAGS_ENBLE = 2,
		//某些动作需要初始化，会设置这个值
		FLAGS_INIT = 4 ,
		//视频播放类，如果遇到click直接删除动作
		//FLAGS_CLICK_DEL = 8,
        //其他可能会标识用途的
	};

	AnimaType acType;
	AnimaLoop loopMode;
	Uint32 lastTime;
	//加速度
	int gVal;
	bool isEnble() { return flags & FLAGS_ENBLE; }
	bool isInit() { return flags & FLAGS_INIT; }
	void setEnble(bool enble);
	void setFlags(int f) { flags |= f; }
	void unSetFlags(int f) { flags &= (~f); }
	int  getFlags() { return flags; }
	static int  getTypeFromStream(BinArray *bin, int pos);
	virtual int Serialize(BinArray *bin);
	virtual bool DeSerialize(BinArray *bin, int *pos);
private:
	int flags;
};

typedef std::shared_ptr<LOAction> LOShareAction;

//NS动画
class LOActionNS : public LOAction{
public:
	LOActionNS();
	~LOActionNS();
	//设置指定格数的动画时间
	void setSameTime(int32_t t, int count);
	int Serialize(BinArray *bin);
	bool DeSerialize(BinArray *bin, int *pos);

	int16_t cellCount = 1;	 //动画分格总数
	int8_t  cellForward = 1; //前进方向, -1表示倒放
	int8_t  cellCurrent = 0; //动画当前的位于的格数
	std::vector<int32_t> cellTimes;  //每格动画的时间
};


//文字动画
class LOActionText :public LOAction {
public:
	LOActionText();
	~LOActionText();
	int Serialize(BinArray *bin);
	bool DeSerialize(BinArray *bin, int *pos);

	int16_t currentPos = 0;  //动画当前运行的位置
	int16_t initPos = 0; //初始化时需要到底的位置
	int16_t loopDelay = -1;   //循环延时
	double perPix = 10.0;     //每毫秒显示的像素数
	LOShareEventHook hook;
};

//smpeg结构
struct SmpegContext{
	SMPEG_Frame *frame;
	SDL_mutex *lock;
	int frameCount;
	//int lastFrameID;

	SmpegContext();
	~SmpegContext();
};

//视频播放
class LOActionMovie :public LOAction {
public:
	LOActionMovie();
	~LOActionMovie();
	char* InitSmpeg(const char *fn);

	SmpegContext context;
	SMPEG *mpeg;
	SMPEG_Info info;
	int frameID;
};


//移动类
class LOActionMove :public LOAction {
public:
        LOActionMove();
        ~LOActionMove();

        SDL_Point startPt;
        SDL_Point endPt;
        Uint32 duration;
};

extern LOAction* CreateActionFromType(int acType);

#endif // !__H_LOACTION_



