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

//基本类型都放在一起，拷贝方便
class LOLayerDataBase {
public:
	LOLayerDataBase();
	int fullid;    //图层号，父id 10位（0-1023），子id 8位（0-254），孙子id 8位(0-254)
	int flags;     //图层标记，比如是否使用缓存，是否有遮片，是否显示
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

protected:
	uint8_t showType;
	uint8_t texType;  //data是哪一种类型的
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

	//纹理使用引用计数
	LOShareTexture texture;

	//每一个动作、事件钩子我们都不确定什么时候删除，所以需要一个引用计数
	//所有的action和eventHook都只有在print时才同步
	//动作组
	std::unique_ptr<std::vector<LOShareAction>> actions; 
	//事件组
	std::unique_ptr<std::vector<LOShareEventHook>> eventHooks;


	bool isShowScale() { return showType & SHOW_SCALE; }
	bool isShowRotate() { return showType & SHOW_ROTATE; }
	bool isShowRect() { return showType & SHOW_RECT; }
	bool isVisiable() { return flags & FLAGS_VISIABLE; }
	bool isChildVisiable() { return flags & FLAGS_CHILDVISIABLE; }
	void GetSimpleDst(SDL_Rect *dst);
	void GetSimpleSrc(SDL_Rect *src);
	void SetVisable(int v);
	void SetShowRect(int x, int y, int w, int h);
	void SetShowType(int show);
	void SetPosition(int ofx, int ofy);
	void SetPosition2(int cx, int cy, double sx, double sy);
	void SetRotate(double ro);
	void SetAlpha(int alp);
	void SetCell(int ce);
	void SetTextureType(int dt);

	//添加敏感类事件时删除
	void SetAction(LOShareAction &ac);
	void SetAction(LOAction *ac);
private:
	//void lockMe();
	//void unLockMe();
	//void cpuDelay();

	//插入移除 actions 和 eventHooks要特别注意
	//图层有一个变量确定是否有 action 和 eventHook失效事件，有的话锁定，删除
	//std::atomic_int threadLock;
};

//typedef std::shared_ptr<LOLayerData> LOShareLayerData;

#endif // !__H_LOLAYERDATA_
