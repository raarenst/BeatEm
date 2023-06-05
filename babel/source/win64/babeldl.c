/* ===================================================================
** Copyright (C) 2002 Ra-Tek. All rights reserved.
**
** File:  babeldl.c
**
** Abstract:
**   
**
** Required Defines:
**   
**
** ===============================================
*/
#include "babeldl.h"
#include <windows.h>
#include <stdio.h>

/* =================================================================== 
**
** Function:  babelDLOpen
**  
** This function is futher described in a header file.
**   
** Implementation details:
**
** =============================================== 
*/
struct BABELDL * 
babelDLOpen(char const* fileName) 
{
    return (struct BABELDL *)LoadLibrary(fileName);
}

/* =================================================================== 
**
** Function:  babelDLSymbol
**  
** This function is futher described in a header file.
**   
** Implementation details:
**
** =============================================== 
*/
void * 
babelDLSymbol(struct BABELDL* handle, char const* symbol) 
{
    return GetProcAddress((HINSTANCE)handle, symbol);
}

/* =================================================================== 
**
** Function:  babelDLClose
**  
** This function is futher described in a header file.
**   
** Implementation details:
**
** =============================================== 
*/
void 
babelDLClose(struct BABELDL* handle) 
{
    FreeLibrary((HANDLE)handle);
}

