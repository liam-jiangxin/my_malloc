#include "MemPool.h"
#include<iostream>
using namespace std;
//这个是Chunk结构体大小
#define MP_CHUNKHEADER sizeof(struct _mp_chunk)
//这个是Chunk结构体指针大小
#define MP_CHUNKEND sizeof(struct _mp_chunk*)

#define MP_lock(lockobj) pthread_mutex_lock(&lockobj->lock); 
#define MP_unlock(lockobj) pthread_mutex_unlock(&lockobj->lock); 

//对齐，使_n变成sizeof(int)的倍数
#define MP_ALIGN_SIZE(_n) (_n + sizeof(int) - ((sizeof(int) - 1) & _n))

//初始化_MP_mem_pool_list，对pool进行操作
void MP_INIT_MEMORY_STRUCT(_MP_Memory *mm, mem_size_t sz){
        mm->pool_size = sz;
        mm->alloced_mem = 0;
        mm->alloc_prog_mem = 0;
        mm->free_list = (_MP_Chunk*) mm->start;
        mm->free_list->is_free = 1;
        mm->free_list->alloced_mem = sz;
        mm->free_list->prev = NULL;
        mm->free_list->next = NULL;
        mm->alloc_list = NULL;   
}

// 把x插到head之前
void MP_DLINKLIST_INS_FRT(_MP_Chunk * &head, _MP_Chunk * &x){
        x->prev = NULL;
        x->next = head;
        if (head) head->prev = x;
        head = x;
}


//删除节点x
void MP_DLINKLIST_DEL (_MP_Chunk * &head, _MP_Chunk * &x){
            if (!x->prev) {
        head = x->next;
        if (x->next) x->next->prev = NULL;
    } else {
        x->prev->next = x->next;
        if (x->next) x->next->prev = x->prev;
    }
}


//功能：得到MemoryPool的pool的数量
void get_memory_list_count(MemoryPool* mp, mem_size_t* mlist_len) {
//如果并行，要加锁
#ifdef MULTI_THREAD
    MP_lock(mp);
#endif
    mem_size_t mlist_l = 0;
    _MP_Memory* mm = mp->mlist;
    while (mm) {
        mlist_l++;
        mm = mm->next;
    }
    *mlist_len = mlist_l;
//解锁
#ifdef MULTI_THREAD
    MP_unlock(mp);
#endif
}

//功能：得到MemoryPool的某一个pool的空闲块数量和已分配块的数量
void get_memory_info(MemoryPool* mp,
                     _MP_Memory* mm,
                     mem_size_t* free_list_len,
                     mem_size_t* alloc_list_len) {
#ifdef MULTI_THREAD
    MP_lock(mp);
#endif
    mem_size_t free_l = 0, alloc_l = 0;
    _MP_Chunk* p = mm->free_list;
    while (p) {
        free_l++;
        p = p->next;
    }

    p = mm->alloc_list;
    while (p) {
        alloc_l++;
        p = p->next;
    }
    *free_list_len = free_l;
    *alloc_list_len = alloc_l;
#ifdef MULTI_THREAD
    MP_unlock(mp);
#endif
}

//功能：输出某一个pool的id
int get_memory_id(_MP_Memory* mm) {
    return mm->id;
}

//功能：申请一个pool
static _MP_Memory* extend_memory_list(MemoryPool* mp, mem_size_t new_mem_sz) {
    //这里注意，要加一个pool的描述结构体的大小
    char* s = (char*) malloc(sizeof(_MP_Memory) + new_mem_sz * sizeof(char));
    if (!s) return NULL;

    _MP_Memory* mm = (_MP_Memory*) s;
    //这个pool的开始处是在描述结构体之后
    mm->start = s + sizeof(_MP_Memory);
    //初始化一个pool
    MP_INIT_MEMORY_STRUCT(mm, new_mem_sz);
    mm->id = mp->last_id++;
    //然后把这个pool加到MemoryPool中pool链表的头部
    mm->next = mp->mlist;
    mp->mlist = mm;
    //返回新申请的一个pool
    return mm;
}

