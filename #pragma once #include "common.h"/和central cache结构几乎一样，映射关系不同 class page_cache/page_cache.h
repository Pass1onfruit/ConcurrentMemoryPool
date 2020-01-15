#pragma once
#include "common.h"

//和central cache结构几乎一样，映射关系不同
class page_cache//管理的span
{
public:
	span* new_span(size_t numpage);
private:
	span_list _spanlists[129];
};

static page_cache page_cache_inst(size_t num);
