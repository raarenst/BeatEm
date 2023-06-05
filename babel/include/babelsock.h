/* ===================================================================
** Copyright (C) 2003 Ra-Tek. All rights reserved.
**
** File: babelsock.h
**
** Abstract:
**   
**
** Required Defines:
**   
**
** ===============================================
*/
#ifndef BABELSOCK_H
#define BABELSOCK_H

/* ========================================================================
** 
** Function summary
**
** int babelSockInit(void);
** int babelSock(int proto);
** int babelSockConnect(int fd, char const *hostName, unsigned short port);
** int babelSockBind(int fd, unsigned short port);
** int babelSockListen(int fd, int qlen);
** int babelSockAccept(int fd);
** int babelSockReadAll(int fd, char *buff, int n);
** int babelSockReadFrom(int fd, char *buff, int n, unsigned long *from, 
**                       unsigned short *port);
** int babelSockWriteAll(int fd, char const *buff, int n);
** int babelSockWriteTo(int fd, const char *buff, int n, unsigned long to, 
**                      unsigned short port);
** int babelSockSelect(int *babelSockReadList, int lengthReadList, 
**                     int timeOut);
** int babelSockGetHostAddrByName(char const *name, unsigned long *addr);
** int babelSockGetHostNameByAddr(unsigned long addr, char *name);
** int babelSockGetHostName(char *name, int nameLength);
** int babelSockClose(int fd);
** int babelSockCleanup(void);
**
** ===============================================
*/

/* 
** Socket types.
**
 */
#define BABELSOCK_TCP			 2			
#define BABELSOCK_UDP			 3

/* 
** Error codes.
**
 */
#define BABELSOCK_OK			 0
#define BABELSOCK_EINIT			-1
#define BABELSOCK_EPROTO		-2
#define BABELSOCK_ESOCKET		-3	
#define BABELSOCK_ESOCKOPT		-4
#define BABELSOCK_EGETHOST		-5
#define BABELSOCK_ECONNECT              -6
#define BABELSOCK_EBIND			-7
#define BABELSOCK_EACCEPT		-8
#define BABELSOCK_EREAD			-9
#define BABELSOCK_EWRITE		-10
#define BABELSOCK_ECLOSE		-11
#define BABELSOCK_ECLEANUP		-12
#define BABELSOCK_EREADFROM		-13
#define BABELSOCK_ENTOA			-14
#define BABELSOCK_EGETHOSTNAME	        -15
#define BABELSOCK_ECONCLOSED		-16
#define BABELSOCK_ESELECT		-17
#define BABELSOCK_ELISTEN               -18
#define BABELSOCK_EWRITETO              -19
#define BABELSOCK_EADDRESS		-20
#define BABELSOCK_ESET_NON_BLOCKING     -21

