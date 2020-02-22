/* ------------------------------------------------------------------------
   phase1.c
   CSCV 452
   @authors: Mark Festejo, Javier Felix, Hassan Martinez
   ------------------------------------------------------------------------ */

#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <stdio.h>
#include <phase1.h>               // Library file in the dir.
#include <usloss.h>               // Library file in the dir.
#include <sys/types.h>            // Library for getpid f(x).
#include <unistd.h>               // Library for getpid f(x).
#include "kernel.h"

/* ------------------------- Prototypes ----------------------------------- */

int sentinel(void *);
extern int start1(char *);
void dispatcher(void);
void launch();
static void enableInterrupts();
static void check_deadlock();
static void insertRL(proc_ptr proc);
int zap(int);
int is_zapped(void);

// Check Code because we have declare similar f(x) under different names.

void checkKernelMode(char *name)            // from Javier's code
int getpid();
int zap(int pid);
int isZapped();
int dispatcher(void);
int readCurrStartTime(void);
void timeSlice(void);
int readTime(void);
void clockHandler();
void disableInterrupts();                   // from Javier's code
void insertReadyList(proc_ptr toInsert);    // same as the inserRL()?
void removeReadyList(proc_ptr toRemove);

/* -------------------------- Globals ------------------------------------- */

/* Patrick's debugging global variable... */
int debugflag = 1;

/* the process table */
proc_struct ProcTable[MAXPROC];

/* Process lists = ReadyList is a linked list of processes pointers */
proc_ptr ReadyList = NULL;

/* current process ID */
proc_ptr Current;

/* the next pid to be assigned */
unsigned int next_pid = SENTINELPID;

/* number of processes running*/
int procCount;

/* -------------------------- Functions ----------------------------------- */
/* ------------------------------------------------------------------------
   Name - startup
   Purpose - Initializes process lists and clock interrupt vector.
         Start up sentinel process and the test process.
   Parameters - none, called by USLOSS
   Returns - nothing
   Side Effects - lots, starts the whole thing

   // call by the main and it is where it start everything. Initialize the startup process table.
   // startup should have a priorety 1.
   // Initialize the interrupt vector (int_vec[CLOCK_DEV] = clock_handler;)

   ----------------------------------------------------------------------- */

void startup() {
   
   // Check if in kernel mode, halt if in user mode.
   checkKernelMode("startup()");
   
   int i;      /* loop index */
   int result; /* value returned by call to fork1() */
   /* initialize the process table */
   if (DEBUG && debugflag) {
       console("startup(): initializing process table, ProcTable[]\n");
   }
   for (i = 0; i < MAXPROC; i++) {
       ProcTable[i].next_proc_ptr = NO_CURRENT_PROCESS;
       ProcTable[i].child_proc_ptr = NO_CURRENT_PROCESS;
       ProcTable[i].next_sibling_ptr = NO_CURRENT_PROCESS;
       ProcTable[i].quit_child_ptr = NO_CURRENT_PROCESS;
       ProcTable[i].name[0] = '\0';
       ProcTable[i].start_arg[0] = '\0';
       ProcTable[i].pid = EMPTY;
       ProcTable[i].ppid = EMPTY;
       ProcTable[i].priority = EMPTY;
       ProcTable[i].status = EMPTY;
       ProcTable[i].start_func = NO_CURRENT_PROCESS;
       ProcTable[i].stack = NO_CURRENT_PROCESS;
       ProcTable[i].stacksize = EMPTY;
       ProcTable[i].isZapped = EMPTY;
   }

   /* Initialize the Ready list, etc. */
   if (DEBUG && debugflag) {
       console("startup(): initializing the Ready & Blocked lists\n");
   }
   ReadyList = NULL;

   /* Initialize the clock interrupt handler */
   int_vec[CLOCK_INT] = clockHandler;

   /* startup a sentinel process */
   if (DEBUG && debugflag) {
       console("startup(): calling fork1() for sentinel\n");
   }
   result = fork1("sentinel", sentinel, NULL, USLOSS_MIN_STACK,
                  SENTINELPRIORITY);
   if (result < 0) {
       if (DEBUG && debugflag)
           console("startup(): fork1 of sentinel returned error, halting...\n");
       halt(1);
   }

   /* start the test process */
   if (DEBUG && debugflag) {
       console("startup(): calling fork1() for start1\n");
   }
   result = fork1("start1", start1, NULL, 2 * USLOSS_MIN_STACK, 1);
   if (result < 0) {
       console("startup(): fork1 for start1 returned an error, halting...\n");
       halt(1);
   }

   console("startup(): Should not see this message! ");
   console("Returned from fork1 call that created start1\n");
   return;
} /* startup */

