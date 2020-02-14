/*
    Example of how to use the Unix (POSIX) fork() system call to create
    new processes inside a loop -- and the rapid growth in the number
    of processes created.  Keep the loop small!!

    To compile (assuming the file is named "unixfork2.c"):
          gcc -Wall -o unixfork2 unixfork2.c
          Then execute:
          unixfork2
 */

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int main (int argc, char *argv[])
{
   pid_t kidpid;
   int i;
   // Under the for loop each iteration will make the parent to remain with it child under a new fork().
   for (i = 0; i < 2; i++)
   {
      kidpid = fork();
      // If the kid PID fail then it terminates the loop.
      if (kidpid < 0)
      {
         fprintf(stderr, "fork failed!\n");
         exit(1);
      }
      if (kidpid == 0)
      {
         printf("I am the child, pid = %d\n", (int)getpid());
         sleep(5);  // sleep() is a f(x) to send the child PID to sleep 5ms.
      }
      else
      {
         printf("I am the parent, pid = %d, my child is %d\n",
                (int)getpid(), (int)kidpid);
      }
   }

   printf("Last printf, pid = %d\n", (int)getpid());
   return 0;

} /* main */
