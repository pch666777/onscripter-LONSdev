/* LONS使用的片段着色器 */
#include "LOShader.h"
#include <SDL.h>
#include <vector>

//std::vector<std::string> shaderCache;
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

//适用于opengl的黑白效果shader
char GL_Fragment_TextureGray[] = \
"varying vec4 v_color;\n"
"varying vec2 v_texCoord;\n"
"uniform sampler2D tex0;\n"
"\n"
"void main()\n"
"{\n"
"    gl_FragColor = texture2D(tex0, v_texCoord) * v_color;\n"
"    gl_FragColor.r = (gl_FragColor.r+gl_FragColor.g+gl_FragColor.b)/3.0;\n"
"    gl_FragColor.g = gl_FragColor.r;\n"
"    gl_FragColor.b = gl_FragColor.r;\n"
"}"
;

char* GLES2_Array[] = { nullptr, GLES2_Fragment_TextureGray };
char* GL_Array[] = { nullptr, GL_Fragment_TextureGray };

//前4字节有特殊用途，[0]为shader id，[1]操作要求，[2]编译是否成功，
//[3]SDL2内部使用，注意只有修改过的SDL2才支持
std::string CreateLonsShader(int vtype) {
	if (vtype > 0) {
		char tmp[] = "\0\0\0\0";
		tmp[0] = vtype & 0xff;
		std::string s(tmp, 4);

		if (G_RenderName == "opengles2") s.append(GLES2_Array[vtype]);
		else if(G_RenderName == "opengl") s.append(GL_Array[vtype]);
		//printf(s.c_str() + 4);
        s.append("\0\0");

		return s;
	}
	return std::string();
}
