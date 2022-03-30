#ifndef __LOLAYERINFO_H__
#define __LOLAYERINFO_H__

#include <SDL.h>
#include <unordered_map>
#include <atomic>
#include "../etc/BinArray.h"
#include "../etc/LOString.h"
#include "LOTexture.h"
#include "LOAnimation.h"

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

extern void GetFormatBit(SDL_PixelFormat *format, char *bit);
extern void GetFormatBit(Uint32 format, char *bit);
extern bool IsGameScale();
extern int GetFullID(int t, int *ids);
extern int GetFullID(int t, int father, int child, int grandson);

#define IDS_LAYER_TYPE 3
#define IDS_LAYER_NORMAL 0
#define IDS_LAYER_CHILD 1
#define IDS_LAYER_GRANDSON 2
extern int GetIDs(int fullid, int pos);
extern void GetTypeAndIds(int *type, int *ids, int fullid);


class LOLayerInfo
{
public:

	//更新数据的类型
	enum CONTROLTYPE {
		CON_NONE = 0,
		CON_DELLAYER = 1,
		CON_UPFILE = 2,
		CON_UPPOS = 4,
		CON_UPPOS2 = 8,
		CON_UPVISIABLE = 16,
		CON_UPAPLHA = 32,
		CON_UPSHOWRECT = 64,
		CON_UPSHOWTYPE = 128 ,
		CON_UPBTN = 256,   //更新按钮定义
		CON_UPCELL = 512,
		//CON_DELBTN = 512,
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

	//显示模式
	enum {
		SHOW_NORMAL = 1,
		SHOW_RECT = 2,
		SHOW_SCALE = 4,
		SHOW_ROTATE = 8
	};

	//enum {
	//	LOAD_NORMAL_NONE,
	//	LOAD_NORMAL_IMG,
	//	LOAD_NORMAL_COLOR,
	//	LOAD_SIMPLE_STR,
	//	LOAD_MULITY_STR,
	//};

	//系统使用的材质

	//正常的sp相当于 父idxx-子id255，孙子id255

	int fullid;    //类型6位，父id 10位（0-1023），子id 8位（0-254），孙子id 8位(0-254)
	int16_t offsetX;   //显示目标左上角位置
	int16_t offsetY;    //显示目标左上角位置
	int16_t alpha;
	int16_t centerX;    //锚点，缩放旋转时有用
	int16_t centerY;

	int16_t showSrcX;  //显示区域的X偏移
	int16_t showSrcY;  //显示区域的Y偏移
	uint16_t showWidth;  //显示区域的宽度
	uint16_t showHeight;  //显示区域的高度

	uint8_t visiable;
	uint8_t cellNum;    //第几格动画
	uint8_t loadType;
	uint8_t usecache;
	uint8_t alphaMode;
	uint16_t tWidth;     //材质的宽度
	uint16_t tHeight;	//材质的高度
	int btnValue;   //按钮值
	
	double scaleX;   //X方向的缩放
	double scaleY;	//y方向的缩放
	double rotate;	//旋转，角度

	LOString *fileName;  //透明样式;文件名
	LOString *maskName;
	LOString *btnStr;    //设置按钮动作
	LOString *textStr;   //显示文字，全格式
	LOtexture *texture;
	LOStack<LOAnimation> *actions;

	LOLayerInfo();
	~LOLayerInfo();


	//重置参数
	void Reset();
	void ResetOther();

	//根据操作关键字复制
	void CopyConWordFrom(LOLayerInfo* info, int keyword, bool ismove);

	//根据操作关键字复制
	bool CopyActionFrom(LOStack<LOAnimation> *&src, bool ismove);

	void GetSimpleDst(SDL_Rect *dst);
	void GetSimpleSrc(SDL_Rect *src);
	const char* BaseFileName();
	const char* BaseMaskName();

	LOAnimation* GetAnimation(int type);

	void ReplaceAction(LOAnimation *ai);
	void Serialize(BinArray *sbin);
	void GetTextEndPosition(int *xx, int *yy, bool isnewline);
	void GetSize(int &xx, int &yy, int &cell);
	bool isNewLayer();
	bool isShowScale() { return showType & SHOW_SCALE; }
	bool isShowRotate() { return showType & SHOW_ROTATE; }
	bool isShowRect() { return showType & SHOW_RECT; }
	int GetLayerControl() { return layerControl; }

	void SetBtn(LOString *btn_s, int btnval);
	void SetVisable(int v);
	void SetShowRect(int x, int y, int w, int h);
	//void SetShowType(int show,bool isOverWrite);
	void SetShowType(int show);
	void SetLayerDelete();
	void SetNewFile(LOtexture *tx);
	void SetPosition(int ofx, int ofy);
	void SetPosition2(int cx, int cy, double sx, double sy);
	void SetRotate(double ro);
	void SetAlpha(int alp);
	void SetCell(int ce);

	void UnsetBtn();

	int GetCellCount();
	void FirstSNC(LOAnimation *ai);
	void AddAlpha(int val);
private:
	int layerControl;   //图层控制信息
	uint8_t showType;
};

//struct LOLayerInfoLoad{
//	enum LOAD_TYPE{
//		LOAD_NORMAL_IMG,
//		LOAD_NORMAL_COLOR,
//		LOAD_SIMPLE_STR,
//		LOAD_MULITY_STR,
//	};
//
//	LOLayerInfo *info;
//	LOAD_TYPE loadtype;
//	bool useCache;
//};

#endif