//进行内存合并
#include "page_cache.h"

span* page_cache::new_span(size_t numpage)
{
	//如果这个位置有就直接取
	if (!_span_lists[numpage]._empty(  ))
	{
		span* span = _span_lists[numpage].begin();
		_span_lists[numpage].pop_front();
		return span;
	}
	for (size_t i = numpage+1; i < MAX_PAGES; ++i)
	{	
		//如果在这里取到了说明取的肯定是比这个页数多的，需要分割
		if (!_span_lists[i]._empty())//每个位置存一个spanlist的数组
		{  
			//分裂
			//这里的span都是未被使用的
			span* _span = _span_lists[i].begin();
			_span_lists[i].pop_front();

			span* splitspan = new span;
			////开始切出去的页号
			//splitspan->_pageid = _span->_pageid + numpage;
			////剩下的page
			//splitspan->_pagesize = _span->_pagesize - numpage;
			splitspan->_pageid = _span->_pageid + _span->_pagesize-numpage;
			splitspan->_pagesize = numpage;
			//_span->_pagesize = numpage;
			//把剩下的span挂起来//_pagesize是多少就挂在对应的页单位下
			_span_lists[splitspan->_pagesize].push_front(splitspan);

			for (PAGE_ID i = 0; i < numpage; ++i)
			{
				_idspan_map[splitspan->_pagesize + i] = splitspan;
			}

			_span->_pagesize -= numpage;
			_span_lists[_span->_pagesize].push_front(_span);

			return splitspan;
		}
		
		//走到这里需要向系统申请了
		void* ptr = system_alloc(MAX_PAGES - 1);
		//直接申请了一个最大页的span
		span* bigspan = new span;
		bigspan->_pageid = (PAGE_ID)ptr >> PAGE_SHIFT;//强转后位移算出页号//几个4k几个4k分隔的页号
		//void* ptrshift = (void*)(id << PAGE_SHIFT);//同理页号也可以算出指针
		bigspan->_pagesize = MAX_PAGES - 1;

		//进行页号和span的映射关系
		for (PAGE_ID i = 0; i < bigspan->_pagesize; ++i)
		{
			//_idSpan_map.insert(std::make_pair(bigspan->_pageid + i, bigspan));
			_idspan_map[bigspan->_pageid + i] = bigspan;
		}
		//直接把bigspan挂进spanlist
		_span_lists[bigspan->_pagesize].push_front(bigspan);

		//这里的递归只会走一次
		return new_span(numpage);
	}
}

void page_cache::release_span_to_pagecache(span* _span)
{
	//页向前合并
	while (1)
	{
		//前一个页的页号
		PAGE_ID prev_id = _span->_pageid - 1;
		auto it = _idspan_map.find(prev_id);
		//前面的页不存在
		if (it == _idspan_map.end())
		{
			break;
		}
		
		span* prev_span = it->second;
		//说明前一个页还在使用中或不存在，不能合并
		if (prev_span->_usecount != 0)
		{
			break;
		}
		//接下来就符合要求合并了
		//页号不变，页大小变
		//prev_span->_pagesize += _span->_pagesize;
		_span->_pageid = prev_span->_pageid;
		_span->_pagesize += prev_span->_pagesize;
		//map页对应到新的span
		for (PAGE_ID i = 0; i < prev_span->_pagesize; ++i)
		{
			_idspan_map[prev_span->_pageid + i] = _span;
		}
		delete prev_span;
		//继续找上一个页，循环
	}

}
//向后合并


//传id返回对应的span
span* page_cache::get_id_to_span(PAGE_ID id)
{
	//return _idspan_map[id];//这样写如果id不存在会插入一个
	//std::map<PAGE_ID, span*>::iterator = _idspan_map.find(id);
	auto it = _idspan_map.find(id);
	if (it != _idspan_map.end())
	{
		return it->second;
	}
	else
	{
		return nullptr;
	}
}

