#define DEBUG2 1

typedef struct mailbox mail_box;
typedef struct mail_slot *slot_ptr;
typedef struct mail_slot mail_slot;
typedef struct mbox_proc *mbox_proc_ptr;
typedef struct mbox_proc mbox_proc;
typedef struct queue queue;

struct mailbox {
   int           mbox_id;
   /* other items as needed... */
   int           status;
   int           total_slots;
   int           slot_size;
   queue         queue_slots; // queue of mail_slots in the mailbox
   queue         blocked_proc_send; // processes blocked on send
   queue         blocked_proc_receive; // processes blocked on receive
};

// mailbox status constants
#define INACTIVE 0
#define ACTIVE 1

struct mail_slot {
   int           mbox_id;
   int           status;
   /* other items as needed... */
   int           slot_id;
   slot_ptr      next_slot_ptr;
   char          message[MAX_MESSAGE];
   int           message_size;
};

// mail_slot status constants
#define EMPTY 0
#define IN_USE 1

struct mbox_proc {
    mbox_proc_ptr   next_mbox_proc;
    slot_ptr        message_received; // mail_slot which will contain the message received
    int             pid;    // process id
    void            *message_ptr;   // pointer for where the received message to point
    int             message_size;
};

// mbox_proc status constants,unsure of values
#define FULL_MBOX
#define NO_MESSAGES

struct queue {
    void        *head;
    void        *tail;
    int         size;
    int         type;   // type of pointer to use
};

// queue status constants
#define QUEUE_SLOT 0
#define QUEUE_PROC 1

struct psr_bits {
    unsigned int cur_mode:1;
    unsigned int cur_int_enable:1;
    unsigned int prev_mode:1;
    unsigned int prev_int_enable:1;
    unsigned int unused:28;
};

union psr_values {
   struct psr_bits bits;
   unsigned int integer_part;
};