/* =================================================================== 
**
** Function: babelSockInit
**  
** Abstract:
**     Initialize babelsock. Call once before further use of 
**     babelsock functions.
**
** Parameters:
**   None
**   
** Return value:  
**     - BABELSOCK_OK if successful
**     - BABELSOCK_EINIT if on error occurred.
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
extern int babelSockInit(void);


/* =================================================================== 
**
** Function: babelSock
**  
** Abstract:
**     Create a socket.
**
** Parameters:
**     proto    - the socket protocol, BABELSOCK_TCP or BABELSOCK_UDP.
**   
** Return value:  
**     - A socket file descriptor if successful.
**     - BABELSOCK_EPROTO if an unknown protocol was specified.
**     - BABELSOCK_ESOCKOPT if the setting of the sock optiones failed.
**     - BABELSOCK_ESOCKET if socket creation failed.
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
extern int babelSock(int proto);


/* =================================================================== 
**
** Function: babelSockConnect
**  
** Abstract:
**     This call specifies a remote endpoint for a previously 
**     created socket. 
**
** Parameters:
**     fd          - A socket file descriptor.
**     hostName    - The name or IP number of the host.
**     port        - The port number used by the host.
**   
** Return value:  
**     - BABELSOCK_OK if successful
**     - BABELSOCK_EGETHOST if gethostbyname failed
**     - BABELSOCK_ECONNECT if connect failed
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
extern int babelSockConnect(int fd, 
                            char const *hostName, 
                            unsigned short port);


/* =================================================================== 
**
** Function: babelSockBind 
**  
** Abstract:
**     This call specifies a local port and IP address for a 
**     previously created socket. Use 0.0.0.0 as IP address
**     for any IP address. 
**
** Parameters:
**     fd      - A socket file descriptor.
**     port    - The port number used by the socket.
**   
** Return value:  
**     - BABELSOCK_OK if successful
**     - BABELSOCK_EGETHOST if gethostbyname failed
**     - BABELSOCK_EBIND if bind failed
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
extern int babelSockBind(int fd, char const *hostName, unsigned short port);

/* =================================================================== 
**
** Function: babelSockGetReceiveBufferSize 
**  
** Abstract:
**
** Parameters:
**     fd      - A socket file descriptor.
**     size    - a buffer to store the size in
**
** Return value:  
**     - BABELSOCK_OK if successful
**     - BABELSOCK_ESOCKOPT if failed
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
extern int babelSockGetReceiveBufferSize(int fd, int *size);

/* =================================================================== 
**
** Function: babelSockSetReceiveBufferSize 
**  
** Abstract:
**
** Parameters:
**     fd      - A socket file descriptor.
**     size    - size of receive buffer
**
** Return value:  
**     - BABELSOCK_OK if successful
**     - BABELSOCK_ESOCKOPT if failed
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
extern int babelSockSetReceiveBufferSize(int fd, int size);

/* =================================================================== 
**
** Function: babelSockSetNonBlocking 
**  
** Abstract:
**
** Parameters:
**     fd      - A socket file descriptor.
**   
** Return value:  
**     - BABELSOCK_OK if successful
**     - BABELSOCK_ESET_NON_BLOCKING if failed
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
extern int babelSockSetNonBlocking(int fd);


/* =================================================================== 
**
** Function: babelSockListen
**  
** Abstract:
**     This function makes the socket passive, i.e., ready 
**     to accept incoming connection requests. 
**
** Parameters:
**     fd      - a socket file descriptor for a server
**     qlen    - The size of the incoming connection request queue,
**               (up to a maximum of 5)
**   
** Return value:  
**     - BABELSOCK_OK if successful.
**     - BABELSOCK_ELISTEN if listen failed.
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
extern int babelSockListen(int fd, int qlen);


/* =================================================================== 
**
** Function: babelSockAccept
**  
** Abstract:
**     Servers use accept to accept the next incoming connection
**     on a passive socket.
**
** Parameters:
**     fd    - A socket file descriptor (for a server).
**   
** Return value:  
**     - A new socket file descriptor for the new connection 
**       if successful.
**     - BABELSOCK_EACCEPT if accept failed.
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
extern int babelSockAccept(int fd);


/* =================================================================== 
**
** Function: babelSockReadAll
**  
** Abstract:
**     Read a number of bytes from a socket. Loops until
**     all bytes are read or an error occurs.     
**
** Parameters:
**     fd      - A socket file descriptor
**     buff    - A buffer to put the read data into.
**     n       - The number of bytes to read.
**   
** Return value:  
**     - The number of read bytes if successful. If this value is not
**       equal to 'n' and no error occured an EOF was reached.
**     - BABELSOCK_EREAD if read failed.
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
extern int babelSockReadAll(int fd, char *buff, int n);


/* =================================================================== 
**
** Function: babelSockReadFrom
**  
** Abstract:
**     Read a number of bytes from a socket and store the
**     senders IP address and port in arguments.
**
** Parameters:
**     fd         - A socket file descriptor
**     buff       - A buffer to put the read data into.
**     n          - The number of bytes to read.
**     from       - IP address of the sender.
**     port       - A pointer to an integer to put the
**                  port number of the sender into.
**   
** Return value:  
**     - The number of read bytes is returned if successful.
**     - BABELSOCK_EREADFROM if read failed.
**     - BABELSOCK_ECONCLOSED if the connection was closed.
**     - BABELSOCK_ENTOA if the address conversion failed
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
extern int babelSockReadFrom(int fd, 
                             char *buff, 
                             int n, 
                             unsigned long *from, 
                             unsigned short *port);


/* =================================================================== 
**
** Function: babelSockWriteAll
**  
** Abstract:
**     Write a number of bytes to a socket.
**
** Parameters:
**     fd      - A socket file descreiptor.
**     buff    - A buffer containing the data to write.
**     n       - The number of bytes to send.
**   
** Return value:  
**     - The number of bytes written, should be 'n'.
**     - BABELSOCK_EWRITE if write failed.
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
extern int babelSockWriteAll(int fd, char const *buff, int n);


/* =================================================================== 
**
** Function: babelSockWriteTo
**  
** Abstract:
**     Write a number of bytes to a socket with the given 
**     destination.
**
** Parameters:
**     fd      - A socket file descreiptor.
**     buff    - A buffer containing the data to write.
**     n       - The number of bytes to send.
**     to      - The IP address of the destination socket.
**     port    - The port number of the destination socket.
**   
** Return value:  
**     - The number of bytes written, should be 'n'.
**     - BABELSOCK_EGETHOST is the host specified does not excist.
**     - BABELSOCK_EWRITETO if the write failed.
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
extern int babelSockWriteTo(int fd, 
                            const char *buff, 
                            int n, 
			    unsigned long to, 
                            unsigned short port);


/* =================================================================== 
**
** Function: babelSockSelect 
**  
** Abstract:
**     Select provides asynchronous I/O by permitting a single 
**     process to wait for the first of any socket descriptors
**     in a specified set to become ready. The caller can also
**     specify a maximum timeout for the wait.
**
** Parameters:
**     babelSockReadList    - Sockets in this list will be watched to
**                           see if a read from this socket will not 
**                           block.
**     lengthReadList      - The length of the read list.
**     timeOut             - The wait time for the select in seconds.
**                           If the time is negative select waits
**                           forever.
**   
** Return value:  
**     - The number of sockets in the list which have data 
**       ready to be read. 
**     - 0 if the time out was reached
**     - BABELSOCK_ESELECT if an error occurred.
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
extern int babelSockSelect(int *babelSockReadList, 
                           int lengthReadList,
                           int timeOut);


/* =================================================================== 
**
** Function: babelSockGetHostAddrByName
**  
** Abstract:
**     Convert a string containing a host name or an IP address in
**     numbers-and-dots notation to an IP address in binary format
**     that can be used in a call to babelSockWriteTo().
**
** Parameters:
**     name       - String containing IP address or hostname.
**     addr       - IP address in binary format.
**   
** Return value:  
**     - BABELSOCK_OK if successful.
**     - BABELSOCK_EADDRESS if conversion failed.
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
extern int babelSockGetHostAddrByName(char const *name, unsigned long *addr);


/* =================================================================== 
**
** Function: babelSockGetHostNameByAddr
**  
** Abstract:
**     Convert an IP address in binary format to a string containing
**     the IP address in numbers-and-dots notation.
**
** Parameters:
**     addr       - IP address in binary format.
**     name       - String containing IP address in numbers-and-dots
**                  format.
**   
** Return value:  
**     BABELSOCK_OK.
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
extern int babelSockGetHostNameByAddr(unsigned long addr, char *name);


/* =================================================================== 
**
** Function: babelSockGetHostName
**  
** Abstract:
**     Get the hostname.
**
** Parameters:
**     name       - Character buffer to hold name.
**     nameLength - Length of character buffer.
**   
** Return value:  
**     - BABELSOCK_OK if successful.
**     - BABELSOCK_EGETHOSTNAME if operation failed.
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
extern int babelSockGetHostName(char *name, int nameLength);


/* =================================================================== 
**
** Function: babelSockClose
**  
** Abstract:
**     Close a babelSocket.
**
** Parameters:
**     fd    - A socket file descriptor.
**   
** Return value:  
**     - BABELSOCK_OK if the socket was closed successfully.
**     - BABELSOCK_ECLOSE if close failed.
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
extern int babelSockClose(int fd);


/* =================================================================== 
**
** Function: babelSockCleanUp
**  
** Abstract:
**
** Parameters:
**     None
**   
** Return value:  
**     - BABELSOCK_OK if successfull.
**     - BABELSOCK_ECLEANUP if cleanup failed.
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
extern int babelSockCleanup(void);

#endif /* BABELSOCK_H */
