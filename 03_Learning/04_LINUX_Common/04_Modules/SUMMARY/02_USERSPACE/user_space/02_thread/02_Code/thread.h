#ifndef THREAD_H
#define THREAD_H
/* Turn THREAD_JOIN or THREAD_DETACH on */

#define MUTEX_USED 1

/* Check thread ID in handler */
#define THREAD_EQUAL 1

/* it will block the main program until the end of a thread */
#define THREAD_JOIN 1

/* it will detach the thread eventhough the thread is in progress */
/* In this case, eventhough thread1 uses 5s, but after 2s, pthread_detach will terminate the thread */
#define THREAD_DETACH 0

/* pthread_exit() -> use at the end of a thread (self-termination) */
/* pthread_cancel() -> terminate a thread (external termination) >< pthread_detach - not killing the thread
                                                                    = mark it as detached, OS will automatically release when it's done*/


/* === [Comdition variable] === */                                               
/* It will signal, lock is expected to on before as condition variable pthread_cond_wait will release lock (unlock)
                                                                       pthread_cond_signal will not */
/*
In this case, if thread 1 runs first, pthread_cond_wait is called first = thread 1 will unlock and wait other thread signals, thread 2 runs and signal, then
                                                            thread 1 can run the rest 
if thread 2 run first, pthread_cond_signal will NOT unlock and this signal is not effective, after thread 2 finishes, thread 1 runs and STUCKs at pthread_cond_wait
*/
#define COND_VAR 1

#endif