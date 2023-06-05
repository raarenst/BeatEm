/* ===================================================================
** Copyright (C) 2002 Ra-Tek. All rights reserved.
**
** File:  babelsem.c
**
** Abstract:
**   
**
** Required Defines:
**   
**
** ===============================================
*/
#include <windows.h>

struct BabelSem;

/* =================================================================== 
**
** Function:  babelSemCreate
**  
** This function is futher described in a header file.
**   
** Implementation details:
**
** =============================================== 
*/
struct BabelSem * 
babelSemCreate(void) 
{
    return (struct BabelSem *)CreateSemaphore(NULL, 0, 2, NULL);
}

/* =================================================================== 
**
** Function:  babelSemDestroy
**  
** This function is futher described in a header file.
**   
** Implementation details:
**
** =============================================== 
*/
void 
babelSemDestroy(struct BabelSem *sem) 
{
    CloseHandle((HANDLE)sem);
}

/* =================================================================== 
**
** Function:  babelSemWait
**  
** This function is futher described in a header file.
**   
** Implementation details:
**
** =============================================== 
*/
void 
babelSemWait(struct BabelSem *sem) 
{
    WaitForSingleObject((HANDLE)sem, INFINITE);
}

/* =================================================================== 
**
** Function:  babelSemSignal 
**  
** This function is futher described in a header file.
**   
** Implementation details:
**
** =============================================== 
*/
void 
babelSemSignal(struct babelSem *sem) 
{
    ReleaseSemaphore((HANDLE)sem, 1, NULL);
}
