
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>


#define DEFAULT_CONNECTIONS  512


extern ngx_module_t ngx_kqueue_module;
extern ngx_module_t ngx_eventport_module;
extern ngx_module_t ngx_devpoll_module;
extern ngx_module_t ngx_epoll_module;
extern ngx_module_t ngx_select_module;


static char *ngx_event_init_conf(ngx_cycle_t *cycle, void *conf);
static ngx_int_t ngx_event_module_init(ngx_cycle_t *cycle);
static ngx_int_t ngx_event_process_init(ngx_cycle_t *cycle);
static char *ngx_events_block(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

static char *ngx_event_connections(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);
static char *ngx_event_use(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static char *ngx_event_debug_connection(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);

static void *ngx_event_core_create_conf(ngx_cycle_t *cycle);
static char *ngx_event_core_init_conf(ngx_cycle_t *cycle, void *conf);


static ngx_uint_t     ngx_timer_resolution;
sig_atomic_t          ngx_event_timer_alarm;

static ngx_uint_t     ngx_event_max_module;

ngx_uint_t            ngx_event_flags;
ngx_event_actions_t   ngx_event_actions;


static ngx_atomic_t   connection_counter = 1;
ngx_atomic_t         *ngx_connection_counter = &connection_counter;


ngx_atomic_t         *ngx_accept_mutex_ptr;
ngx_shmtx_t           ngx_accept_mutex;
ngx_uint_t            ngx_use_accept_mutex;
ngx_uint_t            ngx_accept_events;
ngx_uint_t            ngx_accept_mutex_held;
ngx_msec_t            ngx_accept_mutex_delay;
ngx_int_t             ngx_accept_disabled;


#if (NGX_STAT_STUB)

static ngx_atomic_t   ngx_stat_accepted0;
ngx_atomic_t         *ngx_stat_accepted = &ngx_stat_accepted0;
static ngx_atomic_t   ngx_stat_handled0;
ngx_atomic_t         *ngx_stat_handled = &ngx_stat_handled0;
static ngx_atomic_t   ngx_stat_requests0;
ngx_atomic_t         *ngx_stat_requests = &ngx_stat_requests0;
static ngx_atomic_t   ngx_stat_active0;
ngx_atomic_t         *ngx_stat_active = &ngx_stat_active0;
static ngx_atomic_t   ngx_stat_reading0;
ngx_atomic_t         *ngx_stat_reading = &ngx_stat_reading0;
static ngx_atomic_t   ngx_stat_writing0;
ngx_atomic_t         *ngx_stat_writing = &ngx_stat_writing0;
static ngx_atomic_t   ngx_stat_waiting0;
ngx_atomic_t         *ngx_stat_waiting = &ngx_stat_waiting0;

#endif



static ngx_command_t  ngx_events_commands[] = {

    { ngx_string("events"),
      NGX_MAIN_CONF|NGX_CONF_BLOCK|NGX_CONF_NOARGS,
      ngx_events_block,
      0,
      0,
      NULL },

      ngx_null_command
};


static ngx_core_module_t  ngx_events_module_ctx = {
    ngx_string("events"),
    NULL,
    ngx_event_init_conf
};


ngx_module_t  ngx_events_module = {
    NGX_MODULE_V1,
    &ngx_events_module_ctx,                /* module context */
    ngx_events_commands,                   /* module directives */
    NGX_CORE_MODULE,                       /* module type */
    NULL,                                  /* init master */
    NULL,                                  /* init module */
    NULL,                                  /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    NULL,                                  /* exit process */
    NULL,                                  /* exit master */
    NGX_MODULE_V1_PADDING
};
/**
15.事件模块小结
    1.配置项结构体创建及赋值
    
    2.初始化工作
     ngx_init_cycle接着遍历ngx_modules数组,调用各模块的init_module方法,根据配置项结构体中的
    成员做一些初始化工作。对于ngx_event_core_module模块来说,该函数会根据其配置项结构体的内容初
    始化一些用于统计变量(与事件模块关系不是很大),而ngx_epoll_module则没有实现该方法。
    接下来的工作,就可以开始分不同进程进行了。还记得nginx通过什么样的步骤来开启的子进程的吗？首先
    配置文件中设置的worker_processes配置指令如果为1,则nginx以单进程模式运行,若大与1,则以多进程
    模式运行。

     多进程模式下,nginx会调用ngx_master_process_cycle函数,即master进程的工作循环。在master进程的工作
    循环中,它会先设置一些信号的捕捉函数,别忘了master进程靠各种标志位来选择进行什么样的工作。接着就会调
    用ngx_start_worker_processes函数启动worker_processes个子进程。在ngx_start_worker_processes中,
    调用的核心函数是ngx_spawn_process,其中fork了进程之后,会调用ngx_worker_process_cycle,即子进程的
    工作循环。

    在子进程的工作循环中,第一件事就是调用ngx_worker_process_init,它会调用所有模块的init_process函数。
    对于事件模块中的ngx_event_core_module模块来说,它对应的init_process主要做了以下工作:

    1.初始化定时器
    2.调用配置中指定的事件驱动模块的init方法
    3.若设置了timer_resolution,则定时调用ngx_timer_signal_handler,控制时间精度
    4.预分配连接池(cycle->connections)
    5.预分配读事件(cycle->read_events)
    6.预分配写事件(cycle->write_events)
    7.将对应的读、写事件放置到对应的connections连接中
    8.初始化连接池,当前所有连接均为空闲连接,因此cycle->free_connections指向连接池首部
    9.设置监听套接字上读事件的处理方法为ngx_event_accept,即建立新连接
    10.将监听套接字的读事件添加到事件驱动模块中(若打开了accept_mutex锁不会执行这一步)

    3.处理事件
      接着子进程正式开始工作,而其工作的核心函数就是ngx_process_events_and_timers,
    若开启了accept_mutex并且当前进程的负载还没有达到总连接的7/8,会让进程去获取
    accept_mutex锁,获取到该锁的进程可以处理新连接,没有获取到的只能调用epoll_wait处
    理已有的连接的事件,不过过若在epoll_wait上阻塞了accept_mutex_delay毫秒之后,又可
    以再次尝试去获取锁。调用epoll_wait并处理事件的函数是ngx_process_events,对应epoll
    模块中是ngx_epoll_process_events。
      获取到了accept_mutex锁的进程会将就绪的事件放入对应的post事件队列中延后处理,因为如果
    还是在ngx_process_events函数中调用处理函数handler,会导致进程长时间占有accept_mutex
    锁得不到释放,导致其他进程获取不到该锁。因此ngx_process_events还负责将设置了NGX_POST_EVENTS
    标志位的事件加入post事件队列,注意post事件队列有两个,一个是建立新连接的事件的队列,另一个是其
    他普通的读/写事件的队列。在ngx_process_events_and_timers中先将post事件队列中建立新连接的事
    件处理了,然后释放锁,接着再处理post事件队列的其他事件。

*/

static ngx_str_t  event_core_name = ngx_string("event_core");


static ngx_command_t  ngx_event_core_commands[] = {

    { ngx_string("worker_connections"),
      NGX_EVENT_CONF|NGX_CONF_TAKE1,
      ngx_event_connections,
      0,
      0,
      NULL },

    { ngx_string("use"),
      NGX_EVENT_CONF|NGX_CONF_TAKE1,
      ngx_event_use,
      0,
      0,
      NULL },

    { ngx_string("multi_accept"),
      NGX_EVENT_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      0,
      offsetof(ngx_event_conf_t, multi_accept),
      NULL },

    { ngx_string("accept_mutex"),
      NGX_EVENT_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      0,
      offsetof(ngx_event_conf_t, accept_mutex),
      NULL },

    { ngx_string("accept_mutex_delay"),
      NGX_EVENT_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_msec_slot,
      0,
      offsetof(ngx_event_conf_t, accept_mutex_delay),
      NULL },

    { ngx_string("debug_connection"),
      NGX_EVENT_CONF|NGX_CONF_TAKE1,
      ngx_event_debug_connection,
      0,
      0,
      NULL },

      ngx_null_command
};


static ngx_event_module_t  ngx_event_core_module_ctx = {
    &event_core_name,
    ngx_event_core_create_conf,            /* create configuration */
    ngx_event_core_init_conf,              /* init configuration */

    { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};


ngx_module_t  ngx_event_core_module = {
    NGX_MODULE_V1,
    &ngx_event_core_module_ctx,            /* module context */
    ngx_event_core_commands,               /* module directives */
    NGX_EVENT_MODULE,                      /* module type */
    NULL,                                  /* init master */
    ngx_event_module_init,                 /* init module */
    ngx_event_process_init,                /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    NULL,                                  /* exit process */
    NULL,                                  /* exit master */
    NGX_MODULE_V1_PADDING
};
/**
  惊群问题
  1.accept惊群问题
    当多个进程/线程调用accept监听同一个socket上时,一个新连接的到来就会导致所
 有阻塞在该socket上的进程/线程都被唤醒,但是最后只有一个进程/线程可以accept
 成功,其余的又会重新休眠。
    linux中的解决方案：
    维护了一个等待队列(队列的元素为进程),并且使用了WQ_FLAG_EXCLUSIVE标志位(互斥标志位),
    非exclusive元素会加在等待队列的前面,而exclusive元素会加在等待队列的末尾,当有新连接到
    来时,会遍历等待队列,并且只唤醒第一个exclusive进程(非互斥的进程由于排在队列前面也会被唤醒)
    就退出遍历。

    阻塞在accept上的进程都是互斥的(也就是WQ_FLAG_EXCLUSIVE标志位会被置位),因此现在的linux内核
    调用accept时,多个进程/线程只有一个会被唤醒并建立新连接

  2.epoll导致的惊群问题(nginx主要的问题)
   解决多个epfd(epfd是指调用epoll_create获取的描述符)共同监听同一个socket造成的惊群问题
   以下两个问题实质是epoll_create和fork函数调用的先后问题
   (1)多个进程/线程使用同一个epfd然后调用epoll_wait 
    先调用epoll_create获取epfd,再使用fork,各进程共用同一个epfd
   (2)多个进程/线程有自己的epfd,然后监听同一个socket ---nginx面对的主要问题
    先fork,再调用epoll_create,各进程独享自己的epfd
    nginx的每个worker进程相互独立,拥有自己的epfd,不过根据配置文件中的listen指令都监
    听了同一个端口,调用epoll_wait时,若共同监听的套接字有事件发生,就会造成每个worker进程都被唤醒。

  3.具体引起惊群的过程和解决方法
  3.1 多个进程共用一个epfd

  3.2 多个进程有自己的epfd
   每个进程都有自己的epfd,就不会导致这个问题,不过当多个epfd中都监听了
    同一个socket,还是会出现问题,当该socket上有事件发生时,每个阻塞在epoll_wait上的进程
    还是会引起惊群
    (1)accept_accept锁
    (2)采用EPOLLEXCLUSIVE选项 
      epoll_ctl()的第二个参数，专门为了解决多个epfd句柄监听同一个socket的惊群问题
    该选项是针对具体的socket,即调用epoll_ctl时传入,不过需要注意的是,只能和EPOLL_CTL_ADD
    伴随使用,而不支持和EPOLL_CTL_MOD伴随使用。它是如何起作用的,这个和epoll的实现有关。前面我
    们提到过struct eventpoll结构体中有wq等待队列 而每个监听套接字,都有自己的等待队列。当设置
    了EPOLLEXCLUSIVE选项之后,调用epoll_ctl添加监听套接字,其内部会调用ep_insert函数,其中又
    会回调ep_ptable_queue_proc,它会给设备设置ep_poll_callback回调函数,监听套接字的等待队
    列中其实就是设备等待队列,当设备上有数据时,就会回调ep_poll_callback唤醒进程。添加元素到
    队列中的操作主要如下:
     .....
    init_waitqueue_func_entry(&pwq->wait, ep_poll_callback);
    pwq->whead = whead;
    pwq->base = epi;
    if (epi->event.events & EPOLLEXCLUSIVE)
      add_wait_queue_exclusive(whead, &pwq->wait);
    else
      add_wait_queue(whead, &pwq->wait);
    ......
    以前的版本调用add_wait_queue添加到whead队列中,当该fd的状态有变化时,会轮询队列中所有元素,
    调用ep_poll_callback唤醒进程,导致惊群问题。使用add_wait_queue_exclusive之后,当fd的状态
    有变化时,也会遍历该等待队列,但遇到第一个拥有WQ_FLAG_EXCLUSIVE的进程停止,回调
    ep_poll_callback函数将该进程唤醒,避免了全部进程都被唤醒。

    (3)使用SO_REUSEPORT选项 
    SO_REUSEPORT选项支持多个进程或者线程绑定到同一端口,即允许多个套接字bind()/listen()同一个
    TCP/UDP端口,并且在内核层面实现负载均衡。有了这个选项,我们就不用再使用互斥锁的方式来避免惊群
    问题了,因此现在默认ngx_use_accept_mutex选项为off。要求nginx的版本在1.9.1以上,以及 
    linux内核版本3.9以上。

*/

void
ngx_process_events_and_timers(ngx_cycle_t *cycle)
{
    ngx_uint_t  flags;
    ngx_msec_t  timer, delta;

    if (ngx_timer_resolution) {
        timer = NGX_TIMER_INFINITE;
        flags = 0;

    } else {
        timer = ngx_event_find_timer();
        flags = NGX_UPDATE_TIME;

#if (NGX_WIN32)

        /* handle signals from master in case of network inactivity */

        if (timer == NGX_TIMER_INFINITE || timer > 500) {
            timer = 500;
        }

#endif
    }
    /**
    (2)使用accept_mutex的锁，损失一定的效率避免惊群
     具体实现：使用全局互斥锁,每个子进程在调用epoll_wait()之前先去申请锁,申请到才允许
     将监听套结字加入epoll中。获取不到则等待,并设置了一个负载均衡的算法(当某一个子进程的
     连接数达到的连接池总连接数的7/8时,则不会再尝试去申请锁,而是处理已有的连接)来均衡各
     个进程的任务量。
    */

/* 负载均衡锁,当前进程的连接数数大于总连接数的7/8时,认为处于负载状态,则不会进行新连接的建立
 * 而只是将ngx_accept_disabled减1,否则尝试获取锁进行下一步操作
 */
    if (ngx_use_accept_mutex) {
        if (ngx_accept_disabled > 0) {
            ngx_accept_disabled--;
        } else {
             //尝试获取锁,若获取成功,会把监听套结字加入到当前的epoll事件驱动模块中
            if (ngx_trylock_accept_mutex(cycle) == NGX_ERROR) {
                return            };
         /* ngx_accept_mutex_held标志位为1,代表当前进程已经拥有锁
         * 若当前进程已经拥有锁,则将flag添加上NGX_POST_EVENTS位
         * 表示会延迟处理所有的事件
         * 这是因为我们应该使持锁和解锁之间的事件尽量短
         * 因此将事件放置在post队列中,放在后面处理
         */
            if (ngx_accept_mutex_held) {
                flags |= NGX_POST_EVENTS;
            } else {
                //判断设置定时器
                if (timer == NGX_TIMER_INFINITE
                    || timer > ngx_accept_mutex_delay)
                {
                    timer = ngx_accept_mutex_delay;
                }
            }
        }
    }
    //计算ngx_process_events()使用多长时间
    delta = ngx_current_msec;

    //使用epoll作为驱动机制时
    //ngx_process_events对应ngx_epoll_process_events函数
    (void) ngx_process_events(cycle, timer, flags);

    delta = ngx_current_msec - delta;

    ngx_log_debug1(NGX_LOG_DEBUG_EVENT, cycle->log, 0,
                   "timer delta: %M", delta);

    //执行对应新建连接的延迟队列里面的事件
    ngx_event_process_posted(cycle, &ngx_posted_accept_events);

    //锁的标志位为1，解锁
    if (ngx_accept_mutex_held) {
        ngx_shmtx_unlock(&ngx_accept_mutex);
    }
    ////ngx_process_events方法耗费delta>0时
   //调用ngx_event_expire_timers执行所有的超时事件
    if (delta) {
        ngx_event_expire_timers();//超时事件
    }
    //执行普通事件延迟队列中事件
    ngx_event_process_posted(cycle, &ngx_posted_events);
}


ngx_int_t
ngx_handle_read_event(ngx_event_t *rev, ngx_uint_t flags)
{
    if (ngx_event_flags & NGX_USE_CLEAR_EVENT) {

        /* kqueue, epoll */

        if (!rev->active && !rev->ready) {
            if (ngx_add_event(rev, NGX_READ_EVENT, NGX_CLEAR_EVENT)
                == NGX_ERROR)
            {
                return NGX_ERROR;
            }
        }

        return NGX_OK;

    } else if (ngx_event_flags & NGX_USE_LEVEL_EVENT) {

        /* select, poll, /dev/poll */

        if (!rev->active && !rev->ready) {
            if (ngx_add_event(rev, NGX_READ_EVENT, NGX_LEVEL_EVENT)
                == NGX_ERROR)
            {
                return NGX_ERROR;
            }

            return NGX_OK;
        }

        if (rev->active && (rev->ready || (flags & NGX_CLOSE_EVENT))) {
            if (ngx_del_event(rev, NGX_READ_EVENT, NGX_LEVEL_EVENT | flags)
                == NGX_ERROR)
            {
                return NGX_ERROR;
            }

            return NGX_OK;
        }

    } else if (ngx_event_flags & NGX_USE_EVENTPORT_EVENT) {

        /* event ports */

        if (!rev->active && !rev->ready) {
            if (ngx_add_event(rev, NGX_READ_EVENT, 0) == NGX_ERROR) {
                return NGX_ERROR;
            }

            return NGX_OK;
        }

        if (rev->oneshot && !rev->ready) {
            if (ngx_del_event(rev, NGX_READ_EVENT, 0) == NGX_ERROR) {
                return NGX_ERROR;
            }

            return NGX_OK;
        }
    }

    /* iocp */

    return NGX_OK;
}


ngx_int_t
ngx_handle_write_event(ngx_event_t *wev, size_t lowat)
{
    ngx_connection_t  *c;

    if (lowat) {
        c = wev->data;

        if (ngx_send_lowat(c, lowat) == NGX_ERROR) {
            return NGX_ERROR;
        }
    }

    if (ngx_event_flags & NGX_USE_CLEAR_EVENT) {

        /* kqueue, epoll */

        if (!wev->active && !wev->ready) {
            if (ngx_add_event(wev, NGX_WRITE_EVENT,
                              NGX_CLEAR_EVENT | (lowat ? NGX_LOWAT_EVENT : 0))
                == NGX_ERROR)
            {
                return NGX_ERROR;
            }
        }

        return NGX_OK;

    } else if (ngx_event_flags & NGX_USE_LEVEL_EVENT) {

        /* select, poll, /dev/poll */

        if (!wev->active && !wev->ready) {
            if (ngx_add_event(wev, NGX_WRITE_EVENT, NGX_LEVEL_EVENT)
                == NGX_ERROR)
            {
                return NGX_ERROR;
            }

            return NGX_OK;
        }

        if (wev->active && wev->ready) {
            if (ngx_del_event(wev, NGX_WRITE_EVENT, NGX_LEVEL_EVENT)
                == NGX_ERROR)
            {
                return NGX_ERROR;
            }

            return NGX_OK;
        }

    } else if (ngx_event_flags & NGX_USE_EVENTPORT_EVENT) {

        /* event ports */

        if (!wev->active && !wev->ready) {
            if (ngx_add_event(wev, NGX_WRITE_EVENT, 0) == NGX_ERROR) {
                return NGX_ERROR;
            }

            return NGX_OK;
        }

        if (wev->oneshot && wev->ready) {
            if (ngx_del_event(wev, NGX_WRITE_EVENT, 0) == NGX_ERROR) {
                return NGX_ERROR;
            }

            return NGX_OK;
        }
    }

    /* iocp */

    return NGX_OK;
}


static char *
ngx_event_init_conf(ngx_cycle_t *cycle, void *conf)
{
#if (NGX_HAVE_REUSEPORT)
    ngx_uint_t        i;
    ngx_listening_t  *ls;
#endif

    if (ngx_get_conf(cycle->conf_ctx, ngx_events_module) == NULL) {
        ngx_log_error(NGX_LOG_EMERG, cycle->log, 0,
                      "no \"events\" section in configuration");
        return NGX_CONF_ERROR;
    }

    if (cycle->connection_n < cycle->listening.nelts + 1) {

        /*
         * there should be at least one connection for each listening
         * socket, plus an additional connection for channel
         */

        ngx_log_error(NGX_LOG_EMERG, cycle->log, 0,
                      "%ui worker_connections are not enough "
                      "for %ui listening sockets",
                      cycle->connection_n, cycle->listening.nelts);

        return NGX_CONF_ERROR;
    }

#if (NGX_HAVE_REUSEPORT)

    ls = cycle->listening.elts;
    for (i = 0; i < cycle->listening.nelts; i++) {

        if (!ls[i].reuseport || ls[i].worker != 0) {
            continue;
        }

        if (ngx_clone_listening(cycle, &ls[i]) != NGX_OK) {
            return NGX_CONF_ERROR;
        }

        /* cloning may change cycle->listening.elts */

        ls = cycle->listening.elts;
    }

#endif

    return NGX_CONF_OK;
}


static ngx_int_t
ngx_event_module_init(ngx_cycle_t *cycle)
{
    void              ***cf;
    u_char              *shared;
    size_t               size, cl;
    ngx_shm_t            shm;
    ngx_time_t          *tp;
    ngx_core_conf_t     *ccf;
    ngx_event_conf_t    *ecf;

    cf = ngx_get_conf(cycle->conf_ctx, ngx_events_module);
    ecf = (*cf)[ngx_event_core_module.ctx_index];

    if (!ngx_test_config && ngx_process <= NGX_PROCESS_MASTER) {
        ngx_log_error(NGX_LOG_NOTICE, cycle->log, 0,
                      "using the \"%s\" event method", ecf->name);
    }

    ccf = (ngx_core_conf_t *) ngx_get_conf(cycle->conf_ctx, ngx_core_module);

    ngx_timer_resolution = ccf->timer_resolution;

#if !(NGX_WIN32)
    {
    ngx_int_t      limit;
    struct rlimit  rlmt;

    if (getrlimit(RLIMIT_NOFILE, &rlmt) == -1) {
        ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                      "getrlimit(RLIMIT_NOFILE) failed, ignored");

    } else {
        if (ecf->connections > (ngx_uint_t) rlmt.rlim_cur
            && (ccf->rlimit_nofile == NGX_CONF_UNSET
                || ecf->connections > (ngx_uint_t) ccf->rlimit_nofile))
        {
            limit = (ccf->rlimit_nofile == NGX_CONF_UNSET) ?
                         (ngx_int_t) rlmt.rlim_cur : ccf->rlimit_nofile;

            ngx_log_error(NGX_LOG_WARN, cycle->log, 0,
                          "%ui worker_connections exceed "
                          "open file resource limit: %i",
                          ecf->connections, limit);
        }
    }
    }
#endif /* !(NGX_WIN32) */


    if (ccf->master == 0) {
        return NGX_OK;
    }

    if (ngx_accept_mutex_ptr) {
        return NGX_OK;
    }


    /* cl should be equal to or greater than cache line size */

    cl = 128;

    size = cl            /* ngx_accept_mutex */
           + cl          /* ngx_connection_counter */
           + cl;         /* ngx_temp_number */

#if (NGX_STAT_STUB)

    size += cl           /* ngx_stat_accepted */
           + cl          /* ngx_stat_handled */
           + cl          /* ngx_stat_requests */
           + cl          /* ngx_stat_active */
           + cl          /* ngx_stat_reading */
           + cl          /* ngx_stat_writing */
           + cl;         /* ngx_stat_waiting */

#endif

    shm.size = size;
    ngx_str_set(&shm.name, "nginx_shared_zone");
    shm.log = cycle->log;

    if (ngx_shm_alloc(&shm) != NGX_OK) {
        return NGX_ERROR;
    }

    shared = shm.addr;

    ngx_accept_mutex_ptr = (ngx_atomic_t *) shared;
    ngx_accept_mutex.spin = (ngx_uint_t) -1;

    if (ngx_shmtx_create(&ngx_accept_mutex, (ngx_shmtx_sh_t *) shared,
                         cycle->lock_file.data)
        != NGX_OK)
    {
        return NGX_ERROR;
    }

    ngx_connection_counter = (ngx_atomic_t *) (shared + 1 * cl);

    (void) ngx_atomic_cmp_set(ngx_connection_counter, 0, 1);

    ngx_log_debug2(NGX_LOG_DEBUG_EVENT, cycle->log, 0,
                   "counter: %p, %uA",
                   ngx_connection_counter, *ngx_connection_counter);

    ngx_temp_number = (ngx_atomic_t *) (shared + 2 * cl);

    tp = ngx_timeofday();

    ngx_random_number = (tp->msec << 16) + ngx_pid;

#if (NGX_STAT_STUB)

    ngx_stat_accepted = (ngx_atomic_t *) (shared + 3 * cl);
    ngx_stat_handled = (ngx_atomic_t *) (shared + 4 * cl);
    ngx_stat_requests = (ngx_atomic_t *) (shared + 5 * cl);
    ngx_stat_active = (ngx_atomic_t *) (shared + 6 * cl);
    ngx_stat_reading = (ngx_atomic_t *) (shared + 7 * cl);
    ngx_stat_writing = (ngx_atomic_t *) (shared + 8 * cl);
    ngx_stat_waiting = (ngx_atomic_t *) (shared + 9 * cl);

#endif

    return NGX_OK;
}


#if !(NGX_WIN32)

static void
ngx_timer_signal_handler(int signo)
{
    ngx_event_timer_alarm = 1;

#if 1
    ngx_log_debug0(NGX_LOG_DEBUG_EVENT, ngx_cycle->log, 0, "timer signal");
#endif
}

#endif


static ngx_int_t
ngx_event_process_init(ngx_cycle_t *cycle)
{
    ngx_uint_t           m, i;
    ngx_event_t         *rev, *wev;
    ngx_listening_t     *ls;
    ngx_connection_t    *c, *next, *old;
    ngx_core_conf_t     *ccf;
    ngx_event_conf_t    *ecf;
    ngx_event_module_t  *module;

    ccf = (ngx_core_conf_t *) ngx_get_conf(cycle->conf_ctx, ngx_core_module);
    ecf = ngx_event_get_conf(cycle->conf_ctx, ngx_event_core_module);

    if (ccf->master && ccf->worker_processes > 1 && ecf->accept_mutex) {
        ngx_use_accept_mutex = 1;
        ngx_accept_mutex_held = 0;
        ngx_accept_mutex_delay = ecf->accept_mutex_delay;

    } else {
        ngx_use_accept_mutex = 0;
    }

#if (NGX_WIN32)

    /*
     * disable accept mutex on win32 as it may cause deadlock if
     * grabbed by a process which can't accept connections
     */

    ngx_use_accept_mutex = 0;

#endif

    ngx_queue_init(&ngx_posted_accept_events);
    ngx_queue_init(&ngx_posted_events);

    if (ngx_event_timer_init(cycle->log) == NGX_ERROR) {
        return NGX_ERROR;
    }

    for (m = 0; cycle->modules[m]; m++) {
        if (cycle->modules[m]->type != NGX_EVENT_MODULE) {
            continue;
        }

        if (cycle->modules[m]->ctx_index != ecf->use) {
            continue;
        }

        module = cycle->modules[m]->ctx;

        if (module->actions.init(cycle, ngx_timer_resolution) != NGX_OK) {
            /* fatal */
            exit(2);
        }

        break;
    }

#if !(NGX_WIN32)

    if (ngx_timer_resolution && !(ngx_event_flags & NGX_USE_TIMER_EVENT)) {
        struct sigaction  sa;
        struct itimerval  itv;

        ngx_memzero(&sa, sizeof(struct sigaction));
        sa.sa_handler = ngx_timer_signal_handler;
        sigemptyset(&sa.sa_mask);

        if (sigaction(SIGALRM, &sa, NULL) == -1) {
            ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                          "sigaction(SIGALRM) failed");
            return NGX_ERROR;
        }

        itv.it_interval.tv_sec = ngx_timer_resolution / 1000;
        itv.it_interval.tv_usec = (ngx_timer_resolution % 1000) * 1000;
        itv.it_value.tv_sec = ngx_timer_resolution / 1000;
        itv.it_value.tv_usec = (ngx_timer_resolution % 1000 ) * 1000;

        if (setitimer(ITIMER_REAL, &itv, NULL) == -1) {
            ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                          "setitimer() failed");
        }
    }

    if (ngx_event_flags & NGX_USE_FD_EVENT) {
        struct rlimit  rlmt;

        if (getrlimit(RLIMIT_NOFILE, &rlmt) == -1) {
            ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                          "getrlimit(RLIMIT_NOFILE) failed");
            return NGX_ERROR;
        }

        cycle->files_n = (ngx_uint_t) rlmt.rlim_cur;

        cycle->files = ngx_calloc(sizeof(ngx_connection_t *) * cycle->files_n,
                                  cycle->log);
        if (cycle->files == NULL) {
            return NGX_ERROR;
        }
    }

