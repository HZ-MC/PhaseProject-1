/* checking for release: 3 instances of XXp2 send messages to a zero-slot
 * mailbox, which causes them to block. XXp4 then releases the mailbox.
 * XXp4 is lower priority than the 3 blocked processes.
 */


#include <stdio.h>
#include <strings.h>
#include <usloss.h>
#include <phase1.h>
#include <phase2.h>


int XXp2(char *);
int XXp3(char *);
int XXp4(char *);
char buf[256];


int mbox_id;


int start2(char *arg)
{
   int kid_status, kidpid;
   int result;
   char buffer[]="hello";

   printf("start2(): started\n");
   mbox_id  = MboxCreate(0, 50);
   printf("start2(): MboxCreate returned id = %d\n", mbox_id);

   kidpid   = fork1("XXp2a", XXp2, "XXp2a", 2 * USLOSS_MIN_STACK, 2);
   kidpid   = fork1("XXp2b", XXp2, "XXp2b", 2 * USLOSS_MIN_STACK, 2);
   kidpid   = fork1("XXp2c", XXp2, "XXp2c", 2 * USLOSS_MIN_STACK, 2);

   kidpid   = fork1("XXp3",  XXp3, NULL,    2 * USLOSS_MIN_STACK, 4);

   kidpid = join(&kid_status);
   printf("start2(): joined with kid %d, status = %d\n", kidpid, kid_status);

   kidpid = join(&kid_status);
   printf("start2(): joined with kid %d, status = %d\n", kidpid, kid_status);

   kidpid = join(&kid_status);
   printf("start2(): joined with kid %d, status = %d\n", kidpid, kid_status);

   kidpid = join(&kid_status);
   printf("start2(): joined with kid %d, status = %d\n", kidpid, kid_status);

   result = MboxCondSend(mbox_id, buffer, strlen(buffer)+1);

   if (result == -1)
      printf("failed to send to released mailbox ... success\n");
   else
      printf("test failed result = %d\n",result);

   quit(0);
   return 0; /* so gcc will not complain about its absence... */
} /* start2 */


int XXp2(char *arg)
{
   int result;
   char buffer[20];

   sprintf(buffer, "hello from %s", arg);
   printf("%s(): sending message '%s' to mailbox %d, msg_size = %d\n",
          arg, buffer, mbox_id, strlen(buffer)+1);
   result = MboxSend(mbox_id, buffer, strlen(buffer)+1);
   printf("%s(): after send of message '%s', result = %d\n",
          arg, buffer, result);

   if (result == -3)
      printf("%s(): zap'd by MboxSend() call\n", arg);

   quit(-3);
   return 0;

} /* XXp2 */


int XXp3(char *arg)
{
   int result;

   printf("XXp3(): started\n");

   result = MboxRelease(mbox_id);

   printf("XXp3(): MboxRelease returned %d\n", result);

   quit(-4);
   return 0;
} /* XXp3 */


int XXp4(char *arg)
{

   printf("XXp4(): started and quitting\n");
   quit(-4);

   return 0;
} /* XXp4 */
