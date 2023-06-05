/* ===================================================================
** Copyright (C) 2003 Ra-Tek. All rights reserved.
**
** File: babeltime.h
**
** Abstract:
**   
**
** Required Defines:
**   
**
** ===============================================
*/
#ifndef _BABELTIME_H
#define _BABELTIME_H

/* =================================================================== 
**
** Function:  babelTimeGetCurrentTime
**  
** Abstract: time in milliseconds
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
long
babelTimeGetCurrentTime(void);

void 
babelTimeSleep(unsigned int ms);

#endif /* _BABELTIME_H */
