/* ===================================================================
** Copyright (C) 2003 Ra-Tek. All rights reserved.
**
** File: babelfmt.h
**
** Abstract:
**   This module makes it possible to convert data into a universal
**   representation that can be sent over a network, handling endian
**   problems.
**   
** Required Defines:
**     BABELFMT_SUPPORT_ARRAYS [0 | 1]
**   
**
** ===============================================
*/
#ifndef _BABELFMT_H
#define _BABELFMT_H

#include <stdarg.h>

#define BABELFMT_SIZEOF_L            (sizeof(unsigned long))
#define BABELFMT_SIZEOF_S            (sizeof(unsigned short))
#define BABELFMT_SIZEOF_C            (sizeof(char))
#define BABELFMT_SIZEOF_LEN_ARG      (sizeof(unsigned short))

/* =================================================================== 
**
** Function:  babelFmtEncode
** 
** Abstract:
**   
**
** Parameters:
**   data - The buffer to put the data into.
**   fmt  - A strings specifying the following arguments:
**            c    - 8 bit character.
**            s    - 16 bit integer
**            l    - 32 bit integer.
**            z    - Zero terminated string of 8 bit integers.
**            C<n> - An array of <n> 8 bit integers.
**            S<n> - An array of <n> 16 bit integers.
**            L<n> - An array of <n> 32 bit integers.
**            C*   - A variable length array of a numer of 8 bit
**                   integers specified with a following argument.
**            S*   - A variable length array of a numer of 16 bit
**                   integers specified with a following argument.
**            L*   - A variable length array of a numer of 32 bit
**                   integers specified with a following argument.
**   ...          - Arguments encoded.
**   
** Return value:  
**   The number of bytes used to encode 'fmt' in 'data'.
**  
** Restrictions:
**   
** Error checks:
**   
** See also:
**   
** Example:
**    #include <assert.h>
**    #include <unistd.h>
**    #include "babelfmt.h"
**      
**    #define DATA_MSG_FORMAT "sczlL10s"
**      
**    void sendDataMsg(int sock, short a1, char a2, char* str,
**                     long l1, long const * lArray, short s1)
**    {
**        char data[128];
**        int len;
**        int rv;
**      
**        len = babelFmtEncode(data, MSG_FORMAT,
**                             a1, a2, str, l1, lArray, s1);
**        rv = write(sock, data, len);
**        assert(len == rv);
**    }
** =============================================== 
*/
extern int 
babelFmtEncode(char *data, char const *fmt, ...);

/* =================================================================== 
**
** Function:  babelFmtEncodeVaList
** 
** Abstract:
**   
**
** Parameters:
**   data - The buffer to put the data into.
**   fmt  - A strings specifying arguments, see babelFmtEncode for 
**          more detailed information.
**   ap   - A va_list argument list. See babelFmtEncode for further 
**          information.
**
** Return value:  
**   The number of bytes used to encode 'fmt' in 'data'.
**
** Restrictions:
**      
** Error checks:
**   
** See also:
**   
** Example:
**   int encodeAndWriteAll(int fd,
**                         unsigned char *buff, 
**                         const char *fmt, 
**                         ...) 
**   {
**       va_list ap;
**       int rv;
**       int size;
** 
**       va_start(ap, fmt);
**       size = babelFmtEncodeVaList(buff, fmt, ap);
**       rv = send(fd, buff, size);
**       va_end(ap);
**       if (rv != size)
**           return -1;
**       return rv;
**   }
** =============================================== 
*/
extern int 
babelFmtEncodeVaList(char *data, 
                     char const *fmt, 
                     va_list ap);

/* =================================================================== 
**
** Function:  babelFmtDecode
**  
** Abstract:
**   
**
** Parameters:
**   data - The buffer to put the data into.
**   fmt  - A strings specifying the following arguments:
**            c    - 8 bit character.
**            s    - 16 bit integer
**            l    - 32 bit integer.
**            z    - Zero terminated string of 8 bit integers.
**            C<n> - An array of <n> 8 bit integers.
**            S<n> - An array of <n> 16 bit integers.
**            L<n> - An array of <n> 32 bit integers.
**            C*   - A variable length array of a numer of 8 bit
**                   integers specified with a following argument.
**            S*   - A variable length array of a numer of 16 bit
**                   integers specified with a following argument.
**            L*   - A variable length array of a numer of 32 bit
**                   integers specified with a following argument.
**   ...          - Arguments encoded.
**
** Return value:  
**     The number of bytes decoded from 'data'
**
** Restrictions:
**   
**   
** Error checks:
**   
**
** See also:
**   
**  
** Example:
**   #include <assert.h>
**   #include <unistd.h>
**   #include "babelfmt.h"
**      
**   #define DATA_MSG_FORMAT "sczlL10s"
**    void recvDataMsg(int sock, short* pa1, char* pa2, char* str,
**                     long* pl1, long* lArray, short* ps1)
**    {
**        char data[128];
**        int len;
**        int rv;
**      
**        len = read(sock, data, 128);
**        rv = babelFmtDecode(data, MSG_FORMAT,
**                            pa1, pa2, str, pl1, lArray, ps1);
**        assert(len == rv);
**    }
** =============================================== 
*/
extern int 
babelFmtDecode(char const *data, char const *fmt, ...);

/* =================================================================== 
**
** Function:  babelFmtDecodeVaList
** 
** Abstract:
**   
**
** Parameters:
**     data - The buffer to put the data into.
**     fmt  - A strings specifying arguments, see babelFmtEncode for more
**            detailed information.
**     ap   - A va_list argument list. See babelFmtDecode for further
**            information.
**
** Return value:  
**     The number of bytes decoded from 'data'
**
** Restrictions:  
**   
** Error checks:
**   
** See also:
**   
** Example:
**   readAllAndDecode(int fd,
**                    int size,
**                    const char *fmt,
**                    ...) 
**   {
**       va_list ap;
**       unsigned char *buff;
**       int rv;
**
**       buff = malloc(size);
**       va_start(ap, fmt);
**
**       rv = receive(fd, buff, size);
**       assert(rv == size);
**
**       rv = babelFmtDecodeVaList(buff, fmt, ap);
**       assert(rv == size);
**
**       va_end(ap);
**       free(buff);
**       return rv;
**   }
** =============================================== 
*/
extern int 
babelFmtDecodeVaList(char const *data, 
                     char const *fmt, 
                     va_list ap);

#endif /* _BABELFMT_H */
