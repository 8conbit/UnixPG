
#include <sys/devpoll.h>
#include <sys/poll.h>

int devpoll_init(struct dvpoll* ppoll);
int devpoll_add(int sock);
int devpoll_close(int sock);