/* Example of how to use the Unix (POSIX) waitpid() system call to get
 * the status of children that have exited.  waitpid() can also report
 * on children that have abnormally terminated.
 *
 * To compile (assuming the file is named "unixfork3.c"):
 *    gcc -Wall -o unixfork3 unixfork3.c
 * Then execute:
 *    unixfork3
 */

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

int
main (int argc, char *argv[]) {

   pid_t kidpid[3], returnpid;
   int i, kid_status;

   for (i = 0; i < 3; i++) {
      kidpid[i] = fork();
      if (kidpid[i] < 0) {
         fprintf(stderr, "fork failed!\n");
         exit(1);
      }
      if (kidpid[i] == 0) {
         printf("I am the child, pid = %d\n", (int)getpid());
         sleep(5);
         printf("Child %d exiting...\n", (int)getpid());
         exit((int)getpid());
      }
      else {
         printf("I am the parent, pid = %d, my child is %d\n",
                (int)getpid(), (int)kidpid[i]);
      }
   }

   returnpid = waitpid(kidpid[2], &kid_status, 0);
   if (WIFEXITED(kid_status))
      printf("Parent: Child %d exit kid_status = %d\n", (int)returnpid, WEXITSTATUS(kid_status));
   else
      printf("Parent: Child %d did not use exit\n", (int)returnpid);

   for (i = 0; i < 2; i++) {
      returnpid = waitpid(-1, &kid_status, 0);
      if (WIFEXITED(kid_status))
         printf("Parent: Child %d exit kid_status = %d\n", (int)returnpid, WEXITSTATUS(kid_status));
      else
         printf("Parent: Child %d did not use exit\n", (int)returnpid);
   }

   printf("Last printf, pid = %d\n", (int)getpid());
   return 0;

} /* main */
