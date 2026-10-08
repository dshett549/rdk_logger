/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2016 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

/**
 * @file rdk_dynamic_logger.c
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/select.h>
#include <sys/time.h>

#include "rdk_dynamic_logger.h"
#include "rdk_debug_priv.h"

#define DL_SOCKET_DIR "/run/rdk_logger"
#define DL_SIGNATURE "COMC"
#define DL_SIGNATURE_LEN 4

static int g_dl_socket = -1;
static struct sockaddr_un g_dl_addr;
static pid_t g_dl_owner_pid;
extern char *__progname;

static char * rdk_dyn_log_logLevelToString(rdk_LogLevel log_level)
{
    switch(log_level){
        case RDK_LOG_FATAL:  return "FATAL";
        case RDK_LOG_ERROR:  return "ERROR";
        case RDK_LOG_WARN:   return "WARNING";
        case RDK_LOG_NOTICE: return "NOTICE";
        case RDK_LOG_INFO:   return "INFO";
        case RDK_LOG_DEBUG:  return "DEBUG";
        case RDK_LOG_TRACE:  return "TRACE";
        case RDK_LOG_NONE:   return "NONE";
    }
    return "NONE";
}

static void rdk_dyn_log_validate_component_name(const unsigned char *buf)
{
    unsigned char log_level = 0;
    int app_len, comp_len, i = DL_SIGNATURE_LEN;
    char comp_name[64] = {0};

    if(0 != memcmp(buf,DL_SIGNATURE,i)) {
        return;
    }

    log_level = buf[++i];
    app_len = buf[++i];

        if(app_len == 0 || __progname == NULL ||
            (size_t)app_len != strlen(__progname) ||
            0 != memcmp(buf+(++i),__progname,app_len)) {
        /* The received msg is not intended for this process */
        return;
    }

    i += app_len;
    comp_len = buf[i];
    
    if (comp_len >= sizeof(comp_name)) 
    {
       fprintf(stderr, "Error: component name too long\n");
       return;
    }

    rdk_LogLevel loggingLevel = (rdk_LogLevel) log_level;

    if((loggingLevel >= RDK_LOG_FATAL) && (loggingLevel <= RDK_LOG_NONE))
    {
        memcpy(comp_name,buf+(++i),comp_len);
        rdk_dbg_priv_log_reconfig(comp_name, loggingLevel);
        fprintf(stderr, "Log level change request to %s (%u) for the component %s, is success\n", rdk_dyn_log_logLevelToString(loggingLevel), loggingLevel, comp_name);
    }
    else
    {
        fprintf(stderr, "Log level change request with Invalid input (%u)\n", loggingLevel);
    }

    return;
}

void rdk_dyn_log_process_pending_request()
{
    unsigned char buf[128];
    union {
        struct cmsghdr align;
        char buf[CMSG_SPACE(sizeof(struct ucred))];
    } control;
    struct iovec payload;
    struct msghdr msg;
    struct cmsghdr *cmsg;
    struct timeval tv;
    int numbytes, ret;
    size_t app_len;
    fd_set rfds;

    if(-1 == g_dl_socket || g_dl_owner_pid != getpid())
        return;
    while(1) {
        FD_ZERO(&rfds);
        FD_SET(g_dl_socket, &rfds);

        tv.tv_sec = 0;
        tv.tv_usec = 0;
        ret = select(g_dl_socket+1,&rfds,NULL,NULL,&tv);
        if(ret <= 0)
            break;

        memset(&msg, 0, sizeof(msg));
        payload.iov_base = buf;
        payload.iov_len = sizeof(buf);
        msg.msg_iov = &payload;
        msg.msg_iovlen = 1;
        msg.msg_control = control.buf;
        msg.msg_controllen = sizeof(control.buf);
        if ((numbytes=recvmsg(g_dl_socket, &msg, MSG_DONTWAIT)) == -1) {
            if(errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            fprintf(stderr,"%s recvmsg failed %s\n",__func__,strerror(errno));
            return;
        }

        if(msg.msg_flags & (MSG_TRUNC | MSG_CTRUNC))
            continue;
        for(cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
            if(cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_CREDENTIALS &&
                    cmsg->cmsg_len >= CMSG_LEN(sizeof(struct ucred)))
                break;
        }
        if(cmsg == NULL || ((struct ucred *)CMSG_DATA(cmsg))->uid != 0)
            continue;

        if(numbytes < DL_SIGNATURE_LEN + 4 ||
                (size_t)numbytes != (size_t)buf[DL_SIGNATURE_LEN] + DL_SIGNATURE_LEN + 1)
            continue;
        app_len = buf[DL_SIGNATURE_LEN + 2];
        if(app_len >= (size_t)numbytes - DL_SIGNATURE_LEN - 3 ||
                buf[DL_SIGNATURE_LEN + 3 + app_len] !=
                (size_t)numbytes - DL_SIGNATURE_LEN - 4 - app_len)
            continue;

        rdk_dyn_log_validate_component_name(buf);
    }
}

void rdk_dyn_log_init()
{
    struct stat dir_stat;
    int opt = 1;

    if(g_dl_socket != -1 && g_dl_owner_pid != getpid()) {
        close(g_dl_socket);
        g_dl_socket = -1;
    }
    if(g_dl_socket != -1 || geteuid() != 0)
        return;
    if(mkdir(DL_SOCKET_DIR, 0700) == -1 && errno != EEXIST)
        return;
    if(lstat(DL_SOCKET_DIR, &dir_stat) == -1 || !S_ISDIR(dir_stat.st_mode) ||
            dir_stat.st_uid != 0 || (dir_stat.st_mode & 0777) != 0700)
        return;

    memset(&g_dl_addr, 0, sizeof(g_dl_addr));
    g_dl_addr.sun_family = AF_UNIX;
    if(snprintf(g_dl_addr.sun_path, sizeof(g_dl_addr.sun_path),
            DL_SOCKET_DIR "/%ld.sock", (long)getpid()) >= (int)sizeof(g_dl_addr.sun_path))
        return;

    if ((g_dl_socket = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1) {
        fprintf(stderr, "%s() Socket creation failed. %s\n",__func__,strerror(errno));
        return;
    }

    if (-1 == setsockopt(g_dl_socket,SOL_SOCKET,SO_PASSCRED,&opt,sizeof(int))) {
        fprintf(stderr, "%s() setsockopt error %s\n",__func__,strerror(errno));
        close(g_dl_socket);
        g_dl_socket = -1;
        return;
    }

    unlink(g_dl_addr.sun_path);
    if (-1 == bind(g_dl_socket, (struct sockaddr *)&g_dl_addr, sizeof(g_dl_addr)) ||
            -1 == chmod(g_dl_addr.sun_path, 0600)) {
        fprintf(stderr, "%s() bind error %s\n",__func__,strerror(errno));
        close(g_dl_socket);
        g_dl_socket = -1;
        unlink(g_dl_addr.sun_path);
        return;
    }
    g_dl_owner_pid = getpid();

    //fprintf(stderr, "%sg_dl_socket = %d __progname = %s \n",__func__,g_dl_socket,__progname);
}

void rdk_dyn_log_deinit()
{
    if (g_dl_socket != -1)
    {
        close(g_dl_socket);
        g_dl_socket = -1;
        if(g_dl_owner_pid == getpid())
            unlink(g_dl_addr.sun_path);
    }
}

