
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>


static ngx_inline void *ngx_palloc_small(ngx_pool_t *pool, size_t size,
    ngx_uint_t align);
static void *ngx_palloc_block(ngx_pool_t *pool, size_t size);
static void *ngx_palloc_large(ngx_pool_t *pool, size_t size);


ngx_pool_t *
ngx_create_pool(size_t size, ngx_log_t *log)
{
    ngx_pool_t  *p;
     /* NGX_POOL_ALIGNMENT在core/ngx_palloc.h中宏定义为16
     * ngx_memalign的作用就是将size进行16字节的对齐并分配空间(内部调用memalign对齐并分配空间)
     * malloc/realloc返回的内存地址都是以8字节对齐,如果想要更大粒度的对齐,则可以调用memalign函数
     * 还有一个valloc函数,以页的大小作为对齐长度,内部实现通过memalign实现
     */ 
    p = ngx_memalign(NGX_POOL_ALIGNMENT, size, log);
    if (p == NULL) {
        return NULL;
    }

    /* 初始化管理小块内存池的结构体 */
    //last成员,指向未分配的空闲内存的首地址
    //ngx_pool_t结构体要占一定的空间
    p->d.last = (u_char *) p + sizeof(ngx_pool_t);
    p->d.end = (u_char *) p + size;//尾部
    p->d.next = NULL;//初始化next指针
    p->d.failed = 0; //每次分配失败时会加1,超过4则会将current指针移向下一个小块内存池

    //除去管理内存池的代价之外(ngx_pool_t)真正可用的内存大小
    size = size - sizeof(ngx_pool_t); 

    //当size 符合 小块内存的大小标准时, 其值就为size
    //否则值为NGX_MAX_ALLOC_FROM_POOL,证明其是大块内存
    p->max = (size < NGX_MAX_ALLOC_FROM_POOL) ? size : NGX_MAX_ALLOC_FROM_POOL;

    p->current = p;//指向当前的内存池

    //初始化large、cleanup、log
    p->chain = NULL;
    p->large = NULL;
    p->cleanup = NULL;
    p->log = log;

    return p;
}


void
ngx_destroy_pool(ngx_pool_t *pool)
{
    ngx_pool_t          *p, *n;
    ngx_pool_large_t    *l;
    ngx_pool_cleanup_t  *c;

    //遍历cleanup链表,调用handler
    for (c = pool->cleanup; c; c = c->next) {
        if (c->handler) {
            ngx_log_debug1(NGX_LOG_DEBUG_ALLOC, pool->log, 0,
                           "run cleanup: %p", c);
            c->handler(c->data);
        }
    }

#if (NGX_DEBUG)

    /*
     * we could allocate the pool->log from this pool
     * so we cannot use this log while free()ing the pool
     */
     //遍历large链表,释放大块内存
    for (l = pool->large; l; l = l->next) {
        ngx_log_debug1(NGX_LOG_DEBUG_ALLOC, pool->log, 0, "free: %p", l->alloc);
    }

    for (p = pool, n = pool->d.next; /* void */; p = n, n = n->d.next) {
        ngx_log_debug2(NGX_LOG_DEBUG_ALLOC, pool->log, 0,
                       "free: %p, unused: %uz", p, p->d.end - p->d.last);

        if (n == NULL) {
            break;
        }
    }

#endif

    for (l = pool->large; l; l = l->next) {
        if (l->alloc) {
            ngx_free(l->alloc);
        }
    }
    //遍历小块内存池形成的链表,逐个释放
    for (p = pool, n = pool->d.next; /* void */; p = n, n = n->d.next) {
        ngx_free(p);

        if (n == NULL) {
            break;
        }
    }
}

