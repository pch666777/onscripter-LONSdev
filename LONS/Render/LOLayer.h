/*
*/
#pragma once
#ifndef _LOLAYER_H_
#define _LOLAYER_H_

#include "../Scripter/FuncInterface.h"
#include "LOLayerInfo.h"
#include "LOAnimation.h"
#include "LOFontBase.h"
#include "../etc/LOStack.h"
#include "LOMatrix2d.h"

#include <SDL.h>
#include <vector>
#include <map>
#include <unordered_map>



	////////////////////LOLayer/////////////////////////
	/*
	对于图层来说，所有的操作都是先进入队列，在同步到刷新
	因为图层操作变化非常频繁，需要一个信息缓存来缓存信息，避免频繁出现new/delete
	现阶段的filename和纹理对应设计得非常不好，很容易出bug
	*/

class LOLayer {
public:
	//图层分为两种组织方式，一种是基于树型的层级结构，一种是基于fullID的map结构
	//当前活动的图层记录在ActiveLayerMap中，处理队列中的图层记录中QueLayerMap中
	//分层类对象
	enum SysLayerType {
		LAYER_NSSYS,	//特效运行在此层
		LAYER_DIALOG,   //Text window and text  
		LAYER_SELECTBAR,   // select and bar
		LAYER_OTHER,     //** monochrome and negation effect **
		LAYER_SPRINTEX, //sp2
		LAYER_SPRINT,
		LAYER_STAND,    //standing characters
		LAYER_BG,  //背景图片

		LAYER_BASE_COUNT,
		//LAYER_SPRINT,      //添加图层使用这个，在above还是below由humanz决定，默认这个值是499
		LAYER_CC_USE       //内部使用的图层
	};

	//一些特别的编号
	enum {
		IDEX_DIALOG_TEXT = 5,    //在LAYER_DIALOG层
		IDEX_DIALOG_WINDOW = 50, //在LAYER_DIALOG层
		IDEX_DIALOG_TEXT_CHILD = 100,  //多句标准对话时，挂在在text的子层
		IDEX_NSSYS_EFFECT = 5, //effect 2-18使用的层，在LAYER_NSSYS层
		IDEX_NSSYS_BTN = 6,    //btn定义的按钮，在LAYER_NSSYS层
		IDEX_NSSYS_CUR = 7,    //Cursor在的层，在LAYER_NSSYS层
		IDEX_NSSYS_RMENU = 20,
	};



	//成员///////////
	LOLayerInfo *curInfo;       //当前显示的情况

	int id[3];      //father ID
	bool exbtnHasRun;
	SysLayerType layerType;     //图层所在的组，在new图层时已经把LAYER_SPRINT转换
	LOLayer *parent;   //父对象
	LOLayer *Root;    //在准备阶段是属于哪一个根图层的

	std::map<int, LOLayer*> *childs;
	bool isInit;


	LOLayer(SysLayerType type, int *cid);
	~LOLayer();

	LOMatrix2d matrix;  //变换矩阵

	//应用图层控制
	void UseControl(LOLayerInfo *info, std::map<int, LOLayer*> &btnMap);

	LOAnimation *GetAnimation(LOAnimation::AnimaType type);

	//插入一个子对象，如果子对象已经存在则失败
	bool InserChild(LOLayer *layer);
	bool InserChild(int cid, LOLayer *layer);
	//是否所有的动画都不可用，用于准备删除动画对象
	bool isAllUnable(LOAnimation *ai);

	//坐标是否包含在图层内
	bool isPositionInsideMe(int x, int y);

	bool isVisible();
	bool isChildVisible();
	bool isExbtn() { return curInfo->btnStr; }
	bool isMaxBorder(int index,int val);

	//显示指定格树的NS动画
	void ShowNSanima(int cell);
	void ShowBtnMe();
	void HideBtnMe();

	//查找子对象
	LOLayer *FindChild(const int *cids);
	LOLayer *FindChild(int cid);

	//从子对象队列中移除，同时返回图层（如果有的话），自行决定是否delete
	LOLayer* RemodeChild(int *cids);
	LOLayer* RemodeChild(int cid);
	int GetFullid() { return GetFullID(layerType, &id[0]); }

	void GetShowSrc(SDL_Rect *srcR);

	//获取本层中子图层的使用情况
	void GetLayerUsedState(char *bin, int *ids);

	void ShowMe(SDL_Renderer *render);

	void FreeData();   //释放自身持有的子对象、info对象
	void DoAnimation(LOLayerInfo* info, Uint32 curTime);
	void DoTextAnima(LOLayerInfo *info, LOAnimationText *ai, Uint32 curTime);
	void DoMoveAnima(LOLayerInfo *info, LOAnimationMove *ai, Uint32 curTime);
	void DoScaleAnima(LOLayerInfo *info, LOAnimationScale *ai, Uint32 curTime);
	void DoRotateAnima(LOLayerInfo *info, LOAnimationRotate *ai, Uint32 curTime);
	void DoFadeAnima(LOLayerInfo *info, LOAnimationFade *ai, Uint32 curTime);
	void DoNsAnima(LOLayerInfo *info, LOAnimationNS *ai, Uint32 curTime);
	void SetFullID(int fullid);
	bool GetTextEndPosition(int *xx, int *yy, int *lineH);
	void GetLayerPosition(int *xx, int *yy, int *aph);
	void Serialize(BinArray *sbin);

	//static void GetTypeAndIds(SysLayerType *type,int *ids,int fullid);
	//static int GetFullid(SysLayerType type,int *ids);
	//static int GetFullid(SysLayerType type, int father,int child,int grandson);
	//static int GetIDs(int fullid, int pos);
	//static bool CopySurfaceToTexture(SDL_Surface *su, SDL_Rect *src ,SDL_Texture *tex, SDL_Rect *dst);
private:
	LOMatrix2d GetTranzMatrix() ; //获取图层当前对应的变换矩阵

	void GetInheritScale(double *sx, double *sy);
	void GetInheritOffset(float *ox, float *oy);
	bool isFaterCopyEx();
	LOLayer* DescentFather(LOLayer *father, int *index,const int *ids);
};
//}


#endif // !_LOLAYER_H_
