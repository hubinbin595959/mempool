#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>
#include "mem-pool.h"

/*the lock for mempool operation*/
pthread_mutex_t g_mempool_lock_ = PTHREAD_MUTEX_INITIALIZER;

#if 0
#define     Inc_Ptr(PTR,N)            ((char*)PTR+N)
#define     Dec_Ptr(PTR,N)            ((char*)PTR-N)
#define     nextof(Ptr,BSZ)           (*(void**)Inc_Ptr(Ptr,BSZ))

Block *pool_create(int bsz, int count) {
    Block   *res = (Block *)malloc(sizeof(Block));
    int     realsize = bsz + sizeof(void *);
    void    *_Mem = (void *)calloc(count, realsize);
    int i;
    res->_slots = Linear_New();
    Linear(res->_slots, 10);
    res->_first = _Mem;
    res->_cap = count;
    res->_size = count;
    res->_bsz = bsz;
    for (i = 0; i < count - 1; ++i) {
        *(void **)Inc_Ptr(_Mem , i * realsize + bsz) = Inc_Ptr(_Mem, (i + 1) * realsize);
    }
    Line_Append(res->_slots, _Mem);
    return res;
}

void extend(Block *blk, int sz, int bsz) {
    void *_Mem = NULL;
    int i = 0, realsize = bsz + sizeof(void *);
    _Mem = calloc(sz, bsz + sizeof(void *));
    Line_Append(blk->_slots, _Mem);
    blk->_first = _Mem;
    for (; i < sz - 1; ++i ) {
        *(void **)Inc_Ptr(_Mem , i * realsize + bsz) = Inc_Ptr(_Mem, (i + 1) * realsize);
    }
}

void *pool_malloc(Block *blk) {
    void *res = NULL;
    res = blk->_first;
    if (res == NULL) {
        int sz = blk->_cap * 1.5;
        extend(blk, sz, blk->_bsz);
        blk->_size = sz;
        blk->_cap += sz;
        res = blk->_first;
    }
    blk->_first = nextof(blk->_first, blk->_bsz);
    --blk->_size;
    return res;
}



void pool_free(Block *blk, void **ptr) {
    if (*ptr == NULL) return;
    *(char **)((char *)(*ptr) + blk->_bsz) = blk->_first;
    blk->_first = (*ptr);
    *ptr = NULL;
}


void pool_release(Block *blk) {
    int i = 0, sz = blk->_slots->_size;
    for (; i < sz; ++i )
        free(Line_Get(blk->_slots, i));

    free(blk->_slots->_store);
    free(blk->_slots);
}
#endif

void printtime(char *buf) {
    char buflog[1024] = {0};
    unsigned long millisec = 0;
    struct timeval tv;
    struct tm tm_info;
    gettimeofday(&tv, NULL);
    millisec = tv.tv_usec / 1000;
    localtime_r(&tv.tv_sec, &tm_info);

    sprintf(buflog, "%s", buf);
    printf("[%4d-%02d-%02d %02d:%02d:%02d:%03ld]: %s\n", tm_info.tm_year + 1900, tm_info.tm_mon + 1, tm_info.tm_mday, tm_info.tm_hour, tm_info.tm_min, tm_info.tm_sec, millisec, buflog);
}

unsigned long getTickCount() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

HcMemPool_t *HcMemPoolCreate(int capsize) {
    int count = 0;
    MemBlock_t *memory = NULL;
    HcMemPool_t *pool = (HcMemPool_t *)malloc(sizeof(HcMemPool_t));
    if (!pool) {
        return NULL;
    }

    pool->_slots = Linear_New();
    if (!pool->_slots) {
        return NULL;
    }


    if (capsize <= 0)capsize = 10;
    Linear(pool->_slots, capsize);
    for (count = 0; count < pool->_slots->_cap; count++) {
        memory = MemoryBlock_New();
        MemoryBlockMemset(memory);
        Line_Set(pool->_slots, count, memory);
    }
    return pool;
}

int HcMemPoolDestory(HcMemPool_t *pool) {
    int count = 0;
    MemBlock_t *memory = NULL;
    if (!pool)return -1;
    for (count = 0; count < pool->_slots->_cap; count++) {
        memory = Line_Get(pool->_slots, count);
        if (memory) {
            if(memory->memory)free(memory->memory);
            free(memory);
        }
        memory = NULL;
        Line_Set(pool->_slots, count, NULL);
    }
    free(pool->_slots);
    free(pool);
    return 0;
}

