#include <stdio.h>
#include <phase1.h>
#include <phase2.h>

extern int debugflag2;
extern void disableInterrupts(void);
extern void enableInterrupts(void);
extern void checkKernelMode(char *);


int IO_mailboxes[7]; //mailboxes for clock(1), disks(2), and terminals(4)
mailbox
int IO_blocked = 0; // number of blocked processes on IO_mailboxes
int clock_ticks = 0;

//IO_mailboxes status constants
#define CLOCKBOX 0  // set to 0 because clock will occupy index 0 of IO_mailboxes
#define DISKBOX 1   // set to 1 because disks will occupy indices 1 and 2 of IO_mailboxes
#define TERMBOX 3   // set to 3 because terminals will occupy indices 3 to 6 of IO_mailboxes

/* an error method to handle invalid syscalls */
void nullsys(sysargs *args)
{
    console("nullsys(): Invalid syscall. Halting...\n");
    halt(1);
} /* nullsys */


void clock_handler(int dev, void *unit)
{
    // disable interrupts and check if in kernel mode
    checkKernelMode("clock_handler()");
    disableInterrupts();


   if (DEBUG2 && debugflag2)
      console("clock_handler(): handler called\n");

   // checks if clock device, returns otherwise
   if (dev != CLOCK_DEV) {
       if (DEBUG2 && debugflag2)
           console("clock_handler(): called by another device! Returning...\n");
       return;
   }

   int status;

   if (read_cur_start_time() >= 80000) {
       time_slice();
   }
    // increment that a clock interrupt occured
   clock_ticks++;

   device_input(dev, unit, &status);

   if (clock_ticks % 5 == 0) {
       // conditionally send to check IO_mailbox every 5th clock interrupt
       if (DEBUG2 && debugflag2){
           console("clock_handler(): sending message %s to mailbox %d\n", status, IO_mailboxes[CLOCKBOX]);
       }
       int send_result = MboxCondSend(IO_mailboxes[CLOCKBOX], &status, sizeof(int));
       if (DEBUG2 && debugflag2) {
           console("clock_handler(): send returned %d Halting...\n", send_result);
           halt(1);
       }
   }
    enableInterrupts(); // re-enable interrupts
} /* clock_handler */


void disk_handler(int dev, void *unit)
{
    // disable interrupts and check if in kernel mode
    checkKernelMode("disk_handler()");
    disableInterrupts();


   if (DEBUG2 && debugflag2)
      console("disk_handler(): handler called\n");

    // checks if disk device, returns otherwise
    if (dev != DISK_DEV) {
        if (DEBUG2 && debugflag2)
            console("disk_handler(): called by another device! Returning...\n");
        return;
    }

    if (unit < 0 || unit > DISK_UNITS) {
        console("disk_handler(): invalid units! Halting...\n");
        halt(1);
    }

    // getting device status
    int status;
    int valid = device_input(dev, unit, &status);
    int mbox_id = unit +1;

    //checks if valid unit number, returns otherwise
    if (valid == DEV_INVALID) {
        if (DEBUG2 && debugflag2)
            console("disk_handler(): invalid unit number! Returning...\n");
        return;
    }
    // conditionally send the contents of status register to appropriate IO_mailbox
    MboxCondSend(mbox_id, &status, sizeof(int));

    enableInterrupts(); // enable interrupts

} /* disk_handler */


void term_handler(int dev, void *unit)
{
    // disable interrupts and check if in kernel mode
    checkKernelMode("term_handler()");
    disableInterrupts();


   if (DEBUG2 && debugflag2)
      console("term_handler(): handler called\n");

    // checks if terminal device, returns otherwise
    if (dev != TERM_DEV) {
        if (DEBUG2 && debugflag2)
            console("term_handler(): called by another device! Returning...\n");
        return;
    }

    if (unit < 0 || unit > TERM_UNITS) {
        console("term_handler(): invalid units! Halting...\n");
        halt(1);
    }

    // getting device status
    int status;
    int valid = device_input(dev, unit, &status);
    int mbox_id = unit +1;

    // checks if valid unit number, returns otherwise
    if (valid == DEV_INVALID) {
        if (DEBUG2 && debugflag2)
            console("term_handler(): invalid unit number! Returning...\n");
        return;
    }
    // conditionally send the contents of status register to appropriate IO_mailbox
    MboxCondSend(mbox_id, &status, sizeof(int));

    enableInterrupts(); // re-enable interrupts
} /* term_handler */


void syscall_handler(int dev, void *unit)
{
    // disable interrupts and check if in kernel mode
    checkKernelMode("syscall_handler()");
    disableInterrupts();


   if (DEBUG2 && debugflag2)
      console("syscall_handler(): handler called\n");

   sysargs *sys_ptr = (sysargs*) unit;

   // checks if system call device, returns otherwise
   if (dev != SYSCALL_INT) {
       if (DEBUG2 && debugflag2)
           console("syscall_handler(): called by another device! Returning...\n");
       return;
   }
   // checks if correct system call number, halts otherwise.
   if (sys_ptr->number < 0 || sys_ptr->number >= MAXSYSCALLS) {
       console("syscall_handler(): sys number %d is incorrect. Halting...\n", sys_ptr->number);
       halt(1);
   }


   sys_vec[sys_ptr->number](sys_ptr); // calling appropriate system call handlerNUL
   enableInterrupts(); // re-enable interrupts

} /* syscall_handler */

int waitdevice(int type, int unit, int *status)
{
    // disable interrupts and check if in kernel mode
    checkKernelMode("syscall_handler()");
    disableInterrupts();

    int result;
    int dev_id;
    int clock_id = CLOCKBOX;
    int disk_id = {DISKBOX, DISKBOX + 1};
    int term_id = {TERMBOX, TERMBOX + 1, TERMBOX + 2, TERMBOX + 3};

    switch(type) {
        case CLOCK_DEV:
            dev_id = clock_id;
            break;
        case DISK_DEV:
            if (unit < 0 || unit > 1) {
                console("waitdevice(): invalid unit! Halting...\n");
                halt(1);
            }
            dev_id = disk_id[unit];
            break;
        case TERM_DEV:
            if (unit < 0 || unit > 3) {
                console("waitdevice(): invalid unit! Halting...\n");
                halt(1);
            }
            dev_id = term_id[unit];
            break;
        default:
            console("waitdevice(): invalid type (%d). Halting...\n", type);
            halt(1);
    }

    IO_blocked++;
    result = MboxReceive(dev_id, status, sizeof()int));
    IO_blocked--;

    enableInterrupts(); // re-enable interrupts
    return result == -3 ? -1 : 0;

} /* waitdevice */

// returns 1 if processes blocked on IO, 0 otherwise
int check_io() {
    return IO_blocked > 0 ? 1 : 0;
} /* check_io */