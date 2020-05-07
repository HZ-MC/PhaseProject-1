/*
 *  User-visible definitions for usloss. Users of USLOSS are probably
 *  interested in everything in here.
 */

#if !defined(_usloss_h)
#define _usloss_h

#include <stdarg.h>
#include <signal.h>
#define macos
#ifndef macos
#include <ucontext.h>
#else
#include <sys/ucontext.h>
#endif

typedef struct context {
    void		(*start)();	/* Starting routine. */
    unsigned int	initial_psr;	/* Initial PSR */
    ucontext_t		context;	/* Internal context state */
} context;

/*  Function prototypes for USLOSS functions */
extern int		device_input(unsigned int dev, int unit, int *status);
extern int		device_output(unsigned int dev, int unit, void *arg);
extern void		waitint(void);
extern void		halt(int dumpcore);
extern void		console(char *string, ...);
extern void		vconsole(char *string, va_list ap);
extern void		trace(char *string, ...);
extern void		vtrace(char *string, va_list ap);
extern void		context_init(context *state, unsigned int psr,
			    char *stack, int stackSize, void (*func)(void));
extern void		context_switch(context *old, context *new);
extern unsigned int	psr_get(void);
extern void		psr_set(unsigned int psr);
extern int		sys_clock(void);
extern void		usyscall(void *arg);

/*
 *  This tells how many slots are in the intvec
 *  USLOSS_NUM_INTS = number of device types +  1 (for syscall interrupt)
 */
#define NUM_INTS	6	/* number of interrupts */

/*
 *  This is the interrupt vector table
 */
extern void (*int_vec[NUM_INTS])(int dev, void *arg);

/* 
 *  These are the values for the individual interrupts
 *  in the interrupt vector.
 */
#define CLOCK_INT	0	/* clock */
#define ALARM_INT	1	/* alarm */
#define DISK_INT		2	/* disk */
#define TERM_INT		3	/* terminal */
#define MMU_INT		4	/* MMU */
#define SYSCALL_INT 	5	/* syscall */

#define LOW_PRI_DEV	TERM_INT  /* terminal is lowest priority */


/*
 * These are the different device types. They must be the same values as the
 * interrupts and the USLOSS code conflats the two, but having different macros
 * for devices and interrupts makes the OS code cleaner.
 */

#define CLOCK_DEV 	CLOCK_INT
#define ALARM_DEV 	ALARM_INT
#define DISK_DEV		DISK_INT
#define TERM_DEV		TERM_INT

/*
 * # of units of each device type
 */

#define CLOCK_UNITS	1
#define ALARM_UNITS	1
#define DISK_UNITS	2
#define TERM_UNITS	4
/*
 * Maximum number of units of any device.
 */

#define MAX_UNITS	4

/*
 *  This is the structure used to send a request to
 *  a device.
 */
typedef struct device_request
{
	int opr;
	void *reg1;
	void *reg2;
} device_request;

/*
 *  These are the operations for the disk device
 */
#define DISK_READ	0
#define DISK_WRITE	1
#define DISK_SEEK	2
#define DISK_TRACKS	3

/*
 *  These are the status codes returned by device_output(). In general,
 *  the status code is in the lower byte of the int returned; the upper
 *  bytes may contain other info. See the documentation for the
 *  specific device for details.
 */
#define DEV_READY	0
#define DEV_BUSY		1
#define DEV_ERROR	2

/* 
 * device_output() and device_input() will return DEV_OK if their
 * arguments were valid and the device is ready, DEV_BUSY if the arguments were valid
 * but the device is busy, and DEV_INVALID otherwise. By valid, the device 
 * type and unit must correspond to a device that exists. 
 */

#define DEV_OK		DEV_READY
#define DEV_INVALID	DEV_ERROR

/*
 * These are the fields of the terminal status registers. A call to
 * device_input will return the status register, and you can use these
 * macros to extract the fields. The xmit and recv fields contain the
 * status codes listed above.
 */

#define TERM_STAT_CHAR(status)\
	(((status) >> 8) & 0xff)	/* character received, if any */

#define	TERM_STAT_XMIT(status)\
	(((status) >> 2) & 0x3) 	/* xmit status for unit */

#define	TERM_STAT_RECV(status)\
	((status) & 0x3)		/* recv status for unit */

/*
 * These are the fields of the terminal control registers. You can use
 * these macros to put together a control word to write to the
 * control registers via device_output.
 */

