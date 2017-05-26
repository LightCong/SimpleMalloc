//
//  simple_malloc.c
//  SimpleSTL
//
//  Created by dacuobi on 17/5/22.
//  Copyright © 2017年 dacuobi. All rights reserved.
//

#include "simple_malloc.h"
#include "simple_malloc_internal.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/*--------------------内部全局变量---------------------------*/
#ifdef DEBUG
MemIndex mem_index_arr[MEM_INDEX_NUM]; // 8,16,.....256
PoolHeader *pool_cache;		      // pool 的缓存
PondHeader *pond_lst;
#else
static MemIndex mem_index_arr[MEM_INDEX_NUM]; // 8,16,.....256
static PoolHeader *pool_cache;		      // pool 的缓存
static PondHeader *pond_lst;
#endif
/*--------------------------------------------------------*/

void pond_init(PondHeader *pond)
{
	// 每一个pool 的首地址都要按照pool_size对齐
	if (!pond)
		return;

	pond->next_pond = NULL;

	//计算pond 中第一个pool的地址
	pond->start_pool = (void *)pond + sizeof(PondHeader);
	unsigned long excess =
	    ((unsigned long)pond->start_pool & (unsigned long)POOL_SIZE_MASK);
	if (excess != 0) {
		pond->start_pool =
		    (void *)pond->start_pool + POOL_SIZE - excess;
	}
    pond->end_addr=(void*)((unsigned long)pond+POND_SIZE);

	// 计算这个pond里有多少个pool
	unsigned int max_pool_count=get_pool_max_count(pond);
	unsigned int count = 0;
	PoolHeader *pool_ptr = pond->start_pool;
	while (count < max_pool_count-1) {
		pool_ptr->next =
		    (PoolHeader *)((void *)pool_ptr + POOL_SIZE);
		pool_ptr = pool_ptr->next;
		count++;
	}
	pool_ptr->next = NULL;
}

int fill_pool_cache()
{
	// 申请一个pond，然后改造为pool链表
	void *pond = malloc(POND_SIZE);

	if (!pond) {
		fprintf(stderr, "malloc error %s,%d\n", __FILE__, __LINE__);
		return FALSE;
	}
	pond_init(pond);
	if (!pond_lst) {
		// pond_lst 为空
		pond_lst = pond;
		pond_lst->next_pond = NULL;
	} else {
		PondHeader *raw_pond_lst = pond_lst;
		pond_lst = pond;
		pond_lst->next_pond = raw_pond_lst;
	}

	pool_cache = pond_lst->start_pool;

	return TRUE;
}

void pool_init(PoolHeader *pool, unsigned int size_index)
{
	// pool 投入使用前的初始化
	if (!pool)
		return;
	pool->pre = NULL;
	pool->next = NULL;
	pool->size_index = size_index;
	pool->used_block_count = 0;
	pool->state = IN_USE;
	pool->free_block = (BlockHeader *)((void *)pool + sizeof(PoolHeader));

	// block之间的，最后一个block的next_free_ptr指向null
	unsigned int max_count = get_block_max_count(pool);
	unsigned int count = 0;
	BlockHeader *block = pool->free_block;
	unsigned int size = get_size(size_index);
	while (count < max_count) {
		block->next_free_ptr = (BlockHeader *)((void *)block + size);
		block = block->next_free_ptr;
		count++;
	}
	block->next_free_ptr = NULL;
}

PoolHeader *get_pool_from_cache(unsigned int size_index)
{
    //从pool_cache 里取一个pool
	PoolHeader *new_pool = NULL;
	if (pool_cache) {
		// pool_cache 不为空，从pool_cache里拿
		new_pool = pool_cache;
		pool_cache = pool_cache->next;
	} else {
		if (!fill_pool_cache()) {
			//从系统申请内存失败
			return NULL;
		}
		// pool_cache 不为空，从pool_cache里拿
		new_pool = pool_cache;
		pool_cache = pool_cache->next;
	}

	pool_init(new_pool, size_index);

	return new_pool;
}

void release_pool_to_cache(PoolHeader *pool)
{
    // 当pool里的block全部释放时，释放pool
	if (!pool)
		return;
	pool->state = NOT_IN_USE;
	pool->pre = NULL;

	// 在pool_cache 头部插入pool
	if (!pool_cache) {
		pool_cache = pool;
		pool_cache->next = NULL;
	} else {
		pool->next = pool_cache;
		pool_cache = pool;
	}
}

