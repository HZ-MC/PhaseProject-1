/* ------------------------------------------------------------------------
   phase2.c

   University of Arizona South
   Computer Science 452
   @authors: Javier Felix, Mark Festejo, Hassan Martinez
   ------------------------------------------------------------------------ */

#include <phase1.h>
#include <phase2.h>
#include <usloss.h>

#include "message.h"

/* ------------------------- Prototypes ----------------------------------- */
int start1 (char *);
extern int start2 (char *);
void disableInterrupts(void);
void enableInterrupts(void);
void checkKernelMode(void);
void initBox(int);
void initSlot(int);
void initQueue(queue*, int);
void enqueue(queue*, void*);
void *dequeue(queue*);
void *peek(queue*);


/* -------------------------- Globals ------------------------------------- */

int debugflag2 = 0;

mailbox MailBoxTable[MAXMBOX];  // mail boxes
mail_slot MailSlotTable[MAXSLOTS]; // mail slots
mbox_proc mboxProcTable[MAXPROC];   // processes

// number of mail boxes/slots in use
int num_boxes, num_slots;

// next mail box/slot id to be assigned
int next_mbox_id = 0;
int next_slot_id = 0;
int next_proc = 0;


/* -------------------------- Functions ----------------------------------- */

/* ------------------------------------------------------------------------
   Name - start1
   Purpose - Initializes mailboxes and interrupt vector.
             Start the phase2 test process.
   Parameters - one, default arg passed by fork1, not used here.
   Returns - one to indicate normal quit.
   Side Effects - lots since it initializes the phase2 data structures.
   ----------------------------------------------------------------------- */
int start1(char *arg)
{
   if (DEBUG2 && debugflag2)
      console("start1(): at beginning\n");

    // disable interrupts and check if in kernel mode
    disableInterrupts();
    checkKernelMode("start1()");

   /* Initialize the mail box table, slots, & other data structures.
    * Initialize int_vec and sys_vec, allocate mailboxes for interrupt
    * handlers.  Etc... */

   int i;

   // initializing mailbox table
   for (i = 0; i < MAXMBOX; i++){
        initBox(i);
   }

   // initializing mail slots
   for (i = 0; i < MAXSLOTS; i++){
       initSlot(i);
   }

   // setting mail boxes/slots in use to 0
   num_boxes = num_slots = 0;

   // init IO_mailboxes for interrupt handlers
   IO_mailboxes[CLOCKBOX] = MboxCreate(0, sizeof(int)); // 1 clock
   IO_mailboxes[DISKBOX] = MboxCreate(0, sizeof(int)); // 2 disks
   IO_mailboxes[DISKBOX + 1] = MboxCreate(0, sizeof(int));
   IO_mailboxes[TERMBOX] = MboxCreate(0, sizeof(int)); // 4 terminals
   IO_mailboxes[TERMBOX + 1] = MboxCreate(0, sizeof(int));
   IO_mailboxes[TERMBOX + 2] = MboxCreate(0, sizeof(int));
   IO_mailboxes[TERMBOX + 3] = MboxCreate(0, sizeof(int));

   // init interrupt handlers
   int_vec[CLOCK_INT] = clock_handler;
   int_vec[DISK_INT] = disk_handler;
   int_vec[TERM_INT] = term_handler;
   int_vec[SYSCALL_INT] = syscall_handler;

   // initializing every system call handler as nullsys
   for (i = 0; i < MAXSYSCALLS; i++) {
       sys_vec[i] = nullsys;
   }

   enableInterrupts(); // re-enable interrupts

   /* Create a process for start2, then block on a join until start2 quits */
   if (DEBUG2 && debugflag2)
      console("start1(): fork'ing start2 process\n");
   kid_pid = fork1("start2", start2, NULL, 4 * USLOSS_MIN_STACK, 1);
   if ( join(&status) != kid_pid ) {
      console("start2(): join returned something other than start2's pid\n");
   }

   return 0;
} /* start1 */


/* ------------------------------------------------------------------------
   Name - MboxCreate
   Purpose - gets a free mailbox from the table of mailboxes and initializes it 
   Parameters - maximum number of slots in the mailbox and the max size of a msg
                sent to the mailbox.
   Returns - -1 to indicate that no mailbox was created, or a value >= 0 as the
             mailbox id.
   Side Effects - initializes one element of the mail box array. 
   ----------------------------------------------------------------------- */
