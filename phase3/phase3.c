/* ------------------------------------------------------------------------
   phase3.c

   University of Arizona South
   Computer Science 452
   @authors: Javier Felix, Mark Festejo, Hassan Martinez
   ------------------------------------------------------------------------ */

#include <usloss.h>
#include <phase1.h>
#include <phase2.h>
#include <phase3.h>
#include <libuser.h>
#include <sems.h>
#include <usyscall.h>
#include <string.h>

/*-------------------- Prototypes ------------------------*/
extern int start3();

void check_kernel_mode(char *);
void nullsys3(sysargs *);
void spawn(sysargs *);
void wait(sysargs *);
void terminate(sysargs *);
void sem_create(sysargs *);
int sem_create_real(int);
void semp(sysargs *);
void semp_real(int);
void semv(sysargs *);
void semv_real(int);
void sem_free(sysargs *);
int sem_free_real(int);
void gettimeofday(sysargs *);
void cputime(sysargs *);
void getPID(sysargs *);

int spawn_real(char*, int(*)(char *), char *, int ,int);
int spawn_launch(char *);
int wait_real(int *);
void terminate_real(int);
void empty_proc3(int);
void init_proc3(int);
void init_user_mode();
void init_proc_queue3(proc_queue *, int);
void enqueue3(proc_queue *, proc_ptr3);
proc_ptr3 dequeue3(proc_queue *);
proc_ptr3 peek3(proc_queue *);
void remove_child3(proc_queue *, proc_ptr3);
void enqueue_blocked_proc(proc_ptr3 *, proc_ptr3);
proc_ptr3 dequeue_blocked_proc(proc_ptr3);

/*--------------------- Globals --------------------------*/
int debugflag3 = 0;
semaphore SemTable[MAXSEMS];
int num_sems;
proc_struct3 ProcTable3[MAXPROC];


int start2(char *arg)
{
    int		pid;
    int		status;
    /*
     * Check kernel mode here.
     */
    check_kernel_mode("start2()");

    /*
     * Data structure initialization as needed...
     */

    // initializing every system call handler as nullsys3
    int i;
    for (i = 0; i < MAXSYSCALLS; i++) {
        sys_vec[i] = nullsys3;
    }
    sys_vec[SYS_SPAWN] = spawn;
    sys_vec[SYS_WAIT] = wait;
    sys_vec[SYS_TERMINATE] = terminate;
    sys_vec[SYS_SEMCREATE] = sem_create;
    sys_vec[SYS_SEMP] = semp;
    sys_vec[SYS_SEMV] = semv;
    sys_vec[SYS_SEMFREE] = sem_free;
    sys_vec[SYS_GETTIMEOFDAY] = gettimeofday;
    sys_vec[SYS_CPUTIME] = cputime;
    sys_vec[SYS_GETPID] = getPID;

    // initialize proc table
    for (i = 0; i < MAXSEMS; i++) {
        empty_proc3(i);
    }

    // initialize semaphore table
    for (i = 0; i < MAXSEMS; i++) {
        SemTable[i].id = -1;
        SemTable[i].value = -1;
        SemTable[i].starting_value = -1;
        SemTable[i].priv_mbox_id = -1;
        SemTable[i].mutex_mbox_id = -1;
    }
    num_sems = 0;

    /*
     * Create first user-level process and wait for it to finish.
     * These are lower-case because they are not system calls;
     * system calls cannot be invoked from kernel mode.
     * Assumes kernel-mode versions of the system calls
     * with lower-case names.  I.e., Spawn is the user-mode function
     * called by the test cases; spawn is the kernel-mode function that
     * is called by the syscall_handler; spawn_real is the function that
     * contains the implementation and is called by spawn.
     *
     * Spawn() is in libuser.c.  It invokes usyscall()
     * The system call handler calls a function named spawn() -- note lower
     * case -- that extracts the arguments from the sysargs pointer, and
     * checks them for possible errors.  This function then calls spawn_real().
     *
     * Here, we only call spawn_real(), since we are already in kernel mode.
     *
     * spawn_real() will create the process by using a call to fork1 to
     * create a process executing the code in spawn_launch().  spawn_real()
     * and spawn_launch() then coordinate the completion of the phase 3
     * process table entries needed for the new process.  spawn_real() will
     * return to the original caller of Spawn, while spawn_launch() will
     * begin executing the function passed to Spawn. spawn_launch() will
     * need to switch to user-mode before allowing user code to execute.
     * spawn_real() will return to spawn(), which will put the return
     * values back into the sysargs pointer, switch to user-mode, and
     * return to the user code that called Spawn.
     */
    if (debugflag3) {
        console("Spawning start3...\n");
    }
    pid = spawn_real("start3", start3, NULL, 4*USLOSS_MIN_STACK, 3);
    pid = wait_real(&status);
    if (debugflag3) {
        console("Quitting start2...\n");
    }
    quit(pid);
    return -1;

} /* start2 */