#else

    if (ngx_timer_resolution && !(ngx_event_flags & NGX_USE_TIMER_EVENT)) {
        ngx_log_error(NGX_LOG_WARN, cycle->log, 0,
                      "the \"timer_resolution\" directive is not supported "
                      "with the configured event method, ignored");
        ngx_timer_resolution = 0;
    }

#endif

    cycle->connections =
        ngx_alloc(sizeof(ngx_connection_t) * cycle->connection_n, cycle->log);
    if (cycle->connections == NULL) {
        return NGX_ERROR;
    }

    c = cycle->connections;

    cycle->read_events = ngx_alloc(sizeof(ngx_event_t) * cycle->connection_n,
                                   cycle->log);
    if (cycle->read_events == NULL) {
        return NGX_ERROR;
    }

    rev = cycle->read_events;
    for (i = 0; i < cycle->connection_n; i++) {
        rev[i].closed = 1;
        rev[i].instance = 1;
    }

    cycle->write_events = ngx_alloc(sizeof(ngx_event_t) * cycle->connection_n,
                                    cycle->log);
    if (cycle->write_events == NULL) {
        return NGX_ERROR;
    }

    wev = cycle->write_events;
    for (i = 0; i < cycle->connection_n; i++) {
        wev[i].closed = 1;
    }

    i = cycle->connection_n;
    next = NULL;

    do {
        i--;

        c[i].data = next;
        c[i].read = &cycle->read_events[i];
        c[i].write = &cycle->write_events[i];
        c[i].fd = (ngx_socket_t) -1;

        next = &c[i];
    } while (i);

    cycle->free_connections = next;
    cycle->free_connection_n = cycle->connection_n;

    /* for each listening socket */

    ls = cycle->listening.elts;
    for (i = 0; i < cycle->listening.nelts; i++) {

#if (NGX_HAVE_REUSEPORT)
        if (ls[i].reuseport && ls[i].worker != ngx_worker) {
            continue;
        }
#endif

        c = ngx_get_connection(ls[i].fd, cycle->log);

        if (c == NULL) {
            return NGX_ERROR;
        }

        c->type = ls[i].type;
        c->log = &ls[i].log;

        c->listening = &ls[i];
        ls[i].connection = c;

        rev = c->read;

        rev->log = c->log;
        rev->accept = 1;

#if (NGX_HAVE_DEFERRED_ACCEPT)
        rev->deferred_accept = ls[i].deferred_accept;
#endif

        if (!(ngx_event_flags & NGX_USE_IOCP_EVENT)) {
            if (ls[i].previous) {

                /*
                 * delete the old accept events that were bound to
                 * the old cycle read events array
                 */

                old = ls[i].previous->connection;

                if (ngx_del_event(old->read, NGX_READ_EVENT, NGX_CLOSE_EVENT)
                    == NGX_ERROR)
                {
                    return NGX_ERROR;
                }

                old->fd = (ngx_socket_t) -1;
            }
        }

#if (NGX_WIN32)

        if (ngx_event_flags & NGX_USE_IOCP_EVENT) {
            ngx_iocp_conf_t  *iocpcf;

            rev->handler = ngx_event_acceptex;

            if (ngx_use_accept_mutex) {
                continue;
            }

            if (ngx_add_event(rev, 0, NGX_IOCP_ACCEPT) == NGX_ERROR) {
                return NGX_ERROR;
            }

            ls[i].log.handler = ngx_acceptex_log_error;

            iocpcf = ngx_event_get_conf(cycle->conf_ctx, ngx_iocp_module);
            if (ngx_event_post_acceptex(&ls[i], iocpcf->post_acceptex)
                == NGX_ERROR)
            {
                return NGX_ERROR;
            }

        } else {
            rev->handler = ngx_event_accept;

            if (ngx_use_accept_mutex) {
                continue;
            }

            if (ngx_add_event(rev, NGX_READ_EVENT, 0) == NGX_ERROR) {
                return NGX_ERROR;
            }
        }

#else

        rev->handler = (c->type == SOCK_STREAM) ? ngx_event_accept
                                                : ngx_event_recvmsg;

#if (NGX_HAVE_REUSEPORT)

        if (ls[i].reuseport) {
            if (ngx_add_event(rev, NGX_READ_EVENT, 0) == NGX_ERROR) {
                return NGX_ERROR;
            }

            continue;
        }

#endif

        if (ngx_use_accept_mutex) {
            continue;
        }

#if (NGX_HAVE_EPOLLEXCLUSIVE)

        if ((ngx_event_flags & NGX_USE_EPOLL_EVENT)
            && ccf->worker_processes > 1)
        {
            if (ngx_add_event(rev, NGX_READ_EVENT, NGX_EXCLUSIVE_EVENT)
                == NGX_ERROR)
            {
                return NGX_ERROR;
            }

            continue;
        }

#endif

        if (ngx_add_event(rev, NGX_READ_EVENT, 0) == NGX_ERROR) {
            return NGX_ERROR;
        }

#endif

    }

    return NGX_OK;
}


