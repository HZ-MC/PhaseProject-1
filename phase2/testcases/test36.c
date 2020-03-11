/* A simple test of usyscall and sys_vec.  Makes a call to system trap 
 * number 0.  Should cause USLOSS to halt.
 */

#include <stdio.h>
#include <usloss.h>
#include <phase1.h>
#include <phase2.h>


extern void usyscall(void *arg);

void enableUserMode(){
   psr_set( psr_get() & (~ PSR_CURRENT_MODE) );
}


int start2(char *arg)
{
   sysargs args;

   printf("start2(): putting itself into user mode\n");
   enableUserMode();

   args.number=0;

   printf("start2(): calling usyscall executing syscall 0, this should halt\n");

   usyscall((void *)&args);

   printf("start2(): should not see this message!\n");
   quit(0);

   return 0;


} /* start2 */
