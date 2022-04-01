/*
*/
#pragma once
#ifndef _LOLAYER_H_
#define _LOLAYER_H_

#include "../Scripter/FuncInterface.h"
#include "LOFontBase.h"
#include "../etc/LOStack.h"
#include "LOMatrix2d.h"
#include "LOLayerData.h"

#include <SDL.h>
#include <vector>
#include <map>
#include <unordered_map>
#include <memory>

	////////////////////LOLayer/////////////////////////
	/*
	//新版的图层采用了前台和后台双份操作，action引起的参数变化不会同步到后台，除非显示调用同步命令。
	//感觉这一版的比较合理了
	*/

class LOLayer {
public:
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
	int id[3];
	std::unique_ptr<LOLayerData> curInfo;  //前台数据
	SysLayerType layerType;     //图层所在的组，在new图层时已经把LAYER_SPRINT转换
	//父对象
	LOLayer *parent;
	//根图层的，根图层总是有效的
	LOLayer *rootLyr;

	std::map<int, LOLayer*> *childs;

	//顶级图层不需要后台数据
	LOLayer();
	LOLayer(SysLayerType lyrType);

	//普通图层显然是根据后台数据创建,islink决定是否挂载都图层结构组上
	LOLayer(LOLayerData &data, bool islink);
	~LOLayer();

	LOMatrix2d matrix;  //变换矩阵

	//插入一个子对象，如果子对象已经存在则失败
	bool InserChild(LOLayer *layer);
	bool InserChild(int cid, LOLayer *layer);

	//坐标是否包含在图层内
	bool isPositionInsideMe(int x, int y);

	bool isVisible();
	bool isChildVisible();
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

	void GetShowSrc(SDL_Rect *srcR);

	//获取本层中子图层的使用情况
	void GetLayerUsedState(char *bin, int *ids);

	void ShowMe(SDL_Renderer *render);
	//void DoAnimation(LOLayerInfo* info, Uint32 curTime);
	//void DoTextAnima(LOLayerInfo *info, LOAnimationText *ai, Uint32 curTime);
	//void DoMoveAnima(LOLayerInfo *info, LOAnimationMove *ai, Uint32 curTime);
	//void DoScaleAnima(LOLayerInfo *info, LOAnimationScale *ai, Uint32 curTime);
	//void DoRotateAnima(LOLayerInfo *info, LOAnimationRotate *ai, Uint32 curTime);
	//void DoFadeAnima(LOLayerInfo *info, LOAnimationFade *ai, Uint32 curTime);
	//void DoNsAnima(LOLayerInfo *info, LOAnimationNS *ai, Uint32 curTime);
	bool GetTextEndPosition(int *xx, int *yy, int *lineH);
	void GetLayerPosition(int *xx, int *yy, int *aph);
	void Serialize(BinArray *sbin);

	//将图层挂载到图层结构上
	bool LinkLayer();
	void upData(LOLayerData *data);
	void upDataEx(LOLayerData *data);

	//获得预期的父对象，注意并不是真的已经挂载到父对象上
	//只是根据ids预期父对象，识别返回null
	//static LOLayer* GetExpectFather(int lyrType, int *ids);
	static LOLayer* FindViewLayer(int fullid);
private:
	LOMatrix2d GetTranzMatrix() ; //获取图层当前对应的变换矩阵

	void GetInheritScale(double *sx, double *sy);
	void GetInheritOffset(float *ox, float *oy);
	bool isFaterCopyEx();
	LOLayer* DescentFather(LOLayer *father, int *index,const int *ids);
	void BaseNew(SysLayerType lyrType);
};

//根层直接定义
extern LOLayer G_baseLayer[LOLayer::LAYER_BASE_COUNT];

//每个层级的最大层数量
extern int G_maxLayerCount[3];


#endif // !_LOLAYER_H_
