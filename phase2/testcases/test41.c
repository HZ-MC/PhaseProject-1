/* Test of conditional send and conditional receive that does not rely
 * on a zero-slot mailbox.  Similar to test12.c, but uses a 1-slot mailbox
 * called "pause_mbox" instead of the 0-slot private_mbox.
 */

#include <stdio.h>
#include <strings.h>
#include <usloss.h>
#include <phase1.h>
#include <phase2.h>

int XXp1(char *);
int XXp3(char *);
char buf[256];

int mbox_id, pause_mbox;

int start2(char *arg)
{
   int kid_status, kidpid;

   printf("start2(): started\n");
   mbox_id = MboxCreate(5, 50);
   printf("start2(): MboxCreate returned id = %d\n", mbox_id);
   pause_mbox = MboxCreate(1, 50);
   printf("start2(): MboxCreate returned id = %d\n", pause_mbox);

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

   MboxSend(pause_mbox, NULL, 0);  // should block on mail box

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

   MboxReceive(pause_mbox, NULL, 0);

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
