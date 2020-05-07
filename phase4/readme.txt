Members: Javier Felix, Mark Festejo Hassan Martinez

System used to develop phase 4: Mac OS and UbuntuOS 

Notes: 

- On UbuntuOS, used GDB within the terminal for debugging
- In regards to the script... the script works for compiling and making all the test cases however, it is unable to print out to the text file.
- Test cases were ran without the script within the terminal via "./test##"
- All test case outputs were copy/pasted from the terminal after running each via "./test##"
- With GDB, the test cases were stepped through to ensure the program functions as it should with thecorrect values for termination.
- It seems on the final line of termination on every testcase, it seems get stuck or idle after producing the output.

Test Case Evaluation:

Test 00 -- Passed

- Able to compile the test case and prints out similar output to Professor Xu's output within the terminal, the only differences are the clock times.


Test 01 -- Passed

- Able to compile the test case and prints out similar output to Professor Xu's output within the terminal
- Professor Xu's output has different clocktimes.
- The wait, pid, and status lines also contain differences.
- The wait return value is the same as Professor Xu's, as 0.
- The status value is also the same as Professor Xu's.
- pid is the difference however, it follows a similar pattern, decrementing by 1.

Test 02 -- Passed

- Able to compile the test case and prints out similar output to Professor Xu's output within the terminal.

Test 03 -- Passed

- Able to compile the test case and prints out similar output to Professor Xu's output within the terminal.

Test 04 -- Passed

- Able to compile the test case and prints out similar output to Professor Xu's output within the terminal.

Test 05 -- Passed

- Able to compile the test case and prints out similar output to Professor Xu's output within the terminal.

Test 06 -- Passed

- Able to compile the test case and prints out similar output to Professor Xu's output within the terminal.

Test 07 -- Passed

- Able to compile the test case and prints out similar output to Professor Xu's output within the terminal.

Test 08 -- Passed

- Able to compile the test case and prints out similar output to Professor Xu's output within the terminal.

Test 09 -- Passed

- Able to compile the test case and prints out similar output to Professor Xu's output within the terminal.
- The process quit with status lines contain differences on the process value however, the seem to follow the same pattern of being random processes quitting but the status of each line is the same as Professor Xu's.

Test 10 -- Passed

-Similar evaluation to test09...
- Able to compile the test case and prints out similar output to Professor Xu's output within the terminal.
- The process quit with status lines contain differences on the process value however, the seem to follow the same pattern.
- Differing from the pattern of test09, each process quits in incrementing order with a total of 8 children

Test 11 -- Passed(?)

- The test case does what its supposed to by having 6 processes write 3 to disk0 and 3 to disk1 but like the other processes in previous test cases, they have different pids.
- The outputs seem to be out of order but they are somewhat producing similar results as Professor Xu's.

Test 12 -- Passed

- Able to compile the test case and prints out similar output to Professor Xu's output within the terminal, the only differences are the clock times.

