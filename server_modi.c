/*
AES block size is 128 ( fixed ) 16Byte... 16*64 = 1024
division,  AES
RSA, thread는 나중. clnt 끝난다음에.
char *은 sizeof하면 포인터 사이즈나옴
char []은 그냥 배열사이즈 잘나옴
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>
#include "8condevpoll.h"

#define BUF_SIZE 1024
#define PORT 9797
#define ASIZE 8 

void error_handler(char *message);
int create_serv_sock();

int clnt_sock[MAXCLNT - 1];
struct sockaddr_in clnt_addr[MAXCLNT - 1];
char *clnt_alias[MAXCLNT - 1];
static int num_clnt = 0;// num_clnt_sock
static int num_alias = 0;


int main(int argc, char *argv[]) {
	int serv_sock;
	char message[BUF_SIZE];
	int str_len;
	socklen_t addr_size;
	//devpoll
	int i, j, k, num_ret, efd, wfd;
	struct dvpoll dopoll;
	
	char dup[] = "/dp\n";
	char okmsg[] = "/ok\n";
	char list[4 + ((ASIZE+1)*MAXCLNT)] = "/li "; // /li + (ASIZE+" ")*num of clnt..... last " " is replaced NULL;
	char a_alias[4 + ASIZE + 1 + 1] = "/aa ";
	char temp[ASIZE+1];

	for (i = 0; i < MAXCLNT - 1; i++) {
		clnt_alias[i] = (char*)calloc(ASIZE + 1, sizeof(char)); //alias + NULL
	}

	addr_size = sizeof(clnt_addr);

	serv_sock = create_serv_sock();
	if (serv_sock == -1)
		error_handler("socket() error");

	if ((wfd = open("/dev/poll", O_RDWR)) < 0) {
		close(serv_sock); error_handler("/dev/poll error");
	}

	if (devpoll_init(&dopoll) == -1){
		close(serv_sock);  close(wfd); error_handler("devpoll_init malloc error");
	}

	if (devpoll_add(serv_sock, wfd) == -1) {
		close(serv_sock);  close(wfd); free(dopoll.dp_fds); error_handler("write pollfd to wfd error");
	}


	while (1) {
		printf("\nwait ioctl...\n");
		num_ret = ioctl(wfd, DP_POLL, &dopoll);		//block.
		if (num_ret == -1) {
			close(serv_sock); close(wfd); free(dopoll.dp_fds); error_handler("ioctl DP_POLL failed");
		}
		//////스레드를 2개, event 감지 시 connect accept이면 1번 스레드, rcv면 2번 스레드로? close는?
		//어차피 main도 단일 스레드니까 main에서 rcv하는게 낫겠다. 스레드하나 만들어서 connect하고.. 뮤텍스를 써야겠다.
		//근데 굳이 스레드 써야하는 이유는 뭐지? 어차피 lock 걸릴텐데 lock걸린동안 다른 스레드가 아예 작업을 못하면, 괜히 나누는거 아닌가?
		for (i = 0; i < num_ret; i++) {
			if ((dopoll.dp_fds[i].fd == serv_sock) && (num_clnt < MAXCLNT - 1)) { //clnt request connect
				clnt_sock[num_clnt] = accept(serv_sock, (struct sockaddr*)&clnt_addr[num_clnt], &addr_size);
				if (clnt_sock[num_clnt] == -1) {
					close(clnt_sock[num_clnt]);
					printf("accept error, but keep on\n");
					continue;
				}
				printf("[Accept] clnt sock : %d, addr : %s\n", clnt_sock[num_clnt], inet_ntoa(clnt_addr[num_clnt].sin_addr));
				if (devpoll_add(clnt_sock[num_clnt++], wfd) == -1) {
					close(serv_sock); close(wfd); free(dopoll.dp_fds); error_handler("write pollfd to wfd error");
				}
				printf("num_clnt = %d\n", num_clnt);

			}
			else { // clnt rcv event
				str_len = read(dopoll.dp_fds[i].fd, message, BUF_SIZE);
				for (j = 0; j < num_clnt; j++)
					if (dopoll.dp_fds[i].fd == clnt_sock[j]) break;
				efd = j; //clnt[efd] is event clnt ( == dopoll.dp_fds[i].fd)

				if (str_len == 0) {// event1,  close
					if (close_sock(efd, wfd) == -1) {
						close(serv_sock); close(wfd); free(dopoll.dp_fds); error_handler("write pollfd to wfd error");
					}
				}
				else {// event2, rcv msg
					message[str_len] = '\0'; // if need location change?
					printf("msg = %s\n", message);
 
					switch (message[1]) {
					case 'a': //not yet add alias => num_clnt-1
						for (j = 0; j < num_alias; j++)//alias is being used
							if (!strcmp(&message[3], clnt_alias[j])) {
								printf("alias = %s is dup\n", &message[3]);
								write(dopoll.dp_fds[i].fd, dup, sizeof(dup));
								break;
							}

						if (j == num_alias) {//accept alias
							write(dopoll.dp_fds[i].fd, okmsg, sizeof(okmsg));

							//client alias list send
							printf("num_alias = %d  ", num_alias);
							for (k = 0; k < num_alias; k++) {
								strncat(list, clnt_alias[k], ASIZE);
								strncat(list, " ", sizeof(char));
								printf("k = %d, %s\n", k, list);
							}
							strncat(list, "testbot", sizeof(char) * 8);
							strncat(list, " ", sizeof(char));
							list[strlen(list) - 1] = '\n';
							write(dopoll.dp_fds[i].fd, list, sizeof(list));
							printf("numclnt = %d, send list = %s-----LINE\n\n", num_clnt, list);
							list[4] = '\0';
							strncpy(clnt_alias[efd], &message[3], ASIZE); // NULL is added auto(calloc).

							strncat(a_alias, clnt_alias[efd], ASIZE);
							strncat(a_alias, "\n", sizeof(char));

							for (j = 0; j < num_clnt; j++) {
								if (dopoll.dp_fds[i].fd != clnt_sock[j]) {
									write(clnt_sock[j], a_alias, sizeof(a_alias));
								}
							}

							a_alias[4] = '\0';

							printf("alias is add [%s], clnt %d, [%d]\n\n", clnt_alias[efd], clnt_sock[efd], efd);

							num_alias++;
						}
						break;
					case 'w': //1:1 chat
						j = 0;
						while (message[3 + j] != ' ') { //find alias (/w ??..?? sender content)
							temp[j] = message[3 + j];
							j++;
						}
						temp[j] = '\0';
						j = 0;
						while ((strcmp(temp, clnt_alias[j]) && (j < num_clnt))) {
							j++;
						}
						printf("temp = %s / alias[j] = %s\n", temp, clnt_alias[j]);
						printf("--w--sendto temp = %s clnt[j=%d] = %s\n", temp, j, clnt_alias[j]);
						write(clnt_sock[j], message, str_len);//j is rcv clnt sequence	
						printf("\n\n");
						break;
					case 'c':
						for (j = 0; j < num_clnt; j++) {
							if (dopoll.dp_fds[i].fd != clnt_sock[j]) {
								write(clnt_sock[j], message, str_len);
							}
						}
						break;
					}
				}
			}
		}
	}
	close(wfd);
	free(dopoll.dp_fds);
	close(serv_sock);
	for (i = 0; i < MAXCLNT - 1; i++) {
		free(clnt_alias[i])
	}
	for (i = 0; i < num_clnt; i++) {
		close(clnt_sock[i])
	}
	return 0;
}

int close_sock(int efd, int wfd) {
	int i;
	char d_alias[4 + ASIZE + 1 + 1] = "/da ";	//da + ASIZE + \n + NULL

	printf("***********Disconnect Client %d\n", clnt_sock[efd]);

	if (devpoll_close(clnt_sock[efd], wfd) == -1)
		return -1;

	strncat(d_alias, clnt_alias[efd], ASIZE);
	strncat(d_alias, "\n", sizeof(char));
	printf("d_alias = %s", d_alias);

	for (i = 0; i < num_clnt; i++) {	//d_alias advertisement
		if (clnt_sock[efd] != clnt_sock[i]) {
			write(clnt_sock[i], d_alias, sizeof(d_alias));
		}
	}

	num_clnt--;
	num_alias--;
	clnt_sock[efd] = clnt_sock[num_clnt]; //sort
	clnt_addr[efd] = clnt_addr[num_clnt];
	clnt_alias[efd] = clnt_alias[num_clnt];
}

int create_serv_sock() {
	int serv_sock;
	struct sockaddr_in serv_addr;
	socklen_t optlen;
	int option;

	serv_sock = socket(PF_INET, SOCK_STREAM, 0);
	if (serv_sock == -1)
		error_handler("socket() error");

	optlen = sizeof(option);
	option = 1;
	setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, (void*)&option, optlen);
	//REUSEADDR. time-wait solv.

	memset(&serv_addr, 0, sizeof(serv_addr));//init
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);//INADDR_ANY = serv ip
	serv_addr.sin_port = htons(PORT); //port to network endian

	if (bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
		error_handler("bind() error");

	if (listen(serv_sock, 5) == -1)//keep on listening
		error_handler("listen() error");

	return serv_sock;
}


void error_handler(char *message) {
	int i;

	for (i = 0; i < MAXCLNT - 1; i++) {
		free(clnt_alias[i])
	}
	for (i = 0; i < num_clnt; i++) {
		close(clnt_sock[i])
	}

	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}