int MboxCreate(int slots, int slot_size)
{
    // disable interrupts and check if in kernel mode
    disableInterrupts();
    checkKernelMode("MboxCreate()");

    if (num_boxes == MAXMBOX || slots < 0 || slot_size < 0 || slot_size > MAX_MESSAGE) {
        if (DEBUG2 && debugflag2)
            console("MboxCreate(): max boxes reached or invalid args! Returning -1\n");
        return -1
    }

    // finds next available index for the next mailbox id
    if (next_mbox_id >= MAXMBOX || MailBoxTable[next_mbox_id].status == ACTIVE) {
        for(int i = 0; i < MAXMBOX; i++) {
            if(MailBoxTable[i].status == INACTICE) {
                next_mbox_id = i;
                break;
            }
        }
    }

    mail_box *mbox = &MailBoxTable[next_mbox_id];

    mbox->mbox_id = next_mbox_id++;
    mbox->total_slots = slots;
    mbox->slot_size = slot_size;
    mbox->status = ACTIVE;
    initQueue(&mbox->slots, QUEUE_SLOT);
    initQueue(&mbox->blocked_proc_send,QUEUE_PROC);
    initQueue(&mbox->blocked_proc_receive, QUEUE_PROC);

    num_boxes++;

    if (DEBUG2 && debugflag2) {
        console("MboxCreate(): created mailbox. id: %d, total_slots: %d, slot_size: %d, num_boxes: %d\n",
                mbox->mbox_id, mbox->total_slots, mbox->slot_size, num_boxes)
    }

    enableInterrupts(); // re-enable interupts
    return mbox->mbox_id;

} /* MboxCreate */


/* ------------------------------------------------------------------------
   Name - MboxSend
   Purpose - Put a message into a slot for the indicated mailbox.
             Block the sending process if no slot available.
   Parameters - mailbox id, pointer to data of msg, # of bytes in msg.
   Returns - zero if successful, -1 if invalid args.
   Side Effects - none.
   ----------------------------------------------------------------------- */
int MboxSend(int mbox_id, void *msg_ptr, int msg_size)
{
    // disable interrupts and check if in kernel mode
    disableInterrupts();
    checkKernelMode("MboxSend()");

    if (DEBUG2 && debugflag2) {
        console("MboxSend(): called with mbox_id: %d, msg_ptr: %d, msg_size: %d\n",
                mbox_id, msg_ptr, msg_size);
    }

    // check validity of mbox_id
    if (mbox_id < 0 || mbox_id >= MAXMBOX) {
        if (DEBUG2 && debugflag2) {
            console("MboxSend(): called with invalid mbox_id: %d. Returning -1\n", mbox_id);
        }
        enableInterrupts(); // re-enable interrupts
        return -1;
    }

    // get mailbox
    mailbox *mbox = &MailBoxTable[mbox_id];

    // check validity of arguments
    if (mbox->status == INACTIVE || mbox->slot_size < msg_size || msg_size < 0) {
        if (DEBUG2 && debugflag2) {
            console("MboxSend(): called with invalid arguments! Returning -1\n", mbox_id);
        }
        enableInterrupts(); // re-enable interrupts
        return -1;
    }

    // handle blocked receive
    if (mbox->blocked_proc_receive.size > 0 && (mbox->queue_slots < mbox->total_slots || mbox->total_slots == 0)) {
        mbox_proc_ptr proc = (mbox_proc_ptr)dequeue(&mbox->blocked_proc_receive);
        // message to receiver
        int result = 0;
        if (proc == NULL || proc->message_ptr == NULL || proc->message_size < msg_size) {
            if (DEBUG2 && debugflag2) {
                console("MboxSend(): called with invalid mbox_id: %d. Returning -1\n", mbox_id);
            }
            enableInterrupts(); // re-enable interrupts
            result = -1;
        } else {
            memcpy(proc->message_ptr, msg_ptr, msg_size);
            proc->message_size = msg_size;
            if (DEBUG2 && debugflag2) {
                console("MboxSend(): process %d was given message size %d\n", proc->pid, msg_size);
            }
        }

        if (DEBUG2 && debugflag2) {
            console("MboxSend(): process %d was blocked on received! Unblocking...\n", proc->pid);
        }

        unblock_proc(proc->pid);
        enableInterrupts(); // re-enable interrupts
        return result;
    }

    // if all slots taken, block caller until slots become available
    if (mbox->slots.size == mbox->total_slots) {

        // init proc
        mbox_proc mproc;
        mproc.next_mbox_proc = NULL;
        mproc.pid = getpid();
        mproc.message_ptr = msg_ptr;
        mproc.message_size = msg_size;

        if (DEBUG2 && debugflag2) {
            console("MboxSend(): all slots full! Blocking pid %d...\n", mproc.pid);
        }

        // add to queue of send blocked processes in this mailbox
        enqueue(&mbox->blocked_proc_send, &mproc);
        block_me(FULL_MBOX); // blocking
        disableInterrupts(); // disabling interrupts when unblocked
        enableInterrupts(); // re-enable interrupts
        return 0;
    }

    // if mail slot table overflows, error should halt USLOSS
    if (num_slots == MAXSLOTS) {
        console("MboxSend(): mail slot table overflow! Halting...\n");
        halt(1);
    }

    // creating new slot and adding a message to it
    int slot_id;
    int i;
    if (next_slot_id >= MAXSLOTS || MailSlotTable[next_slot_id].status == IN_USE) {
        for (i = 0; i < MAXSLOTS; i++) {
            if (MailSlotTable[i].status == EMPTY)   {
                next_slot_id = i;
                break;
            }
        }
    }
    slot_ptr slot = &MailSlotTable[next_slot_id];
    slot->slot_id = next_slot_id++;
    slot->status = IN_USE;
    slot->message_size = msg_size;
    num_slots++;

    // copy message into slot
    memcpy(slot->message, msg_ptr, msg_size);

    if (DEBUG2 && debugflag2) {
        console("MboxSend(): created new slot for slot id: %d, message size: %d, total slots: %d\n",
                slot->slot_id, msg_size, num_slots)
    }

    enqueue(&mbox->slots, slot);
    return 0;

} /* MboxSend */




