//
//  mappedptr.h
//  xcode
//
//  Created by jl777 on 7/25/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//

#ifndef xcode_mappedptr_h
#define xcode_mappedptr_h



int32_t portable_mutex_init(portable_mutex_t *mutex)
{
    return(uv_mutex_init(mutex)); //pthread_mutex_init(mutex,NULL);
}

void portable_mutex_lock(portable_mutex_t *mutex)
{
    //printf("lock.%p\n",mutex);
    uv_mutex_lock(mutex); // pthread_mutex_lock(mutex);
}

void portable_mutex_unlock(portable_mutex_t *mutex)
{
    // printf("unlock.%p\n",mutex);
    uv_mutex_unlock(mutex); //pthread_mutex_unlock(mutex);
}

void *portable_thread_create(void *funcp,void *argp)
{
    portable_thread_t *ptr;
    ptr = (uv_thread_t *)malloc(sizeof(portable_thread_t));
    if ( uv_thread_create(ptr,funcp,argp) != 0 ) //if ( pthread_create(ptr,NULL,funcp,argp) != 0 )
    {
        free(ptr);
        return(0);
    } else return(ptr);
}

void fatal(char *str) { printf("FATAL: (%s)\n",str); while ( 1 ) sleep(10); }

#endif
