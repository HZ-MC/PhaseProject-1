/* Queue struct for processes */
typedef struct proc_struct proc_struct;
typedef struct proc_struct * proc_ptr;
typedef struct disk_queue disk_queue;

// #define BLOCKED 0
// #define CHILDREN 1
// #define SLEEP 2

struct disk_queue {
	proc_ptr  head;
	proc_ptr  tail;
	proc_ptr  curr;
	int 	 size;
	int 	 type; /* which proc_ptr to use for next */
};

/* Heap */
typedef struct heap heap;
struct heap {
  int size;
  proc_ptr procs[MAXPROC];
};

/* 
* Process struct for phase 4
*/
struct proc_struct {
  int         pid;
  int 		  mbox_id; 
  int         block_sem;
  int		  wake_time;
  int 		  disk_track;
  int 		  disk_first_sec;
  int 		  disk_sectors;
  void 		  *disk_buffer;
  proc_ptr 	  prev_disk_ptr;
  proc_ptr 	  next_disk_ptr;
  device_request disk_request;
};
