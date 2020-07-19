#include "central_cache.h"
#include "page_cache.h"

span* central_cache::get_one_span(size_t size)
{
	//获取一个有对象的span//如果获取到一个span，span里可能比需求的num大也可能比需求小
	//先不找page cache，从自己里面找
	size_t index = size_class::list_index(size);
	//遍历span list取里面的span
	span_list& spanlist = _span_lists[index];
	span* it = spanlist.begin();
	while (it != spanlist.end())
	{
		if (it->_freelist._empty())//不为空即有对象,可以取
		{
			return it;
		}
		else
		{
			it = it->_next;
		}
	}
	//走到这里即说明没有遇到span，需要向page cache获取一个span
	size_t numpage = size_class::num_move_page(size);
	span* _span = page_cache_inst.new_span(numpage);//需要多少span还需要算
	//把span对象切成对应大小挂到span的freelist中
	//通过pageid算地址 
	char* start = (char*)(_span->_pageid << 12);//<<12左移12位是*4k
	char* end = start + (_span->_pagesize << 12);//pagesize页数
	while (start < end)
	{
		//一个个切出来
		char* obj = start;
		start += size;

		_span->_free_list.push(obj);
	}
	_span->_obj_size = size;
	spanlist.push_front(_span);

	return _span;
}

//从中心缓存获取一段数量的对象给thread cache (四 01：20：00
size_t central_cache::fetch_range_obj(void*& start, void*& end, size_t num, size_t size)//start end截取的段范围
{
	size_t index = size_class::list_index(size);
	span_list& spanlist = _spanlists[index];
	_spanlists->_lock();///

	//获取一个有对象的span，如果没有span了还要从page_cache要
	//已经有一个span了//有可能从spanlist获取的也可能从pagecache获取的
	span* _span = get_one_span(size);
	//把span里的数据取走,但是不一定有num个
	free_list& freelist = _span->_freelist;//如果没有足够的num就返回实际有的num
	size_t actual_num = freelist._pop_range(start, end, num);
	_span->_usecount += actual_num;

	_spanlists->_unlock();///只有一个线程取完了另一个线程才可以进来

	return actual_num;
	//可以做一个优化，把已经空的span给移到后面去，这样找到可用的span就快
}

void central_cache::release_list_to_spans(void* start, size_t size)
{
	size_t index = size_class::list_index(size);
	span_list& spanlist = _spanlists[index];
	_spanlists->_lock();///避免同时释放

	while (start)
	{
		void* next = next_obj(start);
		//
		PAGE_ID id = (PAGE_ID)start >> PAGE_SHIFT;
		//找到对应的span
		span* _span = page_cache_inst.get_id_to_span(id);
		//插回自由链表
		_span->_freelist._push(start);//这里进行的是头插，所以要先保存start
		_span->_usecount--;
		//usecount==0，表示当前span切出去的对象已经全部返回
		//可以将span还给page_cache进行合并（解决内存碎片）
		if (_span->_usecount == 0)
		{
			size_t index = size_class::list_index(_span->_obj_size);
			_spanlists[index]._erase(_span);
			//
			_span->_freelist._clear();
			page_cache_inst.release_span_to_pagecache(_span);
		}

		start = next;
	}
	_spanlists->_unlock();///
}