/*
 *
 *
 */
void spawn(sysargs *args_ptr) {
    check_kernel_mode("spawn()");

    int (*func)(char *);
    char *arg = args_ptr->arg2;
    int stack_size = (int)(args_ptr->arg3);
    int priority = (int)(args_ptr->arg4);
    char *name = (char *) (args_ptr->arg5);

    if (debugflag3) {
        console("spawn(): args are: name: %s, stack size: %d, priority: %d\n", name, stack_size, priority);
    }

    int pid = spawn_real(name, func, arg, stack_size, priority);
    int status = 0;

    if (debugflag3) {
        console("spawn(): spawn pid: %d\n", pid);
    }

    // terminates self if zapped
    if (is_zapped()) {
        terminate_real(1);
    } else {
        init_user_mode();
    }

    args_ptr->arg1 = (void *)(pid);
    args_ptr->arg4 = (void *)(status);

}

int spawn_real(char *name, int (*func)(char *), char *arg, int stack_size, int priority) {
    check_kernel_mode("spawn_real()");

    if (debugflag3) {
        console("spawn_real(): forking process %s...\n", name);
    }

    // fork process and get pid
    int pid = fork1(name, spawn_launch, arg, stack_size, priority);

    if (debugflag3) {
        console("spawn_real(): forked process name: %s, pid: %d\n", name, pid);
    }

    // return -1 if forked failed
    if (pid < 0) {
        return -1;
    }

    // pid obtained, now we can get child table entry
    proc_ptr3 child = &ProcTable3[pid % MAXPROC];
    enqueue3(&ProcTable3[getpid() % MAXPROC].children_queue, child); // add to children queue

    // if spawn_launch hasn't set up proc table entry
    if (child->pid < 0) {
        if (debugflag3) {
            console("spawn_real(): initializing proc table entry for pid %d\n", pid);
        }
        init_proc3(pid);
    }

    child->start_func = func; // give proc starting function
    child->parent_ptr = &ProcTable3[getpid() % MAXPROC]; // set child parent pointer

    // unblock process so spawn_launch can start it
    MboxCondSend(child->mbox_id, 0, 0);
    return pid;
}

int spawn_launch(char *start_arg){
    check_kernel_mode("spawn_launch()");

    if (debugflag3) {
        console("spawn_launch(): launched pid = %d\n", getpid());
    }

    // terminate self if zapped
    if (is_zapped()){
        terminate_real(1);
    }

    // get proc info
    proc_ptr3 proc = &ProcTable3[getpid() % MAXPROC];

    // set up proc table entry if spawn_real has not
    if (proc->pid < 0) {
        if (debugflag3) {
            console("spawn_launch(): initializing proc table entry for pid %d\n", getpid());
        }
        init_proc3(getpid());
        MboxReceive(proc->mbox_id, 0, 0);
    }

    // switch to user mode
    init_user_mode();

    if (debugflag3) {
        console("spawn_launch(): staritng process %d...\n", proc->pid);
    }

    // call function to start process
    int status = proc->start_func(start_arg);

    if (debugflag3) {
        console("spawn_launch(): terminating process %d with status %d\n", proc->pid, status);
    }
    Terminate(status); // terminate process if it hasn't terminated itself
    return 0;
}

void wait(sysargs *args_ptr) {
    check_kernel_mode("wait()");

    int *status = args_ptr->arg2;
    int pid = wait_real(status);

    if (debugflag3) {
        console("wait(): joined with child pid: %d, stauts: %d\n", pid, *status);
    }

    args_ptr->arg1 = (void *) ((long) pid);
    args_ptr->arg2 = (void *) ((long) *status);
    args_ptr->arg4 = (void *) ((long) 0);

    // terminate self if zapped, else switch to user mode
    if (is_zapped()) {
        terminate_real(1);
    } else {
        init_user_mode();
    }
}

int wait_real(int *status) {
    check_kernel_mode("wait_real()");

    if (debugflag3) {
        console("wait_real(): reached wait_real\n");
        int pid = join(status);
        return pid;
    }
}

void terminate(sysargs *args_ptr) {
    check_kernel_mode("terminate()");

    int status = (int) ((long) args_ptr->arg1);
    terminate_real(status);

    // switch to user mode
    init_user_mode();
}

void terminate_real(int status) {
    check_kernel_mode("terminate_real()");

    if (debugflag3) {
        console("terminate_real(): terminated pid: %d, stauts: %d\n", getpid(), status);
    }

    // zap all children
    proc_ptr3 proc = &ProcTable3[getpid() % MAXPROC];
    while (proc->children_queue.size > 0) {
        proc_ptr3 child = dequeue3(&proc->children_queue);
        zap(child->pid);
    }

    // remove self from parent list of children
    remove_child3(&(proc->parent_ptr->children_queue), proc);
    quit(status);
}

