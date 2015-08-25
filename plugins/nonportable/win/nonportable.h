// MIT license

#ifndef NONPORTABLE_WIN_H
#define NONPORTABLE_WIN_H

//Includes
#include <inttypes.h>
#include <winsock2.h>
#include <windows.h>
#include <in6addr.h>
#include <ws2tcpip.h>
#include <io.h>
#include <errno.h>
#include <fcntl.h> /*  _O_BINARY */
#include <stdlib.h>

//Type Definitions
#define in_addr_t uint32_t


//Data Structures
struct iovec
{
 void	*iov_base; /* Base address of a memory region for input or output */
 size_t	iov_len; /* The size of the memory pointed to by iov_base */
};

//Functions
#define pipe(phandles)	_pipe (phandles, 4096, _O_BINARY)




#endif
