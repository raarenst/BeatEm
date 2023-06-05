/* ===================================================================
** Copyright (C) 2003 Ra-Tek. All rights reserved.
**
** File: babelsem.h
**
** Abstract:
**   
**
** Required Defines:
**   
**
** ===============================================
*/
#ifndef _BABELSEM_H
#define _BABELSEM_H

typedef struct BabelSem BabelSem_t;

/* =================================================================== 
**
** Function:  babelSemCreate
**  
** Abstract:
**   
**
** Parameters:
**   None
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
struct BabelSem * 
babelSemCreate(void);

/* =================================================================== 
**
** Function:  babelSemDestroy
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
void 
babelSemDestroy(struct BabelSem *sem);

/* =================================================================== 
**
** Function:  babelSemWait
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
void 
babelSemWait(struct BabelSem *sem);

/* =================================================================== 
**
** Function:  babelSemSignal
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
void 
babelSemSignal(struct BabelSem *sem);

#endif /* _BABELSEM_H */
