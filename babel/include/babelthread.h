/* ===================================================================
** Copyright (C) 2003 Ra-Tek. All rights reserved.
**
** File: babelthread.h
**
** Abstract:
**   
**
** Required Defines:
**   
**
** ===============================================
*/
#ifndef _BABELTHREAD_H
#define _BABELTHREAD_H

typedef struct BabelThread BabelThread_t;

/* Thread local store variable
 */
typedef unsigned long TlsIndex;

enum BabelThreadPrioHint 
{
    BABELTHREAD_PRIOHINT_LOW,
    BABELTHREAD_PRIOHINT_MID,
    BABELTHREAD_PRIOHINT_HIGH
};

/* =================================================================== 
**
** Function:  babelThreadInit
**  
** Abstract:
**   
**
** Parameters:
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
babelThreadInit(void);

/* =================================================================== 
**
** Function:  babelThreadKill
**  
** Abstract:
**   Kill all created threads, after this call all thread handles
**   are illegal.
**   
** Parameters:
**   thread	- A thread handle returned by babelThreadCreate()
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
babelThreadKill(struct BabelThread *thread);

/* =================================================================== 
**
** Function:  babelThreadCreate
**  
** Abstract:
**   This call creates a suspended thread.
**
** Parameters:
**   func     - The thread entry point.
**   data     - Pointer passed to the thread function.
**   prioHint - A priority hint that the thread backent _may_
**              use to improve performance.
**
** Return value:  
**   A thread handle.
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
extern struct BabelThread * 
babelThreadCreate(void* (*func)(void *arg),
		  void *data,
                  enum BabelThreadPrioHint prioHint);

/* =================================================================== 
**
** Function:  babelThreadExit
**  
** Abstract:
**   
**
** Parameters:
**   None
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
babelThreadExit(void);

/* =================================================================== 
**
** Function:  babelThreadSuspend
**  
** Abstract:
**   Suspend execution of 'thread'. Only threads that are
**   running(Resumed) may be suspended.
**   
** Parameters:
**   thread	- A thread handle returned by babelThreadCreate()
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
babelThreadSuspend(struct BabelThread *thread);

/* =================================================================== 
**
** Function:  babelThreadResume
**  
** Abstract:
**   Resume execution of 'thread'. Only threads that are suspended
**   may be resumed.
**   
** Parameters:
**   thread	- A thread handle returned by babelThreadCreate()
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
babelThreadResume(struct BabelThread *thread);

/* =================================================================== 
**
** Function:  babelThreadJoin
**  
** Abstract:
**   
**
** Parameters:
**   thread	- A thread handle returned by babelThreadCreate()
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
void * 
babelThreadJoin(struct BabelThread *thread);

/* =================================================================== 
**
** Function:  babelThreadSleep
**  
** Abstract:
**   This call maked the thread inactive for a number of milli seconds.   
**
** Parameters:
**   ms    - Number of milliseconds for the calling thread to sleep.
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
babelThreadSleep(unsigned int ms);

/* =================================================================== 
**
** Function:  babelThreadYield
**  
** Abstract:
**   
**
** Parameters:
**   None
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
babelThreadYield(void);

/* =================================================================== 
**
** Function:  babelThreadDeleteObj
**  
** Abstract:
**   
**
** Parameters:
**   thread	- A thread handle returned by babelThreadCreate()
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
babelThreadDeleteObj(struct BabelThread *thread);

/* =================================================================== 
**
** Function:  babelThreadTlsAlloc
**  
** Abstract:
**   This call allocates a value on the thread local store.
**
** Parameters:
**   None
**   
** Return value:  
**   A thread local store index number
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
//extern TlsIndex
//babelThreadTlsAlloc(void);

/* =================================================================== 
**
** Function:  babelThreadTlsSetValue
**  
** Abstract:
**   
**
** Parameters:
**   index - the global tls index variable
**   value - the value to store
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
//extern void
//babelThreadTlsSetValue(TlsIndex index, void *value);

/* =================================================================== 
**
** Function:  babelThreadTlsGetValue
**  
** Abstract:
**   
**
** Parameters:
**   index - the global tls index variable 
**   
** Return value:  
**   The value stored in the threads local store.
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
//extern void *
//babelThreadTlsGetValue(TlsIndex index);

/* =================================================================== 
**
** Function:  babelThreadTlsFree
**  
** Abstract:
**   
**
** Parameters:
**      index - the global tls index variable
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
//extern void
//babelThreadTlsFree(TlsIndex index);

#endif /* _BABELTHREAD_H */
