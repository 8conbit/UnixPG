/*
AES block size is 128 ( fixed ) 16Byte... 16*64 = 1024
division,  AES
RSA, thread는 나중. clnt 끝난다음에. alias find hash도. 나중에.
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
void create_serv_sock();
void close_sock(int efd);

int find_alias(char *new_alias);//dup test -> send ok&send list -> add alias
void send_list(int efd);
void add_alias(int efd, char *new_alias);
void whisper(char *msg);

int serv_sock;
int clnt_sock[MAXCLNT - 1];
struct sockaddr_in clnt_addr[MAXCLNT - 1];
char *clnt_alias[MAXCLNT - 1];
static int num_clnt = 0;// num_clnt_sock
static int num_alias = 0;
int wfd = NULL;
struct dvpoll dopoll;

int main(int argc, char *argv[]) {
	char message[BUF_SIZE];
	int str_len;
	socklen_t addr_size;
	int i, j, k, num_ret, efd;
	char dup[] = "/dp\n";

	for (i = 0; i < MAXCLNT - 1; i++) 
		clnt_alias[i] = (char*)calloc(ASIZE + 1, sizeof(char)); //alias + NULL

	addr_size = sizeof(clnt_addr);

	create_serv_sock();

	if ((wfd = open("/dev/poll", O_RDWR)) < 0)
		error_handler("/dev/poll error");

	if (devpoll_init(&dopoll) == -1)
		error_handler("devpoll_init malloc error");

	if (devpoll_add(serv_sock, wfd) == -1)
		error_handler("write pollfd to wfd error");


	while (1) {
		printf("\nwait ioctl...\n");
		num_ret = ioctl(wfd, DP_POLL, &dopoll);		//block.
		if (num_ret == -1)
			error_handler("ioctl DP_POLL failed");

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
				if (devpoll_add(clnt_sock[num_clnt++], wfd) == -1)
					error_handler("write pollfd to wfd error");

				printf("num_clnt = %d\n", num_clnt);

			}
			else { // clnt rcv event
				str_len = read(dopoll.dp_fds[i].fd, message, BUF_SIZE);
				for (j = 0; j < num_clnt; j++)
					if (dopoll.dp_fds[i].fd == clnt_sock[j]) break;

				efd = j; //clnt[efd] is event clnt ( == dopoll.dp_fds[i].fd)

				if (str_len == 0) {// event1,  close
					close_sock(efd);
				}
				else {// event2, rcv msg
					message[str_len] = '\0';
					printf("msg = %s\n", message);
					switch (message[1]) {
					case 'a': //not yet add alias => num_clnt-1
						if (find_alias(&message[3]) == 0) {
							send_list(efd);
							add_alias(efd, &message[3]);
						}
						else {
							printf("alias = %s is dup\n", &message[3]);
							write(clnt_sock[efd], dup, sizeof(dup));
						}
					
						break;
					case 'w': //1:1 chat
						whisper(message);

						break;
					case 'c':
						for (j = 0; j < num_clnt; j++) {
							if (clnt_sock[j] != clnt_sock[efd]) {
								write(clnt_sock[j], message, str_len);
							}
						}
						break;
					default :
						write(clnt_sock[efd], message, str_len);
						break;
					}
				}
			}
		}
	}

	close(wfd);
	free(dopoll.dp_fds);
	close(serv_sock);
	for (i = 0; i < MAXCLNT - 1; i++)
		free(clnt_alias[i]);
	for (i = 0; i < num_clnt; i++)
		close(clnt_sock[i]);
	return 0;
}




void whisper(char *msg) {
	int i = 0;
	char temp[ASIZE + 1];

	while (msg[3 + i] != ' ') { //find alias (/w ??..?? sender content)
		temp[i] = msg[3 + i];
		i++;
	}
	temp[i] = '\0';

	i = find_alias(temp);
	printf("--w--sendto temp = %s alias[i=%d] = %s ****have to same****\n", temp, i, clnt_alias[i]);
	write(clnt_sock[i], msg, strlen(msg)+1);//i is rcv clnt , don't consider snd.	+1 is NULL.
}

int find_alias(char *new_alias) {
	int i;
	
	for (i = 0; i < num_alias; i++) {//alias is being used
		if (!strcmp(new_alias, clnt_alias[i])) {
			return i;
		}
	}
	return 0;
}

void send_list(int efd) {//send ok -> send list -> add alias
	int i;
	char okmsg[] = "/ok\n";
	char list[4 + ((ASIZE + 1)*MAXCLNT)+1] = "/li "; // /li + (ASIZE+" ")*clnts (last " " is replaced '\n') + NULL

	write(clnt_sock[efd], okmsg, sizeof(okmsg));

	printf("num_alias = %d  ", num_alias);	
	for (i = 0; i < num_alias; i++) {	// need to change using hash.
		strncat(list, clnt_alias[i], ASIZE);
		strncat(list, " ", sizeof(char));
	}
	strncat(list, "testbot", sizeof(char) * 8);
	strncat(list, " ", sizeof(char));
	list[strlen(list) - 1] = '\n';
	list[strlen(list)] = '\0';
	write(clnt_sock[efd], list, sizeof(list));
	printf("numclnt = %d, send list = %s-----LINE\n\n", num_clnt, list);

}

void add_alias(int efd, char *new_alias) {
	int i;
	char a_alias[4 + ASIZE + 1 + 1] = "/aa ";

	strncpy(clnt_alias[efd], new_alias, ASIZE); // NULL is added auto(calloc).

	strncat(a_alias, clnt_alias[efd], ASIZE);
	strncat(a_alias, "\n", sizeof(char));

	for (i = 0; i < num_clnt; i++) {
		if (clnt_sock[efd] != clnt_sock[i]) {
			write(clnt_sock[i], a_alias, sizeof(a_alias));
		}
	}

	printf("alias is add [%s], clnt %d, [%d]\n\n", clnt_alias[efd], clnt_sock[efd], efd);

	num_alias++;

}

void close_sock(int efd) {
	int i;
	char d_alias[4 + ASIZE + 1 + 1] = "/da ";	//da + ASIZE + \n + NULL

	printf("***********Disconnect Client %d\n", clnt_sock[efd]);

	if (devpoll_close(clnt_sock[efd], wfd) == -1)
		error_handler("devpoll_close error");

	if(*clnt_alias[efd] != NULL){
		strncat(d_alias, clnt_alias[efd], ASIZE);
		strncat(d_alias, "\n", sizeof(char));
		printf("d_alias = %s", d_alias);
		num_alias--;
	
		for (i = 0; i < num_clnt; i++) {	//d_alias advertisement
			if (clnt_sock[efd] != clnt_sock[i]) {
				write(clnt_sock[i], d_alias, sizeof(d_alias));
			}
		}
	}

	num_clnt--;
	clnt_sock[efd] = clnt_sock[num_clnt]; //sort
	clnt_addr[efd] = clnt_addr[num_clnt];
	clnt_alias[efd] = clnt_alias[num_clnt];

}

void create_serv_sock() {
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

}


void error_handler(char *message) {
	int i;

	close(serv_sock);
	close(wfd);
	free(dopoll.dp_fds);

	for (i = 0; i < MAXCLNT - 1; i++)
		free(clnt_alias[i]);

	for (i = 0; i < num_clnt; i++)
		close(clnt_sock[i]);

	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}


