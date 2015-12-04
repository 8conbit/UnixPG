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


int main(int argc, char *argv[]){
	int serv_sock;
	int clnt_sock[MAXCLNT-1];
	char message[BUF_SIZE];
	char quit[] = "----QUIT\n";
	int str_len;
	struct sockaddr_in clnt_addr;
	socklen_t addr_size;

	int i, j, num_ret, wfd = 0;
	struct pollfd* pollfd = NULL;
	struct dvpoll dopoll;
	int num_clnt=0;
	

	struct pollfd tmp_pfd;

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
	
	tmp_pfd.fd = serv_sock;
	tmp_pfd.events = POLLIN;
	tmp_pfd.revents =0;

	
	if(write(wfd, &tmp_pfd, sizeof(struct pollfd)) != sizeof(struct pollfd)){
		close(wfd); 
		free(pollfd);
		error_handler("write pollfd to wfd(file descriptor of /dev/poll) error");
	}//enroll socket fds to /dev/poll

	dopoll.dp_timeout = -1;//wait serv listen event, clnt rcv event
	dopoll.dp_fds = pollfd;
	dopoll.dp_nfds = MAXCLNT;


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
				clnt_sock[num_clnt]=accept(serv_sock, (struct sockaddr*)&clnt_addr, &addr_size);
				if(clnt_sock[num_clnt]== -1){
   	       			close(clnt_sock[num_clnt]);
					printf("accept error, but keep on\n");
					continue;
				}
				
				printf("[Accept] clnt sock : %d\n", clnt_sock[num_clnt]);
				tmp_pfd.fd = clnt_sock[num_clnt]; 
				tmp_pfd.events = POLLIN; //when disconnect, POLLREMOVE
				tmp_pfd.revents = 0;
				
				if(write(wfd, &tmp_pfd, sizeof(struct pollfd)) != sizeof(struct pollfd)){
					close(wfd);
					free(pollfd);
					error_handler("write pollfd to wfd(file descriptor of /dev/poll) error");
				}
				num_clnt++;
				printf("num_clnt = %d\n", num_clnt);

			}
			else{ // clnt rcv event
				printf("[Recieve]----- i = [%d]\n", i);
				str_len = read(dopoll.dp_fds[i].fd, message, BUF_SIZE);
				
				if(str_len == 0){
					write(dopoll.dp_fds[i].fd, quit, sizeof(quit));	

				
					printf("**********************Disconnect Client %d\n", dopoll.dp_fds[i].fd);
					tmp_pfd.fd = dopoll.dp_fds[i].fd;	
					tmp_pfd.events = POLLREMOVE; 
					tmp_pfd.revents = 0;

					for(j = 0; j < num_clnt; j++) 
						if(dopoll.dp_fds[i].fd == clnt_sock[j]) break;
										
					if(write(wfd, &tmp_pfd, sizeof(struct pollfd)) != sizeof(struct pollfd)){
						close(wfd);
						free(pollfd);
						error_handler("write pollfd to wfd(file descriptor of /dev/poll) error");
					}
					close(dopoll.dp_fds[i].fd);
					clnt_sock[j] = clnt_sock[--num_clnt]; //sort

				}
				else{//if i don't num_clnt--, will send msg multiply
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

void error_handler(char *message){
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}


