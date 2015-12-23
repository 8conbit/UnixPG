
#include <sys/devpoll.h>
#include <sys/poll.h>
#include "8condevpoll.h"

#define MAXCLNT 255

int devpoll_init(struct dvpoll *ppoll) {
	struct pollfd* pollfd;
	pollfd = (struct pollfd*)malloc(sizeof(struct pollfd)*MAXCLNT);
	if (pollfd == NULL) {
		return -1;
	}

	ppoll->dp_timeout = -1;//wait serv listen event, clnt rcv event
	ppoll->dp_fds = pollfd;
	ppoll->dp_nfds = MAXCLNT;

	return 0;

}


int devpoll_add(int sock) {
	struct pollfd tmp_pfd;

	tmp_pfd.fd = sock;
	tmp_pfd.events = POLLIN;
	tmp_pfd.revents = 0;

	if (write(wfd, &tmp_pfd, sizeof(struct pollfd)) != sizeof(struct pollfd))
		return -1;
	else
		return 0;

}


int devpoll_close(int sock) {
	struct pollfd tmp_pfd;

	tmp_pfd.fd = sock;
	tmp_pfd.events = POLLREMOVE;
	tmp_pfd.revents = 0;

	if (write(wfd, &tmp_pfd, sizeof(struct pollfd)) != sizeof(struct pollfd))
		return -1;
	else {
		close(sock);
		return 0;
	}
}



