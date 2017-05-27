//
//  test.c
//  SimpleSTL
//
//  Created by dacuobi on 17/5/26.

//
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "test.h"
#include "simple_malloc.h"
#include "simple_malloc_internal.h"




static int check_pool_num()
{
    int count=0;
    PoolHeader*ptr=pool_cache;
    while(ptr)
    {
        count++;
        ptr=ptr->next;
    }
    return count;

}

void test_macros()
{
    //测试宏
    assert(size_up(0)==0);
    assert(size_up(1)==8);
    
    assert(get_size_index(256)==31);
    assert(get_size_index(8)==0);
    
    assert(get_size(0)==8);
    assert(get_size(31)==256);
    
    unsigned long block_addr=123;
    assert(get_pool_header_addr(block_addr)==0);
    block_addr=4096;
    assert(get_pool_header_addr(block_addr)==4096);
    block_addr=4096*2+1;
    assert(get_pool_header_addr(block_addr)==4096*2);
    
    // 与pool相关的宏
    PoolHeader *pool=(PoolHeader*)malloc(POOL_SIZE);
    pool->size_index=1;
    pool->state=IN_USE;
    assert(get_block_max_count(pool)==253);
    
    assert(pool_is_empty(pool)==TRUE);
    
    pool->used_block_count=253;
    assert(pool_is_full(pool));
    
    PondHeader*pond=(PondHeader*)malloc(POND_SIZE);
    pond->start_pool = (void *)pond + sizeof(PondHeader);
    unsigned long excess =
    ((unsigned long)pond->start_pool & (unsigned long)POOL_SIZE_MASK);
    if (excess != 0) {
        pond->start_pool =
        (void *)pond->start_pool + POOL_SIZE - excess;
    }
    
    assert(((unsigned long)pond->start_pool & (unsigned long)POOL_SIZE_MASK)==0);
    pond->end_addr=(void*)((unsigned long)pond+sizeof(PondHeader)+3*POOL_SIZE);
    assert(get_pool_max_count(pond)==2);
    
    printf("test_macros success!!\n");
    
}

void test_pond()
{
    //测试pond
    simple_install();
    fill_pool_cache();
    assert(pool_cache==pond_lst->start_pool);
    fill_pool_cache();
    assert(get_pool_max_count(pond_lst)==POND_SIZE/POOL_SIZE-1);
    PoolHeader * ptr=pool_cache;
    PoolHeader * pre=pool_cache;
    int count=0;
    while(ptr)
    {
        if(pre!=ptr)
            assert(((unsigned long)ptr-(unsigned long)pre)==POOL_SIZE);
        
        assert(((unsigned long)ptr&POOL_SIZE_MASK)==0);
        count++;
        pre=ptr;
        ptr=ptr->next;
    }
    assert(count==get_pool_max_count(pond_lst));
    assert(pool_cache==pond_lst->start_pool);
    simple_uninstall();
    
    printf("test_pond success!!\n");

}

void test_pool()
{
    //测试pool机制
    simple_install();
    PoolHeader*pool=get_pool_from_cache(2);
    assert(pool->pre==NULL);
    assert(pool->next==NULL);
    assert(pool->used_block_count==0);
    assert(pool->state==IN_USE);
    
    int count=check_pool_num();
    assert(count==POND_SIZE/POOL_SIZE-2);
    
    release_pool_to_cache(pool);
    assert(pool_cache==pool);
    
    count=check_pool_num();
    assert(count==POND_SIZE/POOL_SIZE-1);
    simple_uninstall();
    
    printf("test_pool success!!\n");
}

void test_find_pool()
{
    //测试find_available_pool
    simple_install();
    find_available_pool(2);
    assert(check_pool_num()==POND_SIZE/POOL_SIZE-2);
    
    PoolHeader*available_pool=mem_index_arr[2].available_pool;
    find_available_pool(2);
    assert(check_pool_num()==POND_SIZE/POOL_SIZE-2);
    assert(available_pool==mem_index_arr[2].available_pool);

    PoolHeader*pool=mem_index_arr[2].pool_lst;
    assert(pool!=NULL);
    assert(pool->size_index==2);
    
    pool->used_block_count=get_block_max_count(pool);
    find_available_pool(2);
    
    assert(check_pool_num()==POND_SIZE/POOL_SIZE-3);
    
    PoolHeader*new_pool=mem_index_arr[2].pool_lst;
    assert(new_pool->next==pool);
    assert(pool->pre==new_pool);
    assert(pool->next==NULL);
    
    assert(new_pool==mem_index_arr[2].available_pool);

    simple_uninstall();
    
    printf("test_find_pool success!!\n");
}


void test_block()
{
    //测试block 机制
    simple_install();
    BlockHeader*b1=get_block(11);
    BlockHeader*b2=get_block(12);
    BlockHeader*b3=get_block(13);

    assert((unsigned long)b1+get_size(1)==(unsigned long)b2);
    assert((unsigned long)b2+get_size(1)==(unsigned long)b3);
    
    assert(mem_index_arr[1].available_pool!=NULL);
    PoolHeader*pool=mem_index_arr[1].available_pool;
    assert(pool->free_block==b3->next_free_ptr);
    
    release_block_to_pool(b2, pool);
    assert(pool->free_block==b2);
    
    release_block_to_pool(b3, pool);
    assert(pool->free_block==b3);
    assert(b3->next_free_ptr==b2);

    simple_uninstall();
    
    printf("test_block success!!\n");
}

void test_interface()
{
    //测试接口
    simple_install();
    void * p = simple_malloc(11);
    void * newp= simple_realloc(p, 12);
    assert(p==newp);
    simple_free(p);
    simple_uninstall();
    
    printf("test_interface success!!\n");
    
}



