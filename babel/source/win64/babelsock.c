/* ===================================================================
** Copyright (C) 2003 Ra-Tek. All rights reserved.
**
** File: babelsock.c
**
** Abstract:
**   
**
** Required Defines:
**   
**
** ===============================================
*/
#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <string.h>
#include <stdarg.h>
#include <winsock2.h>

#include "babelsock.h"

static int maxV(int *babelSockList, int length);


/* =================================================================== 
**
** Function: babelSockInit  
**  
** This function is futher described in a header file.
**   
** Implementation details:
**
** =============================================== 
*/
int 
babelSockInit(void) 
{
    WSADATA wsaData;
    WORD versionRequested;

    /* It is safe to call WSAStartup multiple times. Each WSAStartup must 
     * be paired with a WSACleanup.
     */
    versionRequested = MAKEWORD(2, 2);

    if (WSAStartup(versionRequested, &wsaData) != 0) 
    {
        return BABELSOCK_EINIT;
    }

    return BABELSOCK_OK;
}


/* =================================================================== 
**
** Function: babelSock 
**  
** This function is futher described in a header file.
**   
** Implementation details:
**
** =============================================== 
*/
int 
babelSock(int type) 
{
    int rvSock;
    int rvOpt;
    int on;
    
    on = 1;
    switch (type) 
    {
    case BABELSOCK_TCP:
        rvSock = socket(AF_INET, SOCK_STREAM, 0);
        if (rvSock < 0) 
        {
            return BABELSOCK_ESOCKET;
	}
        rvOpt = setsockopt(rvSock, 
                           SOL_SOCKET, 
                           SO_REUSEADDR, 
                           (const char *) &on, 
                           sizeof(on));
        if (rvOpt < 0) 
        {
            (void) closesocket(rvSock);
            return BABELSOCK_ESOCKOPT;
        }
        rvOpt = setsockopt(rvSock, 
                           IPPROTO_TCP,
                           TCP_NODELAY, 
                           (const char *)&on, 
                           sizeof(on));
        if (rvOpt < 0) 
        {
            (void) closesocket(rvSock);
            return BABELSOCK_ESOCKOPT;
        }
        break;
        
    case BABELSOCK_UDP:
        rvSock = socket(AF_INET, SOCK_DGRAM, 0);
        if (rvSock < 0) 
        {
            return BABELSOCK_ESOCKET;
	}
        rvOpt = setsockopt(rvSock, 
                           SOL_SOCKET, 
                           SO_BROADCAST, 
                           (const char *)&on, 
                           sizeof(on));
        if (rvOpt < 0) 
        {
            (void) closesocket(rvSock);
            return BABELSOCK_ESOCKOPT;
	}
        break;

    default:
        return BABELSOCK_EPROTO;
        break;
    }
    return rvSock;
}


/* =================================================================== 
**
** Function: babelSockConnect 
**  
** This function is futher described in a header file.
**   
** Implementation details:
**
** =============================================== 
*/
int 
babelSockConnect(int fd, 
                 char const *hostName, 
                 unsigned short port)
{
    int rv;
    struct sockaddr_in hemAddr;
    struct hostent *hostInfo;

    hostInfo = gethostbyname(hostName);
    if (hostInfo == NULL) 
    {
        return BABELSOCK_EGETHOST;
    }

    memset(&hemAddr, 0, sizeof(hemAddr));
    hemAddr.sin_family = AF_INET;
    hemAddr.sin_port = htons(port);
    hemAddr.sin_addr = *(struct in_addr *)hostInfo->h_addr;
    
    rv = connect(fd, (struct sockaddr *)&hemAddr, sizeof(hemAddr));
    if (rv < 0) 
    {
        return BABELSOCK_ECONNECT; 
    }
    return BABELSOCK_OK;
}