/* ------------------------------------------------------------------------
   Name - finish
   Purpose - Required by USLOSS
   Parameters - none
   Returns - nothing
   Side Effects - none
   ----------------------------------------------------------------------- */

 void finish()
 {
     // Will first check if in kernel mode. Will halt if in user mode.
     checkKernelMode("finish()");

     if (DEBUG && debugflag)
       console("in finish...\n");
 } /* finish */

/* ------------------------------------------------------------------------

   Name - fork1
   Purpose - Gets a new process from the process table and initializes
             information of the process.  Updates information in the
             parent process to reflect this child process creation.
   Parameters - the process procedure address, the size of the stack and
                the priority to be assigned to the child process.
   Returns - the process id of the created child or -1 if no child could
             be created or if priority is not between max and min priority.
   Side Effects - ReadyList is changed, ProcTable is changed, Current
                  process information changed

    // Under this f(x) is use to created a new process under the child who is calling this f(x) is the parent.
    // Entry in the global array to stored the PID.

   ------------------------------------------------------------------------ */

 int fork1(char *name, int (*f)(void *), void *arg, int stacksize, int priority)
 {
     int proc_slot = -1;

    if (DEBUG && debugflag)
       console("fork1(): creating process %s\n", name);

    /* test if in kernel mode; halt if in user mode */

     checkkernelmode("fork1()");
     disableInterrupts();

    /* Return if stack size is too small */

     if (stacksize < USLOSS_MIN_STACK){
         console("fork1(): Stack size is too small.\n");
         return -2;
     }

     //Will return if startfun is NULL.
     if (start_func == NULL){
         if (DEBUG && debugflag)
             console("fork1(): Start function is null.\n");
         return -1;
     }

     //Will retrun if name is NULL.
     if (name == NULL){
         if (DEBUG && debugflag)
             console("fork1(): Name is null.\n");
         return 1;
     }

    /* find an empty slot in the process table */

     proc_slot = next_pid % MAXPROC;
     while (ProcTable[proc_slot].status != EMPTY){
         next_pid++;
         proc_slot = next_pid % MAXPROC;
     }


    /* fill-in entry in process table */
    if ( strlen(name) >= (MAXNAME - 1) ) {
       console("fork1(): Process name is too long.  Halting...\n");
       halt(1);
    }
    strcpy(ProcTable[proc_slot].name, name);
    ProcTable[proc_slot].start_func = start_func;
    if ( arg == NULL )
       ProcTable[proc_slot].start_arg[0] = '\0';
    else if ( strlen(arg) >= (MAXARG - 1) ) {
       console("fork1(): argument too long.  Halting...\n");
       halt(1);
    }
    else
       strcpy(ProcTable[proc_slot].start_arg, arg);

     //This will allocate the stack. fork1 is the only place to do this.

     ProcTable[proc_slot].stack = (char *) malloc(stacksize);
     ProcTable[proc_slot].stacksize = stacksize;


     //Need to check if malloc worked. If not, then we must halt.

     if (ProcTable[proc_slot.stack == NULL]){
         if (DEBUG && debugflag)
             console("fork1(): Malloc has failed. Halting now..\n");
         halt(1);
     }

     //Setting the process id.
     ProcTable[proc_slot].pid = next_pid++;

     //Setting the process priority.
     ProcTable[proc_slot].priority = priority;

     //Increment number of processes.
     numOfProcess++;

    /* Initialize context for this process, but use launch function pointer for
     * the initial value of the process's program counter (PC)
     */
    context_init(&(ProcTable[proc_slot].state), psr_get(),
                 ProcTable[proc_slot].stack,
                 ProcTable[proc_slot].stacksize, launch);

    /* for future phase(s) */
    p1_fork(ProcTable[proc_slot].pid);
    
     // let dispatcher decide which process runs next
    if (start_func != sentinel) { // don't dispatch sentinel!
        dispatcher(); 
    }

    // This will enable interrupts for the parent process
    enableInterrupts();
    
 } /* fork1 */

