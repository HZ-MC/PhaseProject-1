/* Programmers: Javier Felix, Mark Festejo, Hassan Martinez
 * Project Name: Phase 4
 * Date: 05/04/2020
 */

#include <usloss.h>
#include <phase1.h>
#include <phase2.h>
#include <phase3.h>
#include <phase4.h>
#include <provided_prototypes.h>
#include <p4structs.h>
#include <usyscall.h>
#include <stdlib.h> /* needed for atoi() */
#include <stdio.h>
#include <string.h> /* needed for memcpy() */

#define ABS(a, b) (a-b > 0 ? a-b : -(a-b))

int debug4 = 0;
int running;

static int ClockDriver(char *);

static int DiskDriver(char *);

static int TermDriver(char *);

static int TermReader(char *);

static int TermWriter(char *);

extern int start4();

void sleep(sysargs *);

void diskRead(sysargs *);

void diskWrite(sysargs *);

void diskSize(sysargs *);

void termRead(sysargs *);

void termWrite(sysargs *);

int sleep_real(int);

int disk_size_real(int, int *, int *, int *);

int disk_write_real(int, int, int, int, void *);

int disk_read_real(int, int, int, int, void *);

int disk_read_or_write_real(int, int, int, int, void *, int);

int term_read_real(int, int, char *);

int term_write_real(int, int, char *);

void check_kernel_mode(char *);

void empty_proc(int);

void init_proc(int);

void set_user_mode();

void init_disk_queue(disk_queue *);

void add_disk_queue(disk_queue *, proc_ptr);

proc_ptr peek_disk_queue(disk_queue *);

proc_ptr remove_disk_queue(disk_queue *);

void init_heap(heap *);

void heap_add(heap *, proc_ptr);

proc_ptr heap_peek(heap *);

proc_ptr heap_remove(heap *);

/* Globals */
proc_struct ProcTable[MAXPROC];
heap sleep_heap;
int disk_zapped; // indicates if the disk drivers are 'zapped' or not
disk_queue disk_queues[DISK_UNITS]; // queues for disk drivers
int diskpids[DISK_UNITS]; // pids of the disk drivers

// mailboxes for terminal device
int char_recv_mbox[TERM_UNITS]; // receive char
int char_send_mbox[TERM_UNITS]; // send char
int line_read_mbox[TERM_UNITS]; // read line
int line_write_mbox[TERM_UNITS]; // write line
int pid_mbox[TERM_UNITS]; // pid to block
int term_int[TERM_UNITS]; // interupt for term (control writing)

int termProcTable[TERM_UNITS][3]; // keep track of term procs


