/* ===================================================================
** Copyright (C) 2002 Ra-Tek. All rights reserved.
**
** File:  babelthread.c
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
#include <process.h>
#include <stdio.h>
#include <assert.h>

#include "babelthread.h"

#define TOUCH(a) ((a) = (a))

struct BabelThread
{
    void *tid;
};

/* =================================================================== 
**
** Function:  killMe
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
killMe(void)
{
    _endthreadex(0);
}

/* =================================================================== 
**
** Function:  babelThreadKill
**  
** This function is futher described in a header file.
**   
** Implementation details:
**
** =============================================== 
*/
void 
babelThreadKill(struct BabelThread *thread) 
{
    int rv;
    CONTEXT ctxt;

    ctxt.ContextFlags = CONTEXT_CONTROL;
    rv = GetThreadContext(thread->tid, &ctxt);
    assert(rv);
 
    ctxt.Rip = (DWORD64)killMe; // Changed from EIP and (DWORD) not sure it works.
    rv = SetThreadContext(thread->tid, &ctxt);
    assert(rv);

    rv = ResumeThread((HANDLE)thread->tid);
    assert(rv != -1);

    free(thread);
}

/* =================================================================== 
**
** Function:  babelThreadCreate
**  
** This function is futher described in a header file.
**   
** Implementation details:
**
** =============================================== 
*/
struct BabelThread *
babelThreadCreate(void* (*func)(void *arg),
                  void *data,
                  enum BabelThreadPrioHint prioHint) 
{
    struct BabelThread *thread;
    int rv;
    unsigned slask;
    HANDLE tid;
    HANDLE rtid;
    unsigned (__stdcall * wfunc)(void *) =
        (unsigned (__stdcall *)(void *))func;

    tid = (HANDLE)_beginthreadex(NULL, 0, wfunc, 
                                 data, CREATE_SUSPENDED, &slask);

    if(tid == 0) 
    {
        printf("CreateThread failed with error code %ld.\n", GetLastError());
	exit(1);
    }
    switch(prioHint) 
    {
    case BABELTHREAD_PRIOHINT_LOW:
        rv = SetThreadPriority(tid, THREAD_PRIORITY_BELOW_NORMAL);
        assert(rv);
        break;

    case BABELTHREAD_PRIOHINT_MID:
        rv = SetThreadPriority(tid, THREAD_PRIORITY_NORMAL);
        assert(rv);
        break;

    case BABELTHREAD_PRIOHINT_HIGH:
        rv = SetThreadPriority(tid, THREAD_PRIORITY_ABOVE_NORMAL);
        assert(rv);
        break;
    }

    DuplicateHandle(GetCurrentProcess(),
                    tid,
                    GetCurrentProcess(),
                    &rtid,
                    0, FALSE, DUPLICATE_SAME_ACCESS);

    rv = CloseHandle(tid);
    assert(rv);

    thread = malloc(sizeof(struct BabelThread));
    thread->tid = (void *)rtid;

    return thread;
}

/* =================================================================== 
**
** Function:  babelThreadSuspend
**  
** This function is futher described in a header file.
**   
** Implementation details:
**
** =============================================== 
*/
void 
babelThreadSuspend(struct BabelThread *thread) 
{
    int rv;

    rv = SuspendThread((HANDLE)thread->tid);
    assert(rv != -1);
}

/* =================================================================== 
**
** Function:  babelThreadResume
**  
** This function is futher described in a header file.
**   
** Implementation details:
**
** =============================================== 
*/
void 
babelThreadResume(struct BabelThread *thread) 
{
    int rv;

    rv = ResumeThread((HANDLE)thread->tid);
    assert(rv != -1);
}

/* =================================================================== 
**
** Function:  babelThreadExit
**  
** This function is futher described in a header file.
**   
** Implementation details:
**
** =============================================== 
*/
void 
babelThreadExit(void) 
{
    for(;;) 
    {
        ExitThread(0);
    }
}

/* =================================================================== 
**
** Function:  babelThreadInit
**  
** This function is futher described in a header file.
**   
** Implementation details:
**
** =============================================== 
*/
void 
babelThreadInit(void) 
{

}

/* =================================================================== 
**
** Function:  babelThreadYield
**  
** This function is futher described in a header file.
**   
** Implementation details:
**
** =============================================== 
*/
void 
babelThreadYield(void) 
{
    Sleep(0);
}

/* =================================================================== 
**
** Function:  babelThreadJoin
**  
** This function is futher described in a header file.
**   
** Implementation details:
**
** =============================================== 
*/
void* 
babelThreadJoin(struct BabelThread *thread) 
{
    int rv;
    DWORD exitCode;

    WaitForSingleObject(thread->tid, INFINITE);
    rv = GetExitCodeThread(thread->tid, &exitCode);
    assert(rv);
    rv = CloseHandle(thread->tid);
    assert(rv);
    return NULL;
} 

/* =================================================================== 
**
** Function:  babelThreadSleep
**  
** This function is futher described in a header file.
**  
** Implementation details:
**
** =============================================== 
*/
void 
babelThreadSleep(unsigned int ms) 
{
    Sleep(ms);
}

/* =================================================================== 
**
** Function:  babelThreadDeleteObj  
**  
** This function is futher described in a header file.
**   
** Implementation details:
**
** =============================================== 
*/
void 
babelThreadDeleteObj(struct BabelThread *thread)
{
    free(thread);
}

/* =================================================================== 
**
** Function:  babelThreadTlsAlloc
**  
** This function is futher described in a header file.
**   
** Implementation details:
**
** =============================================== 
*/
TlsIndex
babelThreadTlsAlloc(void)
{
    return TlsAlloc();
}

/* =================================================================== 
**
** Function:  babelThreadTlsSetvalue
**  
** This function is futher described in a header file.
**   
** Implementation details:
**
** =============================================== 
*/
void
babelThreadTlsSetValue(TlsIndex index, void *value)
{
    (void)TlsSetValue(index, value); 
}

/* =================================================================== 
**
** Function:  babelThreadTlsGetValue
**  
** This function is futher described in a header file.
**   
** Implementation details:
**
** =============================================== 
*/
void *
babelThreadTlsGetValue(TlsIndex index)
{
    return TlsGetValue(index);
}

/* =================================================================== 
**
** Function:  babelThreadTlsFree
**  
** This function is futher described in a header file.
**   
** Implementation details:
**
** =============================================== 
*/
void
babelThreadTlsFree(TlsIndex index)
{
    (void)TlsFree(index);
}
