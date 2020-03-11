/* Attempt to receive a message from a mailbox using a buffer that is
 * too small for the message.
 */

#include <stdio.h>
#include <usloss.h>
#include <phase1.h>
#include <phase2.h>

char buf[256];
int mbox_id;
int start2(char *arg)
{

   int result;
   char buffer[11];

   printf("start2(): started\n");
   mbox_id = MboxCreate(10, 13);
   printf("start2(): MboxCreate returned id = %d\n", mbox_id);

   result = MboxSend(mbox_id, "hello there", 12);
   printf("start2(): after send of message, result = %d\n", result);

   printf("start2(): Attempting to receive message fm mailbox %d.\n", mbox_id);
   printf("          Should fail here because it's receive buffer is\n");
   printf("          too small\n");
   result = MboxReceive(mbox_id, buffer, 11);
   printf("start2(): after receive of message, result = %d\n", result);
   printf("          message is `%s'\n", buffer);

   if (result == -1){
      printf("start2(): got that message was too big. PASSED!\n");
      quit(0);
   }
   else {
      printf("start2(): FAILED!\n");
      quit(0);
   }

   quit(0);
   return 0; /* so gcc will not complain about its absence... */

} /* start2 */
