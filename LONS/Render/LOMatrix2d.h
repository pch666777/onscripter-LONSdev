/*
用于2D图形计算的矩阵运算
only998 2021-?
平移矩阵   缩放矩阵   旋转矩阵（逆时针，正数）     旋转矩阵（顺时针）
[1 0 tx]  [sx 0 0]   [cos(a) -sin(a) 0]   [cos(a) sin(a)  0]  [0 1 2]
[0 1 ty]  [0 sy 0]   [sin(a) cos(a)  0]   [-sin(a) cos(a) 0]  [3 4 5]
[0 0  1]  [0  0 1]   [  0      0     1]   [  0      0     1]  [6 7 8]
*/
#ifndef __LOMATRIX2D_H__
#define __LOMATRIX2D_H__


struct LOMatrixPoint2d
{
	double x;
	double y;
};

class LOMatrix2d
{
public:
	LOMatrix2d();
	~LOMatrix2d();

	enum {
		MAX_DATA = 9
	};

	double data[MAX_DATA];

	double Get(int a, int b);
	void   Set(int a, int b,double v);
	void Clear();

	LOMatrix2d operator*(const LOMatrix2d& m1);
	LOMatrixPoint2d TransPoint(double x,double y);
	void TransPoint(double *x, double *y);

	static LOMatrix2d GetMoveMatrix(double x,double y);
	static LOMatrix2d GetScaleMatrix(double sx, double sy);
	static LOMatrix2d GetRotateMatrix(double rotate);
};
#endif // !__LOMATRIX2D_H__
