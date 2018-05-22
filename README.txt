General Instructions:
---------------------
(1) We have included a sample input file with name "input.txt"
(2) The source code for Galileo Gen2 board is inside the folder galileo/ with its own Makefile
(3) If you want to run with PI-enabled mutexes, change lines 187, 192 and 193, by changing the attributes
    from PTHREAD_PRIO_NONE to PTHREAD_PRIO_INHERIT

Execution instructions:
-----------------------
(1) make	- Compiles the source code for respective architecture
(2) make run    - Runs the executable 

Parsing instructions:
----------------------
(1) The parser cannot handle multiple blankspaces after the arguments at the line end. 
    Hence each line should terminate with a '\n' immediately after the 
    final argument.
(2) If there are 5 lines, 6th line shouldn't contain any value including '\n'
