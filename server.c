#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <signal.h>
#include <siginfo.h>


#define BUF_SIZE 1024
#define TRUE 1
#define FALSE 0
void error_handling(char *message);

int main(int argc, char *argv[]){
	int serv_sock, clnt_sock;
	char message[BUF_SIZE];
	char quit[] = "----QUIT\n";
	int str_len;
	struct sockaddr_in serv_addr, clnt_addr;
	socklen_t clnt_addr_size, optlen;

	pid_t pid;

	int option;

	struct sigaction act;

	if(argc!=2){
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}
	serv_sock = socket(PF_INET, SOCK_STREAM, 0);
	if(serv_sock == -1 )
		error_handling("socket() error");

	optlen=sizeof(option);
	option=TRUE;
	setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, (void*)&option, optlen);
	//REUSEADDR. time-wait solv.
	
	memset(&serv_addr, 0, sizeof(serv_addr));//init
	serv_addr.sin_family=AF_INET;
	serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);//INADDR_ANY = serv ip
	serv_addr.sin_port=htons(atoi(argv[1])); //port to network endian

	if(bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
		error_handling("bind() error");

	if(listen(serv_sock, 5) == -1)//keep on listening
		error_handling("listen() error");
	
	clnt_addr_size=sizeof(clnt_addr);

	sigemptyset(&act.sa_mask);//signal handle
	act.sa_flags = SA_NOCLDWAIT;
	act.sa_handler = child_handler;
	if(sigaction(SIGCHLD, &act, (struct sigaction *)NULL) < 0){//걍 종료될수도??
		error_hanlder("sigaction error");
		exit(1);
	}

	while(1){ // 종료조건		
		clnt_sock=accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size); 

		if(clnt_sock== -1)
			error_handling("accept() error");
		else
			printf("Connect NEW Client");
		
		switch(pid = fork()){
			case -1: //fork failed
				error_handling("fork failed");
				break;
			case 0: //child
				close(serv_sock);
				while((str_len = read(clnt_sock, message, BUF_SIZE))!=0)
					write(clnt_sock, message, str_len);
				write(clnt_sock, quit, sizeof(quit));
				close(clnt_sock);
				break;
			default :	//parent
				close(clnt_sock);
				break;
		}
	}
	if(pid != 0)
		close(serv_sock);
	return 0;
}

void child_handler(int signo){//child exit
	int status;
	psignal(signo, "Receive Signal:");
	//그냥 wait 쓰면 자식프로세스가 없으면 blocking에 들어가서 accept나 listen 못함.
	while((pid = waitpid(-1, &status, WNOHNG)) > 0)
		printf("success child quit and status %d", status);
		
}
void error_handling(char *message){
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}