void start3(void) {
    char name[128];
    char termbuf[10];
    char diskbuf[10];
    int i;
    int clockPID;
    int pid;
    int status;

    /*
     * Check kernel mode here.
     */
    check_kernel_mode("start3");

    // initialize proc table
    for (i = 0; i < MAXPROC; i++) {
        init_proc(i);
    }

    // sleep queue
    init_heap(&sleep_heap);

    // initialize sys_vec
    sys_vec[SYS_SLEEP] = sleep;
    sys_vec[SYS_DISKREAD] = diskRead;
    sys_vec[SYS_DISKWRITE] = diskWrite;
    sys_vec[SYS_DISKSIZE] = diskSize;
    sys_vec[SYS_TERMREAD] = termRead;
    sys_vec[SYS_TERMWRITE] = termWrite;

    // mboxes for terminal
    for (i = 0; i < TERM_UNITS; i++) {
        char_recv_mbox[i] = MboxCreate(1, MAXLINE);
        char_send_mbox[i] = MboxCreate(1, MAXLINE);
        line_read_mbox[i] = MboxCreate(10, MAXLINE);
        line_write_mbox[i] = MboxCreate(10, MAXLINE);
        pid_mbox[i] = MboxCreate(1, sizeof(int));
    }

    /*
     * Create clock device driver 
     * I am assuming a semaphore here for coordination.  A mailbox can
     * be used instead -- your choice.
     */
    running = semcreate_real(0);
    clockPID = fork1("Clock driver", ClockDriver, NULL, USLOSS_MIN_STACK, 2);
    if (clockPID < 0) {
        console("start3(): Can't create clock driver\n");
        halt(1);
    }
    /*
     * Wait for the clock driver to start. The idea is that ClockDriver
     * will V the semaphore "running" once it is running.
     */

    semp_real(running);

    /*
     * Create the disk device drivers here.  You may need to increase
     * the stack size depending on the complexity of your
     * driver, and perhaps do something with the pid returned.
     */
    int temp;
    for (i = 0; i < DISK_UNITS; i++) {
        sprintf(diskbuf, "%d", i);
        pid = fork1("Disk driver", DiskDriver, diskbuf, USLOSS_MIN_STACK, 2);
        if (pid < 0) {
            console("start3(): Can't create disk driver %d\n", i);
            halt(1);
        }

        diskpids[i] = pid;
        semp_real(running); // wait for driver to start running

        // get number of tracks
        disk_size_real(i, &temp, &temp, &ProcTable[pid % MAXPROC].disk_track);
    }


    /*
     * Create terminal device drivers.
     */

    for (i = 0; i < TERM_UNITS; i++) {
        sprintf(termbuf, "%d", i);
        termProcTable[i][0] = fork1(name, TermDriver, termbuf, USLOSS_MIN_STACK, 2);
        termProcTable[i][1] = fork1(name, TermReader, termbuf, USLOSS_MIN_STACK, 2);
        termProcTable[i][2] = fork1(name, TermWriter, termbuf, USLOSS_MIN_STACK, 2);
        semp_real(running);
        semp_real(running);
        semp_real(running);
    }


    /*
     * Create first user-level process and wait for it to finish.
     * These are lower-case because they are not system calls;
     * system calls cannot be invoked from kernel mode.
     * I'm assuming kernel-mode versions of the system calls
     * with lower-case first letters, as shown in provided_prototypes.h
     */
    pid = spawn_real("start4", start4, NULL, 4 * USLOSS_MIN_STACK, 3);
    pid = wait_real(&status);

    /*
     * Zap the device drivers
     */

    status = 0;

    // zap clock driver
    zap(clockPID);
    join(&status);

    // zap disk drivers
    for (i = 0; i < DISK_UNITS; i++) {
        semv_real(ProcTable[diskpids[i]].block_sem);
        zap(diskpids[i]);
        join(&status);
    }

    // zap termreader
    for (i = 0; i < TERM_UNITS; i++) {
        MboxSend(char_recv_mbox[i], NULL, 0);
        zap(termProcTable[i][1]);
        join(&status);
    }

    // zap termwriter
    for (i = 0; i < TERM_UNITS; i++) {
        MboxSend(line_write_mbox[i], NULL, 0);
        zap(termProcTable[i][2]);
        join(&status);
    }

    // zap termdriver, etc
    char filename[50];
    for (i = 0; i < TERM_UNITS; i++) {
        int ctrl = 0;
        ctrl = TERM_CTRL_RECV_INT(ctrl);
        device_output(TERM_DEV, i, (void *) ((long) ctrl));

        // file stuff
        sprintf(filename, "term%d.in", i);
        FILE *f = fopen(filename, "a+");
        fprintf(f, "last line\n");
        fflush(f);
        fclose(f);

        // actual termdriver zap
        zap(termProcTable[i][0]);
        join(&status);
    }

    // eventually, at the end:
    quit(0);

}

/* Clock Driver */
static int
ClockDriver(char *arg) {
    int result;
    int status;

    // Let the parent know we are running and enable interrupts.
    semv_real(running);
    psr_set(psr_get() | PSR_CURRENT_INT);

    // Infinite loop until we are zap'd
    while (!is_zapped()) {
        result = waitdevice(CLOCK_DEV, 0, &status);
        if (result != 0) {
            return 0;
        }

        // Compute the current time and wake up any processes whose time has come.
        proc_ptr proc;
        while (sleep_heap.size > 0 && sys_clock() >= heap_peek(&sleep_heap)->wake_time) {
            proc = heap_remove(&sleep_heap);
            if (debug4)
                console("ClockDriver: Waking up process %d\n", proc->pid);
            semv_real(proc->block_sem);
        }
    }
    return 0;
}

