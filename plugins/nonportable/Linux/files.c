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

#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>


int32_t os_supports_mappedfiles() { return(1); }
char *os_compatible_path(char *str) { return(str); }
int32_t portable_truncate(char *fname,long filesize) { return(truncate(fname,filesize)); }
char *OS_rmstr() { return("rm"); }

void ensure_directory(char *dirname)
{
    FILE *fp;
    if ( (fp= fopen(os_compatible_path(dirname),"rb")) == 0 )
        mkdir(dirname,511);
    else fclose(fp);
}

void *map_file(char *fname,uint64_t *filesizep,int32_t enablewrite)
{
	void *mmap64(void *addr,size_t len,int32_t prot,int32_t flags,int32_t fildes,off_t off);
	int32_t fd,rwflags,flags = MAP_FILE|MAP_SHARED;
	uint64_t filesize;
    void *ptr = 0;
	*filesizep = 0;
	if ( enablewrite != 0 )
		fd = open(fname,O_RDWR);
	else fd = open(fname,O_RDONLY);
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
#if __i386__
	ptr = mmap(0,filesize,rwflags,flags,fd,0);
#else
	ptr = mmap64(0,filesize,rwflags,flags,fd,0);
#endif
	close(fd);
    if ( ptr == 0 || ptr == MAP_FAILED )
	{
		printf("map_file.write%d: mapping %s failed? mp %p\n",enablewrite,fname,ptr);
		return(0);
	}
	*filesizep = filesize;
	return(ptr);
}
