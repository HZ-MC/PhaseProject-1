/* Programmers: Javier Felix, Mark Festejo, Hassan Martinez
 * Project Name: Phase 3
 * Date: 04/16/2020
 */

#include <usloss.h>
#include <phase1.h>
#include <phase2.h>
#include <phase3.h>
#include <usyscall.h>
#include <libuser.h>
#include <sems.h>
#include <string.h>

/* ------------------------- Prototypes ----------------------------------- */
void check_kernel_mode(char *);
void nullsys3(sysargs *);
void spawn(sysargs *);
void wait(sysargs *);
void terminate(sysargs *);
void semCreate(sysargs *);
int semCreateReal(int);
void semP(sysargs *);
void semPReal(int);
void semV(sysargs *);
void semVReal(int);
void semFree(sysargs *);
int semFreeReal(int);
void getTimeOfDay(sysargs *);
void cpuTime(sysargs *);
void getPID(sysargs *);
int spawnReal(char *, int(*)(char *), char *, int, int);
int spawnLaunch(char *);
int waitReal(int *);
void terminateReal(int);
void emptyProc3(int);
void initProc(int);
void setUserMode();
void initProcQueue3(procQueue*, int);
void enq3(procQueue*, procPtr3);
procPtr3 deq3(procQueue*);
procPtr3 peek3(procQueue*);
void removeChild3(procQueue*, procPtr3);
extern int start3();

void enqBlockedProc(procPtr3*, procPtr3);
procPtr3 deqBlockedProc(procPtr3*);


/* -------------------------- Globals ------------------------------------- */
int debug3 = 0;

// int sems[MAXSEMS];
semaphore SemTable[MAXSEMS];
int numSems;
procStruct3 ProcTable3[MAXPROC];

int
start2(char *arg)
{
    int pid;
    int status;
    /*
     * Check kernel mode here.
     */
    check_kernel_mode("start2");

    /*
     * Data structure initialization as needed...
     */

    // populate system call vec
    int i;
    for (i = 0; i < MAXSYSCALLS; i++) {
        sys_vec[i] = nullsys3;
    }
    sys_vec[SYS_SPAWN] = spawn;
    sys_vec[SYS_WAIT] = wait;
    sys_vec[SYS_TERMINATE] = terminate;
    sys_vec[SYS_SEMCREATE] = semCreate;
    sys_vec[SYS_SEMP] = semP;
    sys_vec[SYS_SEMV] = semV;
    sys_vec[SYS_SEMFREE] = semFree;
    sys_vec[SYS_GETTIMEOFDAY] = getTimeOfDay;
    sys_vec[SYS_CPUTIME] = cpuTime;
    sys_vec[SYS_GETPID] = getPID;

    // populate proc table
    for (i = 0; i < MAXPROC; i++) {
        emptyProc3(i);
    }

    // populate semaphore table
    for (i = 0; i < MAXSEMS; i++) {
        SemTable[i].id = -1;
        SemTable[i].value = -1;
        SemTable[i].startingValue = -1;
        SemTable[i].priv_mBoxID = -1;
        SemTable[i].mutex_mBoxID = -1;
    }

    numSems = 0;

    /*
     * Create first user-level process and wait for it to finish.
     * These are lower-case because they are not system calls;
     * system calls cannot be invoked from kernel mode.
     * Assumes kernel-mode versions of the system calls
     * with lower-case names.  I.e., Spawn is the user-mode function
     * called by the test cases; spawn is the kernel-mode function that
     * is called by the syscallHandler; spawnReal is the function that
     * contains the implementation and is called by spawn.
     *
     * Spawn() is in libuser.c.  It invokes USLOSS_Syscall()
     * The system call handler calls a function named spawn() -- note lower
     * case -- that extracts the arguments from the sysargs pointer, and
     * checks them for possible errors.  This function then calls spawnReal().
     *
     * Here, we only call spawnReal(), since we are already in kernel mode.
     *
     * spawnReal() will create the process by using a call to fork1 to
     * create a process executing the code in spawnLaunch().  spawnReal()
     * and spawnLaunch() then coordinate the completion of the phase 3
     * process table entries needed for the new process.  spawnReal() will
     * return to the original caller of Spawn, while spawnLaunch() will
     * begin executing the function passed to Spawn. spawnLaunch() will
     * need to switch to user-mode before allowing user code to execute.
     * spawnReal() will return to spawn(), which will put the return
     * values back into the sysargs pointer, switch to user-mode, and
     * return to the user code that called Spawn.
     */
    if (debug3)
        console("Spawning start3...\n");
    pid = spawnReal("start3", start3, NULL, USLOSS_MIN_STACK, 3);

    /* Call the waitReal version of your wait code here.
     * You call waitReal (rather than Wait) because start2 is running
     * in kernel (not user) mode.
     */
    pid = waitReal(&status);

    if (debug3)
        console("Quitting start2...\n");

    quit(pid);
    return -1;
} /* start2 */


