/*
动画类型有Base、Text、Fade、Move、Rotate
*/

#pragma once
#include "../etc/BinArray.h"
#include "../etc/LOStack.h"
#include "LOFontBase.h"
#include "LOSurface.h"
#include <string>
#include <vector>
#include <SDL.h>



//namespace LONS {
//全放到一个类里
	class LOAnimation{
	public:
		//循环模式
		enum {
			LOOP_CIRCULAR,  //从头到尾再从头到尾
			LOOP_ONSCE,   //从头到尾执行一次
			LOOP_GOBACK,    //从头到尾再到头一直循环
			LOOP_PTR,     //显示某一格或帧，然后停止
			LOOP_NONE,   //不生效，表示动作不会执行
		};

		enum AnimaType {   //不要修改顺序
			ANIM_NONE = 0,
			ANIM_TEXT = 1,
			ANIM_NSANIM = 2,  //传统的NS动画模式
			ANIM_FADE = 4,
			ANIM_MOVE = 8,
			ANIM_ROTATE = 16,
			ANIM_SCALE = 32,
		};

		enum {
			CON_NONE = 0,
			CON_APPEND = 1,   //添加到动作尾部
			CON_NEW = 2,     //删除现有动作重新添加
			CON_DELETE = 4,  //删除自身
			CON_REPLACE = 8, //替换跟自身同类型的动作
			CON_CLEAR = 16,  //清除列表中的所有动作

		};

		AnimaType type;
		int control;         //控制类别
		int loopMode;        //循环模式，根据具体的类别有不同的意义
		int gval;            //加速度
		bool finishFree;     //执行完成后是否释放
		bool finish;       //是否已经执行完成
		Uint32 lastTime;  //上次运行的时间
		bool   isEnble;   //是否可用

		LOAnimation();
		virtual ~LOAnimation();
		
		//添加到列表，有追加，替换，加入，动作列表三种模式
		void AddTo(LOStack<LOAnimation> **list, int addtype);
		virtual void Serialize(BinArray *sbin);

		static void FreeAnimaList(LOStack<LOAnimation> **list);
	private:

	};

	//动画组元素
	struct LOAniListElement
	{
		int fullid;			//对应的图层
		LOAnimation *aib;  //动画对象
	}; 


	class LOAnimationNS :public LOAnimation {
	public:
		int alphaMode = 0;   //透明模式
		int cellCount = 1;	 //动画分格总数
		int cellForward = 1; //前进方向
		int cellCurrent = 0; //动画当前的位于的格数
		//int loopMode = 0;   //循环方式，0：从头开始循环 1：从头到尾，且只运行一次  2：从头到尾又到头 3：只分格，不运行
		SDL_Surface *bigsu = nullptr;  //超大图的曲面
		int *perTime = nullptr;  //每一格需要的时间，数组

		LOAnimationNS();
		~LOAnimationNS();
		void Serialize(BinArray *sbin);
	};

	//struct LineInfo{
	//	int x;
	//	int y;
	//	int height;
	//	int sumx;    //x终点
	//};

	class LOAnimationText :public LOAnimation {
	public:
		LOAnimationText();
		~LOAnimationText();
		void Serialize(BinArray *sbin);

		int currentPos;  //动画当前运行的位置
		int currentLine; //当前的行
		int loopdelay;   //循环延时，-1表示不循环
		bool isadd = false;
		double perpix;       //每毫秒显示的像素数
		//LOSurface *su;
		LOStack<LineComposition> *lineInfo;  //行信息
	};

	class LOAnimationMove :public LOAnimation {
	public:
		LOAnimationMove();
		void Serialize(BinArray *sbin);

		int fromx;
		int fromy;
		int destx;
		int desty;
		double timer;
	};


	class LOAnimationScale :public LOAnimation {
	public:
		LOAnimationScale();
		void Serialize(BinArray *sbin);
		int centerX;    //中心点，负数表示图像的中心
		int centerY;    //中心点
		double startPerX;   //起点的x比例
		double startPerY;   //起点的y比例
		double endPerX;    //终点的X比例
		double endPerY;    //终点的Y比例
		double timer;
	};

	class LOAnimationRotate :public LOAnimation{
	public:
		LOAnimationRotate();
		void Serialize(BinArray *sbin);
		int centerX;    //中心点，负数表示图像的中心
		int centerY;    //中心点
		int startRo;
		int endRo;
		double timer;
	};

	class LOAnimationFade :public LOAnimation {
	public:
		LOAnimationFade();
		void Serialize(BinArray *sbin);
		int startAlpha;
		int endAlpha;
		double timer;
	};