//功能：找到需要的p对应的内存地址所在的pool
static _MP_Memory* find_memory_list(MemoryPool* mp, void* p) {
    //遍历MemoryPool的poollist
    _MP_Memory* tmp = mp->mlist;
    while (tmp) {
        //如果p夹在这个pool开头和结尾的中间
        if (tmp->start <= (char*) p &&
            tmp->start + mp->pool_size > (char*) p)
            break;
        tmp = tmp->next;
    }

    return tmp;
}

//功能：free一个Chunk,并和前后合并
static int merge_free_chunk(MemoryPool* mp, _MP_Memory* mm, _MP_Chunk* c) {
    //c的位置是chunk组织方式的指针的位置
    _MP_Chunk *p0 = c, *p1 = c;
    //找到c的最前方的free块
    while (p0->is_free) {
        p1 = p0;
        //这个pool里的chunk的组织方式是这样的
        /*
        //chunk头的结构体->分配的数据->指向chunk头结构体的指针
        */
        //这个组织方式就在这里用到了。因为Chunk是用freelist和alloclist两个链表组织起来的，没法按内存上的次序遍历
        //这里通过这个组织方式，实现了在物理内存上的反向遍历
        if ((char*) p0 - MP_CHUNKEND  - MP_CHUNKHEADER <= mm->start) break;
        //p0的值是Chunk头的地址，减去MP_CHUNKEND就是指向上一个Chunk的指针的地址了。通过强制类型转换，就可以赋值了
        p0 = *(_MP_Chunk**) ((char*) p0 - MP_CHUNKEND);
    }
    //从最前面的p1向后遍历，合并每个free
    p0 = (_MP_Chunk*) ((char*) p1 + p1->alloced_mem);
    while ((char*) p0 < mm->start + mp->pool_size && p0->is_free) {
        MP_DLINKLIST_DEL(mm->free_list, p0);
        p1->alloced_mem += p0->alloced_mem;
        //下一块结构体的地址，转化为指针
        p0 = (_MP_Chunk*) ((char*) p0 + p0->alloced_mem);
    }

    *(_MP_Chunk**) ((char*) p1 + p1->alloced_mem - MP_CHUNKEND) = p1;
//这里只解锁，因为后面调用merge_free_chunk的函数MemoryPoolFree会加锁
#ifdef MULTI_THREAD
    MP_unlock(mp);
#endif
    return 0;
}
//功能：初始化MemoryPool
MemoryPool* MemoryPoolInit(mem_size_t maxmempoolsize, mem_size_t mempoolsize) {
    if (mempoolsize > maxmempoolsize) {
        cout<<"MemoryPool_Init MemPool Init ERROR： Mempoolsize is too big!\n"<<endl;
        return NULL;
    }
    //先分配一个MemPool结构体的大小
    MemoryPool* mp = (MemoryPool*) malloc(sizeof(MemoryPool));
    if (!mp) return NULL;
    //进行一些信息初始化
    mp->last_id = 0;
    if (mempoolsize < maxmempoolsize) mp->auto_extend = 1;
    mp->max_pool_size = maxmempoolsize;
    mp->total_pool_size = mp->pool_size = mempoolsize;
    //如果支持多线程，则初始化锁
#ifdef MULTI_THREAD
    pthread_mutex_init(&mp->lock, NULL);
#endif
    //对于每一个pool，顺序是这样的
    //控制结构体_MP_Memory -> 数据
    char* s = (char*) malloc(sizeof(_MP_Memory) +
                             sizeof(char) * mp->pool_size);
    if (!s) return NULL;
    //对第一个pool进行信息的初始化
    mp->mlist = (_MP_Memory*) s;
    mp->mlist->start = s + sizeof(_MP_Memory);
    MP_INIT_MEMORY_STRUCT(mp->mlist, mp->pool_size);
    mp->mlist->next = NULL;
    mp->mlist->id = mp->last_id++;

    return mp;
}