// Spawn function that checks for correctness.
void spawn(sysargs *args)
{
    check_kernel_mode("spawn");

    int (*func)(char *) = args->arg1;
    char *arg = args->arg2;
    int stack_size = (int) ((long)args->arg3);
    int priority = (int) ((long)args->arg4);
    char *name = (char *)(args->arg5);

    if (debug3)
        console("spawn(): args are: name = %s, stack size = %d, priority = %d\n", name, stack_size, priority);

    int pid = spawnReal(name, func, arg, stack_size, priority);
    int status = 0;

    if (debug3)
        console("spawn(): spawnd pid %d\n", pid);

    // terminate self if zapped
    if (is_zapped())
        terminateReal(1);

    // switch to user mode
      setUserMode();

    // swtich back to kernel mode and put values for Spawn
    args->arg1 = (void *) ((long)pid);
    args->arg4 = (void *) ((long)status);
}

int spawnReal(char *name, int (*func)(char *), char *arg, int stack_size, int priority)
{
    check_kernel_mode("spawnReal");

    if (debug3)
        console("spawnReal(): forking process %s... \n", name);

    // fork the process and get its pid
    int pid = fork1(name, spawnLaunch, arg, stack_size, priority);

    if (debug3)
        console("spawnReal(): forked process name = %s, pid = %d\n", name, pid);

    // return -1 if fork failed
    if (pid < 0)
        return -1;

    // now we have the pid, we can get the child table entry
    procPtr3 child = &ProcTable3[pid % MAXPROC];
    enq3(&ProcTable3[getpid() % MAXPROC].childrenQueue, child); // add to children queue

    // if spawnLaunch hasn't done it yet, set up proc table entry
    if (child->pid < 0) {
        if (debug3)
            console("spawnReal(): initializing proc table entry for pid %d\n", pid);
        initProc(pid);
    }
    
    child->startFunc = func; // give proc its starting function
    child->parentPtr = &ProcTable3[getpid() % MAXPROC]; // set child's parent pointer

    // unblock the process so spawnLaunch can start it
    MboxCondSend(child->mboxID, 0, 0);

    return pid;
}


// Will launch usermode processes and terminate it.
int spawnLaunch(char *startArg) {
    check_kernel_mode("spawnLaunch");

    if (debug3)
        console("spawnLaunch(): launched pid = %d\n", getpid());

    // terminate self if zapped
    if (is_zapped())
        terminateReal(1);

    // get the proc info
    procPtr3 proc = &ProcTable3[getpid() % MAXPROC];

    // if spawnReal hasn't done it yet, set up proc table entry
    if (proc->pid < 0) {
        if (debug3)
            console("spawnLaunch(): initializing proc table entry for pid %d\n", getpid());
        initProc(getpid());

        // block until spawnReal is done
        MboxReceive(proc->mboxID, 0, 0);
    }

    // switch to user mode
    setUserMode();

    if (debug3)
        console("spawnLaunch(): starting process %d...\n", proc->pid);

    // call the function to start the process
    int status = proc->startFunc(startArg);

    if (debug3)
        console("spawnLaunch(): terminating process %d with status %d\n", proc->pid, status);

    Terminate(status); // terminate the process if it hasn't terminated itself
    return 0;
}


// Waits for the child process to terminate.
void wait(sysargs *args)
{
    check_kernel_mode("wait");

    int *status = args->arg2;
    int pid = waitReal(status);

    if (debug3) {
        console("wait(): joined with child pid = %d, status = %d\n", pid, *status);
    }

    args->arg1 = (void *) ((long) pid);
    args->arg2 = (void *) ((long) *status);
    args->arg4 = (void *) ((long) 0);

    // terminate self if zapped
    if (is_zapped())
        terminateReal(1);

    // switch back to user mode
    setUserMode();
}

int waitReal(int *status)
{
    check_kernel_mode("waitReal");

    if (debug3)
        console("in waitReal\n");
    int pid = join(status);
    return pid;
}


// Terminates the process
void terminate(sysargs *args)
{
    check_kernel_mode("terminate");

    int status = (int)((long)args->arg1);
    terminateReal(status);
    // switch back to user mode
    setUserMode();
}

void terminateReal(int status)
{
    check_kernel_mode("terminateReal");

    if (debug3)
        console("terminateReal(): terminating pid %d, status = %d\n", getpid(), status);

    // zap all children
    procPtr3 proc = &ProcTable3[getpid() % MAXPROC];
    while (proc->childrenQueue.size > 0) {
        procPtr3 child = deq3(&proc->childrenQueue);
        zap(child->pid);
    }
    // remove self from parent's list of children
    removeChild3(&(proc->parentPtr->childrenQueue), proc);
    quit(status);
}


