#ifndef MPOOL_H
#define MPOOL_H

#include <string.h>
#include <stdlib.h>

/*
* add by hc
* 2019.05.28
*/

#ifdef  __cplusplus
extern "C"{
#endif

typedef struct MemBlock_s
{
    void* memory;
    int is_used;
    int memory_size;
}MemBlock_t;

typedef struct LineTable_s{
    void    **_store;
    int     _cap;
} LineTable_t;

#define MemoryBlock_New() (MemBlock_t*)malloc(sizeof(MemBlock_t))

#define MemoryBlockMemset(MEM) memset((void*)MEM, 0, sizeof(MemBlock_t))

#define Memory_New(LT,SZ) \
{ \
    LT->memory_size = SZ; LT->memory = (void*)malloc(SZ); \
}

#define Linear_New() (LineTable_t*)malloc(sizeof(LineTable_t))

#define Linear(LT,SZ) \
{ \
    LT->_cap = SZ; LT->_store = (void**)calloc(SZ,sizeof(MemBlock_t*)); \
}

#define _Extend(LT) \
{ \
    int _cap = LT->_cap * 2; \
    LT->_store = (void**)realloc(LT->_store,sizeof(MemBlock_t*)*_cap); \
    if(LT->_store != NULL){ \
       LT->_cap = _cap; \
    } \
}

#if 0
#define Line_Append(LT,V)\
{ \
    _Extend(LT); \
    LT->_store[ LT->_size++] = V; \
}

#define _Extend(LT) \
{ \
    int _cap = LT->_cap * 3; \
    LT->_store = (void**)realloc(LT->_store,sizeof(void**)*_cap); \
    if(LT->_store != NULL){ \
       LT->_cap = _cap; \
    } \
}
#endif

#define Line_Get(LT,I)      (LT->_store[I])

#define Line_Set(LT,I,V)    (LT->_store[I] = V)

#if 0
typedef struct
{
    void        *_first;
    int         _cap;
    int         _bsz;
    int         _size;
    LineTable_t   *_slots;
}Block;

Block*  pool_create(int bsz, int count);
void*   pool_malloc(Block* blk);
void    pool_free(Block* blk, void **ptr);
void    pool_release(Block *blk);
#endif

typedef struct HcMemPool_s
{
    LineTable_t *_slots;
}HcMemPool_t;

HcMemPool_t* HcMemPoolCreate(int capsize);

int HcMemPoolDestory(HcMemPool_t* pool);

void* HcMemPoolMemoryMalloc(HcMemPool_t* pool, int size);

int HcMemPoolMemoryFree(HcMemPool_t* pool, void* ptr);

#ifdef  __cplusplus
}
#endif

#endif // MPOOL_H
