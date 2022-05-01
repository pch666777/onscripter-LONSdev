/*
//文字描述
*/

#ifndef __LOTEXTDESCRIBE_H_
#define __LOTEXTDESCRIBE_H_

#include <SDL.h>
#include <vector>
#include <stdint.h>
#include "../etc/SDL_mem.h"
#include "../etc/LOString.h"

//单个文字
struct LOWordElement{
	Uint16 unicode = 0;
	int16_t minx = 0;
	int16_t miny = 0;
	int16_t maxx = 0;
	int16_t maxy = 0;
	int16_t advance = 0;
	int16_t x = 0;
	int16_t y = 0;
	//是否半角字符
	bool isEng = true;
	SDL_Surface *surface = nullptr;
	~LOWordElement() {
		if (surface) FreeSurface(surface);
	}
};


class LOTextDescribe
{
public:
	LOTextDescribe();
	~LOTextDescribe();

	//释放字列表
	void ReleaseWords(bool ischild);
	void ReleaseSurface(bool ischild);
	void FreeChildSurface();
	//历遍对象，获取尺寸，注意设置初始值
	void GetSizeRect(int *left, int *top, int *right, int *bottom);
	//位置从0,0开始
	void SetPositionZero();
	void MovePosition(int mx, int my);

	//父对象，子对象的位置是相对于父对象的
	LOTextDescribe *parent;
	//子对象
	std::vector<LOTextDescribe*> *childs;
	//字列表
	std::vector<LOWordElement*> *words;
	//纹理
	SDL_Surface *surface;

	int x;
	int y;
private:

};

#endif // !__LOTEXTDESCRIBE_H_
