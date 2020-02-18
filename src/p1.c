#include "usloss.h"
#define DEBUG 0
extern int debugflag;

/*
    Every f(x) will print to the console the PID if debugflag is set to '0' except check_io.
    These f(x) are need it by sentinel() in order to work properly.
*/

// Print to the console fork PID.
void p1_fork(int pid)
{
  if(DEBUG && debugflag)
  {
    console("p1_fork() called: pid = %d\n", pid);
  }
} // p1_fork

// Print to the console the old and new PID switch.
void p1_switch(int old, int new)
{
  if(DEBUG && debugflag)
  {
    console("p1_switch() called: old = %d, new = %d\n", old, new);
  }
} // p1_switch

// Print to the console the quit PID.
void p1_quit(int pid)
{
  if(DEBUG && debugflag)
  {
    console("p1_quit() called: pid = %d\n", pid);
  }
} // p1_quit

// Dummy f(x) for now but needs to be updated in phase2.
int check_io()
{
  return 0;
} // check_io
