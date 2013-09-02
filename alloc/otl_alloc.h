/**********************************************
 * File: otl_alloc.h
 * Author: newwy
 * ********************************************/

#ifndef _OTL_ALLOC_H_
#define _OTL_ALLOC_H_

#include <cstdio>
#include <cstdlib>
#include <new>
#include <algorithm>

namespace otl 
{

template < typename _T >
class otl_alloc
{
public:
	static const bool recycle_space;//false; When the alloc destructor, whether to release the space
	static const bool thread_safe;// true; the space alloc is if thread safety
	otl_alloc() {}
	otl_alloc(const otl_alloc &) {}
	template < typename _T1 >
	otl_alloc(const otl_alloc < _T1 > &) {}

	~otl_alloc() {}
	_T * address(_T & _x) const {
		return &_x;
	}
	const _T * address(const _T & _x) const {
		return &_x;
	}
	_T * allocate(size_t _n, const void * = 0) {
		_T * _ret = static_cast < _T * >(malloc(_n * sizeof(_T)));
		return _ret;
	}
	_T * reallocate(size_t _n, void * ptr = 0) {
		if (NULL == ptr) {
			return allocate(_n, ptr);
		}
		return static_cast < _T * >(realloc(ptr, _n * sizeof(_T)));
	}
	void deallocate(_T * ptr, size_t) {
		if (NULL != ptr) {
			free(static_cast < void * > (ptr));
		}
	}
	size_t max_size() const {
		return size_t(-1) / sizeof(_T);
	}
	void construct(_T * ptr, const _T & _val) {
		::new(ptr) _T(_val);
	}
	void destroy(_T * ptr) {
		ptr->~_T();
	}
	int create() {return 0;}
	int destroy() {return 0;}
	void swap(otl_alloc<_T> &) {
	}
	int merge(otl_alloc<_T> &) {
		return 0;
	}

	_T * getp(_T * ptr) {
		return ptr;
	}
		
};

template < typename _T >
inline bool operator == (const otl_alloc < _T > &, const otl_alloc < _T > &) {
	return true;
}

template < typename _T1, typename _T2 >
inline bool operator == (const otl_alloc < _T1 > &, const _T2 &) {
	return false;
}

template < typename _T >
inline bool operator != (const otl_alloc < _T > &, const otl_alloc < _T > &) {
	return false;
}

template < typename _T1, typename _T2 >
inline bool operator != (const otl_alloc < _T1 > &, const _T2 &) {
	return true;
}

template <typename _T>
const bool otl_alloc<_T>::recycle_space = false;
template <typename _T>
const bool otl_alloc<_T>::thread_safe = true;


}

#endif

