/*
 * These are the definitions for phase1 of the project (the kernel).
 */

#ifndef _PHASE1_H
#define _PHASE1_H

#include <usloss.h>

/*
 * Maximum number of processes. 
 */

#define MAXPROC		50

/*
 * Maximum length of a process name
 */

#define MAXNAME		50

/*
 * Maximum length of string argument passed to a newly created process
 */

#define MAXARG		100

/*
 * Maximum number of syscalls.
 */

#define MAXSYSCALLS 	50

/* the highest priority a process can have */
#define HIGHEST_PRIORITY 1
/* the lowest priority a process can have */
#define LOWEST_PRIORITY 6

/* 
 * Function prototypes for this phase.
 */

extern  int             fork1(char *name, int(*func)(char *), char *arg, 
			    int stacksize, int priority);
extern	int		join(int *status);
extern	void		quit(int status);
extern  int		zap(int pid);
extern  int		is_zapped(void);
extern	int		getpid(void);
extern	void		dump_processes(void);
extern  int             block_me(int block_status);
extern  int             unblock_proc(int pid);
extern  int             read_cur_start_time(void);
extern  void            time_slice(void);
extern  void            dispatcher(void);
extern	int		readtime(void);

extern	void		p1_fork(int pid);
extern	void		p1_quit(int pid);
extern	void		p1_switch(int old, int new);

#endif /* _PHASE1_H */