/* ------------------------------------------------------------------------
   Name - launch
   Purpose - Dummy function to enable interrupts and launch a given process
             upon startup.
   Parameters - none
   Returns - nothing
   Side Effects - enable interrupts
   ------------------------------------------------------------------------ */

void launch()
{
    // Variable to pass start_arg at quit().
    int result;
    // if debugflag is et to 0 then print where launch starts.
    if (DEBUG && debugflag){console("launch(): started\n");}
    /* Enable interrupts */
    enableInterrupts();
    /* Call the function passed to fork1, and capture its return value */
    result = Current->start_func(Current->start_arg);
    // if debugflag is set to 0 then print the pointer of the current PID.
    if (DEBUG && debugflag){console("Process %d returned to launch\n", Current->pid);}
    quit(result);
} /* launch */

/* ------------------------------------------------------------------------
   Name - join
   Purpose - Wait for a child process (if one has been forked) to quit.  If
             one has already quit, don't wait.
   Parameters - a pointer to an int where the termination code of the
                quitting process is to be stored.
   Returns - the process id of the quitting child joined on.
        -1 if the process was zapped in the join
        -2 if the process has no children
   Side Effects - If no child process has quit before join is called, the
                  parent is removed from the ready list and blocked.
   ------------------------------------------------------------------------ */

 int join(int *code)
 {

   // If the process is zapped then return -1.
   if(Current->is_zapped == ZAPPED){return -1;}
   // If the process does not have any children return -2.
   if(Current->child_proc_ptr == NULL){return -2;}
   // Process that call join() is blocked until child process quits.
   Current->status = BLOCKED;
   // Calling dispatcher().
   console("join(): calling dispatcher\n");
   dispatcher();
   // *code stored the quit status of the child.
   *code = kid_ptr->exit_code;
   // Return the PID of child process that is join().
   return kid_pid;

 } /* join */

 int zap(int pid)
 {

      // If the process is already zapped while in zap return -1.
      if(Current->is_zapped == ZAPPED){return -1;}
      // If the process has called quit then return 0.
      if(Current->is_zapped == QUIT){return 0;}
      // Default return -1.
      return -1;

     // Need to add
     // if a process tries to zap itself or attempt to zap a non-existent process call halt(1).

 } // zap()

 int is_zapped(void)
 {
   // If the process has been zapped return 1.
   if(Current->is_zapped == ZAPPED){return 1;}
   // Default return 0 since the process has not been zapped.
   return 0;
 } // is_zapped()

 /* ------------------------------------------------------------------------
        Name - isZapped
        Purpose - checks if the process has been zapped by another process.
        Parameters - none
        Returns - 0 if not zapped, 1 otherwise.
        Side Effects -  none
----------------------------------------------------------------------- */
int isZapped(){
    return Current->isZapped;
} // isZapped()

/* ------------------------------------------------------------------------
   Name - quit
   Purpose - Stops the child process and notifies the parent of the death by
             putting child quit info on the parents child completion code
             list.
   Parameters - the code to return to the grieving parent
   Returns - nothing
   Side Effects - changes the parent of pid child completion status list.
   ------------------------------------------------------------------------ */

 // This f(x) is used to print the error under the quit() where it takes walker out with halt(1).
 void quitout(proc_ptr walker)
 {
   // Display the status of the child under the error.
   console("Error! Active child status = %d\n", walker->status);
   console("Error! Process has active children and cannot quit.\n");
   // Setting the halt(1) to get its value.
   halt(1);
 } // quitout()

 void quit(int code)
 {
   console("quit(): checking for active children\n");
   // Checking if the parent has active children to display error/halt(1).
   if(Current->child_proc_ptr != NULL)
   {
     proc_ptr walker = Current->child_proc_ptr;
     // If walker status is not at QUIT state then show error/halt(1).
     if(walker->status != QUIT){void quitout(walker);}
     else
     {
       // While there are other childrens run.
       while(walker->next_sibling_ptr != NULL)
       {
         // Pointing to next child in the node.
         walker = walker->next_sibling_ptr;
         // If walker status is not at QUIT state then show error/halt(1).
         if(walker->status != QUIT){void quitout(walker);}
       }
     }
   }
   // Getting the status to quit.
   console("quit(): change status to QUIT\n");
   Current->status = QUIT;
   console("quit(): status of %s is %d (3 == QUIT)\n", Current->name, Current->status);

   // Check if parent block AND if it is calling join() to unblock it.
   if(Current->parent_ptr->status == BLOCKED && Current->parent_ptr != NULL)
   {
     console("quit(): unblock %s who called join, insert to ready list\n", Current->parent_ptr->name);
     Current->parent_ptr->status = READY;
     insertRL(Current->parent_ptr);
     // printReadyList();
     console("\n");
   }

   // Check parent to quit.
   if(Current->parent_ptr != NULL)
  {
    console("quit(): send exit_code to parent %s\n", Current->parent_ptr->name);
    Current->parent_ptr->exit_code = code;
  }

  // Calling dispatcher() to switch to the next process.
  console("quit(): call dispatcher\n");
  dispatcher();
  p1_quit(Current->pid);

 } /* quit */

