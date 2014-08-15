//
//  mappedptr.h
//  xcode
//
//  Created by jl777 on 7/25/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//

#ifndef xcode_mappedptr_h
#define xcode_mappedptr_h


struct mappedptr
{
	char fname[512];
	void *fileptr,*pending;
	uint64_t allocsize,changedsize;
	int32_t rwflag,actually_allocated;
};


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

portable_thread_t *portable_thread_create(void *funcp,void *argp)
{
    portable_thread_t *ptr;
    ptr = (uv_thread_t *)malloc(sizeof(portable_thread_t));
    if ( uv_thread_create(ptr,funcp,argp) != 0 ) //if ( pthread_create(ptr,NULL,funcp,argp) != 0 )
    {
        free(ptr);
        return(0);
    } else return(ptr);
}

void *jl777malloc(size_t allocsize) { void *ptr = malloc(allocsize); if ( ptr == 0 ) { printf("malloc(%ld) failed\n",allocsize); while ( 1 ) sleep(60); } return(ptr); }
void *jl777calloc(size_t num,size_t allocsize) { void *ptr = calloc(num,allocsize); if ( ptr == 0 ) { printf("calloc(%ld,%ld) failed\n",num,allocsize); while ( 1 ) sleep(60); } return(ptr); }
#define malloc jl777malloc
#define calloc jl777calloc

void fatal(char *str) { printf("FATAL: (%s)\n",str); while ( 1 ) sleep(10); }

double _kb(double n) { return(n / 1024.); }
double _mb(double n) { return(n / (1024.*1024.)); }
double _gb(double n) { return(n / (1024.*1024.*1024.)); }
double _hrs(double n) { return(n / 3600.); }
double _days(double n) { return(n / (3600. * 24.)); }

char *_mbstr(double n)
{
	static char str[100];
	if ( n < 1024*1024*10 )
		sprintf(str,"%.3fkb",_kb(n));
	else if ( n < 1024*1024*1024 )
		sprintf(str,"%.1fMB",_mb(n));
	else
		sprintf(str,"%.2fGB",_gb(n));
	return(str);
}

uint64_t _align16(uint64_t ptrval) { if ( (ptrval & 15) != 0 ) ptrval += 16 - (ptrval & 15); return(ptrval); }

void *alloc_aligned_buffer(uint64_t allocsize)
{
	extern int posix_memalign (void **__memptr, size_t __alignment, size_t __size);
	if ( allocsize > ((uint64_t)192L)*1024*1024*1024 )
		printf("%llu\n",(long long)allocsize),fatal("negative allocsize");
	void *ptr;
	allocsize = _align16(allocsize);
	if ( posix_memalign(&ptr,16,allocsize) != 0 )
	{
		printf("alloc_aligned_buffer can't get allocsize %llu\n",(long long)allocsize);
		//fatal("can't posix_memalign()");
	}
	if ( ((unsigned long)ptr & 15) != 0 )
		printf("%p[%llu]\n",ptr,(long long)allocsize),fatal("alloc_aligned_buffer misaligned");
	if ( allocsize > 1024*1024*1024L )
	{
		void *tmp = ptr;
		long n = allocsize;
		while ( n > 1024*1024*1024L )
		{
			//printf("ptr %p %ld: tmp %p, n %ld\n",ptr,allocsize,tmp,n);
			memset(tmp,0,1024*1024*1024L);
			tmp = (void *)((long)tmp + 1024*1024*1024L);
			n -= 1024*1024*1024L;
			//printf("AFTER ptr %p %ld: tmp %p, n %ld\n",ptr,allocsize,tmp,n);
		}
		if ( n > 0 )
			memset(tmp,0,n);
	}
	else memset(ptr,0,allocsize);
	return(ptr);
}

