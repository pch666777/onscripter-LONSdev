/*
STL的stack不适合自定义的类，也不能历遍，手搓一个，凑合用
*/
#ifndef __LOSTACK_H__
#define __LOSTACK_H__

#include <cassert>
#include <stdlib.h>
#include <cstring>

#include "../etc/LOLog.h"



//注意，这是一个指针型堆栈
//不适合处理非常大的数据
template <typename T>
class LOStack {
public:
	LOStack() {
		length = 0;
		preSize = 5;
		elems = (intptr_t*)malloc(sizeof(intptr_t)*preSize);
		for (int ii = 0; ii < preSize; ii++) {
			memset(elems, 0, sizeof(intptr_t)*preSize);
		}
	};
	~LOStack() {
		free(elems);
	}

	//重置堆栈，注意不会改变已经扩大的大小
	void clear(bool isdel = false) {
		if (isdel) {
			for (int ii = 0; ii < length; ii++) {
				T* v = (T*)(elems[ii]);
				delete v;
			}
		}
		length = 0;
		memset(elems, 0, sizeof(intptr_t)*preSize);
	}


	void clearEmpty() {
		for (int ii = 0; ii < length; ii++) {
			if (!elems[ii]) {
				for (int kk = ii + 1; kk < length; kk++) {
					if (elems[kk]) {
						elems[ii] = elems[kk];
						elems[kk] = NULL;
						break;
					}
				}
			}
		}
		while (length > 0 && elems[length - 1] == NULL) length--;
	}

	//加入堆栈尾部
	T* push(T* v) {
		intptr_t iv = (intptr_t)v;
		elems[length] = iv;
		length++;
		if (length >= preSize) {
			preSize *= 2;
			elems = (intptr_t*)realloc(elems, sizeof(intptr_t)*preSize);
		}
		return v;
	}


	//弹出堆栈
	T* pop(bool isdel = false) {
		if (length > 0) {
			length--;
			if (isdel) {
				delete (T*)elems[length];
				return NULL;
			}
			return (T*)elems[length];
		}
		else return NULL;
	}

	//顶部元素
	T* top() {
		if (length > 0) {
			return (T*)elems[length-1];
		}
		else return NULL;
	}

	T* at(int pos) {
		if (pos < length) return (T*)(elems[pos]);
		else return NULL;
	}

	//堆栈大小
	int size() {
		return length;
	}

	//交换元素
	T* swap(T* v,int pos) {
		if (pos <= length - 1) {
			T* v1 = (T*)elems[pos];
			elems[pos] = (intptr_t)v;
			return v1;
		}
		else return NULL;
	}

	void swap(int pos1,int pos2){
		if(pos1 == pos2) return ;
		intptr_t tt = elems[pos1];
		elems[pos1] = elems[pos2] ;
		elems[pos2] = tt;
	}

	T* removeAt(int index) {
		if (index < 0 || index >= length || length <= 0 ) return NULL;
		T* v1 = (T*)elems[index];
		for (int ii = index; ii < length - 1; ii++) elems[index] = elems[index + 1];
		elems[length - 1] = NULL;
		length--;
		return v1;
	}

	//反转
	void reverse() {
		intptr_t *elems_old = elems;
		elems = (intptr_t*)malloc(sizeof(intptr_t) * preSize);
		int kk = 0;
		for (int ii = length - 1; ii >= 0; ii--) {
			elems[kk] = elems_old[ii];
			kk++;
		}
		free(elems_old);
	}

	LOStack<T>* shallowCopy() {
		LOStack<T>* it = new LOStack<T>;
		it->length = length;
		it->preSize = preSize;
		it->elems = (intptr_t*)malloc(sizeof(intptr_t)*preSize);
		memcpy(it->elems, elems, sizeof(intptr_t)*preSize);
		return it;
	}


	//伪迭代器
	class iterator {
	public:
		iterator(LOStack *s, int cur) {
			stack = s;
			current = cur;
		}
		int getpos() { return current; }
		T* operator*() { return stack->at(current); }

		iterator operator+(const int&t) {
			int pos = current + t;
			if (pos > stack->size()) pos = stack->size();
			iterator it(stack, pos);
			return it;
		}
		iterator operator-(const int&t) {
			int pos = current - t;
			if (pos < 0) pos = stack->size();
			iterator it(stack, pos);
			return it;
		}

		iterator operator++(int) {
			iterator temp = *this;
			current++;
			if (current > stack->size())current = stack->size();
			return temp;
		}
		iterator operator--(int) {
			iterator temp = *this;
			current--;
			if (current < 0)current = stack->size();
			return temp;
		}
		bool operator==(const iterator &t) { return(current == t.current); }
		bool operator!=(const iterator &t) { return(current != t.current); }
	private:
		LOStack* stack;
		int current;
	};

	iterator begin() {
		iterator iter(this, 0);
		return iter;
	}

	iterator end() {
		iterator iter(this, length);
		return iter;
	}

	iterator erase(iterator iter,bool isdel = false) { //删除当前的元素并指向下一个
		if (isdel) {
			T* v = (T*)elems[iter.getpos()];
			delete v;
		}
		for (int ii = iter.getpos(); ii <= length - 2; ii++) {
			elems[ii] = elems[ii + 1]; //下一个元素向前移动
		}
		elems[length - 1] = 0;
		length--;
		return iter;  //只是堆栈内的数组移动，迭代器指向的位置自动变为下一个
	}



private:
	int length;//当前堆栈的大小
	int preSize; //当前堆栈的准备大小
	intptr_t* elems;   //元素指针
};



#endif // !__LOSTACK_H__