//将大块内存释放还给操作系统,小块内存不释放而是复用
//(重置ngx_pool_data_t结构体中的last成员)。
void
ngx_reset_pool(ngx_pool_t *pool)
{
    ngx_pool_t        *p;
    ngx_pool_large_t  *l;

    //遍历large链表,释放申请的大块内存
    for (l = pool->large; l; l = l->next) {
        if (l->alloc) {
            ngx_free(l->alloc);
        }
    }

    //遍历小块内存池链表,重置last
    for (p = pool; p; p = p->d.next) {
        p->d.last = (u_char *) p + sizeof(ngx_pool_t);
        p->d.failed = 0;
    }
    //指向当前的内存池
    pool->current = pool;
    pool->chain = NULL;
    pool->large = NULL;
}

/**
     ngxin的内存池提供了4个接口供用户获取内存。分别是:ngx_palloc、ngx_pnalloc、ngx_pcalloc、
    ngx_pmemalign。
    前三个函数代码的实现都很类似,只是ngx_palloc会分配地址对齐的内存,而ngx_pnalloc分配内存时
    不会进行地址对齐,ngx_pcalloc分配地址对齐的内存后再调用memset进行清0。
    而ngx_pmemalign则按照传入的参数alignment进行地址对齐分配内存,不过有点比较特殊:这样分配的
    内存不管申请的size大小有多小,都会在堆中分配然后挂在large链表上。

*/
void *
ngx_palloc(ngx_pool_t *pool, size_t size)
{
#if !(NGX_DEBUG_PALLOC)
    //若size <= pool->max,证明是小块内存,直接在小块内存池中分配
    //否则直接在堆中分配
    if (size <= pool->max) {
        return ngx_palloc_small(pool, size, 1);
    }
#endif

    return ngx_palloc_large(pool, size);
}


void *
ngx_pnalloc(ngx_pool_t *pool, size_t size)
{
#if !(NGX_DEBUG_PALLOC)
    if (size <= pool->max) {
        return ngx_palloc_small(pool, size, 0);
    }
#endif

    return ngx_palloc_large(pool, size);
}


static ngx_inline void *
ngx_palloc_small(ngx_pool_t *pool, size_t size, ngx_uint_t align)
{
    u_char      *m;
    ngx_pool_t  *p;

    //指向目前可用于分配的内存池
    p = pool->current;

    //尝试申请内存
    do {
        /* ngx_align_ptr是一个宏函数,展开如下:
             * #define ngx_align_ptr(p, a)            \
               (u_char *) (((uintptr_t) (p) + ((uintptr_t) a - 1)) & ~((uintptr_t) a - 1))
             * 这个位运算可能比较难理解
             * ～(a-1)：其中a为2的幂,设右数第n位为非零位,则a-1为右数的n-1位均为1, 则有~(a-1)为
             *最后的n-1位全为0,即此时的值必为a的幂
             * p+(a-1)：这部操作相当于加上a-1为右数的n-1位,比如a为4(0100),则加上(0011)
             * 然后两者相与,得到的值必为a的幂
             */
        m = p->d.last;

        if (align) {
            m = ngx_align_ptr(m, NGX_ALIGNMENT);
        }
        //end代表当前小块内存池的尾部
        //若当前内存池的容量还够size大小
        //则进行分配,并返回分配的内存块的起始地址m
        if ((size_t) (p->d.end - m) >= size) {
            p->d.last = m + size;

            return m;
        }
        //否则查看下一个小块内存池
        p = p->d.next;

    } while (p);
     //如果遍历了所有的小块内存池还是没有找到足够的容量
    //那么就分配一个新的小块内存池
    return ngx_palloc_block(pool, size);
}