ngx_int_t
ngx_send_lowat(ngx_connection_t *c, size_t lowat)
{
    int  sndlowat;

#if (NGX_HAVE_LOWAT_EVENT)

    if (ngx_event_flags & NGX_USE_KQUEUE_EVENT) {
        c->write->available = lowat;
        return NGX_OK;
    }

#endif

    if (lowat == 0 || c->sndlowat) {
        return NGX_OK;
    }

    sndlowat = (int) lowat;

    if (setsockopt(c->fd, SOL_SOCKET, SO_SNDLOWAT,
                   (const void *) &sndlowat, sizeof(int))
        == -1)
    {
        ngx_connection_error(c, ngx_socket_errno,
                             "setsockopt(SO_SNDLOWAT) failed");
        return NGX_ERROR;
    }

    c->sndlowat = 1;

    return NGX_OK;
}


static char *
ngx_events_block(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    char                 *rv;
    void               ***ctx;
    ngx_uint_t            i;
    ngx_conf_t            pcf;
    ngx_event_module_t   *m;

    if (*(void **) conf) {
        return "is duplicate";
    }

    /* count the number of the event modules and set up their indices */

    ngx_event_max_module = ngx_count_modules(cf->cycle, NGX_EVENT_MODULE);

    ctx = ngx_pcalloc(cf->pool, sizeof(void *));
    if (ctx == NULL) {
        return NGX_CONF_ERROR;
    }

    *ctx = ngx_pcalloc(cf->pool, ngx_event_max_module * sizeof(void *));
    if (*ctx == NULL) {
        return NGX_CONF_ERROR;
    }

    *(void **) conf = ctx;

    for (i = 0; cf->cycle->modules[i]; i++) {
        if (cf->cycle->modules[i]->type != NGX_EVENT_MODULE) {
            continue;
        }

        m = cf->cycle->modules[i]->ctx;

        if (m->create_conf) {
            (*ctx)[cf->cycle->modules[i]->ctx_index] =
                                                     m->create_conf(cf->cycle);
            if ((*ctx)[cf->cycle->modules[i]->ctx_index] == NULL) {
                return NGX_CONF_ERROR;
            }
        }
    }

    pcf = *cf;
    cf->ctx = ctx;
    cf->module_type = NGX_EVENT_MODULE;
    cf->cmd_type = NGX_EVENT_CONF;

    rv = ngx_conf_parse(cf, NULL);

    *cf = pcf;

    if (rv != NGX_CONF_OK) {
        return rv;
    }

    for (i = 0; cf->cycle->modules[i]; i++) {
        if (cf->cycle->modules[i]->type != NGX_EVENT_MODULE) {
            continue;
        }

        m = cf->cycle->modules[i]->ctx;

        if (m->init_conf) {
            rv = m->init_conf(cf->cycle,
                              (*ctx)[cf->cycle->modules[i]->ctx_index]);
            if (rv != NGX_CONF_OK) {
                return rv;
            }
        }
    }

    return NGX_CONF_OK;
}


