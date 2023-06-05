/* ===================================================================
** Copyright (C) 2003 Ra-Tek. All rights reserved.
**
** File: babelfloat.h
**
** Abstract:
**   
**
** Required Defines:
**   
**
** ===============================================
*/
#ifndef _BABEL_FLOAT_H
#define _BABEL_FLOAT_H

/*
** Convertion modes
**
*/
typedef enum 
{
	/* The IEEE754 can be used from big or little endian processors
	 * in any combination. 
	 */
	BABEL_FLOAT_IEEE754_SINGLE = 1,
	BABEL_FLOAT_IEEE754_DOUBLE = 2,

} BABEL_FLOAT_CONVERTION_TYPE;

/*
** Return Values
**
*/
#define BABEL_FLOAT_OK      0
#define BABEL_FLOAT_ERROR  -1

/* =================================================================== 
**
** Function:  babelFloatEncode
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
extern int 
babelFloatEncode(unsigned char const *sourceData, 
                 BABEL_FLOAT_CONVERTION_TYPE type,
                 unsigned char *destinationData,
                 int nrVariables);

/* =================================================================== 
**
** Function:  babelFloatDecode
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
extern int
babelFloatDecode(unsigned char const *sourceData, 
                 BABEL_FLOAT_CONVERTION_TYPE type,
                 unsigned char *destinationData,
                 int nrVariables);

#endif /* _BABEL_FLOAT_H */