/* Disk Driver */
static int
DiskDriver(char *arg) {
    int result;
    int status;
    int unit = atoi((char *) arg);     // Unit is passed as arg.

    // get set up in proc table
    init_proc(getpid());
    proc_ptr me = &ProcTable[getpid() % MAXPROC];
    init_disk_queue(&disk_queues[unit]);

    if (debug4) {
        console("DiskDriver: unit %d started, pid = %d\n", unit, me->pid);
    }

    // Let the parent know we are running and enable interrupts.
    semv_real(running);
    psr_set(psr_get() | PSR_CURRENT_INT);

    // Infinite loop until we are zap'd
    while (!is_zapped()) {
        // block on sem until we get request
        semp_real(me->block_sem);
        if (debug4) {
            console("DiskDriver: unit %d unblocked, zapped = %d, queue size = %d\n", unit, is_zapped(),
                    disk_queues[unit].size);
        }
        if (is_zapped()) // check  if we were zapped
            return 0;

        // get request off queue
        if (disk_queues[unit].size > 0) {
            proc_ptr proc = peek_disk_queue(&disk_queues[unit]);
            int track = proc->disk_track;

            if (debug4) {
                console("DiskDriver: taking request from pid %d, track %d\n", proc->pid, proc->disk_track);
            }

            // handle tracks request
            if (proc->disk_request.opr == DISK_TRACKS) {
                device_output(DISK_DEV, unit, &proc->disk_request);
                result = waitdevice(DISK_DEV, unit, &status);
                if (result != 0) {
                    return 0;
                }
            } else { // handle read/write requests
                while (proc->disk_sectors > 0) {
                    // seek to needed track
                    device_request request;
                    request.opr = DISK_SEEK;
                    request.reg1 = &track;
                    device_output(DISK_DEV, unit, &request);
                    // wait for result
                    result = waitdevice(DISK_DEV, unit, &status);
                    if (result != 0) {
                        return 0;
                    }

                    if (debug4) {
                        console("DiskDriver: seeked to track %d, status = %d, result = %d\n", track, status, result);
                    }

                    // read/write the sectors
                    int s;
                    for (s = proc->disk_first_sec; proc->disk_sectors > 0 && s < DISK_TRACK_SIZE; s++) {
                        proc->disk_request.reg1 = (void *) ((long) s);
                        device_output(DISK_DEV, unit, &proc->disk_request);
                        result = waitdevice(DISK_DEV, unit, &status);
                        if (result != 0) {
                            return 0;
                        }

                        if (debug4) {
                            console("DiskDriver: read/wrote sector %d, status = %d, result = %d, buffer = %s\n", s,
                                    status, result, proc->disk_request.reg2);
                        }

                        proc->disk_sectors--;
                        proc->disk_request.reg2 += DISK_SECTOR_SIZE;
                    }

                    // request first sector of next track
                    track++;
                    proc->disk_first_sec = 0;
                }
            }

            if (debug4)
                console("DiskDriver: finished request from pid %d\n", proc->pid, result, status);

            remove_disk_queue(&disk_queues[unit]); // remove proc from queue
            semv_real(proc->block_sem); // unblock caller
        }

    }

    semv_real(running); // unblock parent
    return 0;
}

