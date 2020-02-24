#define DEBUG 0

typedef struct proc_struct proc_struct;

typedef struct proc_struct * proc_ptr;

struct proc_struct
{
   proc_ptr       next_proc_ptr;              // Pointer point to the process control.
   proc_ptr       child_proc_ptr;
   proc_ptr       next_sibling_ptr;
   char           name[MAXNAME];              /* process's name */
   char           start_arg[MAXARG];          /* args passed to process */
   context        state;                      /* current context for process it is also used to avoid using assembly code*/
   short          pid;                        /* process id */
   int            priority;
   int (* start_func) (char *);               /* function where process begins -- launch */
   char          *stack;
   unsigned int   stacksize;
   int            status;                     /* READY, BLOCKED, QUIT, etc. */
   /* other fields as needed... */
   proc_ptr       quit_child_ptr;
   int            exit_code;                  // Variable for the exit code of the process that call quit().
   int            is_zapped;                  // Variable for the zapped or not zapped.
   int            sliceTime;
   int            runTime;
   int            ppid;                       /* parent process id*/
};

struct psr_bits
{
        unsigned int cur_mode:1;
        unsigned int cur_int_enable:1;
        unsigned int prev_mode:1;
        unsigned int prev_int_enable:1;
        unsigned int unused:28;
};

union psr_values
{
   struct psr_bits bits;
   unsigned int integer_part;
};

/* Some useful constants.  Add more as needed... */
#define NO_CURRENT_PROCESS NULL
#define EMPTY -1
#define JOIN_BLOCK 2
#define ZAP_BLOCK 3
#define ZOMBIE 4
#define MINPRIORITY 5
#define MAXPRIORITY 1
#define SENTINELPID 1
#define SENTINELPRIORITY LOWEST_PRIORITY
#define TIMESLICE 80000
#define RUNNING 0
#define ZAPPED 1
#define NOT_ZAPPED 0
#define QUIT 3
#define READY 1
#define BLOCKED 2
