/* LONS使用的片段着色器 */
#ifndef _LOSHADER_H__
#define _LOSHADER_H__

#include <string>


// 1-黑白   2-反色，不用释放char*，由内部管理
extern std::string CreateLonsShader(int vtype);

#endif