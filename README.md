PhaseProject
===

Team: Hassan Martinez, Javier Felix, Mark Festejo
---

* OS Test   = Has some small OS examples different from the project.
* Output    = Has all the testcases into one to test everything on the project.  
* debugging = Tools for debugging under the project.
* fork      = Has the fork simulation files with some additional comments by Hassan.  
* obj       = Should have the compile objects to be link by the linker list.  
* phase1    = Has the provided_phase1 by the professor to start over again.  
* src       = Sources Code that we are working with.  
* usloss    = Has the usloss for installation.  

/********************************* To do list *********************************/  



/********************************* Done list *********************************/ 

* int blockMe(int new_status){}
* int readCurrStarTime(void){}
* void timeSlice(void){}
* int readTime(void){}
* int insertReadyList(proc_ptr){}
* int removeReadyList(proc_ptr){}
* void clockHandler(){}
* int unblockProc(int pid){}
* int getpid(){}
* int isZapped(){}

/******************************* Flow of control ******************************/  
  
main        // USLOSS  
  |  
  V  
startup     // Javier  
  |  
  V  
fork1       // Javier  
  |  
  V  
dispatcher  // Hassan  
  |  
  V         // functions that may quit or go to start/testcase#  
launch      quit()    join()    zap()                             // Hassan  
  |  
  V         // testcase test blocks of code for proper behavior. Returning either (-2), (-1), (>=0)  
start 1     testcase#   // Mark  
  |  
  V  
sentinel                // Mark      
  |  
  V  
deadclock   halt(1)   halt(0)     
  |  
  V  
finish  

/******************************* Flow of control ******************************/  

GitHub Repo for developing and deployment of phase projects for the class CSCV 452.  
