/*
//字体及设置
*/

#ifndef __LOFONT_H_
#define __LOFONT_H_

#include <SDL.h>
#include <SDL_ttf.h>
#include <vector>
#include <map>
#include <memory>
#include "../etc/LOString.h"
#include "../etc/SDL_mem.h"
#include "../etc/BinArray.h"

class LOFont {
public:
	struct FontWord{
		int16_t size = 0;
		int16_t ascent;
		int16_t descent;
		TTF_Font *font = nullptr;
		void Close() {
			if (font) TTF_CloseFont(font);
			font = nullptr;
		}
	};

	struct MemoryTTF{
		BinArray *data = nullptr;
		SDL_RWops *rwops = nullptr;
		~MemoryTTF() {
			if (rwops) SDL_RWclose(rwops);
			if(data) delete data;
		}
	};

	LOFont();
	~LOFont();

	LOString name;
	LOString path;

	//BinArray转移给类
	bool isValid() { return path.length() > 0 || memTTF; }
	bool SetBinData(BinArray *&bin);
	bool SetName(LOString *fn);
	void CloseAll();
	FontWord* GetFont(int size);

	//创建一个字体，如果之前已经创建过了，那么取缓存。fontName为空则创建内置字体
	static LOFont* CreateFont(LOString &fontName);
	static std::map<std::string, std::shared_ptr<LOFont>> fontMap;
	static void FreeAllFont();
private:
	//内存字体可能会被多次使用，比如内建的字体
	std::unique_ptr<MemoryTTF> memTTF;
	std::vector<FontWord*> fontList;

	static void InitBuiltInFont();
};

typedef std::shared_ptr<LOFont> LOShareFont;

#endif // !__LOFONT_H_
