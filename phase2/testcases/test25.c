#include <stdio.h>
#include <usloss.h>
#include <phase1.h>
#include <phase2.h>


/* test overflowing the mailbox, releasing some, then overflowing again */


int start2(char *arg)
{
  int mbox_id;
  int i, result;


  printf("start2(): started, trying to create too many mailboxes...\n");
  /* 4+2+1 = 7, MAXMBOX-7,       10 - 7 = 3 (10-5= 5)*/
  for (i = 1; i <= MAXMBOX - 5; i++) {
    mbox_id = MboxCreate(10, 50);
    if (mbox_id < 0)
      printf("start2(): MailBoxCreate returned id less than zero, id = %d\n",
              mbox_id);
  }


  for(i = 0; i <6; i++){ //release 6 (10-15)
          printf("start2(): calling MboxRelease(%d)\n", i);


          result = MboxRelease(10+i);


          printf("start2(): calling MboxRelease(%d) returned %d\n", i, result);
  }


  //create 8 (overflowing 2)
  for (i = 0; i < 8; i++) {
    mbox_id = MboxCreate(10, 50);
    if (mbox_id < 0)
      printf("start2(): MailBoxCreate returned id less than zero, id = %d\n",
              mbox_id);
    else
        printf("start2(): MailBoxCreate returned id = %d\n", mbox_id);
  }


  quit(0);
  return 0; /* so gcc will not complain about its absence... */
}