#ifndef WIN32
void *map_file(char *fname,uint64_t *filesizep,int32_t enablewrite)
{
	void *mmap64(void *addr,size_t len,int32_t prot,int32_t flags,int32_t fildes,off_t off);
	int32_t fd,rwflags,flags = MAP_FILE|MAP_SHARED;
	uint64_t filesize;
    void *ptr = 0;
	*filesizep = 0;
	if ( enablewrite != 0 )
		fd = open(fname,O_RDWR);
	else
  		fd = open(fname,O_RDONLY);
	if ( fd < 0 )
	{
		//printf("map_file: error opening enablewrite.%d %s\n",enablewrite,fname);
        return(0);
	}
    if ( *filesizep == 0 )
        filesize = (uint64_t)lseek(fd,0,SEEK_END);
    else
        filesize = *filesizep;
    //if ( filesize > MAX_MAPFILE_SIZE ) filesize = MAX_MAPFILE_SIZE;
	// printf("filesize %ld vs expected %ld\n",filesize,*filesizep);
	rwflags = PROT_READ;
	if ( enablewrite != 0 )
		rwflags |= PROT_WRITE;
#ifdef __APPLE__
	ptr = mmap(0,filesize,rwflags,flags,fd,0);
#else
	ptr = mmap64(0,filesize,rwflags,flags,fd,0);
#endif
	close(fd);
    if ( ptr == 0 || ptr == MAP_FAILED )
	{
		printf("map_file.write%d: mapping %s failed? mp %p\n",enablewrite,fname,ptr);
		//fatal("FATAL ERROR");
		return(0);
	}
	//if ( 0 && MACHINEID == 2 )
	//	printf("MAPPED(%s).rw%d %lx %ld %.1fmb    | ",fname,enablewrite,filesize,filesize,(double)filesize/1000000);
	*filesizep = filesize;
	return(ptr);
}

int32_t release_map_file(void *ptr,uint64_t filesize)
{
	int32_t retval;
    if ( ptr == 0 )
	{
		printf("release_map_file: null ptr\n");
		return(-1);
	}
	retval = munmap(ptr,filesize);
	if ( retval != 0 )
		printf("release_map_file: munmap error %p %llu: err %d\n",ptr,(long long)filesize,retval);
	//else
	//	printf("released %p %ld\n",ptr,filesize);
	return(retval);
}

void _close_mappedptr(struct mappedptr *mp)
{
	//if ( MACHINEID == 2 && mp->actually_allocated == 0 )
	// {
	// printf("map_mappedptr warning: (%s) actually_allocated flag is 0?\n",mp->fname);
	// mp->actually_allocated = 0;
	// }
	if ( mp->actually_allocated != 0 && mp->fileptr != 0 )
		free(mp->fileptr);
	else if ( mp->fileptr != 0 )
		release_map_file(mp->fileptr,mp->allocsize);
	mp->fileptr = 0;
}

void close_mappedptr(struct mappedptr *mp)
{
	struct mappedptr tmp;
	tmp = *mp;
	_close_mappedptr(mp);
	memset(mp,0,sizeof(*mp));
	mp->actually_allocated = tmp.actually_allocated;
	mp->allocsize = tmp.allocsize;
	mp->rwflag = tmp.rwflag;
	strcpy(mp->fname,tmp.fname);
}

int32_t open_mappedptr(struct mappedptr *mp)
{
	uint64_t allocsize = mp->allocsize;
	if ( mp->actually_allocated != 0 )
	{
		if ( mp->fileptr == 0 )
			mp->fileptr = alloc_aligned_buffer(mp->allocsize);
		else
			memset(mp->fileptr,0,mp->allocsize);
		return(0);
	}
	else
	{
		if ( mp->fileptr != 0 )
		{
			//printf("opening already open mappedptr, pending %p\n",mp->pending);
			close_mappedptr(mp);
		}
        mp->allocsize = allocsize;
		// printf("calling map_file with expected %ld\n",mp->allocsize);
		mp->fileptr = map_file(mp->fname,&mp->allocsize,mp->rwflag);
		if ( mp->fileptr == 0 || mp->allocsize != allocsize )
		{
			//printf("error mapping(%s) ptr %p mapped %ld vs allocsize %ld\n",mp->fname,mp->fileptr,mp->allocsize,allocsize);
			return(-1);
		}
		//if ( 0 && MACHINEID == 2 )
		//	printf("mapped (%s) -> %p %ld %s\n",mp->fname,mp->fileptr,mp->allocsize,_mbstr(mp->allocsize));
	}
	return(0);
}

