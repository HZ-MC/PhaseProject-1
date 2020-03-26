
/* A test of waitdevice for the clock */

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

   printf("start2(): at beginning, pid = %d\n", getpid());

   kidpid = fork1("XXp1", XXp1, NULL, 2 * USLOSS_MIN_STACK, 3);
   printf("start2(): fork'd child %d\n", kidpid);

   kidpid = join(&kid_status);
   printf("start2(): joined with kid %d, status = %d\n", kidpid, kid_status);

   quit(0);
   return 0; /* so gcc will not complain about its absence... */
} /* start2 */


int XXp1(char *arg)
{
   int result, status;

   printf("XXp1(): started, calling waitdevice for clock\n");

   result = waitdevice(CLOCK_DEV, 0, &status);
   printf("XXp1(): after waitdevice call\n");

   if ( result == -1 ) {
      printf("XXp1(): got zap'd result from waitdevice() call. ");
      printf("Should not have happened!\n");
   }
   else if ( result == 0 )
      printf("XXp1(): status = %d\n", status);
   else
      printf("XXp1(): got %d instead of -1 or 0 from waitdevice\n", result);

   quit(-3);
   return 0; /* so gcc will not complain about its absence... */
} /* XXp1 */