// Creates a semaphore
void semCreate(sysargs *args)
{
    check_kernel_mode("semCreate");

    int value = (long) args->arg1;

    // fails if value is negative or no sems avaliable
    if (value < 0 || numSems == MAXSEMS) {
        args->arg4 = (void*) (long) -1;
    }
    else {
        numSems++;
        int handle = semCreateReal(value);
        args->arg1 = (void*) (long) handle;
        args->arg4 = 0;
    }

    if (is_zapped()) {
        terminateReal(0);
    }
    else {
        setUserMode();
    }
}

int semCreateReal(int value) {
    check_kernel_mode("semCreateReal");

    int i;
    int priv_mBoxID = MboxCreate(value, 0);
    int mutex_mBoxID = MboxCreate(1, 0);

    MboxSend(mutex_mBoxID, NULL, 0);

    for (i = 0; i < MAXSEMS; i++) {
        if (SemTable[i].id == -1) {
            SemTable[i].id = i;
            SemTable[i].value = value;
            SemTable[i].startingValue = value;
            SemTable[i].priv_mBoxID = priv_mBoxID;
            SemTable[i].mutex_mBoxID = mutex_mBoxID;
            initProcQueue3(&SemTable[i].blockedProcs, BLOCKED);
            break;
        }
    }

    int j;
    for (j = 0; j < value; j++) {
        MboxSend(priv_mBoxID, NULL, 0);
    }

    MboxReceive(mutex_mBoxID, NULL, 0);

    return SemTable[i].id;
}


// Contains arguments through sysargs
void semP(sysargs *args)
{
    check_kernel_mode("semP");

    int handle = (long) args->arg1;

    if (handle < 0)
        args->arg4 = (void*) (long) -1;
    else {
        args->arg4 = 0;
        semPReal(handle);
    }

    if (is_zapped()) {
        terminateReal(0);
    }
    else {
        setUserMode();
    }
}

void semPReal(int handle) {
    check_kernel_mode("semPReal");

    // get mutex on this semaphore
    MboxSend(SemTable[handle].mutex_mBoxID, NULL, 0);

    // block if value is 0
    if (SemTable[handle].value == 0) {
        enq3(&SemTable[handle].blockedProcs, &ProcTable3[getpid()%MAXPROC]);
        MboxReceive(SemTable[handle].mutex_mBoxID, NULL, 0);

        int result = MboxReceive(SemTable[handle].priv_mBoxID, NULL, 0);

        // terminate if the semaphore freed while we were blocked
        if (SemTable[handle].id < 0)
            terminateReal(1);

        // get mutex again when we unblock
        MboxSend(SemTable[handle].mutex_mBoxID, NULL, 0);

        if (result < 0) {
            console("semP(): bad receive");
        }
    }

    else {
        SemTable[handle].value -= 1 ;

        int result = MboxReceive(SemTable[handle].priv_mBoxID, NULL, 0);
        if (result < 0) {
            console("semP(): bad receive");
        }
    }


    MboxReceive(SemTable[handle].mutex_mBoxID, NULL, 0);
}



// Contains arguments
void semV(sysargs *args)
{
    check_kernel_mode("semV");

    int handle = (long) args->arg1;

    if (handle < 0)
        args->arg4 = (void*) (long) -1;
    else
        args->arg4 = 0;

    semVReal(handle);

    if (is_zapped()) {
        terminateReal(0);
    }
    else {
        setUserMode();
    }
}

void semVReal(int handle) {
    check_kernel_mode("semVReal");

    MboxSend(SemTable[handle].mutex_mBoxID, NULL, 0);

    // unblock blocked proc
    if (SemTable[handle].blockedProcs.size > 0) {
        deq3(&SemTable[handle].blockedProcs);

        MboxReceive(SemTable[handle].mutex_mBoxID, NULL, 0); // need to receive on mutex so semP can send right after receiving on privmbox

        MboxSend(SemTable[handle].priv_mBoxID, NULL, 0);

        MboxSend(SemTable[handle].mutex_mBoxID, NULL, 0);
    }
    else {
        SemTable[handle].value += 1 ;
    }

    MboxReceive(SemTable[handle].mutex_mBoxID, NULL, 0);
}


// Free arguments
void semFree(sysargs *args)
{
   check_kernel_mode("semFree");

    int handle = (long) args->arg1;

    if (handle < 0)
        args->arg4 = (void*) (long) -1;
    else {
        args->arg4 = 0;
        int value = semFreeReal(handle);
        args->arg4 = (void*) (long) value;
    }

    if (is_zapped()) {
        terminateReal(0);
    }
    else {
        setUserMode();
    }
}

