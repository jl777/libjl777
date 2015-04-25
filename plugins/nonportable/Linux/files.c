
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <memory.h>
#include <math.h>
//#include <time.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>
#include <assert.h>
#include <float.h>
//#include <limits.h>
#include <zlib.h>
#include <pthread.h>

//#include <getopt.h>
//#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
//#include <sys/socket.h>
#include <sys/wait.h>
//#include <sys/time.h>

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
