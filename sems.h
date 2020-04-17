// Queue struct for processes
typedef struct proc_struct3 proc_struct3;
typedef struct proc_struct3 *proc_ptr3;
typedef struct proc_queue proc_queue;

#define BLOCKED 0
#define CHILDREN 1

struct proc_queue {
    proc_ptr3 head;
    proc_ptr3 tail;
    int size;
    int type;
};

// Process struct for phase 3
struct proc_struct3 {
    int pid;
    int mbox_id; // zero-slot mailbox belongs to this mailbox
    int (* start_func)(char *); // function where process begins
    proc_ptr3 next_proc_ptr;
    proc_ptr3 next_sibling_ptr;
    proc_ptr3 parent_ptr;
    proc_queue children_queue;
};

// Semaphore struct

typedef struct semaphore semaphore;
struct semaphore {
    int id;
    int value;
    int starting_value;
    proc_queue blocked_proc;
    int priv_mbox_id;
    int mutex_mbox_id;
};