void sem_create(sysargs *args_ptr) {
    check_kernel_mode("sem_create()");

    int value = (long) args_ptr->arg1;

    // if value is < 0 or no available semaphore, fails
    if (value < 0 || num_sems == MAXSEMS) {
        args_ptr->arg5 = (void *) (long) -1;
    } else {
        num_sems++;
        int handle = sem_create_real(value);
        args_ptr->arg1 = (void *) (long) handle;
        args_ptr->arg4 = 0;
    }

    // terminates self if zapped
    if (is_zapped()) {
        terminate_real(0);
    } else {
        init_user_mode();
    }
}

int sem_create_real(int value) {
    check_kernel_mode("sem_create_real()");

    int i;
    int priv_mbox_id = MboxCreate(value, 0);
    int mutex_mbox_id = MboxCreate(1, 0);

    MboxSend(mutex_mbox_id, NULL, 0);

    for (i = 0; i < MAXSEMS; i++) {
        if (SemTable[i].id == -1) {
            SemTable[i].id = i;
            SemTable[i].value = value;
            SemTable[i].starting_value = value;
            SemTable[i].priv_mbox_id = priv_mbox_id;
            SemTable[i].mutex_mbox_id = mutex_mbox_id;
            init_proc_queue3(&SemTable[i].blocked_proc, BLOCKED);
            break;
        }
    }
    for (i = 0; i < value; i++) {
        MboxSend(priv_mbox_id, NULL, 0);
    }

    MboxReceive(mutex_mbox_id, NULL, 0);
    return SemTable[i].id;
}

void semp(sysargs *args_ptr) {
    check_kernel_mode("semp()");

    int handle = (long) args_ptr->arg1;

    if (handle < 0) {
        args_ptr->arg4 = (void *) (long) -1;
    } else {
        args_ptr->arg4 = 0;
        semp_real(handle);
    }

    // terminate self if zapped, else switch to user mode
    if (is_zapped()) {
        terminate_real(0);
    } else {
        init_user_mode();
    }
}

void semp_real(int handle) {
    check_kernel_mode("semp_real()");

    // get mutex on current semaphore
    MboxSend(SemTable[handle].mutex_mbox_id, NULL, 0);

    // block if value 0
    if (SemTable[handle].value == 0) {
        enqueue3(&SemTable[handle].blocked_proc, &ProcTable3[getpid() % MAXPROC]);
        MboxReceive(SemTable[handle].priv_mbox_id, NULL, 0);

        int result = MboxReceive(SemTable[handle].priv_mbox_id, NULL, 0);

        // terminate if semaphore freed while blocked
        if (SemTable[handle].id < 0) {
            terminate_real(1);

            // get mutex when unblocked
            MboxSend(SemTable[handle].mutex_mbox_id, NULL, 0);
            if (result < 0) {
                console("semp_real(): bad receive");
            }
        }

    } else {
        SemTable[handle].value -= 1;
        int result = MboxReceive(SemTable[handle].priv_mbox_id, NULL, 0);
        if (result < 0) {
            console("semp_real(): bad receive");
        }
        MboxSend(SemTable[handle].mutex_mbox_id, NULL, 0);
    }

}

void semv(sysargs *args_ptr) {
    check_kernel_mode("semv()");

    int handle = (long) args_ptr->arg1;

    if (handle < 0) {
        args_ptr->arg4 = (void*) (long) -1;
    } else {
        args_ptr->arg4 = 0;
    }

    semv_real(handle);

    // terminate self if zapped, else switch to user mode
    if (is_zapped()) {
        terminate_real(0);
    } else {
        init_user_mode();
    }
}

void semv_real(int handle) {
    check_kernel_mode("semv_real()");

    MboxSend(SemTable[handle].mutex_mbox_id, NULL, 0);

    // unblock proc
    if (SemTable[handle].blocked_proc.size > 0) {
        dequeue3(&SemTable[handle].blocked_proc);

        // receive on mutex so semp can send after receive receive on priv_mbox
        MboxReceive(SemTable[handle].mutex_mbox_id, NULL, 0);

        MboxSend(SemTable[handle].priv_mbox_id, NULL, 0);
        MboxSend(SemTable[handle].mutex_mbox_id, NULL, 0);
    } else {
        SemTable[handle].value += 1;
    }

    MboxReceive(SemTable[handle].mutex_mbox_id, NULL, 0);
}

