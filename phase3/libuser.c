/*
 *  File:  libuser.c
 *
 *  Description:  This file contains the interface declarations
 *                to the OS kernel support package.
 *
 */

#include <phase1.h>
#include <phase2.h>
#include <libuser.h>
#include <usyscall.h>
#include <usloss.h>

#define CHECKMODE {    \
    if (psr_get() & PSR_CURRENT_MODE) { \
        console("Trying to invoke syscall from kernel\n"); \
        halt(1);  \
    }  \
}

/*
 *  Routine:  Spawn
 *
 *  Description: This is the call entry to fork a new user process.
 *
 *  Arguments:    char *name    -- new process's name
 *                PFV func      -- pointer to the function to fork
 *                void *arg     -- argument to function
 *                int stacksize -- amount of stack to be allocated
 *                int priority  -- priority of forked process
 *                int  *pid     -- pointer to output value
 *                (output value: process id of the forked process)
 *
 *  Return Value: 0 means success, -1 means error occurs
 *
 */
int Spawn(char *name, int (*func)(char *), char *arg, int stack_size,
    int priority, int *pid)
{
    sysargs sysArg;
    
    CHECKMODE;
    sysArg.number = SYS_SPAWN;
    sysArg.arg1 = (void *) func;
    sysArg.arg2 = arg;
    sysArg.arg3 = (void *) ((long)stack_size);
    sysArg.arg4 = (void *) ((long)priority);
    sysArg.arg5 = name;

    usyscall(&sysArg);

    *pid = (int) ((long)sysArg.arg1);
    return (int) ((long)sysArg.arg4);
} /* end of Spawn */


/*
 *  Routine:  Wait
 *
 *  Description: This is the call entry to wait for a child completion
 *
 *  Arguments:    int *pid -- pointer to output value 1
 *                (output value 1: process id of the completing child)
 *                int *status -- pointer to output value 2
 *                (output value 2: status of the completing child)
 *
 *  Return Value: 0 means success, -1 means error occurs
 *
 */
int Wait(int *pid, int *status)
{
    sysargs sysArg;
    
    CHECKMODE;
    sysArg.number = SYS_WAIT;
    sysArg.arg2 = status;

    usyscall(&sysArg);

    pid = (int *) sysArg.arg1;
    status = (int *) sysArg.arg2;
    return (int) ((long)sysArg.arg4);
} /* end of Wait */


/*
 *  Routine:  Terminate
 *
 *  Description: This is the call entry to terminate
 *               the invoking process and its children
 *
 *  Arguments:   int status -- the commpletion status of the process
 *
 *  Return Value: 0 means success, -1 means error occurs
 *
 */
void Terminate(int status)
{
    sysargs sysArg;

    CHECKMODE;
    sysArg.number = SYS_TERMINATE;
    sysArg.arg1 = (void *) ((long)status);

    usyscall(&sysArg);

    // return (int) ((long)sysArg.arg1);
} /* end of Terminate */


/*
 *  Routine:  SemCreate
 *
 *  Description: Create a semaphore.
 *
 *  Arguments:
 *
 */
int SemCreate(int value, int *semaphore)
{
    sysargs sysArg;

    CHECKMODE;
    sysArg.number = SYS_SEMCREATE;
    sysArg.arg1 = (void *) ((long) value);

    usyscall(&sysArg);

    *semaphore = (int) ((long) sysArg.arg1);
    return (long) sysArg.arg4;
} /* end of SemCreate */


/*
 *  Routine:  SemP
 *
 *  Description: "P" a semaphore.
 *
 *  Arguments:
 *
 */
int SemP(int semaphore)
{
    sysargs sysArg;

    CHECKMODE;
    sysArg.number = SYS_SEMP;
    sysArg.arg1 = (void *) ((long) semaphore);

    usyscall(&sysArg);

    return (long) sysArg.arg4;
} /* end of SemP */


/*
 *  Routine:  SemV
 *
 *  Description: "V" a semaphore.
 *
 *  Arguments:
 *
 */
int SemV(int semaphore)
{
    sysargs sysArg;

    CHECKMODE;
    sysArg.number = SYS_SEMV;
    sysArg.arg1 = (void *) ((long) semaphore);

    usyscall(&sysArg);

    return (long) sysArg.arg4;
} /* end of SemV */


/*
 *  Routine:  SemFree
 *
 *  Description: Free a semaphore.
 *
 *  Arguments:
 *
 */
int SemFree(int semaphore)
{
    sysargs sysArg;

    CHECKMODE;
    sysArg.number = SYS_SEMFREE;
    sysArg.arg1 = (void *) ((long) semaphore);

    usyscall(&sysArg);

    return (long) sysArg.arg4;
} /* end of SemFree */


/*
 *  Routine:  GetTimeofDay
 *
 *  Description: This is the call entry point for getting the time of day.
 *
 *  Arguments:
 *
 */
void GetTimeofDay(int *tod)
{
    sysargs sysArg;

    CHECKMODE;
    sysArg.number = SYS_GETTIMEOFDAY;
    sysArg.arg1 = tod;

    usyscall(&sysArg);
} /* end of GetTimeofDay */


/*
 *  Routine:  CPUTime
 *
 *  Description: This is the call entry point for the process' CPU time.
 *
 *  Arguments:
 *
 */
void CPUTime(int *cpu)
{
    sysargs sysArg;

    CHECKMODE;
    sysArg.number = SYS_CPUTIME;
    sysArg.arg1 = cpu;

    usyscall(&sysArg);
} /* end of CPUTime */


/*
 *  Routine:  GetPID
 *
 *  Description: This is the call entry point for the process' PID.
 *
 *  Arguments:
 *
 */
void GetPID(int *pid)
{
    sysargs sysArg;

    CHECKMODE;
    sysArg.number = SYS_GETPID;
    sysArg.arg1 = pid;

    usyscall(&sysArg);
} /* end of GetPID */

/* end libuser.c */