//功能：内存分配
void* MemoryPoolAlloc(MemoryPool* mp, mem_size_t wantsize) {
    if (wantsize <= 0) return NULL;
    //由于chunk组织方式的特点，要加上控制结构体和控制结构体指针，并让大小和int对齐
    mem_size_t total_needed_size =
            MP_ALIGN_SIZE(wantsize + MP_CHUNKHEADER + MP_CHUNKEND);
    if (total_needed_size > mp->pool_size) return NULL;

    _MP_Memory *mm = NULL, *mm1 = NULL;
    //_free和_not_free是chunk
    _MP_Chunk *_free = NULL, *_not_free = NULL;
#ifdef MULTI_THREAD
    MP_lock(mp);
#endif

    //TODO:把这个goto优化掉
FIND_FREE_CHUNK:
    mm = mp->mlist;
    while (mm) {
        //如果这个pool大小不够，那么就找下一个pool
        if (mp->pool_size - mm->alloced_mem < total_needed_size) {
            mm = mm->next;
            continue;
        }
        //遍历这个pool的free_list中的chunk,算法是first_fit
        _free = mm->free_list;
        _not_free = NULL;

        while (_free) {
            //可以进行分配
            if (_free->alloced_mem >= total_needed_size) {

                // 如果free块分割后剩余内存足够大，大到足以容纳另外一组MP_CHUNKHEADER和MP_CHUNKEND，即分配后可以剩下一个free_chunk，则进行分割
                if (_free->alloced_mem - total_needed_size >MP_CHUNKHEADER + MP_CHUNKEND) {
                    // 从free块头开始分割出alloc块
                    _not_free = _free;
                    //把free的值移到total_needed_size后
                    _free = (_MP_Chunk*) ((char*) _not_free +
                                          total_needed_size);

                    //这里有点难理解
                    //这个pool里的chunk的组织方式是这样的
                    /*
                    //chunk头的结构体->分配的数据->指向chunk头结构体的指针
                    */
                    //所以这里要把以前_free的控制结构体移到后面去
                    *_free = *_not_free;
                    //并更新这个chunk所对应的可分配内存大小
                    _free->alloced_mem -= total_needed_size;
                    //要把指针存到对应的位置
                    *(_MP_Chunk**) ((char*) _free + _free->alloced_mem -
                                    MP_CHUNKEND) = _free;

                    //更新free_list
                    if (!_free->prev) {
                        mm->free_list = _free;
                    } else {
                        _free->prev->next = _free;
                    }
                    if (_free->next) _free->next->prev = _free;
                    //对已经分配的区域进行一些初始化
                    _not_free->is_free = 0;
                    _not_free->alloced_mem = total_needed_size;
                    //同样，更新指针
                    *(_MP_Chunk**) ((char*) _not_free + total_needed_size -
                                    MP_CHUNKEND) = _not_free;
                }
                // 不够,说明当前chunk分配后，剩余内存不足以容纳一个chunk所需要的结构体和结构体指针。则整块分配为alloc
                else {
                    _not_free = _free;
                    //从freelist里面删除
                    MP_DLINKLIST_DEL(mm->free_list, _not_free);
                    _not_free->is_free = 0;
                }
                //把分配后的内存加到alloc_list里面去
                MP_DLINKLIST_INS_FRT(mm->alloc_list, _not_free);
                //修改已分配内存和有效分配内存
                mm->alloced_mem += _not_free->alloced_mem;
                mm->alloc_prog_mem +=(_not_free->alloced_mem - MP_CHUNKHEADER - MP_CHUNKEND);
#ifdef MULTI_THREAD
                MP_unlock(mp);
#endif
                //分配结束，退出
                return (void*) ((char*) _not_free + MP_CHUNKHEADER);
            }
            //继续遍历当前pool的free_list
            _free = _free->next;
        }
        //继续遍历pool
        mm = mm->next;
    }
    //如果所有pool里的free_chunk都不能容纳，则判断是否能够延申内存，申请另一块pool
    if (mp->auto_extend) {
        // 超过总内存限制
        if (mp->total_pool_size >= mp->max_pool_size) {
            cout<<"MemoryPool_Alloc: No enough memory! \n"<<endl;
            #ifdef MULTI_THREAD
                MP_unlock(mp);
            #endif
                return NULL;
        }
        //申请一块新的内存
        mem_size_t add_mem_sz = mp->max_pool_size - mp->pool_size;
        add_mem_sz = add_mem_sz >= mp->pool_size ? mp->pool_size
                                                     : add_mem_sz;
        mm1 = extend_memory_list(mp, add_mem_sz);
        //也可能失败
        if (!mm1) {
            cout<<"MemoryPool_Alloc: No enough memory! \n"<<endl;
            #ifdef MULTI_THREAD
                MP_unlock(mp);
            #endif
                return NULL;
        }
        mp->total_pool_size += add_mem_sz;
        //申请后，重新来找一块free_chunk
        goto FIND_FREE_CHUNK;
    }


}
//功能：释放某个已分配的指针p
int MemoryPoolFree(MemoryPool* mp, void* p) {
    if (p == NULL || mp == NULL) return 1;
#ifdef MULTI_THREAD
    MP_lock(mp);
#endif
    _MP_Memory* mm = mp->mlist;
    //找到p所属的pool
    if (mp->auto_extend) mm = find_memory_list(mp, p);
    //找到p所属的chunk的控制结构体
    _MP_Chunk* ck = (_MP_Chunk*) ((char*) p - MP_CHUNKHEADER);
    //从alloc_list删除这一块
    MP_DLINKLIST_DEL(mm->alloc_list, ck);
    //把这一块加到free_list头部
    MP_DLINKLIST_INS_FRT(mm->free_list, ck);

    //修改相关信息
    ck->is_free = 1;

    mm->alloced_mem -= ck->alloced_mem;
    mm->alloc_prog_mem -= (ck->alloced_mem - MP_CHUNKHEADER - MP_CHUNKEND);
    //merge一下，注意传入的ck位置位于chunk组织的最后一项，即指针位置
    return merge_free_chunk(mp, mm, ck);
}
//功能:清空MemoryPool里面的pool
MemoryPool* MemoryPoolClear(MemoryPool* mp) {
    if (!mp) return NULL;
#ifdef MULTI_THREAD
    MP_lock(mp);
#endif
    _MP_Memory* mm = mp->mlist;
    //把每一块都初始化
    while (mm) {
        MP_INIT_MEMORY_STRUCT(mm, mm->pool_size);
        mm = mm->next;
    }
#ifdef MULTI_THREAD
    MP_unlock(mp);
#endif
    return mp;
}

