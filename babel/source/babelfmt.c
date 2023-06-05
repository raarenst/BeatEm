/* ===================================================================
** Copyright (C) 2002 Ra-Tek. All rights reserved.
**
** File:  babelfmt.c
**
** Abstract:
**   
**
** Required Defines:
**   BABELFMT_SUPPORT_ARRAYS [0 | 1]
**
** ===============================================
*/
#include <stdarg.h>
#include <assert.h>
#include <stdlib.h>
#include "babelfmt.h"

#ifndef BABELFMT_SUPPORT_ARRAYS
#define BABELFMT_SUPPORT_ARRAYS 1
/* error "Please define BABELFMT_SUPPORT_ARRAYS to 0 or 1" */
#endif

/* =================================================================== 
**
** Help functions
**
** =============================================== 
*/
static int 
putChar(char *dst, unsigned char c) 
{
    dst[0] = (c >>  0) & 0xff;
    return 1;
}

static int 
getChar(char const *src, unsigned char* c) 
{
    *c = src[0];
    return 1;
}

static int 
putShort(char *dst, unsigned short s) 
{
    dst[0] = (s >> 0) & 0xff;
    dst[1] = (s >> 8) & 0xff;
    return 2;
}

static int 
getShort(char const *src, unsigned short* s) 
{
    *s = ((unsigned short)src[0] <<  0) +
         ((unsigned short)src[1] <<  8);
    return 2;
}

static int 
putLong(char *dst, unsigned long l) 
{
    dst[0] = (unsigned char)(l >>  0) & 0xff;
    dst[1] = (unsigned char)(l >>  8) & 0xff;
    dst[2] = (unsigned char)(l >> 16) & 0xff;
    dst[3] = (unsigned char)(l >> 24) & 0xff;
    return 4;
}

static int 
getLong(char const *src, unsigned long* l) 
{
    *l = ((unsigned long)src[0] <<  0) +
         ((unsigned long)src[1] <<  8) +
         ((unsigned long)src[2] << 16) +
         ((unsigned long)src[3] << 24);
    return 4;
}


static int 
putString(char *dst, unsigned char* str) 
{
    char *oldDst = dst;

    while((*dst++ = *str++)) 
    {
        /* Do nothing
         */
    }
    return (int)(dst - oldDst);
}


static int 
getString(char const *src, unsigned char* str) 
{
    char const *oldSrc = src;

    while((*str++ = *src++)) 
    {
        /* Do nothing
         */
    }
    return (int)(src - oldSrc);
}

/* =================================================================== 
**
** Function:  babelFmtEncode
**  
** This function is futher described in a header file.
**   
** Implementation details:
**
** =============================================== 
*/
int 
babelFmtEncode(char *data, char const *fmt, ...)
{
    va_list ap;
    int rv;

    va_start(ap, fmt);
    rv = babelFmtEncodeVaList(data, fmt, ap);
    va_end(ap);

    return rv;
}

/* =================================================================== 
**
** Function:  babelFmtEncodevaList
**  
** This function is futher described in a header file.
**   
** Implementation details:
**
** =============================================== 
*/
int 
babelFmtEncodeVaList(char *data, 
                     char const *fmt, 
                     va_list ap)
{
    char *oldData = data;

    for(;;) 
    {
        switch(*fmt++) 
        {
        case 'c':
            data += putChar(data, (unsigned char)va_arg(ap, unsigned long));
            break;

        case 's':
            data += putShort(data, (unsigned short)va_arg(ap, unsigned long));
            break;

        case 'l':
            data += putLong(data, va_arg(ap, unsigned long));
            break;

        case 'z':
            data += putString(data, va_arg(ap, unsigned char*));
            break;
#if BABELFMT_SUPPORT_ARRAYS==1
        case 'C':
            {
                unsigned char* ptr = va_arg(ap, unsigned char*);
                int len;
                int i;

                if(*fmt == '*') 
                {
                    fmt++;
                    len = va_arg(ap, int);
                    data += putShort(data, (unsigned short)len);
                }
                else {
                    len = strtol((char*)fmt, (char**)&fmt, 10);
                }

                for(i = 0; i < len; i++) 
                {
                    data += putChar(data, *ptr++);
                }
            }
            break;

        case 'S':
            {
                unsigned short* ptr = va_arg(ap, unsigned short*);
                int len;
                int i;

                if(*fmt == '*') 
                {
                    fmt++;
                    len = va_arg(ap, int);
                    data += putShort(data, (unsigned short)len);
                }
                else 
                {
                    len = strtol((char*)fmt, (char**)&fmt, 10);
                }

                for(i = 0; i < len; i++) 
                {
                    data += putShort(data, *ptr++);
                }
            }
            break;

        case 'L':
            {
                unsigned long* ptr = va_arg(ap, unsigned long*);
                int len;
                int i;

                if(*fmt == '*') 
                {
                    fmt++;
                    len = va_arg(ap, unsigned int);
                    data += putShort(data, (unsigned short)len);
                }
                else 
                {
                    len = strtol((char*)fmt, (char**)&fmt, 10);
                }

                for(i = 0; i < len; i++) 
                {
                    data += putLong(data, *ptr++);
                }
            }
            break;
#endif
        case '\0':
            return (int)(data - oldData);

        default:
            assert(1);
        }
    }
}

