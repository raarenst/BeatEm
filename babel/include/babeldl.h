/* ===================================================================
** Copyright (C) 2003 Ra-Tek. All rights reserved.
**
** File: babeldl.h
**
** Abstract:
**   
**
** Required Defines:
**   
**
** ===============================================
*/
#ifndef _BABELDL_H
#define _BABELDL_H

struct BABELDL;

/* =================================================================== 
**
** Function:  babelDLOpen
**  
** Abstract:
**   Opens a shared object
**
** Parameters:
**   fileName - The name of the shared object file
**   
** Return value:  
**   A non NULL handle if successful, otherwise NULL.
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
extern struct BABELDL * 
babelDLOpen(char const *fileName);

/* =================================================================== 
**
** Function:  babelDLSymbol
**  
** Abstract:
**    Get a pointer to a symbol in the shared file
**
** Parameters:
**     handle - A handle returned from papiDLOpen
**     symbol - The name of the symbol to get a reference to
**   
** Return value:  
**   A pointer to the requested symbol.
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
extern void * 
babelDLSymbol(struct BABELDL *handle, char const *symbol);

/* =================================================================== 
**
** Function:  babelDLClose
**  
** Abstract:
**   Close the shared object
**
** Parameters:
**   handle - A handle returned from papiDLOpen
**
** Return value:  
**   None
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
extern void 
babelDLClose(struct BABELDL *handle);

#endif /* _BABELDL_H */
