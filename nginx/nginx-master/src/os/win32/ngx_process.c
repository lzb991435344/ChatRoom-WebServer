
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */

#include <ngx_config.h>
#include <ngx_core.h>


int              ngx_argc;
char           **ngx_argv;
char           **ngx_os_argv;

ngx_int_t        ngx_last_process;
ngx_process_t    ngx_processes[NGX_MAX_PROCESSES];


ngx_pid_t
ngx_spawn_process(ngx_cycle_t *cycle, char *name, ngx_int_t respawn)
{
    u_long          rc, n, code;
    ngx_int_t       s;
    ngx_pid_t       pid;
    ngx_exec_ctx_t  ctx;
    HANDLE          events[2];
    char            file[MAX_PATH + 1];

    
    if (respawn >= 0) {
        s = respawn;

    } else {
        for (s = 0; s < ngx_last_process; s++) {
            if (ngx_processes[s].handle == NULL) {
                break;
            }
        }
        if (s == NGX_MAX_PROCESSES) {
            ngx_log_error(NGX_LOG_ALERT, cycle->log, 0,
                          "no more than %d processes can be spawned",
                          NGX_MAX_PROCESSES);
            return NGX_INVALID_PID;
        }
    }

    n = GetModuleFileName(NULL, file, MAX_PATH);

    if (n == 0) {
        ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                      "GetModuleFileName() failed");
        return NGX_INVALID_PID;
    }

    file[n] = '\0';

    ngx_log_debug1(NGX_LOG_DEBUG_CORE, cycle->log, 0,
                   "GetModuleFileName: \"%s\"", file);

    ctx.path = file;
    ctx.name = name;
    ctx.args = GetCommandLine();
    ctx.argv = NULL;
    ctx.envp = NULL;

    pid = ngx_execute(cycle, &ctx);

    if (pid == NGX_INVALID_PID) {
        return pid;
    }

    ngx_memzero(&ngx_processes[s], sizeof(ngx_process_t));

    ngx_processes[s].handle = ctx.child;
    ngx_processes[s].pid = pid;
    ngx_processes[s].name = name;

    ngx_sprintf(ngx_processes[s].term_event, "ngx_%s_term_%P%Z", name, pid);
    ngx_sprintf(ngx_processes[s].quit_event, "ngx_%s_quit_%P%Z", name, pid);
    ngx_sprintf(ngx_processes[s].reopen_event, "ngx_%s_reopen_%P%Z",
                name, pid);

    events[0] = ngx_master_process_event;
    events[1] = ctx.child;

    rc = WaitForMultipleObjects(2, events, 0, 5000);

    ngx_time_update();

    ngx_log_debug1(NGX_LOG_DEBUG_CORE, cycle->log, 0,
                   "WaitForMultipleObjects: %ul", rc);

    switch (rc) {
            /*
            * ev == WAIT_OBJECT_0
            * ev == WAIT_OBJECT_0 + 1
            * ev == WAIT_ABANDONED_0 + 1
            */
    case WAIT_OBJECT_0:

        ngx_processes[s].term = OpenEvent(EVENT_MODIFY_STATE, 0,
                                          (char *) ngx_processes[s].term_event);
        if (ngx_processes[s].term == NULL) {
            ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                          "OpenEvent(\"%s\") failed",
                          ngx_processes[s].term_event);
            goto failed;
        }

        ngx_processes[s].quit = OpenEvent(EVENT_MODIFY_STATE, 0,
                                          (char *) ngx_processes[s].quit_event);
        if (ngx_processes[s].quit == NULL) {
            ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                          "OpenEvent(\"%s\") failed",
                          ngx_processes[s].quit_event);
            goto failed;
        }

        ngx_processes[s].reopen = OpenEvent(EVENT_MODIFY_STATE, 0,
                                       (char *) ngx_processes[s].reopen_event);
        if (ngx_processes[s].reopen == NULL) {
            ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                          "OpenEvent(\"%s\") failed",
                          ngx_processes[s].reopen_event);
            goto failed;
        }

        if (ResetEvent(ngx_master_process_event) == 0) {
            ngx_log_error(NGX_LOG_ALERT, cycle->log, 0,
                          "ResetEvent(\"%s\") failed",
                          ngx_master_process_event_name);
            goto failed;
        }

        break;

    case WAIT_OBJECT_0 + 1:
        if (GetExitCodeProcess(ctx.child, &code) == 0) {
            ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                          "GetExitCodeProcess(%P) failed", pid);
        }

        ngx_log_error(NGX_LOG_ALERT, cycle->log, 0,
                      "%s process %P exited with code %Xl",
                      name, pid, code);

        goto failed;

    case WAIT_TIMEOUT:
        ngx_log_error(NGX_LOG_ALERT, cycle->log, 0,
                      "the event \"%s\" was not signaled for 5s",
                      ngx_master_process_event_name);
        goto failed;

    case WAIT_FAILED:
        ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                      "WaitForSingleObject(\"%s\") failed",
                      ngx_master_process_event_name);

        goto failed;
    }
    //若respawn大于0,代表重启某个进程完成
    //其他的信息不需要更改,所以直接返回
    if (respawn >= 0) {
        return pid;
    }
     //根据传入的不同属性
    //将子进程对应的标识位置位
    switch (respawn) {

    case NGX_PROCESS_RESPAWN:
        ngx_processes[s].just_spawn = 0;
        break;

    case NGX_PROCESS_JUST_RESPAWN:
        ngx_processes[s].just_spawn = 1;
        break;
    }

    //ngx_last_process代表ngx_processes数组中占用了的元素个数
    if (s == ngx_last_process) {
        ngx_last_process++;
    }

    return pid;

failed://创建失败

    if (ngx_processes[s].reopen) {
        ngx_close_handle(ngx_processes[s].reopen);
    }

    if (ngx_processes[s].quit) {
        ngx_close_handle(ngx_processes[s].quit);
    }

    if (ngx_processes[s].term) {
        ngx_close_handle(ngx_processes[s].term);
    }

    TerminateProcess(ngx_processes[s].handle, 2);

    if (ngx_processes[s].handle) {
        ngx_close_handle(ngx_processes[s].handle);
        ngx_processes[s].handle = NULL;
    }

    return NGX_INVALID_PID;
}


ngx_pid_t
ngx_execute(ngx_cycle_t *cycle, ngx_exec_ctx_t *ctx)
{
    STARTUPINFO          si;
    PROCESS_INFORMATION  pi;

    //初始化pi,si
    ngx_memzero(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);

    ngx_memzero(&pi, sizeof(PROCESS_INFORMATION));

    //创建进程
    if (CreateProcess(ctx->path, ctx->args,
                      NULL, NULL, 0, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)
        == 0)
    {
        ngx_log_error(NGX_LOG_CRIT, cycle->log, ngx_errno,
                      "CreateProcess(\"%s\") failed", ngx_argv[0]);

        return 0;
    }

    ctx->child = pi.hProcess;

    if (CloseHandle(pi.hThread) == 0) {
        ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                      "CloseHandle(pi.hThread) failed");
    }

    ngx_log_error(NGX_LOG_NOTICE, cycle->log, 0,
                  "start %s process %P", ctx->name, pi.dwProcessId);
    //返回进程id号
    return pi.dwProcessId;
}
