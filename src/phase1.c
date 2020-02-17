/* ------------------------------------------------------------------------

   phase1.c
   CSCV 452

      All f(x) have to be declared at the kernel mode.

      Call halt(1) if the f(x) is executed in user mode to terminat the simulator.

      Use macro name in the header files.

      Don't use free() under the linux coding box.

      p1_f(x) for now are dummy f(x).
      p1_ need to be in the source code since they are part of the phase.

      f(x) not beeing implemented by me

      int block_me(int new_status){}
      int unblock_proc(int pid){}
      int read_cur_start_time(void){}
      void time_slice(void){}
      int readtime(void){}

   ------------------------------------------------------------------------ */

#include <stdlib.h>
#include <strings.h>
#include <stdio.h>
#include <phase1.h>
#include <usloss.h>
#include "kernel.h"

/* ------------------------- Prototypes ----------------------------------- */

int sentinel(void *);
extern int start1(char *);
void dispatcher(void);
void launch();
static void enableInterrupts();
static void check_deadlock();

/* -------------------------- Globals ------------------------------------- */

/* Patrick's debugging global variable... */
int debugflag = 1;

/* the process table */
proc_struct ProcTable[MAXPROC];

/* Process lists  */
static procPtr ReadyList;

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

 void startup()
 {

     //Will first check if in kernel mode. Will halt if in user mode.
     checkkernelmode("startup()");

     int i;
     int result; /* value returned by call to fork1() */

    /* Initialize the process table */
     if (DEBUG && debugflag)
     {
         console("startup(): initializing the process table\n");
     }

     for(i = 0; i < MAXPROC; i++)
     {
         ProcTable[i].next_proc_ptr = NULL;
         ProcTable[i].child_proc_ptr = NULL;
         ProcTable[i].next_sibling_ptr = NULL;
         ProcTable[i].quit_kids_ptr = NULL;
         ProcTable[i].zapper_ptr = NULL;
         ProcTable[i].slot_number = i;
         ProcTable[i].name[0] = '\0';
         ProcTable[i].start_arg[0] = '0';
         ProcTable[i].pid = -1;
         ProcTable[i].priority = -1;
         ProcTable[i].start_func = NULL;
         ProcTable[i].stack = NULL;

     }

     numOfProcess = 0;
     current = &ProcTable[MAXPROC-1];

    /* Initialize the Ready list, etc. */
    if (DEBUG && debugflag)
       console("startup(): initializing the Ready & Blocked lists\n");
     for (i = 0; i < SENTINELPRIORITY; i++)
     {
         //initprocqueue(&ReadyList[i], READYLIST);
     }

    /* Initialize the clock interrupt handler */

    /* startup a sentinel process */
    if (DEBUG && debugflag)
        console("startup(): calling fork1() for sentinel\n");
    result = fork1("sentinel", sentinel, NULL, USLOSS_MIN_STACK,
                    SENTINELPRIORITY);
    if (result < 0)
    {
       if (DEBUG && debugflag)
          console("startup(): fork1 of sentinel returned error, halting...\n");
          halt(1);
    }

    /* start the test process */
    if (DEBUG && debugflag)
       console("startup(): calling fork1() for start1\n");
    result = fork1("start1", start1, NULL, 2 * USLOSS_MIN_STACK, 1);
    if (result < 0)
    {
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
     //will first check if in kernel mode. Will halt if in user mode.
     checkkernelmode("finish()");

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
     ProcTable[proc_slot].stackSize = stacksize;


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

 } /* fork1 */

/* ------------------------------------------------------------------------
   Name - launch
   Purpose - Dummy function to enable interrupts and launch a given process
             upon startup.
   Parameters - none
   Returns - nothing
   Side Effects - enable interrupts

   // launch() does not need any more work.

   ------------------------------------------------------------------------ */

void launch()
{

    int result;
    if (DEBUG && debugflag)
        console("launch(): started\n");
    /* Enable interrupts */
    enableInterrupts();
    /* Call the function passed to fork1, and capture its return value */
    result = Current->start_func(Current->start_arg);
    if (DEBUG && debugflag)
        console("Process %d returned to launch\n", Current->pid);
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
 // My work that I have to do Hassan.
 int join(int *code)       // WTF void join(int status)
 {
   // *code stored the quit status of the child.
   *code = kid_ptr->exit_code;
   return kid_pid;

   // Has to be syncronices with the childrens and the parents.

   if (/*process does not have any children*/) return -2;

 } /* join */

 void zap()    // wtf int zap(int pid)
 {
     // If a process tries to zap itself or attempt to zap a non-existent process call halt(1).
     // return values      -1 the calling process itself was zapped while in zap
     // 0 the zapped process has called quit.
 }

 // A zombie is a process that has been already terminated/quit(). This need to not be reused.
 // Try to use one loop to take care of the zombie.

 int is_zapped(void)
 {
   if()
   /*


     0 the process has not been zapped.
     1 the process has been zapped.
   */

 }

/* ------------------------------------------------------------------------
   Name - quit
   Purpose - Stops the child process and notifies the parent of the death by
             putting child quit info on the parents child completion code
             list.
   Parameters - the code to return to the grieving parent
   Returns - nothing
   Side Effects - changes the parent of pid child completion status list.
   ------------------------------------------------------------------------ */

 void quit(int code)
 {
   // Treminates the process that is calling this f(x).
   // Exeption handeling for the cases if a process has shildrens you cannon quit().
   // If it is zombie children is okay to quit().
   // Then if there is a print error call halt(1).

   // quit(3) terminated running process

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

  Hassan Notes:

  Need to find the zap f(x) to be implemented or call.
  Need to take care of this thing Hassan.

  Is the scheduled who pick that process to run from the ready quoe. fork1 call dispatcher

  When a particular process is blocked join a child and still all the other child are ative then quit state.
  Then call dispatcher to handle the ready quee.

  order of f(x) that need to be call: fork1(), join(), zap(), quit()
  time-sliced is the time to call the dispatcher by the clock.

  PCB = process control block.

  TestCase 00
  sentinel has a priority of 6.
  start1 has a priority of 1.
  dispatcher run next.

  zap(2) is zapping a PID that is 2 at start1.
  zap(1) is zapper that blocks the zapper.

  zap    Is no block but the zap process is block.
  block

  No direct kill it needs to be zap first.

  ----------------------------------------------------------------------- */

void dispatcher(void)
{
    // f(x) to help insert a pointer to PCB to your ready queue.
    // need to have context_switch is the only way for a new process to run.

    extern void context_switch(context *old, context *new);
    proc_ptr next_process;
    p1_switch(Current->pid, next_process->pid);

    // Dispatcher thread.
    while(TRUE)
    {
      get_next_request(&buf);
      handoff_work(&buf);
    }

    // Worker thread.
    while(TRUE)
    {
      wait_for_work(&buf);
      look_for_page_in_cache(&buf, &page);
      if(page_not_in_cache(&page))
      {
        read_page_from_disk(&buf, &page);
        return _page(&page);
      }
    }
} /* dispatcher */

static void insertRL(proc_ptr proc)
{
  proc_ptr walker, previous;                // Calling pointers for PCB.
  previous = NULL;
  walker = ReadyList;
  while(walker != NULL && walker->priority <= proc->priority)
  {
    previous = walker;
    walker = walker->next_proc_ptr;
  }
  if(previous == NULL)
  {
    proc->next_proc_ptr = ReadyList;
    ReadyList = proc;
  }
  else
  {
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

int sentinel(void *dummy)
{
    if (DEBUG && debugflag)
    {
        console("sentinel(): called\n");
    }
    while (1)
    {
        check_deadlock();
        waitint();
    }
} /* sentinel */

/* check to determine if deadlock has occurred... */
static void check_deadlock()
{
    int procReady = 0;
    int procActive = 0;
    int i;

    for (i = 0; i < MAXPROC; i++)
    {
        if (ProcTable[i].status == READY)
        {
            procReady++;
            procActive++;
        }
        if (ProcTable[i].status == JOIN_BLOCK || ProcTable[i].status == ZAP_BLOCK) { procActive++; }
    }

    if (procReady == 1)
    {
        /* No deadlock. quitting...*/
        if (procActive == 1)
        {
            console("All process completed.");
            halt(0);
        }
        else
        {
            console("check_deadlock(): Processes Active = %d\n", procActive);
            console("check_deadlock(): Processes still present. Halting...\n")
            halt(1);
        }
    }
    else
    {
        return;
    }
} /* check_deadlock */

/*
 * Disables the interrupts.
 */
void disableInterrupts()
{
    /* turn the interrupts OFF if we are in kernel mode */
    if ((PSR_CURRENT_MODE & psr_get()) == 0)
    {
        // not in kernel mode
        console("Kernel Error: Not in kernel mode, may not disable interrupts\n");
        halt(1);
    } else
        /* We ARE in kernel mode */
        psr_set(psr_get() & ~PSR_CURRENT_INT);
} /* disableInterrupts */



//Function to check in kernel mode.
void checkKernelMode(char *name)
{
    if ((PSR_CURRENT_MODE & PsrGet()) == 0)
    {
        console("%s: called while in user mode, by process %d. Halting...\n", name, Current->pid);
        halt(1);
        return 0;
    } else
    {
        return 1;
    }
}
