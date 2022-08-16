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



//printName结构
class PrintNameMap {
public:
	std::string *mapName = nullptr;
	std::map<int, LOLayerData*> *map = nullptr;
	PrintNameMap(const char *fn) {
		mapName = new std::string(fn);
		map = new std::map<int, LOLayerData*>;
	}
	~PrintNameMap() {
		delete mapName;
		delete map;
	}

	void Serialize(BinArray *bin);
	static PrintNameMap* GetPrintNameMap(const char *printName);
private:
	static std::vector<std::unique_ptr<PrintNameMap>> backDataMaps;
};


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
		IDEX_BG_BTNEND = 1022,
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
	int fullID;
	//前台数据
	LOLayerData *data;
	//父对象
	LOLayer *parent;

	//变换矩阵
	LOMatrix2d matrix;

	//是否已经初始化
	bool isinit;

	std::map<int, LOLayer*> *childs;

	//顶级图层不需要后台数据
	LOLayer();
	~LOLayer();
	void reset();
	bool LinkToTree();
	int id_type() { return (fullID >> 26) & 0x3F; }
	int id_0() { return (fullID >> 16) & 0x3ff; }
	int id_1() { return (fullID >> 8) & 0xff; }
	int id_2() { return fullID & 0xff; }

	//插入一个子对象，如果子对象已经存在则失败
	LOLayer* InserChild(int cid, LOLayer *layer);
	LOLayer* RemoveChild(int cid);

	//坐标是否包含在图层内
	bool isPositionInsideMe(int x, int y);

	bool isVisible();
	bool isChildVisible();
	static bool isMaxBorder(int index,int val);

	//显示指定格树的NS动画
	void setBtnShow(bool isshow);
	//设置对象显示在某一格NS动画，并且active纹理
	bool setActiveCell(int cell);

	void GetShowSrc(SDL_Rect *srcR);

	//清理自身及子层的按钮定义
	void unSetBtndefAll();

	//获取自身相对于parent的ID值
	int GetSelfChildID();

	//获取本层中子图层的使用情况
	void GetLayerUsedState(char *bin, int *ids);

	void ShowMe(SDL_Renderer *render);
	void DoAction(LOLayerData *data, Uint32 curTime);
	void DoNsAction(LOLayerData *data, LOActionNS *ai,  Uint32 curTime);
	void DoTextAction(LOLayerData *data, LOActionText *ai, Uint32 curTime);
	bool GetTextEndPosition(int *xx, int *yy, int *lineH);
	void GetLayerPosition(int *xx, int *yy, int *aph);
	void Serialize(BinArray *bin);

	//将图层挂载到图层结构上
	//void upDataNewFile();
	void UpDataToForce();

	//切换图层的悬停激活状态
	//bool setActive(bool isactive);

	//返回真表示事件已经被终止
	bool SendEvent(LOEventHook *e, LOEventQue *aswerQue);
	int checkEvent(LOEventHook *e, LOEventQue *aswerQue);

	static LOLayer* FindLayerIC(int fullid);
	static LOLayer* FindFaterLayerIC(int fullid);
	static void SaveLayer(BinArray *bin);
	static bool LoadLayer(BinArray *bin, int *pos, LOEvPtrMap *evmap);
	static void ResetLayer();
	static LOLayerData* NewLayerData(int fullid);
	static LOLayer* NewLayer(int fullid);
	static LOLayerData* CreateNewLayerData(int fid, const char *printName);
	static LOLayerData* CreateLayerBakData(int fid, const char *printName);
	static LOLayerData* GetInfoData(int fid);
	//static bool useLayerCenter;
	//static void InitBaseLayer();
private:
	LOMatrix2d GetTranzMatrix() ; //获取图层当前对应的变换矩阵

	void GetInheritScale(double *sx, double *sy);
	void GetInheritOffset(float *ox, float *oy);
	bool isFaterCopyEx();
	//图层中心相关的
	static int layerCurrent;
	static std::vector<LOLayer*> layerList;
	static int AddLayer(int size);

	static int dataCurrent;
	static std::vector<LOLayerData*> dataList;
	static int AddLayerData(int size);
};

//根层直接定义
extern LOLayer *G_baseLayer[LOLayer::LAYER_BASE_COUNT];

//每个层级的最大层数量
extern int G_maxLayerCount[3];


#endif // !_LOLAYER_H_
