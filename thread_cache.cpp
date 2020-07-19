#include "thread_cache.h"
#include "central_cache.h"

void* thread_cache::_allocte(size_t size, size_t num)//申请
{
	size_t index = size_class::list_index(size);//算出需要的空间
	free_list& freelist = _freelists[index];
	if (!freelist._empty())//如果我自己的freelist里面有
	{
		return freelist._pop(); //就直接把内存pop取出来
	}
	else//没有内存了，向central_cache取，向下一层交互
	{
		//void* start = fetch_from_central_cache(index, num);
		//从中心缓存获取批量的对象
		return fetch_from_central_cache(size_class::round_up(size));
		//从中心缓存获取对象//此时size是已经在round_up对齐过的
	}
}

void thread_cache::_deallocte(void* ptr, size_t size)//释放
{
	//还给哪个spanlist需要算出来
	size_t index = size_class::list_index(size);//?
	free_list& freelist = _freelists[index];
	freelist._push(ptr);

	//释放判断标准：对象个数满足一定条件||内存大小超过多少
	size_t num = size_class::num_move_size(size);
	if(freelist.num() >= num)
	{
		//release_to_central_cache();
		list_too_long(freelist, num, size);
	}
	

}

void thread_cache::list_too_long(free_list& freelist, size_t num, size_t size)
{
	void* start = nullptr, * end = nullptr;
	freelist._pop_range(start, end, num);//弹出来还给下一层
	next_obj(end) = nullptr;//弹出的一段最后一个不指向空回继续指向原来的下一个
	//需要一个一个还
	central_cache_inst.release_list_to_spans(start, size);

}

//可以对立测试thread_cache
//void* thread_cache::fetch_from_central_cache(size_t index)//在index位置取num个
//{
//	size_t num = size_class::num_move_size(index);///////////////////////////////////////////////////////////////////////////////////////
//
//	size_t size = (index + 1) * 8;//index是自由链表的下标
//	char* start = (char*)malloc(size * num);//找central cache要
//	char* cur = start;
//	for (size_t i = 0; i < num; i++)
//	{
//		char* next = cur + size;//cur跟着next
//		next_obj(cur) = next;
//		cur = next;
//	}
//	//最后一个对象要指向空
//	next_obj(cur) = nullptr;
//	void* head = next_obj(start);
//	void* tail = cur;//cur此时已走到尾
//	_freelists[index]._push_range(head, tail);//push了head到tail的这段范围
//
//	return start;
//}
//获取内存
void* thread_cache::fetch_from_central_cache(size_t size)
{
	size_t num = size_class::num_move_size(size);
	void* start = nullptr; 
	void* end = nullptr;
	//实际返回的内存大小
	size_t actual_num = central_cache_inst.fetch_range_obj(start, end, num, size);
	if (actual_num <= 1)//至少返回一个，不然抛异常
	{
		return start;
	}
	else
	{
		//(12.8    01:18:00
		//算出所属的freelist位置
		size_t index = size_class::list_index(size);
		free_list list = _freelists[index];
		list._push_range(next_obj(start), end, actual_num-1);
		//只返回一个，剩下的用push_range头插挂起来备用
		return start;
	}

	return nullptr;
}
