/*
//字体及设置
*/

#include "LOFont.h"
#include "../etc/LOIO.h"


LOFont LOFont::builtInFont;
std::map<std::string, LOFont*> LOFont::fontMap;


LOFont::LOFont() {
	memTTF = nullptr;
}


LOFont::~LOFont() {
	for (auto iter = fontList.begin(); iter != fontList.end(); iter++) {
		(*iter)->Close();
		delete *iter;
	}
	if (memTTF) delete memTTF;
}


void LOFont::CloseAll() {
	for (auto iter = fontList.begin(); iter != fontList.end(); iter++) {
		(*iter)->Close();
	}
}

bool LOFont::SetBinData(BinArray *&bin) {
	CloseAll();
	if (memTTF) {
		delete memTTF;
		memTTF = nullptr;
	}

	memTTF = new MemoryTTF();
	//数据转移
	memTTF->data = bin;
	bin = nullptr;

	memTTF->rwops = SDL_RWFromMem(memTTF->data->bin, memTTF->data->Length());
	return memTTF->rwops;
}


bool LOFont::SetName(LOString *fn) {
	CloseAll();
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


TTF_Font* LOFont::GetFont(int size) {
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
		if(memTTF) fw->font = TTF_OpenFontRW(memTTF->rwops, 0, fw->size);
		else if (path.length() > 0) fw->font = TTF_OpenFont(path.c_str(), fw->size);
	}
	return fw->font;
}


LOFont* LOFont::CreateFont(LOString &fontName) {
	auto iter = fontMap.find(fontName);
	if (iter != fontMap.end()) return iter->second;
	LOFont *font = new LOFont();
	if (font->SetName(&fontName)) {
		fontMap[fontName] = font;
		return font;
	}
	else {
		fontMap[fontName] = &builtInFont;
		//初始化内置字体
		if (!builtInFont.isValid()) {
			//0对应BUILT_FONT
			BinArray *data = LonsGetBuiltMem(0);
			builtInFont.SetBinData(data);
		}
		return &builtInFont;
	}
}