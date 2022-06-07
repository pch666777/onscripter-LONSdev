/*
//字体及设置
*/

#include "LOFont.h"
#include "../etc/LOIO.h"

std::map<std::string, std::shared_ptr<LOFont>> LOFont::fontMap;


LOFont::LOFont() {
	memTTF = nullptr;
}


LOFont::~LOFont() {
	for (auto iter = fontList.begin(); iter != fontList.end(); iter++) {
		(*iter)->Close();
		delete *iter;
	}	
}


void LOFont::CloseAll() {
	for (auto iter = fontList.begin(); iter != fontList.end(); iter++) {
		(*iter)->Close();
	}
}

bool LOFont::SetBinData(BinArray *&bin) {
	CloseAll();
	memTTF.reset(new MemoryTTF());
	//数据转移
	memTTF->data = bin;
	bin = nullptr;

	memTTF->rwops = SDL_RWFromMem(memTTF->data->bin, memTTF->data->Length());
	return memTTF->rwops;
}


bool LOFont::SetName(LOString *fn) {
	CloseAll();
	memTTF.reset();

	name = *fn;
	path = *fn;
	LOIO::GetPathForRead(path);
	path = path.PathTypeTo(LOString::PATH_LINUX);

	FILE *f = fopen(path.c_str(), "rb");
	if (!f) {
		//路径无效，尝试读取封包内的字体
		path.clear();
		BinArray *ttf_data = LonsReadFileFromPack(*fn);
		if (ttf_data) {
			SetBinData(ttf_data);
			return true;
		}
		else return false;
	}
	fclose(f);
	return true;
}


LOFont::FontWord* LOFont::GetFont(int size) {
	FontWord *fw = nullptr;
	for (auto iter = fontList.begin(); iter != fontList.end() && !fw; iter++) {
		if ((*iter)->size == size) fw = (*iter);
	}
	//没有添加一个
	if (!fw) {
		fw = new FontWord();
		fw->size = size;
		fontList.push_back(fw);
	}
	//检查是否已经打开
	if (!fw->font) {
		if (memTTF) {
			//设置流的读写位置，这是个坑
			SDL_RWseek(memTTF->rwops, 0, RW_SEEK_SET);
			fw->font = TTF_OpenFontRW(memTTF->rwops, 0, fw->size);
		}
		else if (path.length() > 0) fw->font = TTF_OpenFont(path.c_str(), fw->size);
		if (fw->font) {
			fw->descent = TTF_FontDescent(fw->font);
			fw->ascent = TTF_FontAscent(fw->font);
		}
	}
	return fw;
}


LOFont* LOFont::CreateFont(LOString &fontName) {
	auto iter = fontMap.find(fontName);
	if (iter != fontMap.end()) return iter->second.get();

	LOShareFont font(new LOFont());
	if (font->SetName(&fontName)) {
		fontMap[fontName] = font;
		return font.get();
	}
	else {
		iter = fontMap.find("_Built_in_font_");
		//初始化内置字体
		if (iter == fontMap.end()) {
			InitBuiltInFont();
			iter = fontMap.find("_Built_in_font_");
		}

		fontMap[fontName] = iter->second;
		return iter->second.get();
	}
}

//初始化内建字体
void LOFont::InitBuiltInFont() {
	//0对应BUILT_FONT
	BinArray *data = LonsGetBuiltMem(0);
	if (!data) {
		fontMap["_Built_in_font_"] = nullptr;
	}
	else {
		LOShareFont font(new LOFont);
		font->name = "_Built_in_font_";
		font->SetBinData(data);
		fontMap["_Built_in_font_"] = font;
	}
}


//释放所有map中的字体
void LOFont::FreeAllFont() {
	fontMap.clear();
}