/* Terminal Driver */
static int
TermDriver(char *arg) {
    int result;
    int status;
    int unit = atoi((char *) arg);     // Unit is passed as arg.

    semv_real(running);
    if (debug4)
        console("TermDriver (unit %d): running\n", unit);

    while (!is_zapped()) {

        result = waitdevice(TERM_INT, unit, &status);
        if (result != 0) {
            return 0;
        }

        // Try to receive character
        int recv = TERM_STAT_RECV(status);
        if (recv == DEV_BUSY) {
            MboxCondSend(char_recv_mbox[unit], &status, sizeof(int));
        } else if (recv == DEV_ERROR) {
            if (debug4)
                console("TermDriver RECV ERROR\n");
        }

        // Try to send character
        int xmit = TERM_STAT_XMIT(status);
        if (xmit == DEV_READY) {
            MboxCondSend(char_send_mbox[unit], &status, sizeof(int));
        } else if (xmit == DEV_ERROR) {
            if (debug4)
                console("TermDriver XMIT ERROR\n");
        }
    }

    return 0;
}

/* Terminal Reader */
static int
TermReader(char *arg) {
    int unit = atoi((char *) arg);     // Unit is passed as arg.
    int i;
    int receive; // char to receive
    char line[MAXLINE]; // line being created/read
    int next = 0; // index in line to write char

    for (i = 0; i < MAXLINE; i++) {
        line[i] = '\0';
    }

    semv_real(running);
    while (!is_zapped()) {
        // receieve characters
        MboxReceive(char_recv_mbox[unit], &receive, sizeof(int));
        char ch = TERM_STAT_CHAR(receive);
        line[next] = ch;
        next++;

        // receive line
        if (ch == '\n' || next == MAXLINE) {
            if (debug4)
                console("TermReader (unit %d): line send\n", unit);

            line[next] = '\0'; // end with null
            MboxSend(line_read_mbox[unit], line, next);

            // reset line
            for (i = 0; i < MAXLINE; i++) {
                line[i] = '\0';
            }
            next = 0;
        }


    }
    return 0;
}

/* Terminal Writer */
static int
TermWriter(char *arg) {
    int unit = atoi((char *) arg);     // Unit is passed as arg.
    int size;
    int ctrl = 0;
    int next;
    int status;
    char line[MAXLINE];

    semv_real(running);
    if (debug4) {
        console("TermWriter (unit %d): running\n", unit);
    }

    while (!is_zapped()) {
        size = MboxReceive(line_write_mbox[unit], line, MAXLINE); // get line and size

        if (is_zapped()) {
            break;
        }

        // enable xmit interrupt and receive interrupt
        ctrl = TERM_CTRL_XMIT_INT(ctrl);
        device_output(TERM_DEV, unit, (void *) ((long) ctrl));

        // xmit the line
        next = 0;
        while (next < size) {
            MboxReceive(char_send_mbox[unit], &status, sizeof(int));

            // xmit the character
            int x = TERM_STAT_XMIT(status);
            if (x == DEV_READY) {

                ctrl = 0;
                ctrl = TERM_CTRL_CHAR(ctrl, line[next]);
                ctrl = TERM_CTRL_XMIT_CHAR(ctrl);
                ctrl = TERM_CTRL_XMIT_INT(ctrl);

                device_output(TERM_DEV, unit, (void *) ((long) ctrl));
            }

            next++;
        }

        // enable receive interrupt
        ctrl = 0;
        if (term_int[unit] == 1)
            ctrl = TERM_CTRL_RECV_INT(ctrl);
        device_output(TERM_DEV, unit, (void *) ((long) ctrl));
        term_int[unit] = 0;
        int pid;
        MboxReceive(pid_mbox[unit], &pid, sizeof(int));
        semv_real(ProcTable[pid % MAXPROC].block_sem);


    }

    return 0;
}

/* sleep function value extraction */
void sleep(sysargs *args) {
    check_kernel_mode("sleep");
    int seconds = (long) args->arg1;
    int retval = sleep_real(seconds);
    args->arg4 = (void *) ((long) retval);
    set_user_mode();
}