int semFreeReal(int handle) {
    check_kernel_mode("semFreeReal");

    int mutexID = SemTable[handle].mutex_mBoxID;
    MboxSend(mutexID, NULL, 0);

    int privID = SemTable[handle].priv_mBoxID;

    SemTable[handle].id = -1;
    SemTable[handle].value = -1;
    SemTable[handle].startingValue = -1;
    SemTable[handle].priv_mBoxID = -1;
    SemTable[handle].mutex_mBoxID = -1;
    numSems--;

    // terminate procs waiting on this semphore
    if (SemTable[handle].blockedProcs.size > 0) {
        while (SemTable[handle].blockedProcs.size > 0) {
            deq3(&SemTable[handle].blockedProcs);
            int result = MboxSend(privID, NULL, 0);
            if (result < 0) {
                console("semFreeReal(): send error");
            }
        }
        MboxReceive(mutexID, NULL, 0);
        return 1;
    }

    else {
        MboxReceive(mutexID, NULL, 0);
        return 0;
    }
}


// Gets time of day
void getTimeOfDay(sysargs *args)
{
    check_kernel_mode("getTimeOfDay");
    *((int *)(args->arg1)) = sys_clock();
}


// Gets CPU time
void cpuTime(sysargs *args)
{
    check_kernel_mode("cpuTime");
    *((int *)(args->arg1)) = readtime();
}


// Gets PID
void getPID(sysargs *args)
{
    check_kernel_mode("getPID");
    *((int *)(args->arg1)) = getpid();
}


/* an error method to handle invalid syscalls */
void nullsys3(sysargs *args)
{
    console("nullsys(): Invalid syscall %d. Terminating...\n", args->number);
    terminateReal(1);
}


/* initializes proc struct */
void initProc(int pid) {
    check_kernel_mode("initProc()");

    int i = pid % MAXPROC;

    ProcTable3[i].pid = pid;
    ProcTable3[i].mboxID = MboxCreate(0, 0);
    ProcTable3[i].startFunc = NULL;
    ProcTable3[i].nextProcPtr = NULL;
    initProcQueue3(&ProcTable3[i].childrenQueue, CHILDREN);
}


/* empties proc struct */
void emptyProc3(int pid) {
    check_kernel_mode("emptyProc()");

    int i = pid % MAXPROC;

    ProcTable3[i].pid = -1;
    ProcTable3[i].mboxID = -1;
    ProcTable3[i].startFunc = NULL;
    ProcTable3[i].nextProcPtr = NULL;
}


// Check in kernel mode. Halt otherwise
void check_kernel_mode(char *name)
{
    if( (PSR_CURRENT_MODE & psr_get()) == 0 ) {
        console("%s: called while in user mode, by process %d. Halting...\n",
             name, getpid());
        halt(1);
    }
}


// To switch to usermode
void setUserMode()
{
    psr_set( psr_get() & ~PSR_CURRENT_MODE );
}


/* ------------------------------------------------------------------------
  Below are functions that manipulate ProcQueue:
    initProcQueue, enq, deq, removeChild and peek.
   ----------------------------------------------------------------------- */

/* Initialize the given procQueue */
void initProcQueue3(procQueue* q, int type) {
  q->head = NULL;
  q->tail = NULL;
  q->size = 0;
  q->type = type;
}

/* Add the given procPtr3 to the back of the given queue. */
void enq3(procQueue* q, procPtr3 p) {
  if (q->head == NULL && q->tail == NULL) {
    q->head = q->tail = p;
  } else {
    if (q->type == BLOCKED)
      q->tail->nextProcPtr = p;
    else if (q->type == CHILDREN)
      q->tail->nextSiblingPtr = p;
    q->tail = p;
  }
  q->size++;
}

/* Remove and return the head of the given queue. */
procPtr3 deq3(procQueue* q) {
  procPtr3 temp = q->head;
  if (q->head == NULL) {
    return NULL;
  }
  if (q->head == q->tail) {
    q->head = q->tail = NULL;
  }
  else {
    if (q->type == BLOCKED)
      q->head = q->head->nextProcPtr;
    else if (q->type == CHILDREN)
      q->head = q->head->nextSiblingPtr;
  }
  q->size--;
  return temp;
}

/* Remove the child process from the queue */
void removeChild3(procQueue* q, procPtr3 child) {
  if (q->head == NULL || q->type != CHILDREN)
    return;

  if (q->head == child) {
    deq3(q);
    return;
  }

  procPtr3 prev = q->head;
  procPtr3 p = q->head->nextSiblingPtr;

  while (p != NULL) {
    if (p == child) {
      if (p == q->tail)
        q->tail = prev;
      else
        prev->nextSiblingPtr = p->nextSiblingPtr->nextSiblingPtr;
      q->size--;
    }
    prev = p;
    p = p->nextSiblingPtr;
  }
}

/* Return the head of the given queue. */
procPtr3 peek3(procQueue* q) {
  if (q->head == NULL) {
    return NULL;
  }
  return q->head;
}
