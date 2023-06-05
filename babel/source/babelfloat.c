/* ===================================================================
** Copyright (C) 2002 Ra-Tek. All rights reserved.
**
** File:  babelfloat.c
**
** Abstract:
**   
**
** Required Defines:
**   
**
** ===============================================
*/
#include "babelfloat.h"
#include "babelfmt.h"

/* =================================================================== 
**
** Function:  getlong
**  
** Abstract:
**   
**
** Parameters:
**   
** Return value:  
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
static void
getLong(unsigned char const *src, unsigned long* l) 
{
    *l = ((unsigned long)src[0] <<  0) +
         ((unsigned long)src[1] <<  8) +
         ((unsigned long)src[2] << 16) +
         ((unsigned long)src[3] << 24);
}

/* =================================================================== 
**
** Function:  putLong
**  
** Abstract:
**   
**
** Parameters:
**   
** Return value:  
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
static void 
putLong(unsigned char *dst, unsigned long l) 
{
    dst[0] = (unsigned char)(l >>  0) & 0xff;
    dst[1] = (unsigned char)(l >>  8) & 0xff;
    dst[2] = (unsigned char)(l >> 16) & 0xff;
    dst[3] = (unsigned char)(l >> 24) & 0xff;
}

/* =================================================================== 
**
** Function:  swapDouble
**  
** Abstract:
**   
**
** Parameters:
**   
** Return value:  
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
static void
swapDouble(double *d)
{
    unsigned long *l;
    unsigned long tmp;

    l = (unsigned long *)d;
    tmp = l[0];
    l[0] = l[1];
    l[1] = tmp;
}

/* =================================================================== 
**
** Function:  babelFloatEncode
**  
** This function is futher described in a header file.
**   
** Implementation details:
**
** =============================================== 
*/
int 
babelFloatEncode(unsigned char const *sourceData, 
                 BABEL_FLOAT_CONVERTION_TYPE type,
                 unsigned char *destinationData,
                 int nrVariables)
{
    unsigned char *data;
    int i;

    switch (type)
    {
	case BABEL_FLOAT_IEEE754_DOUBLE:
            {
                unsigned long l = 0x12345678;

                data = (unsigned char *)sourceData;
                for (i = 0; i < nrVariables; i++)
                {
                    putLong(destinationData, *(unsigned long *)data);
                    destinationData += sizeof(long);
                    data += sizeof(long);
                    putLong(destinationData, *(unsigned long *)data);
                    destinationData += sizeof(long);
                    data += sizeof(long);
                    if (*(unsigned short *)&l == 0x1234)
                    {
                        /* We have big endian
                         */
                       double *tmp;

                       tmp = (double *)(destinationData - sizeof(double));
                       swapDouble(tmp);
                    }
                }
            }
            return BABEL_FLOAT_OK;

	case BABEL_FLOAT_IEEE754_SINGLE:
            data = (unsigned char *)sourceData;
            for (i = 0; i < nrVariables; i++)
            {
                putLong(destinationData, *(unsigned long *)data);
                destinationData += sizeof(long);
                data += sizeof(long);
            }
            return BABEL_FLOAT_OK;

	default:
            return BABEL_FLOAT_ERROR;
	}
}

/* =================================================================== 
**
** Function:  babelFloatDecode
**  
** This function is futher described in a header file.
**   
** Implementation details:
**
** =============================================== 
*/
int
babelFloatDecode(unsigned char const *sourceData, 
                 BABEL_FLOAT_CONVERTION_TYPE type,
                 unsigned char *destinationData,
                 int nrVariables)
{
    unsigned char *data;
    int i;

    switch (type)
    {
    case BABEL_FLOAT_IEEE754_DOUBLE:
        {
            unsigned long l = 0x12345678;

            data = (unsigned char *)sourceData;
            for (i = 0; i < nrVariables; i++)
            {
                getLong(data, (unsigned long *)destinationData);
                destinationData += sizeof(long);
                data += sizeof(long);
                getLong(data, (unsigned long *)destinationData);
                destinationData += sizeof(long);
                data += sizeof(long);

                if (*(unsigned short *)&l == 0x1234)
                {
                    /* We have big endian
                     */
                    double *tmp;

                    tmp = (double *)(destinationData - sizeof(double));
                    swapDouble(tmp);
                }
            }
        }
        return BABEL_FLOAT_OK;

    case BABEL_FLOAT_IEEE754_SINGLE:
        data = (unsigned char *)sourceData;
        for (i = 0; i < nrVariables; i++)
        {
            getLong(data, (unsigned long *)destinationData);
            destinationData += sizeof(long);
            data += sizeof(long);
        }
        return BABEL_FLOAT_OK;

    default:
        return BABEL_FLOAT_ERROR;
    }
}