/* =================================================================== 
**
** Function: babelSockBind 
**  
** This function is futher described in a header file.
**   
** Implementation details:
**
** =============================================== 
*/
int 
babelSockBind(int fd, char const *hostName, unsigned short port) 
{    
    struct sockaddr_in hemAddr;
    struct hostent *hostInfo;
    int rv;

    hostInfo = gethostbyname(hostName);
    if (hostInfo == NULL) 
    {
        return BABELSOCK_EGETHOST;
    }

    memset(&hemAddr, 0, sizeof(hemAddr));
    hemAddr.sin_family = AF_INET;
    hemAddr.sin_addr = *(struct in_addr *)hostInfo->h_addr;
    hemAddr.sin_port = htons(port);

    rv = bind(fd, (struct sockaddr *)&hemAddr, sizeof(hemAddr));
  
    if (rv < 0) 
    {
        return BABELSOCK_EBIND;
    }
    return BABELSOCK_OK;
}

/* =================================================================== 
**
** Function: babelSockGetReceiveBufferSize 
**  
** This function is futher described in a header file.
**   
** Implementation details:
**
** =============================================== 
*/
int 
babelSockGetReceiveBufferSize(int fd, int *size)
{
    int rv;
    int len = sizeof(size);

    rv = getsockopt(fd, 
                    SOL_SOCKET, 
                    SO_RCVBUF, 
                    (char *)size, 
                    &len);
    if (rv < 0) 
    {
        return BABELSOCK_ESOCKOPT;
    }
    return BABELSOCK_OK;
}

/* =================================================================== 
**
** Function: babelSockSetReceiveBufferSize 
**  
** This function is futher described in a header file.
**   
** Implementation details:
**
** =============================================== 
*/
int 
babelSockSetReceiveBufferSize(int fd, int size)
{
    int rv;

    rv = setsockopt(fd, 
                    SOL_SOCKET, 
                    SO_RCVBUF, 
                    (const char *)&size, 
                    sizeof(size));
    if (rv < 0) 
    {
        return BABELSOCK_ESOCKOPT;
    }
    return BABELSOCK_OK;
}

/* =================================================================== 
**
** Function: babelSockSetNonBlocking 
**  
** This function is futher described in a header file.
**   
** Implementation details:
**
** =============================================== 
*/
int 
babelSockSetNonBlocking(int fd) 
{
    int rv;
    unsigned long non_block;

    non_block = 1;
    rv = ioctlsocket(fd, FIONBIO, &non_block);
    if (rv < 0) 
    {
        return BABELSOCK_ESET_NON_BLOCKING;
    }
    return BABELSOCK_OK;
}


/* =================================================================== 
**
** Function: babelSockListen 
**  
** This function is futher described in a header file.
**   
** Implementation details:
**
** =============================================== 
*/
int 
babelSockListen(int fd, int QLEN) 
{
    int rv;

    rv = listen(fd, QLEN);
    
    if (rv < 0) 
    {
        return BABELSOCK_ELISTEN;
    }
    return BABELSOCK_OK;
}


/* =================================================================== 
**
** Function: babelSockAccept 
**  
** This function is futher described in a header file.
**   
** Implementation details:
**
** =============================================== 
*/
int 
babelSockAccept(int fd) 
{
    struct sockaddr_in sa;
    int addrlen;
    int rv;
    
    addrlen = sizeof(sa);
    rv = accept(fd, (struct sockaddr *)&sa, &addrlen);
    
    if (rv < 0) 
    {
        return BABELSOCK_EACCEPT;
    }
    return rv;
}


/* =================================================================== 
**
** Function: babelSockReadAll 
**  
** This function is futher described in a header file.
**   
** Implementation details:
**
** =============================================== 
*/
int 
babelSockReadAll(int fd, char *buff, int n) 
{
    int rc;
    int cnt;
    int err;

    cnt = n;
    while (cnt > 0) 
    {   
        rc = recv(fd, buff, cnt, 0);
        if (rc == SOCKET_ERROR) 
        {
            err = WSAGetLastError();
            if (err == WSAEINTR) 
            {
                continue;
            }
            return BABELSOCK_EREAD;
        }
        else if (rc == 0) 
        {
            return n - cnt;
        }
        buff += rc;
        cnt -= rc;
    }
    return n;
}


