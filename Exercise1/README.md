Write a program which does the following:
* Create N processes. N to be taken as an argument.
* Each even process waits for a signal. Even by pid.
* Every odd process sends SIGUSR1 signal to one of the even processes created prior to
it. Even process is chosen randomly.
* When an even process receives more than M signals, it terminates itself after sending a
SIGTERM followed by SIGKILL to the last process which has sent SIGUSR1 to it. M is
taken as an argument.
* Everey process should print its pid, pid of the sending process, and the number of
signals received. Should print “Terminated Self” when exiting in case of even process. In
case of odd process, print “Terminated by <pid>”.
