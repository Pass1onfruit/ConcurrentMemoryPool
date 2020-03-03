#pragma once
#include "common.h"
//数组下挂的是多个自由链表挂在span上  
//对虚拟内存的划分 //分段 单位为区域划分 //分页 页以映射为单位划分
//访问虚拟地址通过页表存映射到物理地址
//如果访问的页没有映射，会发生缺页中断，会在屋里内存中建立映射后再返回
//一页  4k 8k
//起到均衡的作用，反复利用内存，尽量不影响新内存
//mmap munmap 共享内存 映射 减映射

//通过返回来的地址除以4k可以得到页号，得知属于哪个span
class central_cache
{
public:
	//和thread cache进行对接，前者要内存
	//从中心缓存获取一点数量的对象给thread cache
	size_t fetch_range_obj(void*& start, void*& end, size_t num, size_t byte_size);//start end截取的段范围
	//将一定数量的对象释放到span跨度
	void release_list_to_spans(void*& start, size_t byte_size);
	//从spanlist或者page cache获取一个span//优先从spanlist获取span，如果spanlist没有了再向page cache获取
	span* get_one_span(span_list* list, size_t byte_size);
	///         1：28


private:
	span_list _spanlists[NFREE_LIST];
};

static central_cache central_cache_inst;//需定义成全局

