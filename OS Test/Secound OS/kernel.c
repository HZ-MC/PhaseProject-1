#include "include/kb.h"

kmain()
{
       clearScreen();
       print("Welcome to NIDOS operating system\nPlease enter a command");
       printch('\n');
       print("NIDOS> ");
       while (1)
       {
            string ch = readStr();
            print("\n");
            print(ch);
            if(strEql(ch,"cmd"))
            {
                    print("\nYou are allready in cmd\n");
            }
            else if(strEql(ch,"clear"))
            {
                    clearScreen();
                    print("NIDOS> ");
            }
            else
            {
                    print("Bad command!");
            }
       }

}