void sem_free(sysargs *args_ptr) {
    check_kernel_mode("sem_free()");

    int handle = (long) args_ptr->arg1;

    if (handle < 0) {
        args_ptr->arg4 = (void*) (long) -1;
    } else {
        args_ptr->arg4 = 0;
        int value = sem_free_real(handle);
        args_ptr->arg4 = (void*)(long) value;
    }

    // terminate self if zapped, else switch to user mode
    if (is_zapped()) {
        terminate_real(0);
    } else {
        init_user_mode();
    }
}

int sem_free_real(int handle) {
    check_kernel_mode("sem_free_real()");

    int mutex_id = SemTable[handle].mutex_mbox_id;
    MboxSend(mutex_id, NULL, 0);

    int priv_id = SemTable[handle].priv_mbox_id;

    SemTable[handle].id = -1;
    SemTable[handle].value = -1;
    SemTable[handle].starting_value = -1;
    SemTable[handle].priv_mbox_id = -1;
    SemTable[handle].mutex_mbox_id = -1;
    num_sems--;

    // terminate processes waiting on current semaphore
    if (SemTable[handle].blocked_proc.size > 0) {
        while (SemTable[handle].blocked_proc.size > 0) {
            dequeue3(&SemTable[handle].blocked_proc);
            int result = MboxSend(priv_id, NULL, 0);
            if (result < 0) {
                console("sem_free_real(): error on send!");
            }
        }
        MboxReceive(mutex_id, NULL, 0);
        return -1;
    } else {
        MboxReceive(mutex_id, NULL, 0);
        return 0;
    }
}

void gettimeofday(sysargs *args_ptr) {
    check_kernel_mode("gettimeofday()");
    *((int*)(args_ptr->arg1)) = sys_clock();
}

void cputime(sysargs *args_ptr) {
    check_kernel_mode("cputime()");
    *((int*)(args_ptr->arg1)) = readtime();
}

void getPID(sysargs *args_ptr) {
    check_kernel_mode("getPID()");

    *((int*)(args_ptr->arg1)) = getpid();

}

void nullsys3(sysargs *args_ptr) {
    console("nullsys3(): invalid syscall %d. Terminating...\n", args_ptr->number);
    terminate_real(1);
}

void init_proc(int pid) {
    check_kernel_mode("init_proc()");

    int i = pid % MAXPROC;

    ProcTable3[i].pid = pid;
    ProcTable3[i].mbox_id = MboxCreate(0, 0);
    ProcTable3[i].start_func = NULL;
    ProcTable3[i].next_proc_ptr = NULL;
    init_proc_queue3(&ProcTable3->children_queue, CHILDREN);
}

void empty_proc3(int pid) {
    check_kernel_mode("empty_proc3()");

    int i = pid % MAXPROC;

    ProcTable3[i].pid = pid;
    ProcTable3[i].mbox_id = -1;
    ProcTable3[i].start_func = NULL;
    ProcTable3[i].next_proc_ptr = NULL;
}

//Function to check in kernel mode.
void check_kernel_mode(char *name) {
    if ((PSR_CURRENT_MODE & psr_get()) == 0) {
        console("%s: called while in user mode, by process %d. Halting...\n", name, getpid());
        halt(1);
    }
} /* checkKernelMode */

// Function to switch to user mode
void init_user_mode(){
    psr_set(psr_get() & ~PSR_CURRENT_MODE);
}

void enqueue3(proc_queue *q, proc_ptr3 p){
    if (q->head == NULL && q->tail == NULL) {
        q->head = q->tail = p;
    } else {
        if (q->type == BLOCKED) {
            q->tail->next_proc_ptr = p;
        } else if (q->type == CHILDREN) {
            q->tail->next_sibling_ptr = p;
        }
        q->tail = p;
    }
    q->size++;
}

proc_ptr3 dequeue3(proc_queue *q) {
    proc_ptr3 curr = q->head;
    if (q->head == NULL) {
        return NULL;
    }
    if (q->head == q->tail) {
        q->head = q-> tail = NULL;
    } else {
        if (q->type == BLOCKED) {
            q->head = q->head->next_proc_ptr;
        } else if (q->type == CHILDREN) {
            q->head = q->head->next_sibling_ptr;
        }
    }
    q->size--;
    return curr;
}

void remove_child3(proc_queue *q, proc_ptr3 child) {
    if (q->head == NULL || q->type != CHILDREN) {
        return;
    }

    if (q->head == child) {
        dequeue3(q);
        return;
    }

    proc_ptr3 prev = q->head;
    proc_ptr3 p = q->head->next_sibling_ptr;

    while (p != NULL) {
        if (p == child) {
            if (p == q->tail) {
                q->tail = prev;
            } else {
                prev->next_sibling_ptr = p->next_sibling_ptr->next_sibling_ptr;
            }
            q->size--;
        }
        prev = p;
        p = p->next_sibling_ptr;
    }
}

proc_ptr3 peek3(proc_queue *q) {
    if (q->head == NULL) {
        return NULL;
    }
    return q->head;
}