/* real sleep function */
int sleep_real(int seconds) {
    check_kernel_mode("sleep_real");

    if (debug4)
        console("sleep_real: called for process %d with %d seconds\n", getpid(), seconds);

    if (seconds < 0) {
        return -1;
    }

    // init/get the process
    if (ProcTable[getpid() % MAXPROC].pid == -1) {
        init_proc(getpid());
    }
    proc_ptr proc = &ProcTable[getpid() % MAXPROC];

    // set wake time
    proc->wake_time = sys_clock() + seconds * 1000000;
    if (debug4) {
        console("sleep_real: set wake time for process %d to %d, adding to heap...\n", proc->pid, proc->wake_time);
    }

    heap_add(&sleep_heap, proc); // add to sleep heap
    if (debug4) {
        console("sleep_real: Process %d going to sleep until %d\n", proc->pid, proc->wake_time);
    }
    semp_real(proc->block_sem); // block the process
    if (debug4) {
        console("sleep_real: Process %d woke up, time is %d\n", proc->pid, sys_clock());
    }
    return 0;
}

/* extract values from sysargs and call disk_read_real */
void diskRead(sysargs *args) {
    check_kernel_mode("diskRead");

    int sectors = (long) args->arg2;
    int track = (long) args->arg3;
    int first = (long) args->arg4;
    int unit = (long) args->arg5;

    int retval = disk_read_real(unit, track, first, sectors, args->arg1);

    if (retval == -1) {
        args->arg1 = (void *) ((long) retval);
        args->arg4 = (void *) ((long) -1);
    } else {
        args->arg1 = (void *) ((long) retval);
        args->arg4 = (void *) ((long) 0);
    }
    set_user_mode();
}

/* extract values from sysargs and call disk_write_real */
void diskWrite(sysargs *args) {
    check_kernel_mode("diskWrite");

    int sectors = (long) args->arg2;
    int track = (long) args->arg3;
    int first = (long) args->arg4;
    int unit = (long) args->arg5;

    int retval = disk_write_real(unit, track, first, sectors, args->arg1);

    if (retval == -1) {
        args->arg1 = (void *) ((long) retval);
        args->arg4 = (void *) ((long) -1);
    } else {
        args->arg1 = (void *) ((long) retval);
        args->arg4 = (void *) ((long) 0);
    }
    set_user_mode();
}

int disk_write_real(int unit, int track, int first, int sectors, void *buffer) {
    check_kernel_mode("disk_write_real");
    return disk_read_or_write_real(unit, track, first, sectors, buffer, 1);
}

int disk_read_real(int unit, int track, int first, int sectors, void *buffer) {
    check_kernel_mode("disk_write_real");
    return disk_read_or_write_real(unit, track, first, sectors, buffer, 0);
}

/*------------------------------------------------------------------------
    disk_read_or_write_real: Reads or writes to the desk depending on the 
                        value of write; write if write == 1, else read.
    Returns: -1 if given illegal input, 0 otherwise
 ------------------------------------------------------------------------*/
int disk_read_or_write_real(int unit, int track, int first, int sectors, void *buffer, int write) {
    if (debug4)
        console("disk_read_or_write_real: called with unit: %d, track: %d, first: %d, sectors: %d, write: %d\n", unit,
                track, first, sectors, write);

    // check for illegal args
    if (unit < 0 || unit > 1 || track < 0 || track > ProcTable[diskpids[unit]].disk_track ||
        first < 0 || first > DISK_TRACK_SIZE || buffer == NULL ||
        (first + sectors) / DISK_TRACK_SIZE + track > ProcTable[diskpids[unit]].disk_track) {
        return -1;
    }

    proc_ptr driver = &ProcTable[diskpids[unit]];

    // init/get the process
    if (ProcTable[getpid() % MAXPROC].pid == -1) {
        init_proc(getpid());
    }
    proc_ptr proc = &ProcTable[getpid() % MAXPROC];

    if (write)
        proc->disk_request.opr = DISK_WRITE;
    else
        proc->disk_request.opr = DISK_READ;
    proc->disk_request.reg2 = buffer;
    proc->disk_track = track;
    proc->disk_first_sec = first;
    proc->disk_sectors = sectors;
    proc->disk_buffer = buffer;

    add_disk_queue(&disk_queues[unit], proc); // add to disk queue 
    semv_real(driver->block_sem);  // wake up disk driver
    semp_real(proc->block_sem); // block

    int status;
    int result = device_input(DISK_DEV, unit, &status);

    if (debug4)
        console("disk_read_or_write_real: finished, status = %d, result = %d\n", status, result);

    return result;
}

