/******************************************************************************
 * Copyright © 2014-2015 The SuperNET Developers.                             *
 *                                                                            *
 * See the AUTHORS, DEVELOPER-AGREEMENT and LICENSE files at                  *
 * the top-level directory of this distribution for the individual copyright  *
 * holder information and the developer policies on copyright and licensing.  *
 *                                                                            *
 * Unless otherwise agreed in a custom licensing agreement, no part of the    *
 * Nxt software, including this file, may be copied, modified, propagated,    *
 * or distributed except according to the terms contained in the LICENSE file *
 *                                                                            *
 * Removal or modification of this copyright notice is prohibited.            *
 *                                                                            *
 ******************************************************************************/

#include <stdio.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <memory.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <time.h>

int32_t conv_date(int32_t *secondsp,char *buf);

int32_t OS_init() { return(0); }
int32_t OS_getpid() { return(getpid()); }
int32_t OS_getppid() { return(getppid()); }
int32_t OS_waitpid(int32_t childpid,int32_t *statusp,int32_t flags) { return(waitpid(childpid,statusp,flags)); }

uint32_t OS_conv_datenum(int32_t datenum,int32_t hour,int32_t minute,int32_t second) // datenum+H:M:S -> unix time
{
    struct tm t;
    memset(&t,0,sizeof(t));
    t.tm_year = (datenum / 10000) - 1900, t.tm_mon = ((datenum / 100) % 100) - 1, t.tm_mday = (datenum % 100);
    t.tm_hour = hour, t.tm_min = minute, t.tm_sec = second;
    return((uint32_t)timegm(&t));
}

int32_t OS_conv_unixtime(int32_t *secondsp,time_t timestamp) // gmtime -> datenum + number of seconds
{
    struct tm t; int32_t datenum; uint32_t checktime; char buf[64];
    t = *gmtime(&timestamp);
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ",&t); //printf("%s\n",buf);
    datenum = conv_date(secondsp,buf);
    if ( (checktime= OS_conv_datenum(datenum,*secondsp/3600,(*secondsp%3600)/60,*secondsp%60)) != timestamp )
    {
        printf("error: timestamp.%lu -> (%d + %d) -> %u\n",timestamp,datenum,*secondsp,checktime);
        return(-1);
    }
    return(datenum);
}

int32_t OS_launch_process(char *args[])
{
    pid_t child_pid;
    if ( (child_pid= fork()) >= 0 )
    {
        if ( child_pid == 0 )
        {
            printf("plugin PID =  %d, parent pid = %d (%s, %s, %s, %s, %s)\n",getpid(),getppid(),args[0],args[1],args[2],args[3],args[4]);
            return(execv(args[0],args));
        }
        else
        {
            printf("parent PID =  %d, child pid = %d\n",getpid(),child_pid);
            return(child_pid);
        }
    }
    else return(-1);
}


// from tweetnacl
void randombytes(unsigned char *x,int xlen)
{
    static int fd = -1;
    int32_t i;
    if (fd == -1) {
        for (;;) {
            fd = open("/dev/urandom",O_RDONLY);
            if (fd != -1) break;
            sleep(1);
        }
    }
    while (xlen > 0) {
        if (xlen < 1048576) i = (int32_t)xlen; else i = 1048576;
        i = (int32_t)read(fd,x,i);
        if (i < 1) {
            sleep(1);
            continue;
        }
        x += i;
        xlen -= i;
    }
}

