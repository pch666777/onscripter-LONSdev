/*
//存放图层的基本信息
//图层信息分为前台和后台，直接通过share_ptr管理
*/

#ifndef __H_LOLAYERDATA_
#define __H_LOLAYERDATA_

#include <stdint.h>
#include <memory>
#include <atomic>
#include "../etc/LOString.h"
#include "LOTexture.h"
#include "LOAction.h"
#include "LOMatrix2d.h"

extern char G_Bit[4];             //默认材质所只用的RGBA模式位置 0-A 1-R 2-G 3-B
extern Uint32 G_Texture_format;   //默认编辑使用的材质格式
extern SDL_Rect G_windowRect;    //整个窗口的位置和大小
extern SDL_Rect G_viewRect;     //实际显示视口的位置和大小
extern int G_gameWidth;         //游戏的实际宽度
extern int G_gameHeight;         //游戏的实际高度
extern float G_gameScaleX;     //游戏X方向的缩放
extern float G_gameScaleY;     //游戏Y方向的缩放，鼠标事件时需要使用
extern int G_gameRatioW;       //游戏比例，默认为1:1
extern int G_gameRatioH;       //游戏比例，默认为1:1
extern int G_fullScreen;       //全屏
extern int G_destWidth;   //目标窗口宽度
extern int G_destHeight;  //目标窗口高度
extern int G_fpsnum;
extern int G_textspeed;
extern bool IsGameScale();
extern int GetFullID(int t, int *ids);
extern int GetFullID(int t, int father, int child, int grandson);

#define IDS_LAYER_TYPE 3
#define IDS_LAYER_NORMAL 0
#define IDS_LAYER_CHILD 1
#define IDS_LAYER_GRANDSON 2
extern int GetIDs(int fullid, int pos);
extern void GetTypeAndIds(int *type, int *ids, int fullid);

class LOLayerData;
class LOLayerRef;

//==============================图层的基本属性====================
//只有具体显示时我们才需要图层关系，因此需要独立出来
class LOLayerRef{
public:
	LOLayerRef();
	~LOLayerRef();
	enum {
		FLAGS_MUST_INIT = 1,
		FLAGS_MUST_SORT = 2,
	};

	void RemoveChild(int fid);
	void reset();
	void LinkToRoot();

	int fullID;
	//父对象
	LOLayerRef *parent;
	//绑定的数据
	LOLayerData *data;
	//子对象，单项链表，从大到小排列
	std::map<int, LOLayerRef*> *childs;

	//变换矩阵
	LOMatrix2d matrix;
private:
	int flags;
};



//===============================================================
//基本类型都放在一起，拷贝方便
class LOLayerData {
public:
	LOLayerData();
	~LOLayerData();
	void reset();
	LOLayerData* reset(int fid);
	LOAction* GetAction(int t);
	int GetCellCount();
	void Serialize(BinArray *bin);
	LOLayerData* RegisterMe(int fid);

	//属性设置的目标应该是明确的
	bool isShowScale() { return showType & SHOW_SCALE; }
	bool isShowRotate() { return showType & SHOW_ROTATE; }
	bool isShowRect() { return showType & SHOW_RECT; }
	bool isVisiable() { return flags & FLAGS_VISIABLE; }
	bool isChildVisiable() { return flags & FLAGS_CHILDVISIABLE; }
	bool isCache() { return flags & FLAGS_USECACHE; }
	bool isDelete() { return flags & FLAGS_DELETE; }
	bool isNewFile() { return flags & FLAGS_NEWFILE; }
	bool isUpData() { return flags & FLAGS_UPDATA; }
	bool isUpDataEx() { return flags & FLAGS_UPDATAEX; }
	bool isBtndef() { return flags & FLAGS_BTNDEF; }
	bool isActive() { return flags & FLAGS_ACTIVE; }
	bool isForce() { return flags & FLAGS_ISFORCE; }
	bool isFloatMode() { return flags & FLAGS_FLOATMODE; }
	bool isNothing() { return flags == 0 && upflags == 0; }

	void SetAction(LOAction *ac);
	void SetAction(LOShareAction &ac);
	void SetVisable(int v);
	void SetShowRect(int x, int y, int w, int h);
	void SetShowType(int show);
	void SetPosition(int ofx, int ofy);
	void SetPosition2(int cx, int cy, double sx, double sy);
	void SetRotate(double ro);
	void SetAlpha(int alp);
	void SetAlphaMode(int mode);
	bool SetCell(LOActionNS *ac, int ce);
	void SetCellNum(int ce);
	void SetTextureType(int dt);
	void SetNewFile(LOShareTexture &tex);
	void SetDelete();
	void SetBtndef(LOString *s, int val, bool isleft, bool isright);
	void unSetBtndef();
	void FirstSNC();
	void GetSimpleSrc(SDL_Rect *src);
	void GetSimpleDst(SDL_Rect *dst);

	enum {
		UP_BTNVAL = 1,
		UP_OFFX = 2,
		UP_OFFY = 4,
		UP_ALPHA = 8,
		UP_CENX = 16,
		UP_CENY = 32,
		UP_SRCX = 64,
		UP_SRCY = 128,
		UP_SHOWW = 256,
		UP_SHOWH = 512,
		UP_CELLNUM = 1024,
		UP_ALPHAMODE = 2048,
		UP_SCALEX = 0x1000,
		UP_SCALEY = 0x2000,
		UP_ROTATE = 0x4000,
		UP_BTNSTR = 0x8000,
		//跟新文件必定挂钩的 keystr buildstr texture  textype
		//UP_NEWFILE = 0x10000,
		//UP_ACTIONS = 0x20000,
		UP_SHOWTYPE = 0x40000,
		UP_VISIABLE = 0x80000,