/* extract values from sysargs and call disk_size_real */
void diskSize(sysargs *args) {
    check_kernel_mode("diskSize");
    int unit = (long) args->arg1;
    int sector, track, disk;
    int retval = disk_size_real(unit, &sector, &track, &disk);
    args->arg1 = (void *) ((long) sector);
    args->arg2 = (void *) ((long) track);
    args->arg3 = (void *) ((long) disk);
    args->arg4 = (void *) ((long) retval);
    set_user_mode();
}

/*------------------------------------------------------------------------
    disk_size_real: Puts values into pointers for the size of a sector, 
    number of sectors per track, and number of tracks on the disk for the 
    given unit. 
    Returns: -1 if given illegal input, 0 otherwise
 ------------------------------------------------------------------------*/
int disk_size_real(int unit, int *sector, int *track, int *disk) {
    check_kernel_mode("disk_size_real");

    // check for illegal args
    if (unit < 0 || unit > 1 || sector == NULL || track == NULL || disk == NULL) {
        if (debug4)
            console("disk_size_real: given illegal argument(s), returning -1\n");
        return -1;
    }

    proc_ptr driver = &ProcTable[diskpids[unit]];

    // get the number of tracks for the first time
    if (driver->disk_track == -1) {
        // init/get the process
        if (ProcTable[getpid() % MAXPROC].pid == -1) {
            init_proc(getpid());
        }
        proc_ptr proc = &ProcTable[getpid() % MAXPROC];

        // set variables
        proc->disk_track = 0;
        device_request request;
        request.opr = DISK_TRACKS;
        request.reg1 = &driver->disk_track;
        proc->disk_request = request;

        add_disk_queue(&disk_queues[unit], proc); // add to disk queue 
        semv_real(driver->block_sem);  // wake up disk driver
        semp_real(proc->block_sem); // block

        if (debug4)
            console("disk_size_real: number of tracks on unit %d: %d\n", unit, driver->disk_track);
    }

    *sector = DISK_SECTOR_SIZE;
    *track = DISK_TRACK_SIZE;
    *disk = driver->disk_track;
    return 0;
}

void termRead(sysargs *args) {
    if (debug4)
        console("termRead\n");
    check_kernel_mode("termRead");

    char *buffer = (char *) args->arg1;
    int size = (long) args->arg2;
    int unit = (long) args->arg3;

    long retval = term_read_real(unit, size, buffer);

    if (retval == -1) {
        args->arg2 = (void *) ((long) retval);
        args->arg4 = (void *) ((long) -1);
    } else {
        args->arg2 = (void *) ((long) retval);
        args->arg4 = (void *) ((long) 0);
    }
    set_user_mode();
}

int term_read_real(int unit, int size, char *buffer) {
    if (debug4)
        console("term_read_real\n");
    check_kernel_mode("term_read_real");

    if (unit < 0 || unit > TERM_UNITS - 1 || size < 0) {
        return -1;
    }
    char line[MAXLINE];
    int ctrl = 0;

    //enable term interrupts
    if (term_int[unit] == 0) {
        if (debug4)
            console("term_read_real enable interrupts\n");
        ctrl = TERM_CTRL_RECV_INT(ctrl);
        device_output(TERM_DEV, unit, (void *) ((long) ctrl));
        term_int[unit] = 1;
    }
    int retval = MboxReceive(line_read_mbox[unit], &line, MAXLINE);

    if (debug4)
        console("term_read_real (unit %d): size %d retval %d \n", unit, size, retval);

    if (retval > size) {
        retval = size;
    }
    memcpy(buffer, line, retval);

    return retval;
}