static char *
ngx_event_connections(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_event_conf_t  *ecf = conf;

    ngx_str_t  *value;

    if (ecf->connections != NGX_CONF_UNSET_UINT) {
        return "is duplicate";
    }

    value = cf->args->elts;
    ecf->connections = ngx_atoi(value[1].data, value[1].len);
    if (ecf->connections == (ngx_uint_t) NGX_ERROR) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "invalid number \"%V\"", &value[1]);

        return NGX_CONF_ERROR;
    }

    cf->cycle->connection_n = ecf->connections;

    return NGX_CONF_OK;
}


static char *
ngx_event_use(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_event_conf_t  *ecf = conf;

    ngx_int_t             m;
    ngx_str_t            *value;
    ngx_event_conf_t     *old_ecf;
    ngx_event_module_t   *module;

    if (ecf->use != NGX_CONF_UNSET_UINT) {
        return "is duplicate";
    }

    value = cf->args->elts;

    if (cf->cycle->old_cycle->conf_ctx) {
        old_ecf = ngx_event_get_conf(cf->cycle->old_cycle->conf_ctx,
                                     ngx_event_core_module);
    } else {
        old_ecf = NULL;
    }


    for (m = 0; cf->cycle->modules[m]; m++) {
        if (cf->cycle->modules[m]->type != NGX_EVENT_MODULE) {
            continue;
        }

        module = cf->cycle->modules[m]->ctx;
        if (module->name->len == value[1].len) {
            if (ngx_strcmp(module->name->data, value[1].data) == 0) {
                ecf->use = cf->cycle->modules[m]->ctx_index;
                ecf->name = module->name->data;

                if (ngx_process == NGX_PROCESS_SINGLE
                    && old_ecf
                    && old_ecf->use != ecf->use)
                {
                    ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                               "when the server runs without a master process "
                               "the \"%V\" event type must be the same as "
                               "in previous configuration - \"%s\" "
                               "and it cannot be changed on the fly, "
                               "to change it you need to stop server "
                               "and start it again",
                               &value[1], old_ecf->name);

                    return NGX_CONF_ERROR;
                }

                return NGX_CONF_OK;
            }
        }
    }

    ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                       "invalid event type \"%V\"", &value[1]);

    return NGX_CONF_ERROR;
}


