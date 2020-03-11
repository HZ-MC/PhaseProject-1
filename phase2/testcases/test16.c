/* tests for exceeding the number of slots. start2 creates mailboxes whose
 * total slots will exceed the system limit. start2 then starts doing
 * conditional sends to each slot of each mailbox until the return code
 * of conditional send comes back as -2
 */

#include <stdio.h>
#include <usloss.h>
#include <phase1.h>
#include <phase2.h>

int mboxids[50];
int start2(char *arg)
{
   int i, j, result;

   printf("start2(): started, trying to exceed mailslots...\n");
   for (i = 0; i < 50; i++) {
      mboxids[i] = MboxCreate(55, 50);
      if (mboxids[i] < 0)
         printf("start2(): MailBoxCreate returned id less than zero, id = %d\n",
                 mboxids[i]);
   }

   for (i=0; i<50; i++) {
      for (j=0; j<55; j++) {
         result = MboxCondSend(mboxids[i], NULL, 0);
            if (result == -2) {
               printf("finally the slots are over at mailbox %d and slot %d\n",
                       i, j);
               quit(0);
               return(0);
            }
      }
   }

   quit(0);
   return 0; /* so gcc will not complain about its absence... */
} /* start2 */
