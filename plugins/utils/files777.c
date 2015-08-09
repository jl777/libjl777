//
//  files777.c
//  crypto777
//
//  Created by James on 4/9/15.
//  Copyright (c) 2015 jl777. All rights reserved.
//

#ifdef DEFINES_ONLY
#ifndef crypto777_files777_h
#define crypto777_files777_h
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include "../common/system777.c"
#include "utils777.c"

// rest of file needs to be OS portable
struct mappedptr
{
	char fname[512];
	void *fileptr,*pending;
	uint64_t allocsize,changedsize;
	int32_t rwflag,actually_allocated;
};

void *loadfileM(char *fname,char **bufp,uint64_t *lenp,uint64_t *allocsizep);
void *loadfile(uint64_t *allocsizep,char *fname);

void ensure_dir(char *dirname);
long ensure_filesize(char *fname,long filesize,int32_t truncateflag);
int32_t compare_files(char *fname,char *fname2);
long copy_file(char *src,char *dest);
void delete_file(char *fname,int32_t scrubflag);

int32_t release_map_file(void *ptr,uint64_t filesize);
void close_mappedptr(struct mappedptr *mp);
int32_t open_mappedptr(struct mappedptr *mp);
int32_t sync_mappedptr(struct mappedptr *mp,uint64_t len);
void *init_mappedptr(void **ptrp,struct mappedptr *mp,uint64_t allocsize,int32_t rwflag,char *fname);
void *filealloc(struct mappedptr *M,char *fname,struct alloc_space *mem,long size);
void *tmpalloc(char *coinstr,struct alloc_space *mem,long size);

#endif
#else
#ifndef crypto777_files777_c
#define crypto777_files777_c

#ifndef crypto777_files777_h
#define DEFINES_ONLY
#include "files777.c"
#undef DEFINES_ONLY
#endif

void *_loadfile(char *fname,char **bufp,uint64_t *lenp,uint64_t *allocsizep)
{
    FILE *fp;
    int64_t  filesize,buflen = *allocsizep;
    char *buf = *bufp;
    *lenp = 0;
    if ( (fp= fopen(os_compatible_path(fname),"rb")) != 0 )
    {
        fseek(fp,0,SEEK_END);
        filesize = ftell(fp);
        if ( filesize == 0 )
        {
            fclose(fp);
            *lenp = 0;
            return(0);
        }
        if ( filesize > buflen-1 )
        {
            *allocsizep = filesize+1;
            *bufp = buf = realloc(buf,(long)*allocsizep);
        }
        rewind(fp);
        if ( buf == 0 )
            printf("Null buf ???\n");
        else
        {
            if ( fread(buf,1,(long)filesize,fp) != (unsigned long)filesize )
                printf("error reading filesize.%ld\n",(long)filesize);
            buf[filesize] = 0;
        }
        fclose(fp);
        *lenp = filesize;
    }
    return(buf);
}

void *loadfile(uint64_t *allocsizep,char *fname)
{
    uint64_t filesize = 0; char *buf = 0;
    *allocsizep = 0;
    return(_loadfile(fname,&buf,&filesize,allocsizep));
}
cJSON *check_conffile(int32_t *allocflagp,cJSON *json)
{
    char buf[MAX_JSON_FIELD],*filestr;
    uint64_t allocsize;
    cJSON *item;
    *allocflagp = 0;
    if ( json == 0 )
        return(0);
    copy_cJSON(buf,cJSON_GetObjectItem(json,"filename"));
    if ( buf[0] != 0 && (filestr= loadfile(&allocsize,buf)) != 0 )
    {
        if ( (item= cJSON_Parse(filestr)) != 0 )
        {
            json = item;
            *allocflagp = 1;
            printf("parsed (%s) for JSON\n",buf);
        }
        free(filestr);
    }
    return(json);
}

int32_t compare_files(char *fname,char *fname2)
{
    int32_t offset,errs = 0;
    long len,len2;
    char buf[8192],buf2[8192];
    FILE *fp,*fp2;
    if ( (fp= fopen(os_compatible_path(fname),"rb")) != 0 )
    {
        if ( (fp2= fopen(os_compatible_path(fname2),"rb")) != 0 )
        {
            while ( (len= fread(buf,1,sizeof(buf),fp)) > 0 && (len2= fread(buf2,1,sizeof(buf2),fp2)) == len )
                if ( (offset= memcmp(buf,buf2,len)) != 0 )
                    printf("compare error at offset.%d: (%s) src.%ld vs. (%s) dest.%ld\n",offset,fname,ftell(fp),fname2,ftell(fp2)), errs++;
            fclose(fp2);
        }
        fclose(fp);
    }
    return(errs);
}