#define TERM_CTRL_CHAR(ctrl, ch)\
	((ctrl) | (((ch) & 0xff) << 8))/* char to send, if any */

#define	TERM_CTRL_XMIT_INT(ctrl)\
	((ctrl) | 0x4)			/* enable xmit interrupts */

#define	TERM_CTRL_RECV_INT(ctrl)\
	((ctrl) | 0x2)			/* enable recv interrupts */

#define TERM_CTRL_XMIT_CHAR(ctrl)\
	((ctrl) | 0x1)			/* xmit the char in the upper bits */


/*
 *  Size of disk sector (in bytes) and number of sectors in a track
 */
#define DISK_SECTOR_SIZE		512
#define DISK_TRACK_SIZE		16

/*
 * Processor status word (PSR) fields. Current is the current mode
 * and interrupt values, prev are the values prior to the last
 * interrupt. The interrupt handler moves current into prev on an
 * interrupt, and restores current from prev on returning.
 */

#define PSR_CURRENT_MODE 	0x1
#define PSR_CURRENT_INT		0x2
#define PSR_PREV_MODE		0x4
#define PSR_PREV_INT		0x8

#define PSR_CURRENT_MASK	0x3
#define PSR_PREV_MASK		0xc
#define PSR_MASK 		(PSR_CURRENT_MASK | PSR_PREV_MASK)

/*
 * Length of a clock tick.
 */

#define CLOCK_MS	20

/*
 * Minimum stack size. 
 */

#define USLOSS_MIN_STACK (80 * 1024)

/*
 * Routines that USLOSS invokes on startup and shutdown. Must be defined by the OS.
 */

extern void startup(void);
extern void finish(void);


/*
 * MMU definitions.
 */

 #define USLOSS_MMU_NUM_TAG	4	/* Maximum number of tags in MMU */

/*
 * Error codes
 */
#define USLOSS_MMU_OK		0	/* Everything hunky-dory */
#define USLOSS_MMU_ERR_OFF	1	/* MMU not enabled */
#define USLOSS_MMU_ERR_ON	2	/* MMU already initialized */
#define USLOSS_MMU_ERR_PAGE	3	/* Invalid page number */
#define USLOSS_MMU_ERR_FRAME	4	/* Invalid frame number */
#define USLOSS_MMU_ERR_PROT	5	/* Invalid protection */
#define USLOSS_MMU_ERR_TAG	6	/* Invalid tag */
#define USLOSS_MMU_ERR_REMAP	7	/* Page already mapped */
#define USLOSS_MMU_ERR_NOMAP	8	/* Page not mapped */
#define USLOSS_MMU_ERR_ACC	9	/* Invalid access bits */
#define USLOSS_MMU_ERR_MAPS	10	/* Too many mappings */

/*
 * Protections
 */
#define USLOSS_MMU_PROT_NONE	0	/* Page cannot be accessed */
#define USLOSS_MMU_PROT_READ	1	/* Page is read-only */
#define USLOSS_MMU_PROT_RW	3	/* Page can be both read and written */

/*
 * Causes
 */
#define USLOSS_MMU_FAULT	1	/* Address was in unmapped page */
#define USLOSS_MMU_ACCESS	2	/* Access type not permitted on page */

/*
 * Access bits
 */
#define USLOSS_MMU_REF		1	/* Page has been referenced */
#define USLOSS_MMU_DIRTY	2	/* Page has been written */

/*
 * Function prototypes for MMU routines. See the MMU documentation.
 */

extern int 	USLOSS_MmuInit(int numMaps, int numPages, int numFrames);
extern void	*USLOSS_MmuRegion(int *numPagesPtr);
extern int	USLOSS_MmuDone(void);
extern int	USLOSS_MmuMap(int tag, int page, int frame, int protection);
extern int	USLOSS_MmuUnmap(int tag, int page);
extern int	USLOSS_MmuGetMap(int tag, int page, int *framePtr, int *protPtr);
extern int	USLOSS_MmuGetCause(void);
extern int	USLOSS_MmuSetAccess(int frame, int access);
extern int	USLOSS_MmuGetAccess(int frame, int *accessPtr);
extern int	USLOSS_MmuSetTag(int tag);
extern int	USLOSS_MmuGetTag(int *tagPtr);
extern int	USLOSS_MmuPageSize(void);
extern int	USLOSS_MmuTouch(void *addr);


#endif	/*  _usloss_h */

