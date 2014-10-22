//
//  main.c
//  libtest
//
//  Created by jl777 on 8/13/14.
//  Copyright (c) 2014 jl777. MIT License.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>
#include "SuperNET.h"

extern int32_t IS_LIBTEST;

int main(int argc,const char *argv[])
{
    long stripwhite_ns(char *buf,long len);
	char *retstr,cmdstr[1024],buf[1024],buf2[1024];
    IS_LIBTEST = 1;
    SuperNET_start("SuperNET.conf",0);
    while ( 1 )
    {
        memset(buf,0,sizeof(buf));
        fgets(buf,sizeof(buf),stdin);
        stripwhite_ns(buf,(int32_t)strlen(buf));
        if ( strcmp("p",buf) == 0 )
            strcpy(buf2,"\"getpeers\"}'");
        else if ( strcmp("q",buf) == 0 )
            exit(0);
        else if ( buf[0] == 'P' && buf[1] == ' ' )
            sprintf(buf2,"\"ping\",\"destip\":\"%s\"}'",buf+2);
        else strcpy(buf2,buf);
        sprintf(cmdstr,"{\"requestType\":%s",buf2);
        retstr = SuperNET_JSON(cmdstr);
        printf("input.(%s) -> (%s)\n",cmdstr,retstr);
        if ( retstr != 0 )
            free(retstr);
    }
    return(0);
}

int32_t SuperNET_broadcast(char *msg,int32_t duration) { return(0); }
int32_t SuperNET_narrowcast(char *destip,unsigned char *msg,int32_t len) { return(0); }
