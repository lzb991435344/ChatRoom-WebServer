
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_PALLOC_H_INCLUDED_
#define _NGX_PALLOC_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


/*
 * NGX_MAX_ALLOC_FROM_POOL should be (ngx_pagesize - 1), i.e. 4095 on x86.
 * On Windows NT it decreases a number of locked pages in a kernel.
 */
/*关于大/小块的区分标准,是由NGX_MAX_ALLOC_FROM_POOL宏定义决定的:
#define NGX_MAX_ALLOC_FROM_POOL （ngx_pagesize - 1)。
ngx_pagesize是内存中每页的大小,x86下是4KB,即4096(可通过getpagesize()获取每页大小)。
由此可见,在不同的机器上,小块内存和大块内存之间的界线并不是一个定值。当申请的内存
小于NGX_MAX_ALLOC_FROM_POOL + sizeof(ngx_pool_t)时,则为小块内存,需要通过
ngx_pool_data_t d来管理;反之,则在堆中分配,通过large管理。*/

#define NGX_MAX_ALLOC_FROM_POOL  (ngx_pagesize - 1)

#define NGX_DEFAULT_POOL_SIZE    (16 * 1024)

#define NGX_POOL_ALIGNMENT       16
#define NGX_MIN_POOL_SIZE                                                     \
    ngx_align((sizeof(ngx_pool_t) + 2 * sizeof(ngx_pool_large_t)),            \
              NGX_POOL_ALIGNMENT)


typedef void (*ngx_pool_cleanup_pt)(void *data);

typedef struct ngx_pool_cleanup_s  ngx_pool_cleanup_t;

//用于释放比如文件等非内存的资源
struct ngx_pool_cleanup_s {
    ngx_pool_cleanup_pt   handler;//清理的方法,函数指针
    void                 *data;//handler指向的方法的参数
    ngx_pool_cleanup_t   *next;//指向下一个ngx_pool_cleanup_t结构体
};


typedef struct ngx_pool_large_s  ngx_pool_large_t;


struct ngx_pool_large_s {
    ngx_pool_large_t     *next;//指向下一块大的内存
    void                 *alloc;//指向调用ngx_alloc分配的大块内存
};


typedef struct {
    u_char               *last; //指向未分配的空闲内存的首地址
    u_char               *end;//指向当前小块内存池的末尾

    /* 指向下一个小块内存池
   * 注意next成员的数据类型并不是ngx_pool_data_t,而是ngx_pool_t
   */
    ngx_pool_t           *next;

    /* 当剩余的空间不足以分配出小块内存时,failed加1
   * 当failed大于4后,ngx_pool_t的current指针会指向下一个小块内存池
   */
    ngx_uint_t            failed;
} ngx_pool_data_t;


struct ngx_pool_s {
    /* 管理小块的内存池。
   * 当该内存池中的空间不足时,会再分配一个ngx_pool_data_t,并使用
   *ngx_pool_data_t结构体中的成员next连接起来最终形成一个单链表
   */
    ngx_pool_data_t       d;
    size_t                max;/* 该值用于判断申请的内存属于大块还是小块 */
   
   /* 当有多个小块内存池形成链表时,current指向分配内存时第一个小块内存池
   * 其实意思就是指向链表中可以用于分配内存的第一个内存池
   * 这样就不用在分配时从头开始遍历了
   */
    ngx_pool_t           *current;
    ngx_chain_t          *chain;

  /* 申请大块的内存直接从堆中分配
   * 由于需要在内存池释放时同时也要释放内存
   * 因此需要管理分配的大块内存
   * 于是就把每次分配的大块内存通过large组成一个单链表
   */
    ngx_pool_large_t     *large;

    /* 将所有需要释放的资源(比如文件等)通过cleanup组成一个单链表 */
    ngx_pool_cleanup_t   *cleanup;

    ngx_log_t            *log; /* 内存池执行中输出日志的对象 */
};


typedef struct {
    ngx_fd_t              fd;
    u_char               *name;
    ngx_log_t            *log;
} ngx_pool_cleanup_file_t;


ngx_pool_t *ngx_create_pool(size_t size, ngx_log_t *log);
void ngx_destroy_pool(ngx_pool_t *pool);
void ngx_reset_pool(ngx_pool_t *pool);

void *ngx_palloc(ngx_pool_t *pool, size_t size);
void *ngx_pnalloc(ngx_pool_t *pool, size_t size);
void *ngx_pcalloc(ngx_pool_t *pool, size_t size);
void *ngx_pmemalign(ngx_pool_t *pool, size_t size, size_t alignment);
ngx_int_t ngx_pfree(ngx_pool_t *pool, void *p);


ngx_pool_cleanup_t *ngx_pool_cleanup_add(ngx_pool_t *p, size_t size);
void ngx_pool_run_cleanup_file(ngx_pool_t *p, ngx_fd_t fd);
void ngx_pool_cleanup_file(void *data);
void ngx_pool_delete_file(void *data);


#endif /* _NGX_PALLOC_H_INCLUDED_ */
