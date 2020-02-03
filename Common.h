//主头文件和一些公共的函数
#pragma once  //保证每个头文件只被包含一次//
#include <iostream>
#include <assert.h>

using namespace std;

//个数都有num，字节数都用size
const size_t MAX_SIZE = 64 * 1024;//64k //用const不用宏，调试时可以看见值
const size_t NFREE_LIST = MAX_SIZE / 8;//free_list的大小
const size_t MAX_PAGES = 129;//0~16找central cache调，没有锁没有竞争并发性能高，17~128页找page cache，超过128就直接找系统
const size_t PAGE_SHIFT = 12; // 4k为页移位

inline void*& next_obj(void* obj)//取下一个对象//内联
{
	return *((void**)obj);
}

class free_list//自由链表储存申请了的内存//头删最好O(1)
{
public:
	void _push(void* obj)//还空间时//头插自由链表
	{
		next_obj(obj) = _freelist;
		_freelist = obj;
	}

	void _push_range(void* head, void* tail)//范围push
	{
		next_obj(tail) = _freelist;
		_freelist = head;
	}

	void* _pop()//拿一个对象出来//取走对象时需要// 头删
	{
		void* obj = _freelist;//_freelist指向的第一个
		_freelist = next_obj(obj);
	}

	size_t _pop_range(void* start, void* end, size_t num)
	{
		size_t actual_num;
		void* prev = nullptr;//prev是cur的前一位
		void* cur = _freelist;
		for (;(actual_num < num) && (cur != nullptr); ++actual_num)//走到最后cur指向空了prev指向尾
		{
			prev = cur;
			cur = next_obj(cur);
		}
		start = _freelist;
		end = prev;
		_freelist = cur;
		
		return actual_num;
	}

	bool _empty()
	{
		return _freelist == nullptr;
	}

private:
	void* _freelist    = nullptr;//默认初始化为空
};

class size_class
{
public:
	static size_t list_index(size_t size)//index内存在自由链表中的位置
	{
		if (size % 8 == 0)//8的整数倍
			return size / 8 - 1;
		else
			return size / 8;
	}
};

//central cache
//span 跨度 管理页为单位的内存对象，本质是方便做合并，解决内存碎片问题//
#ifdef _WIN32//针对windows
typedef unsigned int PAGE_ID;
#else
typedef unsigned long long PAGE_ID;//long long // 2^64
#endif
/////
#ifdef _X86
typedef unsigned int PAGE_ID;
#else
typedef unsigned long long PAGE_ID;//long long // 2^64
#endif

struct span
{
	PAGE_ID _pageid;//页号 //不能直接用整形 64位下存不下
	int _pagesize;//页数量
	free_list _freelist;//对象自由链表
	int _usecount;//内存块对象使用计数
	// size_t objsize;//对象大小
	span* _next;//双向链表 //span之间互相连接
	span* _prev;//
};

class span_list
{
public:
	span_list()
	{
		_head = new span;
		_head->_next = _head;
		_head->_prev = _head;
	}

	span* begin()
	{
		return _head->_next;
	}
	span* end()//双向带头循环链表，end是头
	{
		return _head;
	}

	void _insert(span* pos, span* newspan)//没用迭代器，用原生指针
	{
		span* prev = pos->_prev;
		prev->_next = newspan;
		newspan->_next = pos;
		pos->_prev = newspan;
		newspan->_prev = prev;
	}

	void _erase(span* pos)
	{
		assert(pos != _head);
		span* prev = pos->_prev;
		span* next = pos->_next;
		prev->_next = next;
		next->_prev = prev;
	}

	void push_back(span* newspan)
	{
		_insert(_head, newspan);
	}

	void pop_back()
	{
		_erase(_head->_prev);
	}

	void push_front(span* newspan)
	{
		_insert(_head->_next, newspan);
	}

	void pop_front()
	{
		_erase(_head->_next);
	}

private:
	span* _head;
};

//控制大概12%的内存碎片
class size_class
{
public:
	// 控制在[1%，10%]左右的内碎片浪费
	//8倍增长                    
	// 在[1,128]范围时 以8byte对齐 freelist[0,16) //到16是开区间 //以8字节对齐有128/8=16个自由链表
	// [129,1024] 16byte对齐 freelist[16,72)	  //以16字节对齐有(1024-128)/16 = 56 56+之前的16=72//减去128这个范围已经算过了// 15 / (128 + 16) = 10%最高浪费// 15 / 1024 = 1%最小浪费
	// [1025,8*1024] 128byte对齐 freelist[72,128) //8*1024-1024=56 56+72=128// 127 / (1024 + 128) = 11%最高浪费
	// [8*1024+1,64*1024] 1024byte对齐 freelist[128,184) //
	static size_t _round_up(size_t size, int align)//alignment对其数
	{
		return (size + align - 1) & (~(align - 1));
	}

	// [9-16] + 7 = [16-23] -> 16 8 4 2 1
	// [17-32] + 15 = [32,47] ->32 16 8 4 2 1
	static inline size_t round_up(size_t size)
	{
		//if (size % 8 != 0)//效率低
		//{
		//	return (size / 8 + 1) * 8;
		//}
		//else
		//	return size / 8;
		//除法的效率是最低的，要转换成乘法，除的越小代价越大，乘法要转换成加法//加法最快
		assert(size <= MAX_SIZE);

		if (size <= 128)
		{
			return _round_up(size, 8);
		}
		else if (size <= 1024)
		{
			return _round_up(size, 16);
		}
		else if (size <= 8192)
		{
			return _round_up(size, 128);
		}
		else if (size <= 65536)
		{
			return _round_up(size, 1024);
		}

		return -1;
	}

	// [9-16] + 7 = [16-23]
	static size_t _list_index(size_t size, int align_shift)
	{
		return ((size + (1 << align_shift) - 1) >> align_shift) - 1;
	}

	static size_t list_index(size_t size)
	{
		//if (size % 8 == 0)
		//{
		//	  return size / 8 - 1;
		//}
		//else
		//{
		//	  return size / 8;
		//}

		assert(size <= MAX_SIZE);

		// 每个区间有多少个链
		static int group_array[4] = { 16, 56, 56, 56 };
		if (size <= 128)
		{
			return _list_index(size, 3);
		}
		else if (size <= 1024)
		{
			return _list_index(size - 128, 4) + group_array[0];
		}
		else if (size <= 8192)
		{
			return _list_index(size - 1024, 7) + group_array[1] + group_array[0];
		}
		else if (size <= 65536)
		{
			return _list_index(size - 8192, 10) + group_array[2] + group_array[1] +
				group_array[0];
		}

		return -1;
	}

	//[2,512]之间
	static size_t num_move_size(size_t size)
	{
		if (size == 0)
			return 0;
		int num = (int)(MAX_SIZE / size);
		if (num < 2)
			num = 2;
		if (num > 512)
			num = 512;
		return num;
	}

	//计算一次向系统获取几个页
	static size_t num_move_page(size_t size)
	{
		size_t num = num_move_size(size);
		size_t npage = num * size;

		npage >>= 12;//除以4k
		if (npage == 0)//最少返回一页
			npage = 1;

		return npage;
	}
};