long copy_file(char *src,char *dest)
{
    long allocsize,len = -1;
    char *buf;
    FILE *srcfp,*destfp;
    if ( (srcfp= fopen(os_compatible_path(src),"rb")) != 0 )
    {
        if ( (destfp= fopen(os_compatible_path(dest),"wb")) != 0 )
        {
            allocsize = 1024 * 1024 * 128L;
            buf = malloc(allocsize);
            while ( (len= fread(buf,1,allocsize,srcfp)) > 0 )
                if ( (long)fwrite(buf,1,len,destfp) != len )
                    printf("write error at (%s) src.%ld vs. (%s) dest.%ld\n",src,ftell(srcfp),dest,ftell(destfp));
            len = ftell(destfp);
            fclose(destfp);
            free(buf);
        }
        fclose(srcfp);
    }
    if ( len == 0 || compare_files(src,dest) != 0 )
        printf("Error copying files (%s) -> (%s)\n",src,dest), len = -1;
    return(len);
}

long ensure_filesize(char *fname,long filesize,int32_t truncateflag)
{
    FILE *fp;
    char *zeroes;
    long i,n,allocsize = 0;
printf("ensure_filesize.(%s) %ld %s | ",fname,filesize,_mbstr(filesize));
    if ( filesize == 0 )
        getchar();
    if ( (fp= fopen(os_compatible_path(fname),"rb")) != 0 )
    {
        fseek(fp,0,SEEK_END);
        allocsize = ftell(fp);
        fclose(fp);
        //printf("(%s) exists size.%ld\n",fname,allocsize);
    }
    else
    {
        //printf("try to create.(%s)\n",fname);
        if ( (fp= fopen(os_compatible_path(fname),"wb")) != 0 )
            fclose(fp);
    }
    if ( allocsize < filesize )
    {
        //printf("filesize.%ld is less than %ld\n",filesize,allocsize);
        if ( (fp= fopen(os_compatible_path(fname),"ab")) != 0 )
        {
			zeroes = myaligned_alloc(16*1024*1024);
            memset(zeroes,0,16*1024*1024);
            n = filesize - allocsize;
            while ( n > 16*1024*1024 )
            {
                fwrite(zeroes,1,16*1024*1024,fp);
                n -= 16*1024*1024;
                fprintf(stderr,".");
            }
            for (i=0; i<n; i++)
                fputc(0,fp);
            fclose(fp);
			aligned_free(zeroes);
        }
        return(filesize);
    }
    else if ( allocsize*truncateflag > filesize )
    {
        portable_truncate(fname,filesize);
        return(filesize);
    }
    else return(allocsize);
}

int32_t open_mappedptr(struct mappedptr *mp)
{
	uint64_t allocsize = mp->allocsize;
    if ( mp->actually_allocated != 0 )
	{
		if ( mp->fileptr == 0 )
			mp->fileptr = myaligned_alloc(mp->allocsize);
		else memset(mp->fileptr,0,mp->allocsize);
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
	}
	return(0);
}

void *init_mappedptr(void **ptrp,struct mappedptr *mp,uint64_t allocsize,int32_t rwflag,char *fname)
{
    char *_mbstr(double);
	uint64_t filesize;
	mp->actually_allocated = !os_supports_mappedfiles();
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
    if ( rwflag != 0 && mp->actually_allocated == 0 && allocsize != 0 )
        allocsize = ensure_filesize(fname,allocsize,0);
	if ( open_mappedptr(mp) != 0 )
	{
printf("init_mappedptr %s.rwflag.%d | ",fname,rwflag);
        if ( allocsize != 0 )
			printf("error mapping(%s) rwflag.%d ptr %p mapped %llu vs allocsize %llu %s\n",fname,rwflag,mp->fileptr,(long long)mp->allocsize,(long long)allocsize,_mbstr(allocsize));
        else allocsize = mp->allocsize;
		if ( rwflag != 0 && allocsize != mp->allocsize )
		{
			filesize = mp->allocsize;
			if  ( mp->fileptr != 0 )
				release_map_file(mp->fileptr,mp->allocsize);
			mp->allocsize = allocsize;
			mp->changedsize = (allocsize - filesize);
			open_mappedptr(mp);
			if ( mp->fileptr == 0 || mp->allocsize != allocsize )
				printf("SECOND error mapping(%s) ptr %p mapped %llu vs allocsize %llu\n",fname,mp->fileptr,(long long)mp->allocsize,(long long)allocsize);
		}
	}
	if ( ptrp != 0 )
		(*ptrp) = mp->fileptr;
    return(mp->fileptr);
}

int32_t sync_mappedptr(struct mappedptr *mp,uint64_t len)
{
    static uint64_t Sync_total;
	int32_t err;
	if ( mp->actually_allocated != 0 )
		return(0);
	if ( len == 0 )
		len = mp->allocsize;
	err = msync(mp->fileptr,len,MS_SYNC);
	if ( err != 0 )
		printf("sync (%s) len %llu, err %d\n",mp->fname,(long long)len,err);
    Sync_total += len;
	return(err);
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
	return(retval);
}

