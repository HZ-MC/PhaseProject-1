
/* A test of waitdevice for all four terminals.
 * XXp1 tests terminal 0.
 * XXp2 tests terminal 1.
 * XXp3 tests terminal 2.
 * XXp4 tests terminal 3.
 */

#include <stdio.h>
#include <strings.h>
#include <stdlib.h>
#include <usloss.h>
#include <phase1.h>
#include <phase2.h>

int XXp1(char *);
int XXp3(char *);
char buf[256];

int start2(char *arg)
{
   int kid_status, kidpid;
   int control = 0;

   printf("start2(): started\n");

   control = TERM_CTRL_RECV_INT(control);

   printf("start2(): calling device_output to enable receive interrupts, ");
   printf("control = %d\n", control);

   device_output(TERM_DEV, 0, (void *)control);
   device_output(TERM_DEV, 1, (void *)control);
   device_output(TERM_DEV, 2, (void *)control);
   device_output(TERM_DEV, 3, (void *)control);

   kidpid = fork1("XXp0", XXp1, "0", 2 * USLOSS_MIN_STACK, 3);
   kidpid = fork1("XXp1", XXp1, "1", 2 * USLOSS_MIN_STACK, 3);
   kidpid = fork1("XXp2", XXp1, "2", 2 * USLOSS_MIN_STACK, 3);
   kidpid = fork1("XXp3", XXp1, "3", 2 * USLOSS_MIN_STACK, 3);

   kidpid = join(&kid_status);
   printf("start2(): joined with kid %d, status = %d\n\n", kidpid, kid_status);
   kidpid = join(&kid_status);
   printf("start2(): joined with kid %d, status = %d\n\n", kidpid, kid_status);
   kidpid = join(&kid_status);
   printf("start2(): joined with kid %d, status = %d\n\n", kidpid, kid_status);
   kidpid = join(&kid_status);
   printf("start2(): joined with kid %d, status = %d\n\n", kidpid, kid_status);

   quit(0);
   return 0; /* so gcc will not complain about its absence... */
} /* start2 */


int XXp1(char *arg)
{
   int result, status;
   int terminal = atoi(arg);

   printf("XXp%d(): started, calling waitdevice for terminal %d\n",
          terminal, terminal);

   result = waitdevice(TERM_DEV, terminal, &status);
   printf("XXp%d(): after waitdevice call\n", terminal);

   if ( result == -1 ) {
      printf("XXp%d(): got zap'd result from waitdevice() call. ", terminal);
      printf("Should not have happened!\n");
   }
   else if ( result == 0 )
      printf("XXp%d(): status = %d\n", terminal, status);
   else
      printf("XXp%d(): got %d instead of -1 or 0 from waitdevice\n",
             terminal, result);

   printf("XXp%d(): receive status for terminal 1 = %d\n",
          terminal, TERM_STAT_RECV(status));
   printf("XXp%d(): character received = %c\n",
          terminal, TERM_STAT_CHAR(status));

   quit(-3);
   return 0; /* so gcc will not complain about its absence... */

} /* XXp1 */
