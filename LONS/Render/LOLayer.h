/*
*/
#pragma once
#ifndef _LOLAYER_H_
#define _LOLAYER_H_

#include "../Scripter/FuncInterface.h"
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
		//在LAYER_DIALOG层
		IDEX_DIALOG_TEXT = 5,    //在LAYER_DIALOG层
		IDEX_DIALOG_WINDOW = 50, //在LAYER_DIALOG层
		IDEX_DIALOG_TEXT_CHILD = 100,  //多句标准对话时，挂在在text的子层
		//在LAYER_NSSYS层
		IDEX_NSSYS_EFFECT = 4, //effect 2-18使用的层，在LAYER_NSSYS层
		//5也被IDEX_NSSYS_EFFECT使用
		IDEX_NSSYS_MOVIE = 6,  //avi,mpegplay在此层
		IDEX_NSSYS_BTN = 10,    //btn定义的按钮，在LAYER_NSSYS层
		IDEX_NSSYS_CUR = 11,    //Cursor在的层，在LAYER_NSSYS层

		IDEX_NSSYS_RMENU = 20,
		IDEX_BG_BTNEND = 1022,

		IDEX_LD_LEFT = 13,
		IDEX_LD_CENTER = 12,
		IDEX_LD_RIGHT = 11,
	};

	enum {
		//传递的事件没有被处理
		SENDRET_NONE,
		//传递的事件已经被相应，但需要被继续传递
		SENDRET_CHANGE,
		//传递的事件已经结束
		SENDRET_END
	};

	//成员///////////
	int id[3];
	SysLayerType layerType;     //图层所在的组，在new图层时已经把LAYER_SPRINT转换
	//前台数据
	std::unique_ptr<LOLayerData> data;
	//父对象
	LOLayer *parent;
	//根图层的，根图层总是有效的
	LOLayer *rootLyr;

	//变换矩阵
	LOMatrix2d matrix;

	//是否已经初始化
	bool isinit;
	//是否已经激活悬停


	std::map<int, LOLayer*> *childs;

	//顶级图层不需要后台数据
	LOLayer();
	LOLayer(SysLayerType lyrType);
	LOLayer(int fullid);

	~LOLayer();

	//插入一个子对象，如果子对象已经存在则失败
	//bool InserChild(LOLayer *layer);
	bool InserChild(int cid, LOLayer *layer);
	//void releaseForce();
	//void releaseBack();

	//坐标是否包含在图层内
	bool isPositionInsideMe(int x, int y);

	bool isVisible();
	bool isChildVisible();
	static bool isMaxBorder(int index,int val);

	//显示指定格树的NS动画
	void setBtnShow(bool isshow);
	//设置对象显示在某一格NS动画，并且active纹理
	bool setActiveCell(int cell);

	LOLayer *FindChild(int cid);
	LOLayer* RemodeChild(int cid);

	void GetShowSrc(SDL_Rect *srcR);

	//获取自身相对于parent的ID值
	int GetSelfChildID();

	//获取本层中子图层的使用情况
	void GetLayerUsedState(char *bin, int *ids);

	void ShowMe(SDL_Renderer *render);
	void DoAction(LOLayerData *data, Uint32 curTime);
	void DoNsAction(LOLayerData *data, LOActionNS *ai,  Uint32 curTime);
	void DoTextAction(LOLayerData *data, LOActionText *ai, Uint32 curTime);
	void DoMovieAction(LOLayerData *data, LOActionMovie *ai, Uint32 curTime);
        //bool GetTextEndPosition(int *xx, int *yy, int *lineH);
	void GetLayerPosition(int *xx, int *yy, int *aph);
	//void Serialize(BinArray *bin);
	void SerializeForce(BinArray *bin);
	bool DeSerializeForce(BinArray *bin, int *pos);
	void SerializeBak(BinArray *bin);
	bool DeSerializeBak(BinArray *bin, int *pos);

	//将图层挂载到图层结构上
	//void upDataNewFile();
	void UpDataToForce();

	//切换图层的悬停激活状态
	//bool setActive(bool isactive);

	//返回真表示事件已经被终止
	bool SendEvent(LOEventHook *e, LOEventQue *aswerQue);
	int checkEvent(LOEventHook *e, LOEventQue *aswerQue);

	//获得预期的父对象，注意并不是真的已经挂载到父对象上
	//只是根据ids预期父对象，识别返回null
	//static LOLayer* GetExpectFather(int lyrType, int *ids);
	static void RenderBaseA(SDL_Renderer *render, LOtexture *tex, SDL_Vertex *vec, int vecCount);
	static LOLayer* FindViewLayer(int fullid, bool isRemove);
	static void NoUseLayerForce(LOLayer *lyr);
	static void NoUseLayerForce(int fid);
	static LOLayer* CreateLayer(int fullid);
	static LOLayer* FindLayerInCenter(int fullid);
	static LOLayer* GetLayer(int fullid);
	static LOLayer* LinkLayerLeve(LOLayer *lyr);
	static void ResetLayer();
	static void InitBaseLayer();
	static std::map<int, LOLayer*> layerCenter;
private:
	LOMatrix2d GetTranzMatrix() ; //获取图层当前对应的变换矩阵

	void GetInheritScale(double *sx, double *sy);
	void GetInheritOffset(float *ox, float *oy);
	bool isFaterCopyEx();
	static LOLayer* DescentFather(LOLayer *father, int *index,const int *ids);
	void BaseNew(SysLayerType lyrType);
};

//根层直接定义
extern LOLayer *G_baseLayer[LOLayer::LAYER_BASE_COUNT];

//每个层级的最大层数量
extern int G_maxLayerCount[3];


#endif // !_LOLAYER_H_
