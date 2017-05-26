//
//  simple_malloc_internal.h
//  SimpleSTL
//
//  Created by dacuobi on 17/5/26.
//  Copyright © 2017年 dacuobi. All rights reserved.
//

#ifndef simple_malloc_internal_h
#define simple_malloc_internal_h
#define ALIGNMENT 8
#define ALIGN_MASK 7
#define ALIGN_SHIFT 3

#define SMALL_MEM_MAX 256
#define MEM_INDEX_NUM 32

#define POOL_SIZE 4096 // 4kb
#define POOL_SIZE_MASK 4095

#define POND_SIZE (256 << 10) // 256kb

// pool 的状态
#define IN_USE 1
#define NOT_IN_USE 0

// block 按block_size向上对齐
#define size_up(n) (((n) + ALIGN_MASK) & ~(unsigned int)ALIGN_MASK)

// 通过block size 获取mem index
#define get_size_index(size) (((size)-1) >> ALIGN_SHIFT)

// 通过 mem index 获取 block size
#define get_size(size_index) (((size_index) + 1) << ALIGN_SHIFT)

// pool 地址按pool_size 向下对齐
#define get_pool_header_addr(block_addr)                                       \
(((unsigned long)(block_addr)) & ~(unsigned long)POOL_SIZE_MASK)

//获取pool可容纳的block数
#define get_block_max_count(pool)                                              \
((POOL_SIZE - sizeof(PoolHeader)) / (get_size((pool)->size_index)))

//判断pool是否已满
#define pool_is_full(pool)                                                     \
((pool)->used_block_count == get_block_max_count((pool)))

//判断pool内block是否已全部被释放
#define pool_is_empty(pool)                                                    \
((!(pool)->used_block_count) && (pool->state == IN_USE))

//通过pool_header的内容，判断pool的地址是否合法
#define is_pool_addr(pool)                                                     \
((pool) && ((pool)->state == IN_USE) &&                                \
((pool)->used_block_count <= get_block_max_count((pool))) &&           \
((pool)->size_index > 0) && ((pool)->size_index < MEM_INDEX_NUM) &&   \
(mem_index_arr[(pool)->size_index].size_index == (pool)->size_index))

//计算pond内含有pool数目的上限
#define get_pool_max_count(pond)                                               \
((((pond)->end_addr) - ((void *)(pond)->start_pool)) / POOL_SIZE)

typedef union BlockHeader {
    // 实际分配的内存单元，按8字节对齐，大小取值只能为8，16，.....256
    union BlockHeader *next_free_ptr;
    unsigned char data[0];
} BlockHeader;

#pragma pack(push, format, 8)
typedef struct PoolHeader {
    // pool:block的集合,一个pool内只能有一种大小的block
    // 一个pool的内存大小是4kb
    
    struct PoolHeader *pre;
    struct PoolHeader *next;
    unsigned int
    size_index; // 标识pool挂在哪个memindex下，也就指示了pool内block的大小
    unsigned int used_block_count;
    unsigned int state;
    BlockHeader *free_block;
    
} PoolHeader;

typedef struct PondHeader {
    // pond:pool的集合
    struct PondHeader *next_pond;
    PoolHeader *start_pool;
    void*end_addr;
} PondHeader;

typedef struct MemIndex {
    //含有某种大小block的pool的索引
    PoolHeader *pool_lst;
    PoolHeader *available_pool;
    unsigned int
    size_index; //指示了在mem_index_arr中的位置，指示了所属pool内block的位置
    
} MemIndex;
#pragma pack(pop)


/*-------------------内部函数声明----------------------------*/
#ifdef DEBUG
BlockHeader *get_block(unsigned int size);
void release_block_to_pool(BlockHeader *block,PoolHeader *pool);
int find_available_pool(unsigned int size_index);
PoolHeader *get_pool_from_cache(unsigned int size_index);
void release_pool_to_cache(PoolHeader *pool);
void pond_init();
int fill_pool_cache();
void pool_init(PoolHeader *pool, unsigned int size_index);

extern MemIndex mem_index_arr[MEM_INDEX_NUM]; // 8,16,.....256
extern PoolHeader *pool_cache;		      // pool 的缓存
extern PondHeader *pond_lst;

#else
static BlockHeader *get_block(unsigned int size);
static void release_block_to_pool(BlockHeader *block,PoolHeader *pool);
static int find_available_pool(unsigned int size_index);
static PoolHeader *get_pool_from_cache(unsigned int size_index);
static void release_pool_to_cache(PoolHeader *pool);
static void pond_init();
static int fill_pool_cache();
static void pool_init(PoolHeader *pool, unsigned int size_index);
#endif
/*--------------------------------------------------------*/


#endif /* simple_malloc_internal_h */