//只在当前源文件内部使用
static void *
ngx_palloc_block(ngx_pool_t *pool, size_t size)
{
    u_char      *m;
    size_t       psize;
    ngx_pool_t  *p, *new;

    //获取需要分配的大小(ngx_pool_t加上小块内存池的大小)
    psize = (size_t) (pool->d.end - (u_char *) pool);

    //以NGX_POOL_ALIGNMENT对齐,申请内存
    m = ngx_memalign(NGX_POOL_ALIGNMENT, psize, pool->log);
    if (m == NULL) {
        return NULL;
    }

    //新的ngx_pool_t
    new = (ngx_pool_t *) m;

    //由于该ngx_pool_t只用于小块内存池的管理
    //因此只初始化小块内存池有关的成员
    new->d.end = m + psize;
    new->d.next = NULL;
    new->d.failed = 0;

    //对齐空闲内存的首地址
    //分配大小为size的内存出去
    m += sizeof(ngx_pool_data_t);
    m = ngx_align_ptr(m, NGX_ALIGNMENT);
    new->d.last = m + size;

    /* 遍历之前尝试分配大小为size的内存时的小块内存池
     * 将其的failed成员依次加1
     * 若超过4,则将current指向下一个内存池
     */
    for (p = pool->current; p->d.next; p = p->d.next) {
        if (p->d.failed++ > 4) {
            pool->current = p->d.next;
        }
    }
    //将刚分配的ngx_pool_t连接到小块内存池的链表上
    p->d.next = new;

    return m;
}


static void *
ngx_palloc_large(ngx_pool_t *pool, size_t size)
{
    void              *p;
    ngx_uint_t         n;
    ngx_pool_large_t  *large;

     //直接调用ngx_alloc分配内存
    p = ngx_alloc(size, pool->log);
    if (p == NULL) {
        return NULL;
    }

    n = 0;

    //将刚分配的大块内存挂到large链表上
    for (large = pool->large; large; large = large->next) {
        if (large->alloc == NULL) {
            large->alloc = p;
            return p;
        }

         //若第4次还是没有找到可以挂上的地方
        //则退出遍历(这是为了防止large链表的节点多了,造成效率的损失)
        if (n++ > 3) {
            break;
        }
    }

     /* 直接申请一个新的ngx_pool_large_t结构体挂在large上 */
    //申请一个ngx_pool_large_t结构体
    large = ngx_palloc_small(pool, sizeof(ngx_pool_large_t), 1);
    if (large == NULL) {
        ngx_free(p);
        return NULL;
    }

    //将刚分配的大块内存的信息存储在ngx_pool_large_t上
    large->alloc = p;

    //链表的头插法
    large->next = pool->large;
    pool->large = large;

    return p;
}


void *
ngx_pmemalign(ngx_pool_t *pool, size_t size, size_t alignment)
{
    void              *p;
    ngx_pool_large_t  *large;

    //申请按alignment对齐的内存
    p = ngx_memalign(alignment, size, pool->log);
    if (p == NULL) {
        return NULL;
    }

    //申请一个ngx_pool_large_t的结构体
    //用于存储p的信息(起始地址)
    large = ngx_palloc_small(pool, sizeof(ngx_pool_large_t), 1);
    if (large == NULL) {
        ngx_free(p);
        return NULL;
    }

    //挂在large链表上
    large->alloc = p;
    large->next = pool->large;
    pool->large = large;

    return p;
}

//ngx_pfree函数用于释放大块内存
ngx_int_t
ngx_pfree(ngx_pool_t *pool, void *p)
{
    ngx_pool_large_t  *l;

    for (l = pool->large; l; l = l->next) {
        if (p == l->alloc) {
            ngx_log_debug1(NGX_LOG_DEBUG_ALLOC, pool->log, 0,
                           "free: %p", l->alloc);
            //直接将对应的ngx_pool_large_t结构体中的alloc成员赋为NULL
            //之所以没有选择将该结点撤出链表是为了复用
            //以免下次又要重新对ngx_pool_large_t进行申请
            ngx_free(l->alloc);//释放内存
            l->alloc = NULL;

            return NGX_OK;
        }
    }

    return NGX_DECLINED;
}


void *
ngx_pcalloc(ngx_pool_t *pool, size_t size)
{
    void *p;

    p = ngx_palloc(pool, size);
    if (p) {
        ngx_memzero(p, size);
    }

    return p;
}

