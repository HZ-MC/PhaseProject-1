
/* A test of waitdevice for a terminal.  Can be used to test other
 * three terminals as well.
 */

#include <stdio.h>
#include <strings.h>
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

   device_output(TERM_DEV, 1, (void *)control);

   kidpid = fork1("XXp1", XXp1, NULL, 2 * USLOSS_MIN_STACK, 3);

   kidpid = join(&kid_status);
   printf("start2(): joined with kid %d, status = %d\n", kidpid, kid_status);

   quit(0);
   return 0; /* so gcc will not complain about its absence... */
} /* start2 */


int XXp1(char *arg)
{
   int result, status;

   printf("XXp1(): started, calling waitdevice for terminal 1\n");

   result = waitdevice(TERM_DEV, 1, &status);
   printf("XXp1(): after waitdevice call\n");

   if ( result == -1 ) {
      printf("XXp1(): got zap'd result from waitdevice() call. ");
      printf("Should not have happened!\n");
   }
   else if ( result == 0 )
      printf("XXp1(): status = %d\n", status);
   else
      printf("XXp1(): got %d instead of -1 or 0 from waitdevice\n", result);

   printf("XXp1(): receive status for terminal 1 = %d\n",
          TERM_STAT_RECV(status));
   printf("XXp1(): character received = %c\n", TERM_STAT_CHAR(status));

   quit(-3);
   return 0; /* so gcc will not complain about its absence... */
} /* XXp1 */
