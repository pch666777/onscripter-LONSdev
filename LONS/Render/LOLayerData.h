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

//基本类型都放在一起，拷贝方便
class LOLayerDataBase {
public:
	LOLayerDataBase();
	void resetMe();

	int fullid;    //图层号，父id 10位（0-1023），子id 8位（0-254），孙子id 8位(0-254)
	int flags;     //图层标记，比如是否使用缓存，是否有遮片，是否显示
	int btnval;    //btn的值
	int16_t offsetX;    //显示目标左上角位置
	int16_t offsetY;    //显示目标左上角位置
	int16_t alpha;      //透明度
	int16_t centerX;    //锚点，缩放旋转时有用
	int16_t centerY;

	int16_t showSrcX;      //显示区域的X偏移
	int16_t showSrcY;     //显示区域的Y偏移
	uint16_t showWidth;   //显示区域的宽度
	uint16_t showHeight;  //显示区域的高度

	uint8_t cellNum;      //第几格动画
	uint8_t alphaMode;
	uint16_t tWidth;     //材质的宽度
	uint16_t tHeight;	//材质的高度

	double scaleX;   //X方向的缩放
	double scaleY;	//y方向的缩放
	double rotate;	//旋转，角度

	uint8_t texType;  //data是哪一种类型的
protected:
	uint8_t showType;
};

class LOLayerData :public LOLayerDataBase{
public:
	LOLayerData();
	LOLayerData(const LOLayerData &obj);
	~LOLayerData();

	//禁用赋值
	const LOLayerData& operator=(const LOLayerData &obj) = delete;

	//拷贝构造

	//显示模式
	enum {
		SHOW_NORMAL = 1,
		SHOW_RECT = 2,
		SHOW_SCALE = 4,
		SHOW_ROTATE = 8
	};

	enum {
		FLAGS_VISIABLE = 1,
		FLAGS_CHILDVISIABLE = 2,
		FLAGS_USECACHE = 4 ,
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
	};


	//透明模式
	enum {
		//TRANS_ALPHA = 'a',
		//TRANS_TOPLEFT = 'l',
		//TRANS_COPY = 'c',
		//TRANS_STRING = 's',
		//TRANS_DIRECT = '#',
		//TRANS_PALLETTE = '!',
		//TRANS_TOPRIGHT = 'r',
		//TRANS_MASK = 'm'
		TRANS_ALPHA,
		TRANS_TOPLEFT,
		TRANS_COPY,
		TRANS_STRING,
		TRANS_DIRECT,
		TRANS_PALLETTE,
		TRANS_TOPRIGHT,
		TRANS_MASK
	};

	/*
	LOString *fileName;  //透明样式;文件名
	LOString *maskName;
	LOtexture *texture;
	LOStack<LOAnimation> *actions;
	*/
	//文本直接复制一份即可，无需引用计数
	//文件名或者显示的文字或者执行的命令
	std::unique_ptr<LOString> fileTextName;
	//遮片名称
	std::unique_ptr<LOString> maskName;
	//按钮字符串
	std::unique_ptr<LOString> btnStr;

	//纹理使用引用计数
	LOShareTexture texture;

	//每一个动作、事件钩子我们都不确定什么时候删除，所以需要一个引用计数
	//所有的action和eventHook都只有在print时才同步
	//动作组
	std::unique_ptr<std::vector<LOShareAction>> actions; 

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
	void GetSimpleDst(SDL_Rect *dst);
	void GetSimpleSrc(SDL_Rect *src);
	int GetCellCount();
	void SetVisable(int v);
	void SetShowRect(int x, int y, int w, int h);
	void SetShowType(int show);
	void SetPosition(int ofx, int ofy);
	void SetPosition2(int cx, int cy, double sx, double sy);
	void SetRotate(double ro);
	void SetAlpha(int alp);
	bool SetCell(LOActionNS *ac, int ce);
	void SetTextureType(int dt);
	void SetNewFile(LOShareBaseTexture &base);
	void SetDelete();
	void SetBtndef(LOString *s, int val);
	void unSetBtndef();

	//将动画的初始信息同步到layerinfo上
	void FirstSNC();
	//void upData(LOLayerData *data);
	//void upDataEx(LOLayerData *data);

	//添加敏感类事件时删除
	void SetAction(LOShareAction &ac);
	void SetAction(LOAction *ac);
	LOAction *GetAction(LOAction::AnimaType acType);
private:
	bool isinit;
	//void lockMe();
	//void unLockMe();
	//void cpuDelay();

	//插入移除 actions 和 eventHooks要特别注意
	//图层有一个变量确定是否有 action 和 eventHook失效事件，有的话锁定，删除
	//std::atomic_int threadLock;
};

//typedef std::shared_ptr<LOLayerData> LOShareLayerData;

#endif // !__H_LOLAYERDATA_
