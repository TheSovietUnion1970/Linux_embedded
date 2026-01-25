#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include "sig.h"
 
void sigint_handler(int num)
{
	printf("\nsigint_handler: %d\n", num);
#if (TERMINATE_END)
	exit(EXIT_SUCCESS);
#endif
}


void sigterm_handler(int num)
{
    printf("sigterm_handler: %d\n", num);
#if (TERMINATE_END)
	exit(EXIT_SUCCESS);
#endif
}
 
int main()
{
    // register signal (Ctrl C)
  	if (signal(SIGINT, sigint_handler) == SIG_ERR) {
		fprintf(stderr, "Cannot handle SIGINT\n");
		exit(EXIT_FAILURE);
	}
    // terminate process gracefully
  	if (signal(SIGTERM, sigterm_handler) == SIG_ERR) {
		fprintf(stderr, "Cannot handle SIGTERM\n");
		exit(EXIT_FAILURE);
	}
    // SIGKILL can not be re-requested -> terminate process forcefully

    printf("process ID: %d\n", getpid());

#if (BLOCKING_SIG)
    sigset_t new_set, old_set;
    
    sigemptyset(&old_set);
    sigemptyset(&new_set);
    sigaddset(&new_set, SIGINT | SIGTERM);  // Add SIGINT to new_set

    // Block SIGINT
    if (sigprocmask(SIG_BLOCK, &new_set, &old_set) == 0) {
        printf("SIGINT or SIGTERM is now blocked. Try pressing Ctrl+C...\n");
    }

    // Sleep while SIGINT is blocked
    sleep(5);

    // Unblock SIGINT (restore old mask)
    printf("Unblocking SIGINT and SIGTERM now.\n");
    sigprocmask(SIG_SETMASK, &old_set, NULL);
#endif

	while (1)
	{
		// do nothing.
	 	printf("hello\n");
		sleep(2);
	}

}