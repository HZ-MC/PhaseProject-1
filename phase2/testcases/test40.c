/* Another test of receive buffer size being too small.  In this case,
 * the receiving process is blocked first since there are no messages.
 * When the message is sent later, the receiver should get the return
 * result that indicates the receive buffer size is too small.
 */

#include <stdio.h>
#include <usloss.h>
#include <phase1.h>
#include <phase2.h>

int XXp1(char *);
char buf[256];
int mbox_id;
int start2(char *arg)
{

   int result, kidpid;
   char buffer[11];

   printf("start2(): started\n");
   mbox_id = MboxCreate(1, 13);
   printf("start2(): MboxCreate returned id = %d\n", mbox_id);
   kidpid = fork1("XXp1", XXp1, NULL, 2 * USLOSS_MIN_STACK, 4);
   printf("start2(): Attempting to receive message from mailbox %d\n", mbox_id);
   printf("          But it is blocked because the slots are empty.\n");
   printf("          Then it should fail due to buffer size too small\n");

   result = MboxReceive(mbox_id, buffer, 11);

   printf("start2(): after receive of message, result = %d\n", result);
   printf("          message is `%s'\n", buffer);
   if (result == -1){
      printf("start2(): got that message was too big. PASSED!\n");
   }
   else {
      printf("start2(): FAILED!\n");
      quit(0);
   }

   printf("start2(): joining with child\n");
   join(&result);

   quit(0);
   return 0; /* so gcc will not complain about its absence... */

} /* start2 */


int XXp1(char *arg)
{
   int  result;

   printf("XXp1(): started\n");
   printf("XXp1(): arg = `%s'\n", arg);
   result = MboxSend(mbox_id, "hello there", 12);
   printf("XXp1(): after send of message, result = %d\n", result);

   quit(-3);
   return 0;

} /* XXp1 */