int find_available_pool(unsigned int size_index)
{
    // 寻找一个某种block尺寸的 可以进行内存分配的pool
	assert(size_index >= 0 && size_index < 32);
	PoolHeader *pool_ptr = mem_index_arr[size_index].pool_lst;
	while (pool_ptr) {
		if (!pool_is_full(pool_ptr)) {
			break;
		}
		pool_ptr = pool_ptr->next;
	}

	if (pool_ptr) {
		// 找到一个有空闲block的pool
		mem_index_arr[size_index].available_pool = pool_ptr;
		return TRUE;
	} else {
		// 当前没有空闲的pool了
		PoolHeader *new_pool = get_pool_from_cache(size_index);
		if (!new_pool) {
			// 无法从pool_cache中后去pool
			mem_index_arr[size_index].available_pool = NULL;
			return FALSE;
		} else {
			//在头部插入new_pool
			PoolHeader *raw_pool_lst =
			    mem_index_arr[size_index].pool_lst;
			mem_index_arr[size_index].pool_lst = new_pool;

			new_pool->next = raw_pool_lst;

			if (raw_pool_lst)
				raw_pool_lst->pre = new_pool;

			mem_index_arr[size_index].available_pool = new_pool;
			return TRUE;
		}
	}
}

BlockHeader *get_block(unsigned int size)
{
    // 获取一个适配size大小的block
	unsigned int block_size = size_up(size); // 对应的block尺寸
	unsigned int size_index = get_size_index(block_size); // 获取size_index

	assert(size_index >= 0 && size_index < 32);

	if (!mem_index_arr[size_index].available_pool) {
		// available_pool指向空，get_block 第一次调用
		if (!find_available_pool(size_index))
			return NULL; // 无法获取一个合适的pool，因为内存申请失败
	}

	if (pool_is_full(mem_index_arr[size_index].available_pool)) {
		// pool 满了
		if (!find_available_pool(size_index))
			return NULL; // 无法获取一个合适的pool，因为内存申请失败
	}

	PoolHeader *available_pool = mem_index_arr[size_index].available_pool;
	// 获取block
	BlockHeader *block = available_pool->free_block;
	available_pool->free_block = (BlockHeader *)(block->next_free_ptr);
	available_pool->used_block_count += 1;

	return block;
}

void release_block_to_pool(BlockHeader *block, PoolHeader *pool)
{
    //释放block
	assert(block && pool);

	BlockHeader *raw_block_lst = pool->free_block;
	pool->free_block = block;
	block->next_free_ptr = raw_block_lst;
	pool->used_block_count -= 1;

	if (pool_is_empty(pool)) {
		//从pool链上卸掉这个pool
		if (!pool->pre) {
			//第一个pool
			if (!pool->next) {
				//只有一个pool
				mem_index_arr[pool->size_index].pool_lst = NULL;
			} else {
				PoolHeader *next_pool = pool->next;
				mem_index_arr[pool->size_index].pool_lst =
				    next_pool;
				next_pool->pre = NULL;
			}
		} else {
			PoolHeader *pre_pool = pool->pre;
			if (!pool->next) {
				pre_pool->next = NULL;
				//最后一个pool
			} else {
				PoolHeader *next_pool = pool->next;
				pre_pool->next = next_pool;
				next_pool->pre = pre_pool;
			}
		}
		find_available_pool(pool->size_index);
		release_pool_to_cache(pool);
	}
}

/*----------------------------接口实现------------------------*/
void simple_install()
{
	// simple malloc 库的初始化接口
	pool_cache = NULL;
	pond_lst = NULL;
	int i;
	for (i = 0; i < MEM_INDEX_NUM; i++) {
		mem_index_arr[i].available_pool = NULL;
		mem_index_arr[i].pool_lst = NULL;
		mem_index_arr[i].size_index = i;
	}
}

void simple_uninstall()
{
	// simple malloc 库的卸载
	// 释放所有的pond
	while (pond_lst) {
		PondHeader *del_pond = pond_lst;
		pond_lst = pond_lst->next_pond;
		free(del_pond);
	}
}

void *simple_malloc(unsigned int size)
{

	if (size > SMALL_MEM_MAX) {
		//超过256，直接申请
		return malloc(size);
	}
	if (size == 0)
		size = ALIGNMENT; // 分配字节不能为0

	BlockHeader *block = get_block(size);
	return block;
}

void simple_free(void *ptr)
{
	if (!ptr)
		return;

	PoolHeader *pool_header = (PoolHeader *)(get_pool_header_addr(ptr));
	if (!is_pool_addr(pool_header)) {
		free(ptr);
	}
	release_block_to_pool((BlockHeader *)ptr, pool_header);
}

void *simple_realloc(void *ptr, unsigned int new_size)
{
    if(ptr==NULL||new_size==0)
        return NULL;
    
    PoolHeader *pool_header = (PoolHeader *)(get_pool_header_addr(ptr));
    
    // judge pool_header
    if (!is_pool_addr(pool_header)) {
        return realloc(ptr, new_size);
    }
    
    unsigned int block_size=get_size(pool_header->size_index);
    
    if(new_size<block_size)
        return ptr;
    
    simple_free(ptr);
    ptr=simple_malloc(new_size);
    return ptr;
}


/*----------------------------接口实现------------------------*/
