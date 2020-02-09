#include "central_cache.h"
#include "page_cache.h"

span* central_cache::get_one_span(span_list* list, size_t size)
{
	//获取一个有对象的span//如果获取到一个span，span里可能比需求的num大也可能比需求小
	//先不找page cache，从自己里面找
	size_t index = size_class::list_index(size);
	//遍历span list取里面的span
	span_list& spanlist = _span_lists[index];
	span* it = spanlist.begin();
	while (it != spanlist.end())
	{
		if (it->_freelist._empty())//不为空即有对象
		{
			return it;
		}
		else
		{
			it = it->_next;
		}
	}

	//走到这里即说明没有遇到span，需要向page cache要span
	size_t numpage = size_class::num_move_page(size);
	span* _span = page_cache_inst.new_span(numpage);
	//把span对象切成对应大小挂到span的freelist中
	char* start = (char*)(_span->_pageid << 12);//<<12左移12位是*4k
	char* end = start + (_span->_pagesize << 12);//pagesize页数
	while (start < end)
	{
		char* obj = start;
		start += size;

		_span->_free_list.push(obj);
	}
	_span->_obj_size = size;
	spanlist.push_front(_span);

	return _span;
}


size_t central_cache::fetch_range_obj(void*& start, void*& end, size_t num, size_t size)//start end截取的段范围
{
	//已经有一个span了//有可能从spanlist获取的也可能从pagecache获取的
	span* _span = get_one_span(size);
	//把span里的数据取走
	free_list& freelist = _span->_freelist;//如果没有足够的num就返回实际有的num
	size_t actual_num = freelist._pop_range(start, end, num);
	_span->_usecount += actual_num;

	return actual_num;
}
