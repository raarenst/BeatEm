/* =================================================================== 
** Copyright (C) 2002 Ra-Tek. All rights reserved.
**
** File:  babeltime.c
**
** Abstract:
**   
**
** Required Defines:
**   
**
** ===============================================
*/
#include <time.h>
#include "babeltime.h"

//clock_t start_time_ms;

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
long
babelTimeGetCurrentTime(void) 
{
    return (long)time(NULL);
    //return (long long)clock();
}