/* =================================================================== 
**
** Function: babelSockReadFrom 
**  
** This function is futher described in a header file.
**   
** Implementation details:
**
** =============================================== 
*/
int 
babelSockReadFrom(int fd, 
                  char *buff, 
                  int n, 
                  unsigned long *from, 
                  unsigned short *port) 
{
    int rv;
    struct sockaddr_in fsin;
    int alen;

    alen = sizeof(struct sockaddr_in);
    rv = recvfrom(fd, buff, n, 0, (struct sockaddr *)&fsin, &alen);
    if (rv < 0) 
    {
        return BABELSOCK_EREADFROM;
    }
    if (rv == 0) 
    {
        return BABELSOCK_ECONCLOSED;
    }
    *port = ntohs(fsin.sin_port);
    *from = ntohl(fsin.sin_addr.s_addr);
    
    return rv;
}


/* =================================================================== 
**
** Function: babelSockWriteAll 
**  
** This function is futher described in a header file.
**   
** Implementation details:
**
** =============================================== 
*/
int 
babelSockWriteAll(int fd, char const *buff, int n) 
{
    int rc;

    rc = send(fd, buff, n, 0);
    if (rc != n) 
    {
        return BABELSOCK_EWRITE;
    }
    return rc;
}


/* =================================================================== 
**
** Function: babelSockWriteTo 
**  
** This function is futher described in a header file.
**   
** Implementation details:
**
** =============================================== 
*/
int 
babelSockWriteTo(int fd, 
                 const char *buff, 
                 int n, 
                 unsigned long to, 
                 unsigned short port)
{
    int alen;
    int rv;
    struct sockaddr_in hemAddr;
    
    memset(&hemAddr, 0, sizeof(hemAddr));
    hemAddr.sin_family = AF_INET;
    hemAddr.sin_port = htons(port);
    hemAddr.sin_addr.s_addr = htonl(to);

    alen = sizeof(hemAddr);
    rv = sendto(fd, buff, n, 0, (struct sockaddr *)&hemAddr, alen);
    if (rv < 0) 
    {
        return BABELSOCK_EWRITETO;
    }
    return rv;
}


/* =================================================================== 
**
** Function: babelSockSelect 
**  
** This function is futher described in a header file.
**   
** Implementation details:
**
** =============================================== 
*/
int 
babelSockSelect(int *babelSockReadList, 
                int lengthReadList,
                int timeOut)
{
    fd_set rfds;
    int i;
    int ii;
    int rv;
    int maxVal;
    struct timeval tv;
    struct timeval *tv_p;

    /*  Prepare select arguments.
     */
    i = 0;
    FD_ZERO(&rfds);                   
    while(i < lengthReadList) 
    {
        FD_SET(babelSockReadList[i], &rfds);
        i = i + 1;
    }
    i = 0;
    
    /*  If the timeOut is less than zero set tv_p to NULL indicating that
     *  the select statement should wait forever if no event occurs.
     */
    if (timeOut < 0) 
    { 
        tv_p = NULL;
    } 
    else 
    {
        tv.tv_sec = 0;
        tv.tv_usec = timeOut;
        tv_p = &tv;
    }
    maxVal = maxV(&babelSockReadList[0], lengthReadList);  
    
    /*  The select command.
     */
    rv = select(maxVal + 1, &rfds, NULL, NULL, tv_p);
    if (rv < 0) 
    {
        return BABELSOCK_ESELECT;
    }
    if (rv == 0) /* Time out */ 
    {   
        return 0;
    }
    
    /*  Fix the read list return values.
     */
    i = 0;                      
    ii = 0;
    while (i < lengthReadList) 
    {
        if (FD_ISSET(babelSockReadList[i], &rfds)) 
        {
            babelSockReadList[ii] = babelSockReadList[i];
            ii = ii + 1;
        }
        i = i + 1;
    } 
    return rv;
}


