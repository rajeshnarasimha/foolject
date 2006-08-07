#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#ifdef POSIX

typedef enum {
	FALSE,
	TRUE
}BOOL;

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h> 
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <limits.h>
#include <dirent.h>

typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
typedef struct sockaddr *PSOCKADDR;

#define INVALID_SOCKET  (SOCKET)(~0)
#define SOCKET_ERROR            (-1)
#define INADDR_NONE             0xffffffff
#define SD_BOTH         0x02

#define closesocket	close
#define _snprintf snprintf

typedef int HANDLE;

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#endif

#endif
