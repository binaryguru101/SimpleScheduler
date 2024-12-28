**HOW TO RUN**

--> Compile in terminal using 'gcc -o shell s.out' 
--> After compilation type './shell <NCPU> <TSLICE>'
--> submit files like 1.out, 2.out etc in the following manner 

- submit ./1.out
- submit ./2.out

--> The Process ID of each process will be mentioned
--> Type 'run'
--> By default the priority is 1 for each process

--->The scheduler will receive a signal and start running 

--> The fib values will be printed for the files you submitted 

--> When the processes terminates, it will say "NO PROCESSES REMAINING"

--> 'exit' will terminate the shell and the scheduler

--> 'set' will let you enter a new TSLICE for the program

--> 'show' will let you see the items present in the P.Queue

--> 'history' does it's usual task and prints the history of the commands

--> **EXAMPLE OF WHAT PRINTS WHEN SCHEDULER TERMINATES"

History:
Number:  PID  FNAME  Priority  t_exec  t_wait
1        1554  ./2.out    1    10.000     0      
2        1534  ./1.out    1    10.000     0      