//功能：删除MemoryPool
int MemoryPoolDestroy(MemoryPool* mp) {
    if (mp == NULL) return 1;
#ifdef MULTI_THREAD
    MP_lock(mp);
#endif
    //把每一个pool都free掉
    _MP_Memory *mm = mp->mlist, *mm1 = NULL;
    while (mm) {
        mm1 = mm;
        mm = mm->next;
        free(mm1);
    }
#ifdef MULTI_THREAD
    MP_unlock(mp);
    pthread_mutex_destroy(&mp->lock);
#endif
    free(mp);
    return 0;
}

//MemoryPool总内存
mem_size_t GetTotalMemory(MemoryPool* mp) {
    return mp->total_pool_size;
}

//遍历pool，获得有效分配内存（加上chunk头和chunk头指针的）
mem_size_t GetUsedMemory(MemoryPool* mp) {
#ifdef MULTI_THREAD
    MP_lock(mp);
#endif
    mem_size_t total_alloc = 0;
    _MP_Memory* mm = mp->mlist;
    while (mm) {
        total_alloc += mm->alloced_mem;
        mm = mm->next;
    }
#ifdef MULTI_THREAD
    MP_unlock(mp);
#endif
    return total_alloc;
}
//遍历pool，获得有效分配内存（没加上chunk头和chunk头指针的）
mem_size_t GetProgMemory(MemoryPool* mp) {
#ifdef MULTI_THREAD
    MP_lock(mp);
#endif
    mem_size_t total_alloc_prog = 0;
    _MP_Memory* mm = mp->mlist;
    while (mm) {
        total_alloc_prog += mm->alloc_prog_mem;
        mm = mm->next;
    }
#ifdef MULTI_THREAD
    MP_unlock(mp);
#endif
    return total_alloc_prog;
}
//两个比例
float MemoryPoolGetUsage(MemoryPool* mp) {
    return (float) GetUsedMemory(mp) / GetTotalMemory(mp);
}

float MemoryPoolGetProgUsage(MemoryPool* mp) {
    return (float) GetProgMemory(mp) / GetTotalMemory(mp);
}

#undef MP_CHUNKHEADER
#undef MP_CHUNKEND
#undef MP_ALIGN_SIZE
