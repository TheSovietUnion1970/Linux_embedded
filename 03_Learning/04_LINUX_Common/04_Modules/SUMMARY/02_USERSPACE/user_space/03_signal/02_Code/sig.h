#ifndef SIG_H
#define SIG_H

/* TERMINATE_END is used to end the process that uses SIGINT and SIGTERM */
/* else can not terminated by SIGINT or SIGTERM -> must use SIGKILL */
#define TERMINATE_END 1

/* If set, SIGINT or SIGTERM is blocked for 5s */
#define BLOCKING_SIG 1

#endif