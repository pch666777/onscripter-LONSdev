/*
用于2D图形计算的矩阵运算
only998 2021-?
*/

#include "LOMatrix2d.h"
#include <math.h>
#include <SDL.h>

LOMatrix2d::LOMatrix2d(){
	Clear();
}

LOMatrix2d::~LOMatrix2d(){

}

//重置矩阵
void LOMatrix2d::Clear() {
	for (int ii = 0; ii < MAX_DATA; ii++) {
		data[ii] = 0;
	}
	data[0] = 1;
	data[4] = 1;
	data[8] = 1;
}

//pos = a * 3 + b;
double LOMatrix2d::Get(int a, int b) {
	int pos = a * 3 + b;
	return data[pos];
}

void LOMatrix2d::Set(int a, int b, double v) {
	int pos = a * 3 + b;
	data[pos] = v;
}


//生成一个平移矩阵
LOMatrix2d LOMatrix2d::GetMoveMatrix(double x, double y) {
	LOMatrix2d ma;
	ma.Set(0, 2,x);
	ma.Set(1, 2, y);
	return ma;
}

//生成一个缩放矩阵
LOMatrix2d LOMatrix2d::GetScaleMatrix(double sx, double sy) {
	LOMatrix2d ma;
	ma.Set(0, 0, sx);
	ma.Set(1, 1, sy);
	return ma;
}

//生成一个旋转矩阵
LOMatrix2d LOMatrix2d::GetRotateMatrix(double rotate) {
	LOMatrix2d ma;
	if (fabs(rotate) < 0.0001) return ma;  //没转
	double rot = fabs(rotate)*M_PI / 180;  //转为弧度
	ma.Set(0, 0, cos(rot));
	ma.Set(1, 1, cos(rot));
	if (rotate > 0) { //逆时针
		ma.Set(0, 1, 0-sin(rot));
		ma.Set(1, 0, sin(rot));
	}
	else { //顺时针
		ma.Set(0, 1, sin(rot));
		ma.Set(1, 0, 0-sin(rot));
	}
	return ma;
}

LOMatrix2d LOMatrix2d::operator*(const LOMatrix2d& m1) {
	LOMatrix2d ma;
	for (int ii = 0; ii < MAX_DATA; ii++) {
		int row = ii / 3;
		int col = ii % 3;
		ma.data[ii] = 0;
		for (int kk = 0; kk < 3; kk++) ma.data[ii] += data[row * 3 + kk] * m1.data[col + kk * 3];
	}
	return ma;
}


//根据矩阵变换坐标点
void LOMatrix2d::TransPoint(double *x, double *y) {
	double m[] = { *x,*y,1 };
	*x = data[0] * m[0] + data[1] * m[1] + data[2] * m[2];
	*y = data[3] * m[0] + data[4] * m[1] + data[5] * m[2];
}

LOMatrixPoint2d LOMatrix2d::TransPoint(double x, double y) {
	LOMatrixPoint2d p;
	TransPoint(&p.x, &p.y);
	return p;
}