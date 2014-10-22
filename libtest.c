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

int main(int argc,const char *argv[])
{
    long stripwhite_ns(char *buf,long len);
	char *retstr,cmdstr[1024],buf[1024];
    SuperNET_start("SuperNET.conf","209.126.70.159");
    while ( 1 )
    {
        memset(buf,0,sizeof(buf));
        fgets(buf,sizeof(buf),stdin);
        stripwhite_ns(buf,(int32_t)strlen(buf));
        sprintf(cmdstr,"{\"requestType\":%s",buf);
        retstr = SuperNET_JSON(cmdstr);
        printf("input.(%s) -> (%s)\n",cmdstr,retstr);
        if ( retstr != 0 )
            free(retstr);
    }
    return(0);
}

int32_t SuperNET_broadcast(char *msg,int32_t duration) { return(0); }
int32_t SuperNET_narrowcast(char *destip,unsigned char *msg,int32_t len) { return(0); }