static char *
ngx_event_debug_connection(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
#if (NGX_DEBUG)
    ngx_event_conf_t  *ecf = conf;

    ngx_int_t             rc;
    ngx_str_t            *value;
    ngx_url_t             u;
    ngx_cidr_t            c, *cidr;
    ngx_uint_t            i;
    struct sockaddr_in   *sin;
#if (NGX_HAVE_INET6)
    struct sockaddr_in6  *sin6;
#endif

    value = cf->args->elts;

#if (NGX_HAVE_UNIX_DOMAIN)

    if (ngx_strcmp(value[1].data, "unix:") == 0) {
        cidr = ngx_array_push(&ecf->debug_connection);
        if (cidr == NULL) {
            return NGX_CONF_ERROR;
        }

        cidr->family = AF_UNIX;
        return NGX_CONF_OK;
    }

#endif

    rc = ngx_ptocidr(&value[1], &c);

    if (rc != NGX_ERROR) {
        if (rc == NGX_DONE) {
            ngx_conf_log_error(NGX_LOG_WARN, cf, 0,
                               "low address bits of %V are meaningless",
                               &value[1]);
        }

        cidr = ngx_array_push(&ecf->debug_connection);
        if (cidr == NULL) {
            return NGX_CONF_ERROR;
        }

        *cidr = c;

        return NGX_CONF_OK;
    }

    ngx_memzero(&u, sizeof(ngx_url_t));
    u.host = value[1];

    if (ngx_inet_resolve_host(cf->pool, &u) != NGX_OK) {
        if (u.err) {
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                               "%s in debug_connection \"%V\"",
                               u.err, &u.host);
        }

        return NGX_CONF_ERROR;
    }

    cidr = ngx_array_push_n(&ecf->debug_connection, u.naddrs);
    if (cidr == NULL) {
        return NGX_CONF_ERROR;
    }

    ngx_memzero(cidr, u.naddrs * sizeof(ngx_cidr_t));

    for (i = 0; i < u.naddrs; i++) {
        cidr[i].family = u.addrs[i].sockaddr->sa_family;

        switch (cidr[i].family) {

#if (NGX_HAVE_INET6)
        case AF_INET6:
            sin6 = (struct sockaddr_in6 *) u.addrs[i].sockaddr;
            cidr[i].u.in6.addr = sin6->sin6_addr;
            ngx_memset(cidr[i].u.in6.mask.s6_addr, 0xff, 16);
            break;
#endif

        default: /* AF_INET */
            sin = (struct sockaddr_in *) u.addrs[i].sockaddr;
            cidr[i].u.in.addr = sin->sin_addr.s_addr;
            cidr[i].u.in.mask = 0xffffffff;
            break;
        }
    }

