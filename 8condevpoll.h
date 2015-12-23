#pragma once
#include <sys/devpoll.h>
#include <sys/poll.h>
#include <stdlib.h>
   
#ifndef NULL
#define NULL 0
#endif

#define MAXCLNT 255

int devpoll_init(struct dvpoll* ppoll);
int devpoll_add(int sock, int wfd);
int devpoll_close(int sock, int wfd);
