/******************************************************************************
 * Copyright © 2014-2015 The SuperNET Developers.                             *
 *                                                                            *
 * See the AUTHORS, DEVELOPER-AGREEMENT and LICENSE files at                  *
 * the top-level directory of this distribution for the individual copyright  *
 * holder information and the developer policies on copyright and licensing.  *
 *                                                                            *
 * Unless otherwise agreed in a custom licensing agreement, no part of the    *
 * SuperNET software, including this file may be copied, modified, propagated *
 * or distributed except according to the terms contained in the LICENSE file *
 *                                                                            *
 * Removal or modification of this copyright notice is prohibited.            *
 *                                                                            *
 ******************************************************************************/

#include <sys/mman.h>
#include <io.h>
#include <share.h>
#include <errno.h>
#include <string.h>
#include <windows.h>
#include <fcntl.h>
#ifdef _WIN32
#include <stdio.h>
#include <inttypes.h>
#endif

int32_t os_supports_mappedfiles() { return(1); }
char *OS_rmstr() { return("del"); }

char *os_compatible_path(char *str)
{
    int32_t i;
    for (i=0; str[i]!=0; i++)
        if ( str[i] == '/' )
            str[i] = '\\';
    return(str);
}

void ensure_directory(char *dirname)
{
    FILE *fp;
    if ( (fp= fopen(os_compatible_path(dirname),"rb")) == 0 )
#ifndef _WIN32
        mkdir(dirname,511);
#else
        mkdir(dirname);
#endif
    else fclose(fp);
}

int32_t portable_truncate(char *fname,long filesize)
{
    return(truncate(fname,filesize));
}

void *map_file(char *fname,uint64_t *filesizep,int32_t enablewrite)
{
	int32_t fd,rwflags,flags = MAP_FILE|MAP_SHARED;
	uint64_t filesize;
    void *ptr = 0;
	*filesizep = 0;
	if ( enablewrite != 0 )
		fd = _sopen(fname, _O_RDWR | _O_BINARY, _SH_DENYNO);
	else fd = _sopen(fname, _O_RDONLY | _O_BINARY, _SH_DENYNO);
	if ( fd < 0 )
	{
		printf("map_file: error opening enablewrite.%d %s\n",enablewrite,fname);
        return(0);
	}
    if ( *filesizep == 0 )
        filesize = (uint64_t)lseek(fd,0,SEEK_END);
    else filesize = *filesizep;
 	rwflags = PROT_READ;
	if ( enablewrite != 0 )
		rwflags |= PROT_WRITE;
	ptr = mmap(0,filesize,rwflags,flags,fd,0);
	_close(fd);
    if ( ptr == 0 || ptr == MAP_FAILED )
	{
		printf("map_file.write%d: mapping %s failed? mp %p\n",enablewrite,fname,ptr);
		return(0);
	}
	*filesizep = filesize;
	return(ptr);
}

