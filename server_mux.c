#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/devpoll.h>
#include <sys/poll.h>


#define BUF_SIZE 1024
#define MAXCLNT 255 
#define PORT 9797

void error_handler(char *message);
int create_serv_sock();
int devpoll_add(int sock);
int devpoll_close(int sock);

int wfd;

int main(int argc, char *argv[]){
	int serv_sock;
	int clnt_sock[MAXCLNT-1];
	char message[BUF_SIZE];
	char quit[] = "----QUIT\n";
	int str_len;
	struct sockaddr_in clnt_addr[MAXCLNT-1];
	socklen_t addr_size;

	int i, j, num_ret;
	struct pollfd* pollfd = NULL;
	struct dvpoll dopoll;
	int num_clnt=0;
	
	addr_size = sizeof(clnt_addr);

	serv_sock = create_serv_sock();
	if(serv_sock == -1 )
		error_handler("socket() error");


	if((wfd = open("/dev/poll", O_RDWR)) < 0)
		error_handler("/dev/poll error");

	pollfd = (struct pollfd*)malloc(sizeof(struct pollfd)*MAXCLNT);	
	if(pollfd == NULL){
		close(wfd);
		error_handler("malloc error");
	}
	dopoll.dp_timeout = -1;//wait serv listen event, clnt rcv event
	dopoll.dp_fds = pollfd;
	dopoll.dp_nfds = MAXCLNT;
	
	if(devpoll_add(serv_sock) == -1){
		close(wfd); 
		free(pollfd);
		error_handler("write pollfd to wfd(file descriptor of /dev/poll) error");
	}


	while(1){  
		printf("wait ioctl...\n");
		num_ret = ioctl(wfd, DP_POLL, &dopoll);
		printf("num_ret = %d, ", num_ret);
		if(num_ret == -1){
			close(wfd);
			free(pollfd);
			error_handler("ioctl DP_POLL failed");
		}

		for(i = 0; i < num_ret; i++){
			if((dopoll.dp_fds[i].fd == serv_sock) && (num_clnt < MAXCLNT-1)){ //serv listen
				clnt_sock[num_clnt]=accept(serv_sock, (struct sockaddr*)&clnt_addr[num_clnt], &addr_size);
				if(clnt_sock[num_clnt]== -1){
   	       			close(clnt_sock[num_clnt]);
					printf("accept error, but keep on\n");
					continue;
				}
				
				printf("[Accept] clnt sock : %d, addr : %s\n", clnt_sock[num_clnt], inet_ntoa(clnt_addr[num_clnt].sin_addr));

				if(devpoll_add(clnt_sock[num_clnt++]) == -1){
					close(wfd);
					free(pollfd);
					error_handler("write pollfd to wfd(file descriptor of /dev/poll) error");
				}
				printf("num_clnt = %d\n", num_clnt);

			}
			else{ // clnt rcv event
				printf("[Recieve]----- i = [%d]\n", i);
				str_len = read(dopoll.dp_fds[i].fd, message, BUF_SIZE);
				
				if(str_len == 0){
					write(dopoll.dp_fds[i].fd, quit, sizeof(quit));	
					printf("**********************Disconnect Client %d\n", dopoll.dp_fds[i].fd);
					
					for(j = 0; j < num_clnt; j++) 
						if(dopoll.dp_fds[i].fd == clnt_sock[j]) break;
										
					if(devpoll_close(dopoll.dp_fds[i].fd) == -1){
						close(wfd);
						free(pollfd);
						error_handler("write pollfd to wfd(file descriptor of /dev/poll) error");
					}

					close(dopoll.dp_fds[i].fd);
					num_clnt--;
					clnt_sock[j] = clnt_sock[num_clnt]; //sort
					clnt_addr[j] = clnt_addr[num_clnt];

				}
				else{
					for(j = 0; j < num_clnt; j++){
						if(dopoll.dp_fds[i].fd != clnt_sock[j])
							write(clnt_sock[j], message, str_len);
					}
				}
			}
		}	
	}
	close(wfd);
	free(pollfd);
	close(serv_sock);
	return 0;
}

int create_serv_sock(){
	int serv_sock;
	struct sockaddr_in serv_addr;
	socklen_t optlen;
	int option;


	serv_sock = socket(PF_INET, SOCK_STREAM, 0);
	if(serv_sock == -1 )
		error_handler("socket() error");
	
	optlen=sizeof(option);
	option=1;
	setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, (void*)&option, optlen);
	//REUSEADDR. time-wait solv.
	
	memset(&serv_addr, 0, sizeof(serv_addr));//init
	serv_addr.sin_family=AF_INET;
	serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);//INADDR_ANY = serv ip
	serv_addr.sin_port=htons(PORT); //port to network endian

	if(bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
		error_handler("bind() error");

	if(listen(serv_sock, 5) == -1)//keep on listening
		error_handler("listen() error");
	
	return serv_sock;
}

void devpoll_init(){

}

int devpoll_add(int sock){
	struct pollfd tmp_pfd;

	tmp_pfd.fd = sock;
	tmp_pfd.events = POLLIN;
	tmp_pfd.revents =0;
	
	if(write(wfd, &tmp_pfd, sizeof(struct pollfd)) != sizeof(struct pollfd))
		return -1;
	else
		return 0;
}


int devpoll_close(int sock){
	struct pollfd tmp_pfd;

	tmp_pfd.fd = sock;
	tmp_pfd.events = POLLREMOVE;
	tmp_pfd.revents =0;

	if(write(wfd, &tmp_pfd, sizeof(struct pollfd)) != sizeof(struct pollfd))
		return -1;
	else
		return 0;
}


void error_handler(char *message){
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}