void termWrite(sysargs *args) {
    if (debug4) {
        console("termWrite\n");
    }
    check_kernel_mode("termWrite");

    char *text = (char *) args->arg1;
    int size = (long) args->arg2;
    int unit = (long) args->arg3;

    long retval = term_write_real(unit, size, text);

    if (retval == -1) {
        args->arg2 = (void *) ((long) retval);
        args->arg4 = (void *) ((long) -1);
    } else {
        args->arg2 = (void *) ((long) retval);
        args->arg4 = (void *) ((long) 0);
    }
    set_user_mode();
}

int term_write_real(int unit, int size, char *text) {
    if (debug4) {
        console("term_write_real\n");
    }
    check_kernel_mode("term_write_real");

    if (unit < 0 || unit > TERM_UNITS - 1 || size < 0) {
        return -1;
    }

    int pid = getpid();
    MboxSend(pid_mbox[unit], &pid, sizeof(int));

    MboxSend(line_write_mbox[unit], text, size);
    semp_real(ProcTable[pid % MAXPROC].block_sem);
    return size;
}

/* ------------------------------------------------------------------------
   Name - check_kernel_mode
   Purpose - Checks if we are in kernel mode and prints an error messages
              and halts USLOSS if not.
   Parameters - The name of the function calling it, for the error message.
   Side Effects - Prints and halts if we are not in kernel mode
   ------------------------------------------------------------------------ */
void check_kernel_mode(char *name) {
    if ((PSR_CURRENT_MODE & psr_get()) == 0) {
        console("%s: called while in user mode, by process %d. Halting...\n",
                name, getpid());
        halt(1);
    }
}

/* ------------------------------------------------------------------------
   Name - set_user_mode
   Purpose - switches to user mode
   Parameters - none
   Side Effects - switches to user mode
   ------------------------------------------------------------------------ */
void set_user_mode() {
    psr_set(psr_get() & ~PSR_CURRENT_MODE);
}

/* initializes proc struct */
void init_proc(int pid) {
    check_kernel_mode("init_proc()");

    int i = pid % MAXPROC;

    ProcTable[i].pid = pid;
    ProcTable[i].mbox_id = MboxCreate(0, 0);
    ProcTable[i].block_sem = semcreate_real(0);
    ProcTable[i].wake_time = -1;
    ProcTable[i].disk_track = -1;
    ProcTable[i].next_disk_ptr = NULL;
    ProcTable[i].prev_disk_ptr = NULL;
}

/* empties proc struct */
void empty_proc(int pid) {
    check_kernel_mode("empty_proc()");

    int i = pid % MAXPROC;

    ProcTable[i].pid = -1;
    ProcTable[i].mbox_id = -1;
    ProcTable[i].block_sem = -1;
    ProcTable[i].wake_time = -1;
    ProcTable[i].next_disk_ptr = NULL;
    ProcTable[i].prev_disk_ptr = NULL;
}

/* ------------------------------------------------------------------------
  Functions for the disk_queue and heap.
   ----------------------------------------------------------------------- */

/* Initialize the given disk_queue */
void init_disk_queue(disk_queue *q) {
    q->head = NULL;
    q->tail = NULL;
    q->curr = NULL;
    q->size = 0;
}

/* Adds the proc pointer to the disk queue in sorted order */
void add_disk_queue(disk_queue *q, proc_ptr p) {
    if (debug4) {
        console("add_disk_queue: adding pid %d, track %d to queue\n", p->pid, p->disk_track);
    }

    // first add
    if (q->head == NULL) {
        q->head = q->tail = p;
        q->head->next_disk_ptr = q->tail->next_disk_ptr = NULL;
        q->head->prev_disk_ptr = q->tail->prev_disk_ptr = NULL;
    } else {
        // find the right location to add
        proc_ptr prev = q->tail;
        proc_ptr next = q->head;
        while (next != NULL && next->disk_track <= p->disk_track) {
            prev = next;
            next = next->next_disk_ptr;
            if (next == q->head) {
                break;
            }
        }
        if (debug4) {
            console("add_disk_queue: found place, prev = %d\n", prev->disk_track);
        }
        prev->next_disk_ptr = p;
        p->prev_disk_ptr = prev;
        if (next == NULL) {
            next = q->head;
        }
        p->next_disk_ptr = next;
        next->prev_disk_ptr = p;
        if (p->disk_track < q->head->disk_track) {
            q->head = p; // update head
        }
        if (p->disk_track >= q->tail->disk_track) {
            q->tail = p; // update tail
        }
    }
    q->size++;
    if (debug4) {
        console("add_disk_queue: add complete, size = %d\n", q->size);
    }
}