/* ------------------------------------------------------------------------
   Name - MboxReceive
   Purpose - Get a msg from a slot of the indicated mailbox.
             Block the receiving process if no msg available.
   Parameters - mailbox id, pointer to put data of msg, max # of bytes that
                can be received.
   Returns - actual size of msg if successful, -1 if invalid args.
   Side Effects - none.
   ----------------------------------------------------------------------- */
int MboxReceive(int mbox_id, void *msg_ptr, int msg_size)
{
} /* MboxReceive */

int MboxCondSend(int mbox_id, void *msg_ptr, int msg_size)
{

}

int MboxCondReceive(int mbox_id, void *msg_ptr, int msg_size)
{
    return receive(mbox_id, msg_ptr, msg_size, 1)
}

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
    if ((PSR_CURRENT_MODE & psr_get()) == 0) {
        console("%s: called while in user mode, by process %d. Halting...\n", name, getpid());
        halt(1);
    }
} /* checkKernelMode */

// initializes mailbox
void initBox(int i) {
    MailBoxTable[i].mbox_id = -1;
    MailBoxTable[i].status = INACTIVE;
    MailBoxTable[i].total_slots = -1;
    MailBoxTable[i].slot_size = -1;
    num_boxes--;
} /* initBox */

// initializes mail slot
void initSlot(int i) {
    MailSlotTable[i].mbox_id = -1;
    MailSlotTable[i].status = EMPTY;
    MailSlotTable[i].slot_id = -1;
    num_slots--;
} /* initSlot */

// initializes a queue
void initQueue(queue *q, int type) {
    q->head = NULL;
    q->tail = NULL;
    q->size = 0;
    q->type = type;
} /* initQueue */

// Inserts given pointer to the back of a queue
void enqueue(queue *q, void *p) {
    if (q->head == NULL && q->tail == NULL) {
        q->head = q->tail = p;
    } else {
        if (q->type == QUEUE_SLOT) {
            ((slot_ptr)(q->tail))->next_slot_ptr = p;
        } else if (q->type == QUEUE_PROC) {
            ((mbox_proc_ptr)(q->tail))->next_mbox_proc = p;
        }
        q->tail = p;
    }
    q->size++;
} /* enqueue */

// Removes and returns the head of a queue
void *dequeue(queue *q) {
    void *curr = q->head;
    if (q->head == NULL) {
        return NULL;
    }
    if (q->head == q->tail) {
        q->head = q->tail = NULL;
    } else {
        if (p->type == QUEUE_SLOT) {
            q->head = ((slot_ptr)(q->head))->next_slot_ptr;
        } else if (q->type == QUEUE_PROC) {
            q->head = ((mbox_proc_ptr)(q->head))->next_mbox_proc;
        }
    }
    q->size--;
    return curr;
} /* dequeue */

// returns the head of a queue
void *peek(queue *q) {
    return q->head;
} /* peek */