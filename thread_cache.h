#pragma once //使所有头文件只被包一次，避免重复

//解决性能问题//
//内存碎片问题//申请内存必须是连续的//有内存，内存不连续申请不出来
//申请效率问题//
#include "common.h"
// 哈希/散列表 //映射
//直接映射
//int a[256] // 找字符下标//如 a ascii码97，a[97]++,再出现再++
//

//线程缓存// 是每个线程独有的//一次申请大于64k则直接向系统要
//thread cache不够了找central cache要（单位是由页切开的链表freelist），central cache不够了向page cache要（单位 页）

class thread_cache
{
public:
	void* _allocte(size_t size, size_t num);//申请内存//size对象的大小
	void _deallocte(void* ptr, size_t size);//释放
	//从中心缓存获取对象
	void* fetch_from_central_cache(size_t index);//一次取一定批量的内存//从central cache取内存要加锁
	//void*有返回值													  
	//使用时一个返回，剩下的放到自由链表对应位置，下次再要内存就不用到central cache取
	//如果自由链表中对象超过一定长度就还给中心缓存
	void list_too_long(free_list& freelist, size_t num, size_t size);


private:
	free_list _freelists[NFREE_LIST];//映射对齐的8字节数组//8 16 32...

};

static thread_cache* tls_thread_cache = nullptr;

//线程TLS：线程局部储存
//_declspec (thread) static thread_cache* tls_thread_cache = nullptr;
//加了_declspec (thread)就是属于每个线程独享的