/* =================================================================== 
**
** Function:  maxV
**  
** Abstract:
**   
**
** Parameters:
**     babelSockList - a list of socket file descriptors
**     length       - the length of the list
**   
** Return value:  
**   the largest socket value in the list
**   
** Restrictions:
**   
** Error checks:
**   
** See also:
**   
** Example:
**
** =============================================== 
*/
static int 
maxV(int *babelSockList, int length) 
{
    int i;
    int maxVal;

    i = 0;
    maxVal = 0;
    while(i < length) 
    {
        if (babelSockList[i] > maxVal) 
        {
            maxVal = babelSockList[i];
	}
        i = i + 1;
    }
    return maxVal;
}


/* =================================================================== 
**
** Function: babelSockGetHostAddrByName 
**  
** This function is futher described in a header file.
**   
** Implementation details:
**
** =============================================== 
*/
int 
babelSockGetHostAddrByName(char const *name, unsigned long *addr) 
{
    struct sockaddr_in hemAddr;
    struct hostent *hostInfo;
    
    memset(&hemAddr, 0, sizeof(hemAddr));
    /* FIXME */
    hemAddr.sin_family = AF_INET;
    
    hostInfo = gethostbyname(name);
    if (hostInfo != NULL) 
    {
        hemAddr.sin_addr = *(struct in_addr *)hostInfo->h_addr;
    } 
    else 
    {
        hemAddr.sin_addr.s_addr = inet_addr(name);
        if (hemAddr.sin_addr.s_addr == 0) 
        {
            return BABELSOCK_EADDRESS;
	}
    }

    /* The address is converted back to host byte order since
     * that is what babelSockWriteTo() expects.
     */
    *addr = ntohl(hemAddr.sin_addr.s_addr);

    return BABELSOCK_OK;
}


/* =================================================================== 
**
** Function: babelSockGetHostNameByAddr 
**  
** This function is futher described in a header file.
**   
** Implementation details:
**
** =============================================== 
*/
int 
babelSockGetHostNameByAddr(unsigned long addr, char *name) 
{
    struct sockaddr_in hemAddr;
    
    memset(&hemAddr, 0, sizeof(hemAddr));
    /* FIXME */
    hemAddr.sin_family = AF_INET;
    hemAddr.sin_addr.s_addr = htonl(addr);
    strcpy(name, inet_ntoa(hemAddr.sin_addr));
    return BABELSOCK_OK;
}


/* =================================================================== 
**
** Function: babelSockGetHostName 
**  
** This function is futher described in a header file.
**   
** Implementation details:
**
** =============================================== 
*/
int 
babelSockGetHostName(char *name, int nameLength) 
{
    int rv;
    
    rv = gethostname(name, nameLength);
    if (rv < 0) 
    {
        return BABELSOCK_EGETHOSTNAME;
    }
    return BABELSOCK_OK;
}


/* =================================================================== 
**
** Function: babelSockClose 
**  
** This function is futher described in a header file.
**   
** Implementation details:
**
** =============================================== 
*/
int 
babelSockClose(int fd) 
{
    int rv;


    shutdown(fd, SD_BOTH);    
    rv = closesocket(fd);
    if (rv < 0) 
    {
        return BABELSOCK_ECLOSE;
    }
    return BABELSOCK_OK;
}


/* =================================================================== 
**
** Function: babelSocketCleanup
**  
** This function is futher described in a header file.
**   
** Implementation details:
**
** =============================================== 
*/
int 
babelSockCleanup(void) 
{
    (void) WSACleanup();
    return BABELSOCK_OK;
}