#else

    ngx_conf_log_error(NGX_LOG_WARN, cf, 0,
                       "\"debug_connection\" is ignored, you need to rebuild "
                       "nginx using --with-debug option to enable it");

#endif

    return NGX_CONF_OK;
}


static void *
ngx_event_core_create_conf(ngx_cycle_t *cycle)
{
    ngx_event_conf_t  *ecf;

    ecf = ngx_palloc(cycle->pool, sizeof(ngx_event_conf_t));
    if (ecf == NULL) {
        return NULL;
    }

    ecf->connections = NGX_CONF_UNSET_UINT;
    ecf->use = NGX_CONF_UNSET_UINT;
    ecf->multi_accept = NGX_CONF_UNSET;
    ecf->accept_mutex = NGX_CONF_UNSET;
    ecf->accept_mutex_delay = NGX_CONF_UNSET_MSEC;
    ecf->name = (void *) NGX_CONF_UNSET;

#if (NGX_DEBUG)

    if (ngx_array_init(&ecf->debug_connection, cycle->pool, 4,
                       sizeof(ngx_cidr_t)) == NGX_ERROR)
    {
        return NULL;
    }

#endif

    return ecf;
}


static char *
ngx_event_core_init_conf(ngx_cycle_t *cycle, void *conf)
{
    ngx_event_conf_t  *ecf = conf;

#if (NGX_HAVE_EPOLL) && !(NGX_TEST_BUILD_EPOLL)
    int                  fd;
#endif
    ngx_int_t            i;
    ngx_module_t        *module;
    ngx_event_module_t  *event_module;

    module = NULL;

#if (NGX_HAVE_EPOLL) && !(NGX_TEST_BUILD_EPOLL)

    fd = epoll_create(100);

    if (fd != -1) {
        (void) close(fd);
        module = &ngx_epoll_module;

    } else if (ngx_errno != NGX_ENOSYS) {
        module = &ngx_epoll_module;
    }

#endif

#if (NGX_HAVE_DEVPOLL) && !(NGX_TEST_BUILD_DEVPOLL)

    module = &ngx_devpoll_module;

#endif

#if (NGX_HAVE_KQUEUE)

    module = &ngx_kqueue_module;

#endif

#if (NGX_HAVE_SELECT)

    if (module == NULL) {
        module = &ngx_select_module;
    }

#endif

    if (module == NULL) {
        for (i = 0; cycle->modules[i]; i++) {

            if (cycle->modules[i]->type != NGX_EVENT_MODULE) {
                continue;
            }

            event_module = cycle->modules[i]->ctx;

            if (ngx_strcmp(event_module->name->data, event_core_name.data) == 0)
            {
                continue;
            }

            module = cycle->modules[i];
            break;
        }
    }

    if (module == NULL) {
        ngx_log_error(NGX_LOG_EMERG, cycle->log, 0, "no events module found");
        return NGX_CONF_ERROR;
    }

    ngx_conf_init_uint_value(ecf->connections, DEFAULT_CONNECTIONS);
    cycle->connection_n = ecf->connections;

    ngx_conf_init_uint_value(ecf->use, module->ctx_index);

    event_module = module->ctx;
    ngx_conf_init_ptr_value(ecf->name, event_module->name->data);

    ngx_conf_init_value(ecf->multi_accept, 0);
    ngx_conf_init_value(ecf->accept_mutex, 0);
    ngx_conf_init_msec_value(ecf->accept_mutex_delay, 500);

    return NGX_CONF_OK;
}