/* =================================================================== 
**
** Function:  babelFmtDecode
**  
** This function is futher described in a header file.
**   
** Implementation details:
**
** =============================================== 
*/
int 
babelFmtDecode(char const *data, char const* fmt, ...)
{
    va_list ap;
    int rv;

    va_start(ap, fmt);
    rv = babelFmtDecodeVaList(data, fmt, ap);
    va_end(ap);

    return rv;
}

/* =================================================================== 
**
** Function:  babelFmtDecodeVaList
**  
** This function is futher described in a header file.
**   
** Implementation details:
**
** =============================================== 
*/
int 
babelFmtDecodeVaList(char const *data, 
                     char const* fmt, 
                     va_list ap)
{
    char const *oldData = data;

    for(;;) 
    {
        switch(*fmt++) 
        { 
        case 'c':
            data += getChar(data, va_arg(ap, unsigned char*));
            break;

        case 's':
            data += getShort(data, va_arg(ap, unsigned short*));
            break;

        case 'l':
            data += getLong(data, va_arg(ap, unsigned long*));
            break;

        case 'z':
            data += getString(data, va_arg(ap, unsigned char*));
            break;
#if BABELFMT_SUPPORT_ARRAYS==1
       case 'C':
            {
                unsigned char* ptr = va_arg(ap, unsigned char*);
                unsigned short len;
                int i;

                if(*fmt == '*') 
                {
                    unsigned short* lenRet = 
			(unsigned short*)va_arg(ap, unsigned int*);
                    fmt++;
                    data += getShort(data, &len);
                    if(lenRet != NULL) 
                    {
                        *lenRet = len;
                    }
                }
                else 
                {
                    len = (unsigned short)strtol((char*)fmt, 
                                                 (char**)&fmt, 10);
                }

                for(i = 0; i < len; i++) 
                {
                    data += getChar(data, ptr++);
                }
            }
            break;

        case 'S':
            {
                unsigned short* ptr = va_arg(ap, unsigned short*);
                unsigned short len;
                int i;

                if(*fmt == '*') 
                {
		    unsigned short* lenRet = 
			(unsigned short*)va_arg(ap, int*);
                    fmt++;
                    data += getShort(data, &len);
                    if(lenRet != NULL) {
                        *lenRet = len;
                    }
                }
                else 
                {
                    len = (unsigned short)strtol((char*)fmt, 
                                                 (char**)&fmt, 10);
                }

                for(i = 0; i < len; i++) 
                {
                    data += getShort(data, ptr++);
                }
            }
            break;

         case 'L':
            {
                unsigned long* ptr = va_arg(ap, unsigned long*);
                unsigned short len;
                int i;

                if (*fmt == '*') 
                {
                    unsigned short* lenRet =
			(unsigned short*)va_arg(ap, unsigned int*);
                    fmt++;
                    data += getShort(data, &len);
                    if(lenRet != NULL) 
                    {
                        *lenRet = len;
                    }
                }
                else 
                {
                    len = (unsigned short)strtol((char*)fmt, 
                                                 (char**)&fmt, 10);
                }

                for(i = 0; i < len; i++) 
                {
                    data += getLong(data, ptr++);
                }
            }
            break;
#endif
        case '\0':
            return (int)(data - oldData);

        default:
            assert(1);
        }
    }
}
