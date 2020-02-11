# PhaseProject

Team: Hassan Martinez, Javier Felix, Mark Festejo

GitHub Repo for developing and deployment of phase projects for the class CSCV 452.

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
