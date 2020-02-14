/*
    Example of how to use the Unix (POSIX) fork() system call to create
    new processes.

    To compile (assuming the file is named "unixfork1.c"):
          gcc -Wall -o unixfork1 unixfork1.c
          Then execute:
          unixfork1
 */

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int main (int argc, char *argv[])
{
   // Creates the process ID local variable for kid PID.
   pid_t kidpid;
   // fork() is a library f(x) to created a new process under the CPU.
   // If it passes then it becomes the child process of what was the previows one.
   kidpid = fork();
   // If the f(x) returns a negative value something went wrong.
   if (kidpid < 0)
   {  /* both parent and child execute this if statement! */
      fprintf(stderr, "fork failed!\n");
      // exit(1) is the f(x) to terminated the process that went wrong.
      exit(1);
   }
   // If the f(x) is 0 the child copy everything from the parent and print its PID.
   if (kidpid == 0)
   { /* both parent & child ask this, get different answers! The getpid() is a f(x) that gets the PID for the running instances.*/
      printf("I am the child, pid = %d\n", (int)getpid());
   }
   // If it is some random positive value then the process is by the parent.
   else
   {
      printf("I am the parent, pid = %d, my child is %d\n",
             (int)getpid(), (int)kidpid);
   }
   /* both parent & child execute this */
   printf("Last printf, pid = %d\n", (int)getpid());
   return 0;
} /* main */
