#ifndef NONPORTABLE_WIN_H
#define NONPORTABLE_WIN_H

//Includes
#include <inttypes.h>
#include <windows.h>
#include <winsock2.h>
#include <in6addr.h>
#include <io.h>
#include <errno.h>
#include <fcntl.h> /*  _O_BINARY */

//Type Definitions



//Data Structures
struct iovec
{
 void	*iov_base; /* Base address of a memory region for input or output */
 size_t	iov_len; /* The size of the memory pointed to by iov_base */
};

//Functions
#define pipe(phandles)	_pipe (phandles, 4096, _O_BINARY)




#endif