void *HcMemPoolMemoryMalloc(HcMemPool_t *pool, int size) {
    int count = 0, found = 0, oldcapsize = 0;
    MemBlock_t *memory = NULL;
    if (!pool) {
        return NULL;
    }

    pthread_mutex_lock(&g_mempool_lock_);

    for (count = 0; count < pool->_slots->_cap; count++) {
        memory = Line_Get(pool->_slots, count);
        if (memory) {
            if (memory->is_used == 0 && memory->memory_size >= size) {
                found = 1;
                printf("found an unused memoryblock size: %d\n", memory->memory_size);
                break;
            } else if (memory->is_used == 0 && memory->memory_size == 0) {
                found = 1;
                printf("found an unalloc memoryblock need malloc size: %d\n", size);
                Memory_New(memory, size);
                break;
            }
        }
    }
    if (!found) {
        oldcapsize = pool->_slots->_cap;
        printf("%d memoryblock not found an good memory, extend the pool count: %d\n", oldcapsize, count); 
        _Extend(pool->_slots);
        for (count = oldcapsize; count < pool->_slots->_cap; count++) {
            memory = MemoryBlock_New();
            MemoryBlockMemset(memory);
            Line_Set(pool->_slots, count, memory);
        }
        memory = Line_Get(pool->_slots, oldcapsize);
        Memory_New(memory, size);
        memory->is_used = 1;
    } else {
        memory->is_used = 1;
    }

    pthread_mutex_unlock(&g_mempool_lock_);

    return memory->memory;
}

int HcMemPoolMemoryFree(HcMemPool_t *pool, void *ptr) {
    int count = 0, found = 0;
    MemBlock_t *memory = NULL;
    if (!pool) {
        return -1;
    }

    pthread_mutex_lock(&g_mempool_lock_);
    for (count = 0; count < pool->_slots->_cap; count++) {
        memory = Line_Get(pool->_slots, count);
        if (memory) {
            if (memory->memory == ptr) {
                found = 1;
                memory->is_used = 0;
                printf("found the memoryblok set it unused\n");
                break;
            }
        }
    }
    if(!found)printf("not found the memorybloc to free\n");
    pthread_mutex_unlock(&g_mempool_lock_);
    return found == 1 ? 0 : -1;
}

void HcMemPoolPrintStatus(HcMemPool_t *pool){
    int count = 0;
    MemBlock_t *memory = NULL;
    if (!pool) {
        return -1;
    }
    printf("==================pool status start====================\n");
    pthread_mutex_lock(&g_mempool_lock_);
    for (count = 0; count < pool->_slots->_cap; count++) {
        memory = Line_Get(pool->_slots, count);
        if (memory) {
            printf("memory size: %d\n", memory->memory_size);
            printf("memory is_used: %d\n", memory->is_used);
            printf("memory address: %p\n", memory->memory);
        }
    }
    printf("==================pool status end====================\n");
    pthread_mutex_unlock(&g_mempool_lock_);
}

#if 1
int main(int argc, char *argv[]) {
    char* ptr1 = NULL;
    char* ptr2 = NULL;
    int random = 0;
    timer_t start = 0, end = 0;
    HcMemPool_t* pool = HcMemPoolCreate(10);
    HcMemPoolPrintStatus(pool);

    while(1)
    {
        srand(time(NULL));
        random = (int)((1024) * (rand() / (RAND_MAX + 1.0)));
        printf("===rand: %d===\n", random);

        //ptr1 = HcMemPoolMemoryMalloc(pool, 1024);
        //printtime("pool malloc start...");
        start = getTickCount();
        ptr2 = HcMemPoolMemoryMalloc(pool, random);
        HcMemPoolMemoryFree(pool, ptr2);
        //printtime("pool malloc end...");
        end = getTickCount();

        printf("mempool diff time: %ld\n", end-start);

        //printtime("malloc start...");
        //printtime("malloc startex...");
        start = getTickCount();
        ptr1 = (char*)malloc(random);
        free(ptr1);
        end = getTickCount();

        printf("malloc diff time: %ld\n", end-start);

        //printtime("malloc end...");
        //printtime("malloc endex...");

        /*HcMemPoolPrintStatus(pool);
        HcMemPoolMemoryFree(pool, 0x21212313);
        HcMemPoolMemoryFree(pool, ptr1);
        HcMemPoolPrintStatus(pool);
        sleep(1);*/
        usleep(50000);
    }
    return 0;
}
#endif
