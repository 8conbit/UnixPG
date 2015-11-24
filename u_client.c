#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 1024
void error_handling(char *message);

int main(int argc, char* argv[]){
	int sock;
	struct sockaddr_in serv_addr;
	char message[BUF_SIZE];
	int snd_len, rcv_len, rcv_buf;
	char serv_ip[] = "125.129.123.2";
	int serv_port = 9797;

	sock = socket(PF_INET, SOCK_STREAM, 0);
	if(sock == -1)
		error_handling("socket() error");
	
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family=AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(serv_ip);
	serv_addr.sin_port = htons(serv_port);

	if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
		error_handling("connect() error!");
	else
		puts("Connect...");

	while(1){
		fputs("Input message(/quit to quit): ", stdout);
		fgets(message, BUF_SIZE, stdin);
		if(!strcmp(message, "/quit\n"))
				break;

		snd_len = write(sock, message, strlen(message));			
		rcv_len = 0;
		while(snd_len > rcv_len){
			rcv_buf = read(sock, (message+rcv_len), BUF_SIZE-1);

			if(rcv_buf == -1)
				error_handling("read() error!");
			else
				rcv_len+=rcv_buf;
		}

		message[rcv_len] = '\0';		
		printf("Message from server : %s \n", message);
	}

	shutdown(sock, SHUT_WR); //half-close
	read(sock, message, BUF_SIZE);
	printf("%s", message);
	close(sock);
	return 0;
}

void error_handling(char *message){
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
