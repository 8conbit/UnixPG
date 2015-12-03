struct pollfd{
	int fd;
	short events;
	short revents;
}
sys/poll.h에 정의되어 있는 pollfd 구조체다.
이 구조체 내의 fd에 모니터링할 file descriptor를 지정한다.
즉 file descriptor의 수만큼 pollfd 구조체가 필요하다. 따라서 배열로(*)

struct dvpoll{
	struct pollfd* dp_fds;
	int dp_nfds;
	int dp_timeout;
}

위에서 만든 pollfd구조체를 dp_fds에 할당한다.
dp_nfds의 n은 number. 즉 dp_nfds는 size of buffer in terms of the number of -pollfd- entries it contains.
dp_timeout = 0이면 ioctl은 즉시 return,
		   -1이면 ioctl은 events 발생할 때 까지 block.

time out으로 return하면 ioctl은 0을 return.
ioctl이 오류나면 -1을 return
ioctl이 성공하면 the number of valid -pollfd- entries를 리턴.


int fd = open("/dev/poll", O_RDWR);
ssize_t n = write(int fd, struct pollfd buf[], int bufsize);
int n = ioctl(int fd, DP_POLL, struct dvpoll* arg);
int n = ioctl(int fd, DP_ISPOLLED, struct pollfd* pfd);

write는 성공시 write한 byte 수, 실패시 -1을 return.

DP_POLL ioctl은 poll events가 일어났는지 검색하기 위해서 쓴다.
이경우 revents는 사용하지 않는다.
********&dopoll should be used to save ioctl return informaiton.
if input the "struct pollfd buf[]" parameter at write(), *******MUST deliver buf variable to need to update(add, remove) 
ex)
		tmp_pfd.fd =dopoll.dp_fds[i].fd;
        tmp_pfd.events = POLLREMOVE;
        tmp_pfd.revents = 0;

        if(write(wfd, &tmp_pfd, sizeof(struct pollfd)) != sizeof(struct pollfd))
-----------------------------------------------------------

DP_ISPOLLED ioctl은 해당 fd가 pfd에 포함되어있는지 확인하기 위해 쓴다. 

이경우 revents는 currently polled events 정보를 나타낸다.


MUST!!! you should remove a closed file descriptor from a monitored poll set. 

