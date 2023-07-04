/* LONS使用的片段着色器 */
#include "LOShader.h"
#include <SDL.h>
#include <vector>

std::vector<std::string> shaderCache;
extern std::string G_RenderName;  //渲染器使用的驱动名称

//适用于GLES2的黑白效果shader
char GLES2_Fragment_TextureGray[] = \
"uniform sampler2D u_texture;\n"                                \
"varying mediump vec4 v_color;\n"                               \
"varying SDL_TEXCOORD_PRECISION vec2 v_texCoord;\n"             \
"\n"                                                            \
"void main()\n"                                                 \
"{\n"                                                           \
"    mediump vec4 abgr = texture2D(u_texture, v_texCoord);\n"   \
"    gl_FragColor = vec4((abgr.b+abgr.g+abgr.r)/3.0);\n"        \
"    gl_FragColor.a = abgr.a;\n"                                \
"    gl_FragColor *= v_color;\n"                                \
"}\n"                                                           \
;

char* GLES2_Array[] = { nullptr, GLES2_Fragment_TextureGray };

std::string CreateLonsShader(int vtype) {
	if (vtype > 0) {
		char tmp[] = "\0\0\0\0";
		tmp[0] = vtype & 0xff;
		std::string s(tmp, 4);

		if (G_RenderName == "opengles2") s.append(GLES2_Array[vtype]);

		s.append("\0\0");

		return s;
	}
	return std::string();
}