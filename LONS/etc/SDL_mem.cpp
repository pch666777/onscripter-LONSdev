/*
//由于SDL使用内存池，因此SDL分配的对象不好检测内存泄漏情况
//为了在debug模式下检测SDL对象的分配情况，控制所有的对象生成并记录
*/

#include "SDL_mem.h"

#ifdef SDL_MEM_CHECK

MemoryCheck _surface_mem;
MemoryCheck _texture_mem;



void MemoryCheck::MallocAdress(void *data) {
	if (!data) return;
	_mutex.lock();
	count++;

	//if (this == &_texture_mem && (count == 1 || count == 3)) {
	//	int debugbreak = 1;
	//}

	map[(intptr_t)data] = count;
	_mutex.unlock();
}

bool MemoryCheck::FreeAdress(void *data) {
	bool isfree = true;
	if (!data) return true;
	_mutex.lock();
	auto iter = map.find((intptr_t)data);
	if (iter != map.end()) map.erase(iter);
	else isfree = false;
	_mutex.unlock();
	return isfree;
}


SDL_Surface *CreateRGBSurfaceWithFormat(Uint32 flags, int width, int height, int depth, Uint32 format) {
	SDL_Surface *su = SDL_CreateRGBSurfaceWithFormat(flags, width, height, depth, format);
	_surface_mem.MallocAdress(su);
	return su;
}

SDL_Surface * LIMG_Load_RW(SDL_RWops *src, int freesrc) {
	SDL_Surface *su = IMG_Load_RW(src, freesrc);
	_surface_mem.MallocAdress(su);
	return su;
}

SDL_Surface * LTTF_RenderGlyph_Blended(TTF_Font *font, Uint16 ch, SDL_Color fg) {
	SDL_Surface *su = TTF_RenderGlyph_Blended(font, ch, fg);
	_surface_mem.MallocAdress(su);
	return su;
}

SDL_Surface * LTTF_RenderGlyph_Shaded(TTF_Font *font, Uint16 ch, SDL_Color fg, SDL_Color bg) {
	SDL_Surface *su = TTF_RenderGlyph_Shaded(font, ch, fg, bg);
	_surface_mem.MallocAdress(su);
	return su;
}

void FreeSurface(SDL_Surface * surface) {
	if (!_surface_mem.FreeAdress(surface))  printf("unkown adress of surface!\n");
	SDL_FreeSurface(surface);
}


SDL_Texture * CreateTexture(SDL_Renderer * renderer, Uint32 format, int access, int w, int h) {
	SDL_Texture *tex = SDL_CreateTexture(renderer, format, access, w, h);
	_texture_mem.MallocAdress(tex);
	return tex;
}

void DestroyTexture(SDL_Texture * texture) {
	if(!_texture_mem.FreeAdress(texture)) printf("unkown texture adress!\n");
	SDL_DestroyTexture(texture);
}

SDL_Texture * CreateTextureFromSurface(SDL_Renderer * renderer, SDL_Surface * surface) {
	SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surface);
	_texture_mem.MallocAdress(tex);
	return tex;
}

#endif // SDL_MEM_CHECK