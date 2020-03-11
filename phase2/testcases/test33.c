/* do a send to a mailbo xof a message that is exactly equal to the max
 * message size specified for that mailbox when it was created.
 * Then receive that message.
 */

#include <stdio.h>
#include <usloss.h>
#include <phase1.h>
#include <phase2.h>


int XXp1(char *);
char buf[256];


int start2(char *arg)
{
   int mbox_id;
   int result;
   char buffer[80];

   printf("start2(): started\n");
   mbox_id = MboxCreate(10, 12);
   printf("start2(): MboxCreate returned id = %d\n", mbox_id);

   printf("start2(): sending message to mailbox %d\n", mbox_id);
   result = MboxSend(mbox_id, "hello there", 12);
   printf("start2(): after send of message, result = %d\n", result);

   printf("start2(): attempting to receive message from mailbox %d\n", mbox_id);
   result = MboxReceive(mbox_id, buffer, 80);
   printf("start2(): after receive of message, result = %d\n", result);
   printf("          message is `%s'\n", buffer);

   quit(0);
   return 0; /* so gcc will not complain about its absence... */

} /* start2 */
