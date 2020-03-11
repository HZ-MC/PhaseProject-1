
/* Creates a 5-slot mailbox. Creates XXp1 that conditionally sends eight hello
 * messages * to the mailbox, five of which should succeed and three will
 * return -2.  XXp1 then blocks on a receive on its private mailbox.
 * Creates XXp3 which receives the five hello messages that should be available
 * from the slots.  XXp3 then sends to XXp1's private mailbox.  Since XXp3 is
 * lower priority than XXp1, XXp1 runs again.
 * XXp1 wakes up from its private mailbox and sends eight goodbye messages to
 * the mailbox, five of which should succeed and three will return -2.  XXp1
 * then quits.
 * XXp3 should pick up the five good-bye messages from the mailbox and quit.
 */

#include <stdio.h>
#include <strings.h>
#include <usloss.h>
#include <phase1.h>
#include <phase2.h>

int XXp1(char *);
int XXp3(char *);
char buf[256];

int mbox_id, private_mbox;

int start2(char *arg)
{
   int kid_status, kidpid;

   printf("start2(): started\n");
   mbox_id = MboxCreate(5, 50);
   printf("start2(): MboxCreate returned id = %d\n", mbox_id);
   private_mbox = MboxCreate(0, 50);
   printf("start2(): MboxCreate returned id = %d\n", private_mbox);

   kidpid = fork1("XXp1",  XXp1, NULL,    2 * USLOSS_MIN_STACK, 3);
   kidpid = fork1("XXp3",  XXp3, NULL,    2 * USLOSS_MIN_STACK, 4);

   kidpid = join(&kid_status);
   printf("start2(): joined with kid %d, status = %d\n", kidpid, kid_status);

   kidpid = join(&kid_status);
   printf("start2(): joined with kid %d, status = %d\n", kidpid, kid_status);

   quit(0);
   return 0; /* so gcc will not complain about its absence... */
} /* start2 */


int XXp1(char *arg)
{
   int i, result;
   char buffer[20];

   printf("XXp1(): started\n");

   for (i = 0; i < 8; i++) {
      printf("XXp1(): conditionally sending message #%d to mailbox %d\n",
             i, mbox_id);
      sprintf(buffer, "hello there, #%d", i);
      result = MboxCondSend(mbox_id, buffer, strlen(buffer)+1);
      printf("XXp1(): after conditional send of message #%d, result = %d\n",
             i, result);
   }

   MboxReceive(private_mbox, NULL, 0);

   for (i = 0; i < 8; i++) {
      printf("XXp1(): conditionally sending message #%d to mailbox %d\n",
             i, mbox_id);
      sprintf(buffer, "good-bye, #%d", i);
      result = MboxCondSend(mbox_id, buffer, strlen(buffer)+1);
      printf("XXp1(): after conditional send of message #%d, result = %d\n",
             i, result);
   }

   quit(-3);
   return 0; /* so gcc will not complain about its absence... */
} /* XXp1 */


int XXp3(char *arg)
{
   char buffer[100];
   int i = 0, result, count;

   printf("XXp3(): started\n");

   count = 0;
   while ( (result = MboxCondReceive(mbox_id, buffer, 100)) >= 0 ) {
      printf("XXp3(): conditionally received message #%d from mailbox %d\n",
             i, mbox_id);
      printf("        message = `%s'\n", buffer);
      count++;
   }
   printf("XXp3(): After loop, result is negative; result = %d\n", result);
   printf("XXp3(): received %d hello messages from mailbox %d\n",
          count, mbox_id);

   MboxSend(private_mbox, NULL, 0);

   count = 0;
   while ( (result = MboxCondReceive(mbox_id, buffer, 100)) >= 0 ) {
      printf("XXp3(): conditionally received message #%d from mailbox %d\n",
             i, mbox_id);
      printf("        message = `%s', result = %d\n", buffer, result);
      count++;
   }
   printf("XXp3(): After loop, result is negative; result = %d\n", result);
   printf("XXp3(): received %d good-bye messages from mailbox %d\n",
          count, mbox_id);

   quit(-4);
   return 0;
} /* XXp3 */
