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
//    return (clock() * 1000 / CLOCKS_PER_SEC);
}

/* =================================================================== 
**
** Function:  babelTimeSleep
**  
** This function is futher described in a header file.
**  
** Implementation details:
**
** =============================================== 
*/
void 
babelTimeSleep(unsigned int ms) 
{
    struct timespec ts;

    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000;

    nanosleep(&ts, &ts);
}