/*
//由于SDL使用内存池，因此SDL分配的对象不好检测内存泄漏情况
//为了在debug模式下检测SDL对象的分配情况，控制所有的对象生成并记录
*/

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <unordered_map>
#include <mutex>

//#define SDL_MEM_CHECK


#ifdef SDL_MEM_CHECK

class MemoryCheck {
public:
	void MallocAdress(void *data);
	bool FreeAdress(void *data);
private:
	int32_t count = 0;
	std::unordered_map<intptr_t, uint32_t> map;
	std::mutex _mutex;
};


extern SDL_Surface *CreateRGBSurfaceWithFormat(Uint32 flags, int width, int height, int depth, Uint32 format);
extern void FreeSurface(SDL_Surface *surface);
extern SDL_Surface * LIMG_Load_RW(SDL_RWops *src, int freesrc);
extern SDL_Surface * LTTF_RenderGlyph_Blended(TTF_Font *font, Uint16 ch, SDL_Color fg);
extern SDL_Surface * LTTF_RenderGlyph_Shaded(TTF_Font *font,Uint16 ch, SDL_Color fg, SDL_Color bg);
extern SDL_Texture * CreateTexture(SDL_Renderer * renderer, Uint32 format, int access, int w, int h);
extern void DestroyTexture(SDL_Texture * texture);
extern SDL_Texture * CreateTextureFromSurface(SDL_Renderer * renderer, SDL_Surface * surface);
#else

#define CreateRGBSurfaceWithFormat SDL_CreateRGBSurfaceWithFormat
#define FreeSurface SDL_FreeSurface
#define LIMG_Load_RW IMG_Load_RW
#define LTTF_RenderGlyph_Blended TTF_RenderGlyph_Blended
#define LTTF_RenderGlyph_Shaded TTF_RenderGlyph_Shaded
#define CreateTexture SDL_CreateTexture
#define DestroyTexture SDL_DestroyTexture
#define CreateTextureFromSurface SDL_CreateTextureFromSurface

#endif // SDL_MEM_CHECK

