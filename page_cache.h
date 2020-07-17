#pragma once
#include "common.h"

//会一次性就向系统要128页
//和central cache结构几乎一样，映射关系不同
class page_cache//管理的span
{
public:
	span* new_span(size_t numpage);
	//向系统申请numpage页内存挂到spanlist
	//void system_alloc(size_t numpage);
	void release_span_to_pagecache(span* _span);
	span* get_id_to_span(PAGE_ID id); //传id返回对应的span
private:
	span_list _span_lists[MAX_PAGES];//
	std::map<PAGE_ID, span*> _idspan_map;
};

static page_cache page_cache_inst(size_t num);