void _close_mappedptr(struct mappedptr *mp)
{
	if ( mp->actually_allocated != 0 && mp->fileptr != 0 )
        aligned_free(mp->fileptr);
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

/*void *permalloc(char *coinstr,struct alloc_space *mem,long size,int32_t selector)
{
    static long counts[100],totals[sizeof(counts)/sizeof(*counts)],n;
    struct mappedptr M;
    char fname[1024];
    int32_t i;
    counts[0]++;
    totals[0] += size;
    counts[selector]++;
    totals[selector] += size;
    //printf("mem->used %ld size.%ld | size.%ld\n",mem->used,size,mem->size);
    if ( (mem->used + size) > mem->size )
    {
        for (i=0; i<(int)(sizeof(counts)/sizeof(*counts)); i++)
            if ( counts[i] != 0 )
                printf("(%s %.1f).%d ",_mbstr(totals[i]),(double)totals[i]/counts[i],i);
        printf(" | ");
        printf("permalloc new space.%ld %s | selector.%d itemsize.%ld total.%ld n.%ld ave %.1f | total %s n.%ld ave %.1f\n",mem->size,_mbstr(mem->size),selector,size,totals[selector],counts[selector],(double)totals[selector]/counts[selector],_mbstr2(totals[0]),counts[0],(double)totals[0]/counts[0]);
        memset(&M,0,sizeof(M));
        sprintf(fname,"/tmp/%s.space.%ld",coinstr,n);
        os_compatible_path(fname);
        // delete_file(fname,0);
        if ( size > mem->size )
            mem->size = size;
        if ( init_mappedptr(0,&M,mem->size,1,fname) == 0 )
        {
            printf("couldnt create mapped file.(%s)\n",fname);
            exit(-1);
        }
        n++;
        mem->ptr = M.fileptr;
        mem->used = 0;
        if ( mem->size == 0 )
        {
            mem->size = size;
            return(mem->ptr);
        }
    }
    return(memalloc(mem,size,0));
}*/

void ensure_dir(char *dirname)
{
    FILE *fp;
    char fname[512],cmd[512];
    sprintf(fname,"%s/tmp",dirname);
    if ( (fp= fopen(os_compatible_path(fname),"rb")) == 0 )
    {
        sprintf(cmd,"mkdir %s",os_compatible_path(dirname));
        if ( system(os_compatible_path(cmd)) != 0 )
            printf("error making subdirectory (%s) %s (%s)\n",cmd,dirname,fname);
        fp = fopen(os_compatible_path(fname),"wb");
        if ( fp != 0 )
            fclose(fp);
        if ( (fp= fopen(os_compatible_path(fname),"rb")) == 0 )
        {
            printf("failed to create.(%s) in (%s)\n",fname,dirname);
            exit(-1);
        } else printf("ensure_dir(%s) created.(%s)\n",dirname,fname);
    }
    fclose(fp);
}

void delete_file(char *fname,int32_t scrubflag)
{
    FILE *fp;
    char cmdstr[1024];
    long i,fpos;
    if ( (fp= fopen(os_compatible_path(fname),"rb+")) != 0 )
    {
        printf("delete(%s)\n",fname);
        if ( scrubflag != 0 )
        {
            fseek(fp,0,SEEK_END);
            fpos = ftell(fp);
            rewind(fp);
            for (i=0; i<fpos; i++)
                fputc(0xff,fp);
            fflush(fp);
        }
        fclose(fp);
        sprintf(cmdstr,"%s %s",OS_rmstr(),fname);
        if ( system(os_compatible_path(cmdstr)) != 0 )
            printf("error deleting file.(%s)\n",cmdstr);
    }
}

void *filealloc(struct mappedptr *M,char *fname,struct alloc_space *mem,long size)
{
    //printf("mem->used %ld size.%ld | size.%ld\n",mem->used,size,mem->size);
    printf("filemalloc.(%s) new space.%ld %s\n",fname,mem->size,_mbstr(size));
    memset(M,0,sizeof(*M));
    //fix_windows_insanity(fname);
    mem->size = size;
    if ( init_mappedptr(0,M,mem->size,1,fname) == 0 )
    {
        printf("couldnt create mapped file.(%s)\n",fname);
        exit(-1);
    }
    mem->ptr = M->fileptr;
    mem->used = 0;
    return(M->fileptr);
}

void *tmpalloc(char *coinstr,struct alloc_space *mem,long size)
{
    static int32_t n;
    char fname[1024]; struct mappedptr M;
    if ( (mem->used + size) > mem->size )
    {
        sprintf(fname,"/tmp/%s.space.%d",coinstr,n++);
        if ( mem->size == 0 )
            mem->size = 1024 * 1024 * 128L * ((strcmp(coinstr,"BTC") == 0) ? 8 : 1);
        if ( filealloc(&M,fname,mem,MAX(mem->size,size)) == 0 )
        {
            printf("couldnt map tmpfile %s\n",fname);
            return(0);
        }
    }
    return(memalloc(mem,size,0));
}

#endif
#endif