/* ------------------------------------------------------------------------
  Name - dispatcher
  Purpose - dispatches ready processes.  The process with the highest
            priority (the first on the ready list) is scheduled to
            run.  The old process is swapped out and the new process
            swapped in.
  Parameters - none
  Returns - nothing
  Side Effects - the context of the machine is changed
  ----------------------------------------------------------------------- */

void dispatcher(void)
{

    // Check the priority of the Current process if it is lower that the ReadyList return.
    if(Current != NULL && Current->priority <= ReadyList->priority && Current->status == RUNNING){return;}

    // Setting variables for the next and old processes.
    proc_ptr next_process = Current;
    proc_ptr old_process = ReadyList;
    // Double set variable for same value between next_process & Current.
    Current = next_process;

    // If the old_process is NULL means is the first one then run this.
    if(old_process == NULL)
    {
      // Setting the next_process to RUNNING state.
      next_process->status = RUNNING;
      // Removing it from the ReadyList PID.
      removeFromRL(next_process->pid);
      // Display to the console about the context_switch name.
      console("dispatcher(); context_switch to %s\n", next_process->name);
      // Switching the context of the next process state.
      context_switch(NULL, &next_process-> state);
    }
    // If the old_process has QUIT
    else if(old_process->status == QUIT)
    {
      // Setting the next_process to RUNNING state.
      next_process->status = RUNNING;
      // Removing it from the ReadyList PID.
      removeFromRL(next_process->pid);
      // Display to the console about the context_switch names of old & new processes.
      console("dispatcher(): context_switch from %s to %s\n", old_process->name, next_process->name);
      printReadyList();
      console("\n");
      // Switching the context of the state of old and next processes.
      context_switch(&old_process->state, &next_process->state);
    }
    else
    {
      // Setting the next_process to RUNNING state.
      next_process->status = RUNNING;
      // Removing it from the ReadyList PID.
      removeFromRL(next_process->pid);

      // As long the old_process is not BLOCKED run.
      if(old_process->status != BLOCKED)
      {
        // Setting the old_process to RUNNING state.
        old_process->status = READY;
        // Add it to the ReadyList.
        insertRL(old_process);
        // Display to the console.
        printReadyList();
        console("\n");
      }
      // Display to the console about the context_switch names of old & new processes.
      console("dispatcher(): context_switch from %s to %s\n", old_process->name, next_process->name);
      // Switching the context of the state of old and next processes.
      context_switch(&old_process->state, &next_process->state);
    }

    // p1_switch(Current->pid, next_process->pid);

} /* dispatcher */

static void insertRL(proc_ptr proc)
{
  // Calling pointers for PCB.
  proc_ptr walker, previous;
  previous = NULL;
  walker = ReadyList;
  while(walker != NULL && walker->priority <= proc->priority)
  {
    previous = walker;
    walker = walker->next_proc_ptr;
  }
  if(previous == NULL)
  {
    // Move the process at the front of the ReadyList.
    proc->next_proc_ptr = ReadyList;
    ReadyList = proc;
  }
  else
  {
    // Move the process after the previous.
    previous->next_proc_ptr = proc;
    proc->next_proc_ptr = walker;
  }
  return;

} // insertRL()

