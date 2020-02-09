#include "thread_cache.h"
#include "central_cache.h"

void* thread_cache::_allocte(size_t size, size_t num)//申请
{
	size_t index = size_class::list_index(size);//
	free_list& freelist = _freelists[index];
	if (!freelist._empty())//如果不为空
	{
		return freelist._pop();
	}
	else//没有内存了，向内存池取
	{
		//void* start = fetch_from_central_cache(index, num);

		return fetch_from_central_cache(size_class::round_up(size));//从中心缓存获取对象//此时size是已经对齐过的
	}
}

void thread_cache::_deallocte(void* ptr, size_t size)//释放
{
	size_t index;//?
	free_list& freelist = _freelists[index];
	freelist._push(ptr);

	//if()
	//release_to_central_cache();

}

void* thread_cache::fetch_from_central_cache(size_t index)//在index位置取num个
{
	size_t num = 20;///////////////////////////////////////////////////////////////////////////////////////

	size_t size = (index + 1) * 8;//index是自由链表的下标
	char* start = (char*)malloc(size * num);//找central cache要
	char* cur = start;
	for (size_t i = 0; i < num; i++)
	{
		char* next = cur + size;//cur跟着next
		next_obj(cur) = next;
		cur = next;
	}
	//最后一个对象要指向空
	next_obj(cur) = nullptr;
	void* head = next_obj(start);
	void* tail = cur;//cur此时已走到尾
	_freelists[index]._push_range(head, tail);//push了head到tail的这段范围

	return start;
}
//获取内存
void* thread_cache::fetch_from_central_cache(size_t size)
{
	size_t num = size_class::num_move_size(size);
	void* start = nullptr;
	void* end = nullptr;
	size_t actual_num = central_cache_inst.fetch_range_obj(start, end, num, size);//实际返回的num
	if (actual_num == 1)//至少返回一个，不然抛异常
	{
		return start;
	}
	else
	{
		size_t index = size_class::list_index(size);
		free_list list = _freelists[index];
		list._push_range();
	}

	return nullptr;
}
