gcc polls.c -o ps -DUSE_EPOLL
gcc client.c -o c

=== select() vs poll() vs epoll() ===
[4]
Max fds:	1024		Large		very large
T complexity:	On		On		O1
Trigger:	POLL		POLL		POLL + EDGE
Window?:	Y		Y(adaptive)	N, APIs - epoll_create, epoll_ctl, epoll_wait


=== Definitions ===
 - one thread - wait - multi fds which one is ready for r/w - without blocking