/*nginx实现的内存池也支持管理除了内存之外的资源,比如文件,它们会随着内存池的释放也一起
被释放。对应ngx_pool_t结构体中管理该功能的成员为ngx_pool_cleanup_t *cleanup,之前已经
说过,它是一个单链表,每个结点对应着需要释放的资源。

*/ngx_pool_cleanup_t *
ngx_pool_cleanup_add(ngx_pool_t *p, size_t size)
{
    ngx_pool_cleanup_t  *c;

    //申请ngx_pool_cleanup_t结构体,用于管理
    c = ngx_palloc(p, sizeof(ngx_pool_cleanup_t));
    if (c == NULL) {
        return NULL;
    }

    //当传入的size不为0时则申请size大小的内存
    //然后赋给data,就可以利用该内存传递参数了
    //否则将data赋为NULL
    if (size) {
        c->data = ngx_palloc(p, size);
        if (c->data == NULL) {
            return NULL;
        }

    } else {
        c->data = NULL;
    }

    //初始化ngx_pool_cleanup_t的其他成员
    c->handler = NULL;
    c->next = p->cleanup;

    p->cleanup = c;

    ngx_log_debug1(NGX_LOG_DEBUG_ALLOC, p->log, 0, "add cleanup: %p", c);

    return c;
}

//该函数的作用对应于ngx_free,即提前释放资源。
void
ngx_pool_run_cleanup_file(ngx_pool_t *p, ngx_fd_t fd)
{
    ngx_pool_cleanup_t       *c;
    ngx_pool_cleanup_file_t  *cf;

    //遍历cleanup链表
    for (c = p->cleanup; c; c = c->next) {
        //如果设置的handler函数指针指向的是为释放资源的函数
        if (c->handler == ngx_pool_cleanup_file) {

            cf = c->data;

            if (cf->fd == fd) {
                c->handler(cf);
                c->handler = NULL;
                return;
            }
        }
    }
}

//以关闭文件来释放资源的方法,可设置为handler的值。
void
ngx_pool_cleanup_file(void *data)
{
    ngx_pool_cleanup_file_t  *c = data;

    ngx_log_debug1(NGX_LOG_DEBUG_ALLOC, c->log, 0, "file cleanup: fd:%d",
                   c->fd);
    //关闭该文件描述符指向的文件
    if (ngx_close_file(c->fd) == NGX_FILE_ERROR) {
        ngx_log_error(NGX_LOG_ALERT, c->log, ngx_errno,
                      ngx_close_file_n " \"%s\" failed", c->name);
    }
}


//以删除文件来释放资源的方法,可设置为handler的值。
void
ngx_pool_delete_file(void *data)
{
    ngx_pool_cleanup_file_t  *c = data;

    ngx_err_t  err;

    ngx_log_debug2(NGX_LOG_DEBUG_ALLOC, c->log, 0, "file cleanup: fd:%d %s",
                   c->fd, c->name);
    //删除文件
    if (ngx_delete_file(c->name) == NGX_FILE_ERROR) {
        err = ngx_errno;

        if (err != NGX_ENOENT) {
            ngx_log_error(NGX_LOG_CRIT, c->log, err,
                          ngx_delete_file_n " \"%s\" failed", c->name);
        }
    }

    if (ngx_close_file(c->fd) == NGX_FILE_ERROR) {
        ngx_log_error(NGX_LOG_ALERT, c->log, ngx_errno,
                      ngx_close_file_n " \"%s\" failed", c->name);
    }
}


#if 0

static void *
ngx_get_cached_block(size_t size)
{
    void                     *p;
    ngx_cached_block_slot_t  *slot;

    if (ngx_cycle->cache == NULL) {
        return NULL;
    }

    slot = &ngx_cycle->cache[(size + ngx_pagesize - 1) / ngx_pagesize];

    slot->tries++;

    if (slot->number) {
        p = slot->block;
        slot->block = slot->block->next;
        slot->number--;
        return p;
    }

    return NULL;
}

#endif
