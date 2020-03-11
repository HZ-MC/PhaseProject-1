#include <stdio.h>
#include <phase1.h>
#include <phase2.h>
#include "message.h"

extern int debugflag2;
extern void disableInterrupts(void);
extern void enableInterrupts(void);
extern void requireKernelMode(char *);

int IO_mailboxes[7]; //mailboxes for clock(1), disks(2), and terminals(4)
int IO_blocked = 0; // number of blocked processes on IO_mailboxes

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
    disableInterrupts();
    requireKernelMode("clock_handler()");

   if (DEBUG2 && debugflag2)
      console("clock_handler(): handler called\n");

   // checks if clock device, returns otherwise
   if (dev != CLOCK_DEV) {
       if (DEBUG2 && debugflag2)
           console("clock_handler(): called by another device! Returning...\n");
       return;
   }

   // counter for every 5 interrupts
   static int count = 0;
   count++;
   if (count == 5) {
       int status;
       device_input(dev, 0, &status);
       // conditionally send to check IO_mailbox every 5th clock interrupt
       MboxCondSend(IO_mailboxes[CLOCKBOX], &status, sizeof(int));
       count = 0;
   }
    time_slice(); // call time_slice()
    enableInterrupts(); // re-enable interrupts
} /* clock_handler */


void disk_handler(int dev, void *unit)
{
    // disable interrupts and check if in kernel mode
    disableInterrupts();
    requireKernelMode("disk_handler()");

   if (DEBUG2 && debugflag2)
      console("disk_handler(): handler called\n");

    // checks if disk device, returns otherwise
    if (dev != DISK_DEV) {
        if (DEBUG2 && debugflag2)
            console("disk_handler(): called by another device! Returning...\n");
        return;
    }

    // getting device status
    long unit = (long) unit;
    int status;
    int valid = device_input(dev, unit, &status);

    //checks if valid unit number, returns otherwise
    if (valid != DEV_OK) {
        if (DEBUG2 && debugflag2)
            console("disk_handler(): invalid unit number! Returning...\n");
        return;
    }
    // conditionally send the contents of status register to appropriate IO_mailbox
    MboxCondSend(IO_mailboxes[DISKBOX + unit], &status, sizeof(int));

    enableInterrupts(); // enable interrupts

} /* disk_handler */


void term_handler(int dev, void *unit)
{
    // disable interrupts and check if in kernel mode
    disableInterrupts();
    requireKernelMode("term_handler()");

   if (DEBUG2 && debugflag2)
      console("term_handler(): handler called\n");

    // checks if terminal device, returns otherwise
    if (dev != TERM_DEV) {
        if (DEBUG2 && debugflag2)
            console("term_handler(): called by another device! Returning...\n");
        return;
    }

    // getting device status
    long unit = (long) unit;
    int status;
    int valid = device_input(dev, unit, &status);

    // checks if valid unit number, returns otherwise
    if (valid != DEV_OK) {
        if (DEBUG2 && debugflag2)
            console("term_handler(): invalid unit number! Returning...\n");
        return;
    }
    // conditionally send the contents of status register to appropriate IO_mailbox
    MboxCondSend(IO_mailboxes[TERMBOX + unit], &status, sizeof(int));

    enableInterrupts(); // re-enable interrupts
} /* term_handler */


void syscall_handler(int dev, void *unit)
{
    // disable interrupts and check if in kernel mode
    disableInterrupts();
    requireKernelMode("syscall_handler()");

   if (DEBUG2 && debugflag2)
      console("syscall_handler(): handler called\n");

   sysargs *sys_ptr;
   sys_ptr = (sysargs*) unit;

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

   sys_vec[sys_ptr->number](sys_ptr); // calling appropriate system call handler
   enableInterrupts(); // re-enable interrupts

} /* syscall_handler */

int waitdevice(int type, int unit, int *status)
{
    // disable interrupts and check if in kernel mode
    disableInterrupts();
    requireKernelMode("syscall_handler()");
    int result = 0;
    switch(type)
    {
        case CLOCK_DEV:
            result = CLOCKBOX;
            break;
        case DISK_DEV:
            result = DISKBOX;
            break;
        case TERM_DEV:
            result = TERMBOX;
            break;
        default:
            console("waitdevice(): invalid type (%d). Halting...\n", type);
            halt(1);
    }
    IO_blocked++;
    MboxReceive(IO_mailboxes[result+unit], status, sizeof(int));
    IO_blocked--;

    enableInterrupts(); // re-enable interrupts
    if (isZapped()){
        return -1;
    } else {
        return 0;
    }

} /* waitdevice */

// returns 1 if processes blocked on IO, 0 otherwise
int check_io() {
    if (IO_blocked > 0) {
        return 1;
    } else {
        return 0;
    }
} /* check_io */