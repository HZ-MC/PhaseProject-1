# PhaseProject

Team: Hassan Martinez, Javier Felix, Mark Festejo

GitHub Repo for developing and deployment of phase projects for the class CSCV 452.

/******************************* Flow of control ******************************/

main        // USLOSS
  |
  V
startup
  |
  V
fork1
  |
  V
dispatcher
  |
  V         // functions that may quit or go to start/testcase#
launch      quit()    join()    zap()
  |
  V         // testcase test blocks of code for proper behavior. Returning either (-2), (-1), (>=0)
start 1     testcase#
  |
  V
sentinel
  |
  V
deadclock   halt(1)   halt(0)
  |
  V
finish

/******************************* Flow of control ******************************/