/* ------------------------------------------------------------------------

   Name - sentinel
   Purpose - The purpose of the sentinel routine is two-fold.  One
             responsibility is to keep the system going when all other
         processes are blocked.  The other is to detect and report
         simple deadlock states.
   Parameters - none
   Returns - nothing
   Side Effects -  if system is in deadlock, print appropriate error
           and halt.

     // Sentinel is the terminator for good and bad. It is call the last f(x) in the program.

   ----------------------------------------------------------------------- */

int sentinel(void *dummy) {
   if (DEBUG && debugflag) {
       console("sentinel(): called\n");
   }
   while (1) {
       check_deadlock();
       waitint();
   }
} /* sentinel */

/* check to determine if deadlock has occurred... */
static void check_deadlock() {
    int procReady = 0;
    int procActive = 0;
    int i;

    for (i = 0; i < MAXPROC; i++) {
        if (ProcTable[i].status == READY) {
            procReady++;
            procActive++;
        }

        if (ProcTable[i].status == JOIN_BLOCK || ProcTable[i].status == ZAP_BLOCK) {
            procActive++;
        }
    }

    if (procReady == 1) {
        /* No deadlock. quitting...*/
        if (procActive == 1) {
            console("All process completed.");
            halt(0);
        } else {
            console("check_deadlock(): Processes Active = %d\n", procActive);
            console("check_deadlock(): Processes still present. Halting...\n")
            halt(1);
        }
    } else {
        return;
    }
} /* check_deadlock */

/*
 * Disables the interrupts.
 */
void disableInterrupts() {
   /* turn the interrupts OFF iff we are in kernel mode */
   if ((PSR_CURRENT_MODE & psr_get()) == 0) {
       //not in kernel mode
       console("Kernel Error: Not in kernel mode, may not disable interrupts\n");
       halt(1);
   } else
       /* We ARE in kernel mode */
       psr_set(psr_get() & ~PSR_CURRENT_INT);
} /* disableInterrupts */

void enableInterrupts() {
    /* turn the interrupts ON iff we are in kernel mode */
    if ((PSR_CURRENT_MODE & psr_get()) == 0) {
        //not in kernel mode
        console("Kernel Error: Not in kernel mode, may not disable interrupts\n");
        halt(1);
    } else
        /* We ARE in kernel mode */
        psr_set(psr_get() | PSR_CURRENT_INT);
} /* enableInterrupts */

//Function to check in kernel mode.
void checkKernelMode(char *name) {
    if ((PSR_CURRENT_MODE & PsrGet()) == 0) {
        console("%s: called while in user mode, by process %d. Halting...\n", name, Current->pid);
        halt(1);
        return 0;
    } else {
        return 1;
    }
} /* checkKernelMode */

/* ------------------------------------------------------------------------
        Name - getpid
        Purpose - Get the current process pid
        Parameters - none
        Returns - current process pid
        Side Effects -  none
----------------------------------------------------------------------- */
int getpid() {
    return Current->pid;
} // getpid()

/* ------------------------------------------------------------------------
        Name - blockMe
        Purpose - Block current process by changing status to new status
        Parameters - an int new status which will determine if the current process is to be blocked.
        Returns - -1 if process was zapped while blocked. 0 if block was successful.
        Side Effects -  none
----------------------------------------------------------------------- */
int blockMe(int new_status) {
    checkKernelMode("blockMe");
    if (new_status < 10) {
        console("blockMe(): New status not greater than  or equal to 10. Halting...\n");
        halt(1);
    }
    Current->status = new_status;
    removeReadyList(Current);
    dispatcher();
    if (isZapped()) {
        if (DEBUG && debugflag) {
            console("blockMe(): Process was zapped while blocked.\n");
            return -1;
        }
    }
    return 0;
} // blockMe()