/* Returns the next proc on the disk queue */
proc_ptr peek_disk_queue(disk_queue *q) {
    if (q->curr == NULL) {
        q->curr = q->head;
    }

    return q->curr;
}

/* Returns and removes the next proc on the disk queue */
proc_ptr remove_disk_queue(disk_queue *q) {
    if (q->size == 0) {
        return NULL;
    }

    if (q->curr == NULL) {
        q->curr = q->head;
    }

    if (debug4) {
        console("remove_disk_queue: called, size = %d, curr pid = %d, curr track = %d\n", q->size, q->curr->pid,
                q->curr->disk_track);
    }

    proc_ptr temp = q->curr;

    if (q->size == 1) { // remove only node
        q->head = q->tail = q->curr = NULL;
    } else if (q->curr == q->head) { // remove head
        q->head = q->head->next_disk_ptr;
        q->head->prev_disk_ptr = q->tail;
        q->tail->next_disk_ptr = q->head;
        q->curr = q->head;
    } else if (q->curr == q->tail) { // remove tail
        q->tail = q->tail->prev_disk_ptr;
        q->tail->next_disk_ptr = q->head;
        q->head->prev_disk_ptr = q->tail;
        q->curr = q->head;
    } else { // remove other
        q->curr->prev_disk_ptr->next_disk_ptr = q->curr->next_disk_ptr;
        q->curr->next_disk_ptr->prev_disk_ptr = q->curr->prev_disk_ptr;
        q->curr = q->curr->next_disk_ptr;
    }

    q->size--;

    if (debug4) {
        console("remove_disk_queue: done, size = %d, curr pid = %d, curr track = %d\n", q->size, temp->pid,
                temp->disk_track);
    }

    return temp;
}


/* Setup heap */
void init_heap(heap *h) {
    h->size = 0;
}

/* Add to heap */
void heap_add(heap *h, proc_ptr p) {
    // start from bottom and find correct place
    int i, parent;
    for (i = h->size; i > 0; i = parent) {
        parent = (i - 1) / 2;
        if (h->procs[parent]->wake_time <= p->wake_time) {
            break;
        }
        // move parent down
        h->procs[i] = h->procs[parent];
    }
    h->procs[i] = p; // put at final location
    h->size++;
    if (debug4) {
        console("heap_add: Added proc %d to heap at index %d, size = %d\n", p->pid, i, h->size);
    }
}

/* Return min process on heap */
proc_ptr heap_peek(heap *h) {
    return h->procs[0];
}

/* Remove earliest waking process from the heap */
proc_ptr heap_remove(heap *h) {
    if (h->size == 0) {
        return NULL;
    }
    proc_ptr removed = h->procs[0]; // remove min
    h->size--;
    h->procs[0] = h->procs[h->size]; // put last in first spot

    // re-heapify
    int i = 0, left, right, min = 0;
    while (i * 2 <= h->size) {
        // get locations of children
        left = i * 2 + 1;
        right = i * 2 + 2;

        // get min child
        if (left <= h->size && h->procs[left]->wake_time < h->procs[min]->wake_time) {
            min = left;
        }
        if (right <= h->size && h->procs[right]->wake_time < h->procs[min]->wake_time) {
            min = right;
        }

        // swap current with min child if needed
        if (min != i) {
            proc_ptr temp = h->procs[i];
            h->procs[i] = h->procs[min];
            h->procs[min] = temp;
            i = min;
        } else {
            break; // otherwise we're done
        }
    }
    if (debug4) {
        console("heap_remove: Called, returning pid %d, size = %d\n", removed->pid, h->size);
    }
    return removed;
}