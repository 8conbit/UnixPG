#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <signal.h>
#include <siginfo.h>
#include <sys/types.h>
#include <sys/wait.h>


#define BUF_SIZE 1024
#define PORT 9797;
void child_handler(int signo);
void error_handler(char *message);

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
	serv_addr.sin_port=htons(9797); //port to network endian

	if(bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
		error_handler("bind() error");

	if(listen(serv_sock, 5) == -1)//keep on listening
		error_handler("listen() error");
	
	clnt_addr_size=sizeof(clnt_addr);

	sigemptyset(&act.sa_mask);//signal handle
	act.sa_flags = 0; // ????? SA_NOCLDWAIT ???????
	act.sa_handler = child_handler;
	if(sigaction(SIGCHLD, &act, (struct sigaction *)NULL) == -1){
		error_handler("sigaction error");
		exit(1);
	}

	while(1){ //왜 accept와 listen을 parent에 넣지 않아도 되는지 잘 이해가... 
		clnt_sock=accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size); 

		if(clnt_sock== -1){
//quit시 여기를 한번 들렸다가는데 accept함수가 blocking되지 않기 때문에 돌아가고 있는 중에, child가 close하면서 소켓이 소멸할 때 accept가 -1을 리턴하는듯.
			continue;
		}
		else
			printf("Connect NEW Client\n");
		
		switch(pid = fork()){
			case -1: //fork failed
				error_handler("fork failed");
				break;
			case 0: //child
				close(serv_sock);
				while((str_len = read(clnt_sock, message, BUF_SIZE))!=0)
					write(clnt_sock, message, str_len);
				write(clnt_sock, quit, sizeof(quit));
				close(clnt_sock);
				return 0;	
			default :	//parent
				close(clnt_sock);
				break;
		}
	}
	close(serv_sock);
	return 0;
}

void child_handler(int signo){//child exit
	int status;
	pid_t pid;
	//그냥 wait 쓰면 자식프로세스가 없으면 blocking에 들어가서 accept나 listen 못함. SIGCHLD로 호출했으니 굳이 waitpid반복문 안써도 된다.
	pid = waitpid(-1, &status, WNOHANG);
	printf("%dchild exit.  status %d\n", pid, status);
		
}
void error_handler(char *message){
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}


