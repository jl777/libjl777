//
//  mappedptr.h
//  xcode
//
//  Created by jl777 on 7/25/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//

#ifndef xcode_mappedptr_h
#define xcode_mappedptr_h

#ifdef _WIN32
#include <fcntl.h>
#include "mman-win.h"
#endif

/*void *portable_thread_create(void *funcp,void *argp)
{
    portable_thread_t *ptr;
    ptr = (uv_thread_t *)malloc(sizeof(portable_thread_t));
    if ( uv_thread_create(ptr,funcp,argp) != 0 ) //if ( pthread_create(ptr,NULL,funcp,argp) != 0 )
    {
        free(ptr);
        return(0);
    } else return(ptr);
}*/

void fatal(char *str) { printf("FATAL: (%s)\n",str); while ( 1 ) portable_sleep(10); }

#endif
