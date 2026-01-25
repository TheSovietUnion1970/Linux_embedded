#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include "fork.h"

int status;

#if (SIGNAL_USED)
void func(int signum)
{
    printf("Im in func(), signum = %d\n", signum);
    wait(NULL); // call to wait any child process ends
}
#endif

int main(int argc, char const *argv[])   /* Cấp phát stack frame cho hàm main() */
{
    /* code */
    pid_t child_pid;                /* Lưu trong stack frame của main() */
    int counter = 2;                /* Lưu trong frame của main() */

    printf("Initial counter: %d\n", counter);

    child_pid = fork();         
    if (child_pid >= 0) {
        if (0 == child_pid) {       /* Process con */
            printf("\nIm the child process, counter: %d\n", ++counter);
            printf("My PID is: %d, my parent PID is: %d\n", getpid(), getppid());
            sleep(2);
#if (ORPHAN_CHILD)
            while (1);
#endif
        } else {                    /* Process cha */
            printf("\nIm the parent process, counter: %d\n", ++counter);
            printf("My PID is: %d\n", getpid());
#if (SIGNAL_USED)
            signal(SIGCHLD, func);
            signal(SIGKILL, func);
#endif
#if (!ORPHAN_CHILD)
#if (!ZOMEBIE_CHILD)
            // waitpid(child_pid, &status, 0);
            wait(&status);
#endif
	        while (1);
#endif
        }
    } else {
        printf("fork() unsuccessfully\n");      // fork() return -1 nếu lỗi.
    }

    return 0;
}