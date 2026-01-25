#ifndef FORK_H
#define FORK_H
/* Can only turn ZOMEBIE_CHILD or ORPHAN_CHILD on */

/* Parent doesnot call wait/waitpid[wait until the end of child process] in time before the end of child process */
/* => child process is a Zombie */
#define ZOMEBIE_CHILD 1

/* Parent dies before the end of child process */
/* => child process is an Orphan, adopted by init/systemd */
/* ps -o ppid= -p <pid>: check the parent pid */
/* [HERE]: adopted by /lib/systemd/systemd by PID 1*/
#define ORPHAN_CHILD 0

/* Can replace SIGCHLD with values define in 'kill -l' */
/* In this case, the child process ended, it will raise signal to call func
    or can usee 'kill -17 <child PID/PID>' */
/* SIGKILL cannot be happend in this test due to the end of main.c */
#define SIGNAL_USED 1

#endif