		UP_NEWFILE = -1,

		//更新了哪一个action，使用LOAction的类型标识
		UP_ACTION_ALL = -1,
	};

	enum {
		FLAGS_VISIABLE = 1,
		FLAGS_CHILDVISIABLE = 2,
		FLAGS_USECACHE = 4,
		FLAGS_DELETE = 8,
		//只更新了位置、缩放等简单数值
		FLAGS_UPDATA = 16,
		FLAGS_NEWFILE = 32,
		//更新了事件钩子和动作
		FLAGS_UPDATAEX = 64,
		//该图层被设置btn
		FLAGS_BTNDEF = 128,
		//图层正处于鼠标悬停激活状态
		FLAGS_ACTIVE = 256,
		//响应左键
		FLAGS_LEFTCLICK = 0x200,
		//响应右键
		FLAGS_RIGHTCLICK = 0x400,
		//响应长按
		FLAGS_LONGCLICK = 0x800,
		//响应悬停
		FLAGS_MOUSEMOVE = 0x1000,
		//释放时，只有前台和后台同时处于不适用状态才会删除
		//是否处于前台
		FLAGS_ISFORCE = 0x2000,
		//是否处于浮点模式
		FLAGS_FLOATMODE = 0x4000,

		//该layerData已经被使用
		FLAGS_HASUSED = 0x40000000,
	};

	//显示模式
	enum {
		SHOW_NORMAL = 1,
		SHOW_RECT = 2,
		SHOW_SCALE = 4,
		SHOW_ROTATE = 8
	};

	int flags;
	int upflags;    //更新了那些值
	int upaction;
	int btnval;    //btn的值
	int fullID;
	float offsetX;    //显示目标左上角位置
	float offsetY;    //显示目标左上角位置
	int16_t alpha;      //透明度
	int16_t centerX;    //锚点，缩放旋转时有用
	int16_t centerY;

	int16_t showSrcX;      //显示区域的X偏移
	int16_t showSrcY;     //显示区域的Y偏移
	float showWidth;   //显示区域的宽度
	float showHeight;  //显示区域的高度

	uint8_t cellNum;      //第几格动画
	uint8_t alphaMode;
	//data是哪一种类型的
	uint8_t texType;
	//uint8_t visiable;   //是否可见

	double scaleX;   //X方向的缩放
	double scaleY;	//y方向的缩放
	double rotate;	//旋转，角度

	//复用式纹理的索引key
	std::unique_ptr<LOString> keyStr;
	//按钮字符串
	std::unique_ptr<LOString> btnStr;
	//loadsp时的字符串，保证读档时可以完整重新
	std::unique_ptr<LOString> buildStr;
	//纹理使用引用计数
	LOShareTexture texture;
	//每一个动作我们都不确定什么时候删除，所以需要一个引用计数
	std::unique_ptr<std::vector<LOShareAction>> actions;

	uint8_t showType;

	void *layerRef;
	LOLayerData *bakData;
};





/*
//数据由前台数据和后台数据组成，set时总是设置后台数据, get时则根据upflags确定返回前台还是后台数据
class LOLayerData{
public:
	LOLayerData();
	~LOLayerData();
	//禁用赋值
	const LOLayerData& operator=(const LOLayerData &obj) = delete;

	//透明模式
	enum {
		TRANS_ALPHA = 'a',
		TRANS_TOPLEFT = 'l',
		TRANS_COPY = 'c',
		TRANS_STRING = 's',
		TRANS_DIRECT = '#',
		TRANS_PALLETTE = '!',
		TRANS_TOPRIGHT = 'r',
		TRANS_MASK = 'm'
		//TRANS_ALPHA,
		//TRANS_TOPLEFT,
		//TRANS_COPY,
		//TRANS_STRING,
		//TRANS_DIRECT,
		//TRANS_PALLETTE,
		//TRANS_TOPRIGHT,
		//TRANS_MASK
	};

	int fullid;
	LOLayerDataBase *cur;
	LOLayerDataBase *bak;

	//get类函数都会检查后台，如果后台有更新则返回后台数据
	//void GetSimpleDst(SDL_Rect *dst);
	//void GetSimpleSrc(SDL_Rect *src);
	bool GetVisiable();
	int GetOffsetX();
	int GetOffsetY();
	int GetCenterX();
	int GetCenterY();
	int GetAlpha();
	int GetShowWidth();
	int GetShowHeight();
	int GetCellCount();
	double GetScaleX();
	double GetScaleY();
	double GetRotate();

	void SetDefaultShowSize(bool isforce);
	
	inline LOLayerDataBase *GetBase(bool isforce);

	//将动画的初始信息同步到layerinfo上
	void UpdataToForce();
	void ReleaseForce();
	void ReleaseBak();
	//总是新建
	void NewBak();
	//空的才新建
	void EmptyNewBak();

	void Serialize(BinArray *bin);
private:
	bool isinit;
};
*/

//环形缓存结构
class LOLayerDataManager {
public:
	LOLayerDataManager();
	~LOLayerDataManager();

	//空白的，DataBase无效后均指向它
	LOLayerDataBase *emptyData;

	void GetDataBase(LOLayerDataBase *&base);
	void GetEmpty(LOLayerDataBase *&base);
private:
	void AddDataList(int size);
	//当前循环的位置
	int current;
	//指针
	std::vector<LOLayerDataBase*> DataList;
};

extern LOLayerDataManager LOLayerDataCenter;

#endif // !__H_LOLAYERDATA_
