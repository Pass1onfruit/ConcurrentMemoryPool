//进行内存合并
#include "page_cache.h"

span* page_cache::new_span(size_t numpage)
{
	//如果这个位置有就直接取
	if (!_spanlists[numpage]._empty())
	{
		span* span = _span_lists[numpage].begin();
		_span_lists[numpage].pop_front();
		return span;
	}
	for (size_t i = numpage+1; i < MAX_PAGES; ++i)
	{	
		//如果在这里取到了说明取的肯定是比这个页数多的，需要分割
		if (!_spanlists[i]._empty())//每个位置存一个spanlist的数组//这里的span都是未被使用的
		{
			span* span = _span_lists[i].begin();
			_span_lists[i].pop_front();

			span* splitspan = new span;
			splitspan->_pageid = span->_pageid + numpage;
			splitspan->_pagesize = span->_pagesize - numpage;

			span->_pagesize = numpage;

			_span_lists[splitspan->_pagesize].push_front(splitspan);

			return span;
		}
		
		void* ptr = system_alloc(MAX_PAGES - 1);

		span* bigspan = new span;
		bigspan->_pageid = (PAGE_ID)ptr >> PAGE_SHIFT;
		bigspan->_pagesize = MAX_PAGES - 1;

		for (PAGE_ID i = 0; i < bigspan->_pagesize; ++i)
		{
			//_idSpanMap.insert(std::make_pair(bigspan->_pageid + i, bigspan));
			_id_span_,map[bigspan->_pageid + i] = bigspan;
		}

		_span_lists[bigspan->_pagesize].push_front(bigspan);

		return new_span(numpage);
	}
}