void sync_mappedptr(struct mappedptr *mp,uint64_t len)
{
    static uint64_t Sync_total;
	int32_t err;
	if ( mp->actually_allocated != 0 )
		return;
	if ( len == 0 )
		len = mp->allocsize;
	//printf("sync mp.%p len.%ld\n",mp,len);
	err = msync(mp->fileptr,len,MS_ASYNC);
	if ( err != 0 )
		printf("sync (%s) len %llu, err %d\n",mp->fname,(long long)len,err);
	else if ( 0 )
	{
		release_map_file(mp->fileptr,mp->allocsize);
		mp->fileptr = 0;
		mp->fileptr = map_file(mp->fname,&mp->allocsize,mp->rwflag);
		if ( mp->fileptr == 0 )
			fatal("couldn't get mp->fileptr after sync and close");
		Sync_total += len;
	}
}

void *init_mappedptr(void **ptrp,struct mappedptr *mp,uint64_t allocsize,int32_t rwflag,char *fname)
{
	FILE *fp;
	uint64_t i,n,filesize;
    //printf("init_mappedptr %s.rwflag.%d\n",fname,rwflag);
    if ( fname != 0 )
	{
		if ( strcmp(mp->fname,fname) == 0 )
		{
			if ( mp->fileptr != 0 )
			{
				release_map_file(mp->fileptr,mp->allocsize);
				mp->fileptr = 0;
			}
			open_mappedptr(mp);
			if ( ptrp != 0 )
				(*ptrp) = mp->fileptr;
			return(mp->fileptr);
		}
		strcpy(mp->fname,fname);
	}
	else mp->actually_allocated = 1;
	mp->rwflag = rwflag;
	mp->allocsize = allocsize;
	if ( open_mappedptr(mp) != 0 )
	{
		if ( allocsize != 0 )
			printf("error mapping(%s) rwflag.%d ptr %p mapped %llu vs allocsize %llu %s\n",fname,rwflag,mp->fileptr,(long long)mp->allocsize,(long long)allocsize,_mbstr(allocsize));
		if ( rwflag != 0 )
		{
			filesize = mp->allocsize;
			if  ( mp->fileptr != 0 )
				release_map_file(mp->fileptr,mp->allocsize);
			mp->allocsize = allocsize;
			mp->changedsize = (allocsize - filesize);
			if ( filesize < allocsize )
			{
				if ( (fp=fopen(fname,"ab")) != 0 )
				{
					char *zeroes;
                    zeroes = valloc(16*1024*1024);
					memset(zeroes,0,16*1024*1024);
					n = allocsize - filesize;
					while ( n > 16*1024*1024 )
					{
						fwrite(zeroes,1,16*1024*1024,fp);
						n -= 16*1024*1024;
						fprintf(stderr,".");
					}
					for (i=0; i<n; i++)
						fputc(0,fp);
					fclose(fp);
                    free(zeroes);
                }
			}
			else if ( filesize > allocsize )
				truncate(fname,allocsize);
			open_mappedptr(mp);
			if ( mp->fileptr == 0 || mp->allocsize != allocsize )
				printf("SECOND error mapping(%s) ptr %p mapped %llu vs allocsize %llu\n",fname,mp->fileptr,(long long)mp->allocsize,(long long)allocsize);
		}
	}
	if ( ptrp != 0 )
		(*ptrp) = mp->fileptr;
    return(mp->fileptr);
}
#endif

#endif