/* ------------------------------------------------------------------------
        Name - unblockProc
        Purpose - Unblock process if conditions are met
        Parameters - pid of process
        Returns - -2 if unblock was unsuccessful, -1 if process was zapped, 0 if unblock was successful.
        Side Effects -  none
----------------------------------------------------------------------- */
int unblockProc(int pid) {
    checkKernelMode("unblockProc");
    if (ProcTable[pid % MAXPROC].pid == EMPTY) {
        if (DEBUG && debugflag) {
            console("unblockProc(): Unblock not possible, process does not exist.\n");
        }
        return -2;
    }
    if (pid == Current->pid){
        if (DEBUG && debugflag) {
            console("unblockProc(): Unblock not possible, process is running.\n");
        }
        return -2;
    }
    if(ProcTable[pid % MAXPROC].status <= 10) {
        if (DEBUG && debugflag) {
            console("unblockProc(): Unblock not possible, process is blocked on a status less than 11.\n");
        }
        return -2;
    }
    if(isZapped()) {
        if (DEBUG && debugflag) {
            console("unblockProc(): Unblock not possible, calling process is zapped.\n");
        }
        return -1;
    }

    // Unblock possible, will process in unblocking and insert to ReadyList.
    ProcTable[pid % MAXPROC].status = READY;
    insertReadyList(&ProcTable[pid % MAXPROC]);
    dispatcher();
    return 0;
} // unblockProc()
/* ------------------------------------------------------------------------
        Name - readCurrStartTime
        Purpose - Get start time slice of current process.
        Parameters - void
        Returns - start time slice of current process.
        Side Effects -  none
----------------------------------------------------------------------- */
int readCurrStartTime(void) {
    return Current->sliceTime;
} // readCurrStartTime()

/* ------------------------------------------------------------------------
        Name - timeSlice
        Purpose - To ensure the current process has not exceeded the alotted 80 millisecond timeslice.
        Parameters - void
        Returns - none
        Side Effects -  none
----------------------------------------------------------------------- */
void timeSlice(void) {
    int time;
    time = readTime();
    if (time >= TIMESLICE) {
        dispatcher();
    } else {
        return;
    }
} // timeSlice()

/* ------------------------------------------------------------------------
        Name - readTime
        Purpose - Get difference of current USLOSS time and current process start time slice.
        Parameters - void
        Returns - time difference.
        Side Effects -  none
----------------------------------------------------------------------- */
int readTime(void) {
    int currTime, startTime, time;
    startTime = readCurrStartTime();
    currTime = sys_clock();
    time = currTime - startTime;
    return time;
} // readTime()

void clockHandler(){
    if (DEBUG && debugflag) {
        console("clockHandler(): called.\n");
    }
    timeSlice();
} // clockHandler()

void insertReadyList(proc_ptr toInsert) {
    if (ReadyList == NULL) {
        ReadyList = toInsert;
    } else if (toInsert->priority < ReadyList->priority) {
        toInsert->next_proc_ptr = ReadyList;
        ReadyList = toInsert;
    } else {
        proc_ptr prev = NULL;
        proc_ptr curr;
        for (curr = ReadyList; curr != NULL; curr = curr->next_proc_ptr) {
            if (curr->priority > toInsert->priority) {
                prev->next_proc_ptr = toInsert;
                toInsert->next_proc_ptr = current;
                break;
            }
            prev = curr;
        }
    }
    if (DEBUG && debugflag) {
        console("insertReadyList(): %s inserted to ReadyList.\n", toInsert->name);
        console("insertReadyList(): %s is front of the list.\n", ReadyList->name);
    }
} // insertReadyList()

void removeReadyList(proc_ptr toRemove) {
    proc_ptr curr;
    proc_ptr prev = NULL;
    if (ReadyList == toRemove) {
        ReadyList = toRemove->next_proc_ptr;
        if (DEBUG && debugflag) {
            console("removeReadyList(): %s removed from ReadyList.\n", toRemove->name_;)
        }
    }
    for (curr = ReadyList; curr != NULL; curr->next_proc_ptr) {
        if (curr->pid == to toRemove->pid) {
            prev->next_proc_ptr = toRemove->next_proc_ptr;
            if (DEBUG && debugflag) {
                console("removeReadyList(): %s removed from ReadyList.\n", toRemove->name_;)
            }
            break;
        }
        prev = curr;
    }
} // removeReadyList()
/*
    Print process information to the console.

    Each PCB in the process table its
      PID, parent's PID, priority, process status,
        # of children, CPU time consumed, and name.
*/
void dump_processes(void)
{
  return 0;
} // dump_processes()
