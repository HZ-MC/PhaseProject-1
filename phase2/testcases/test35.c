/* test border cases on MboxCreate */
#include <stdio.h>
#include <usloss.h>
#include <phase1.h>
#include <phase2.h>


int start2(char *arg)
{
   int mbox_id;
   char buffer[80];
   int result;

   printf("start2(): started, creating with slot_size too big\n");
   mbox_id = MboxCreate(10, MAX_MESSAGE+1);
   printf("start2(): MailBoxCreate returned id = %d\n", mbox_id);
   if (mbox_id == -1) {
      printf("start2(): passed...\n");
   }
   else {
      printf("start2(): failed...\n");
      quit(0);
   }

   printf("start2(): started, creating with slot_size -1\n");
   mbox_id = MboxCreate(10, -1);
   printf("start2(): MailBoxCreate returned id = %d\n", mbox_id);
   if (mbox_id == -1) {
      printf("start2(): passed...\n");
   }
   else {
      printf("start2(): failed...\n");
      quit(0);
   }

   printf("start2(): started, creating with slot_num -1\n");
   mbox_id = MboxCreate(-1, 10);
   printf("start2(): MailBoxCreate returned id = %d\n", mbox_id);
   if (mbox_id == -1) {
      printf("start2(): passed...\n");
   }
   else {
      printf("start2(): failed...\n");
      quit(0);
   }

   printf("start2(): started creating with slot_num 2, slot_size 0\n");
   mbox_id = MboxCreate(2, 0);
   printf("start2(): MailBoxCreate returned id = %d\n", mbox_id);
   if (mbox_id == 7 || mbox_id == 0) {  // 7 for groups, 0 for solo
      printf("start2(): passed...\n");
   }
   else {
      printf("start2(): failed...\n");
      quit(0);
   }

   printf("start2(): sending message to mailbox %d\n", mbox_id);
   result = MboxSend(mbox_id, "hello there", 0);
   printf("start2(): after send of message, result = %d\n", result);

   if (result == 0) {
      printf("start2(): passed...\n");
   }
   else {
      printf("start2(): failed...\n");
      quit(0);
   }

   printf("start2(): sending message to mailbox %d\n", mbox_id);
   result = MboxSend(mbox_id, NULL, 0);
   printf("start2(): after send of message, result = %d\n", result);

   if (result == 0) {
      printf("start2(): passed...\n");
   }
   else {
      printf("start2(): failed...\n");
      quit(0);
   }

   printf("start2(): attempting to receive message from mailbox %d\n", mbox_id);
   result = MboxReceive(mbox_id, NULL, 0);
   printf("start2(): after receive of message, result = %d\n", result);
   if (result == 0) {
      printf("start2(): passed...\n");
   }
   else {
      printf("start2(): failed...\n");
      quit(0);
   }

   printf("start2(): attempting to receive message from mailbox %d\n", mbox_id);
   result = MboxReceive(mbox_id, buffer, 80);
   printf("start2(): after receive of message, result = %d\n", result);
   if (result == 0) {
      printf("start2(): passed...\n");
   }
   else {
      printf("start2(): failed...\n");
      quit(0);
   }

   quit(0);
   return 0; /* so gcc will not complain about its absence... */

} /* start2 */
