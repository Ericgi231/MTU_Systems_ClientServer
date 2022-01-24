#include "r_client.h"

/* Add any includes needed*/
#include <strings.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>

/*Opcodes for RPC calls*/
#define open_call   1 
#define close_call  2
#define read_call   3
#define write_call  4
#define seek_call   5
#define pipe_call   6
#define dup2_call   7

int entry(int argc, char *argv[]);
int socketfd;

/* main - entry point for client applications.
 *
 * You are expected to develop a client program which will provide an 
 * environment into which we can plug an application and execute it 
 * using the remote versions of the supported calls. The client program 
 * therefore should expect a <hostname> <portnumber> pair as its first 
 * two arguments and attempt to connect the server. Once it connects, it
 * should call the user program which has a function called entry 
 * analogous to the main program in an ordinary C program. The entry 
 * routine should have the argv and argc arguments and return an integer 
 * value, just like the ordinary main. The client program should strip 
 * off the first two arguments, create a new argv array and call the entry procedure. 
 * Finally, when entry exits, the return value should be returned from the 
 * main procedure of the client.
 */
int main(int argc, char *argv[]){
	if (argc < 5) {
		printf("./rclient <hostname> <portnumber> <input_file> <output_file>\n");
		return 0;
	}
	char *remhost; u_short remport;
	struct sockaddr_in remote;
	struct hostent *h;
	remhost = argv[1]; remport = atoi(argv[2]);
	socketfd = socket( AF_INET, SOCK_STREAM, 0 );
	bzero((char *) &remote, sizeof(remote));
	remote.sin_family = AF_INET;
	h = gethostbyname(remhost);
	bcopy((char *)h->h_addr, (char *)&remote.sin_addr, h->h_length);
	remote.sin_port = htons(remport);
	connect(socketfd, (struct sockaddr *)&remote, sizeof(remote));

	int nargc= argc-2;
	char *nargv[] = {argv[0], argv[3], argv[4]};

	int res = entry(nargc, nargv);
	char eof = EOF;
	write(socketfd, &eof, 1);
	return res;
}

int buildByte(int L, int val, char *msg){
	msg[L++] = (val) & 0xff;
	return L;
}

int buildInt(int L, int val, char *msg){
	msg[L++] = (val >> 24) & 0xff;
	msg[L++] = (val >> 16) & 0xff;
	msg[L++] = (val >> 8) & 0xff;
	msg[L++] = (val ) & 0xff;
	return L;
}

/* r_open
 * remote open
 */
int r_open(const char *pathname, int flags, int mode){
	//vars
	const char * p = pathname;
	while(*p) p++;
	int u_l = p-pathname;

	//encode
	int L = 0;
	char *msg = malloc(11 + u_l);
	L = buildByte(L, open_call, msg);

	L = buildByte(L, (u_l >> 8), msg);
	L = buildByte(L, u_l, msg);
	for (int i = 0; i < u_l; i++) {
		L = buildByte(L, pathname[i], msg);
	}

	L = buildInt(L, flags, msg);
	L = buildInt(L, mode, msg);

	// printf("pre send\n");
	// printf("flags: %d\n", flags);
	// printf("mode: %d\n", mode);

	//send
	write(socketfd, msg, L);

	//receive
	read(socketfd, msg, 8);
	int in_msg = (msg[0] << 24) | (msg[1] << 16) | (msg[2] << 8) | msg[3];
	int in_err = (msg[4] << 24) | (msg[5] << 16) | (msg[6] << 8) | msg[7];

	//free
	free(msg);

	//return
	errno = in_err;
	return in_msg;
}

/* r_close
 * remote close
 */
int r_close(int fd){
	//encode
	int L = 0;
	char *msg = malloc(5);
	L = buildByte(L, close_call, msg);
	L = buildInt(L, fd, msg);

	//send
	write(socketfd,msg,L);

	//receive
	msg = malloc(8);
	read(socketfd, msg, 8);
	int in_msg = (msg[0] << 24) | (msg[1] << 16) | (msg[2] << 8) | msg[3];
	int in_err = (msg[4] << 24) | (msg[5] << 16) | (msg[6] << 8) | msg[7];

	//free
	free(msg);

	//return
	errno = in_err;
	return in_msg;
}

/* r_read
 * remote read
 */
int r_read(int fd, void *buf, int count){
	//encode
	int L = 0;
	char *msg = malloc(9);
	L = buildByte(L, read_call, msg);
	L = buildInt(L, fd, msg);
	L = buildInt(L, count, msg);

	//send
	write(socketfd,msg,L);

	//receive
	msg = malloc(count+8);
	read(socketfd, msg, count+8);
	int in_msg = (msg[0] << 24) | (msg[1] << 16) | (msg[2] << 8) | msg[3];
	int in_err = (msg[4] << 24) | (msg[5] << 16) | (msg[6] << 8) | msg[7];

	char *buffer = (char*)buf;
	for (int i = 0; i < in_msg; i++) {
		buffer[i] = msg[i+8];
	}

	//free
	free(msg);

	//return
	errno = in_err;
	return in_msg;
}

/* r_write
 * remote write
 */
int r_write(int fd, const void *buf, int count){
	//encode
	int L = 0;
	char *msg = malloc(9 + count);
	L = buildByte(L, write_call, msg);

	L = buildInt(L, fd, msg);
	L = buildInt(L, count, msg);

	const char *buffer = buf;
	for (int i = 0; i < count; i++) {
		L = buildByte(L, buffer[i], msg);
	}

	//send
	write(socketfd, msg, L);

	//receive
	read(socketfd, msg, 8);
	int in_msg = (msg[0] << 24) | (msg[1] << 16) | (msg[2] << 8) | msg[3];
	int in_err = (msg[4] << 24) | (msg[5] << 16) | (msg[6] << 8) | msg[7];

	//free
	free(msg);

	//return
	errno = in_err;
	return in_msg;
}

/* r_lseek
 * remote seek
 */
int r_lseek(int fd, int offset, int whence){
	//encode
	int L = 0;
	char *msg = malloc(13);
	L = buildByte(L, seek_call, msg);
	L = buildInt(L, fd, msg);
	L = buildInt(L, offset, msg);
	L = buildInt(L, whence, msg);

	// printf("fd: %d\n", fd);
	// printf("offset: %d\n", offset);
	// printf("Whence: %d\n", whence);

	//send
	write(socketfd,msg,L);

	//receive
	read(socketfd, msg, 8);
	int in_msg = (msg[0] << 24) | (msg[1] << 16) | (msg[2] << 8) | msg[3];
	int in_err = (msg[4] << 24) | (msg[5] << 16) | (msg[6] << 8) | msg[7];

	//free
	free(msg);

	//return
	errno = in_err;
	return in_msg;
}

/* r_pipe
 * remote pipe
 */
int r_pipe(int pipefd[2]){
	//send
	char opcode = pipe_call;
	write(socketfd, &opcode, 1);

	//receive
	char msg[16];
	read(socketfd, msg, 16);
	int in_msg = (msg[0] << 24) | (msg[1] << 16) | (msg[2] << 8) | msg[3];
	int in_err = (msg[4] << 24) | (msg[5] << 16) | (msg[6] << 8) | msg[7];
	pipefd[0]  = (msg[8] << 24) | (msg[9] << 16) | (msg[10] << 8) | msg[11];
	pipefd[1]  = (msg[12] << 24) | (msg[13] << 16) | (msg[14] << 8) | msg[15];

	//return
	errno = in_err;
	return in_msg;
}

/* r_dup2
 * remote dup2
 */
int r_dup2(int oldfd, int newfd){
	//encode
	int L = 0;
	char *msg = malloc(9);
	L = buildByte(L, dup2_call, msg);
	L = buildInt(L, oldfd, msg);
	L = buildInt(L, newfd, msg);

	// printf("old: %d\n", oldfd);
	// printf("new: %d\n", newfd);

	//send
	write(socketfd,msg,L);

	//receive
	msg = malloc(8);
	read(socketfd, msg, 8);
	int in_msg = (msg[0] << 24) | (msg[1] << 16) | (msg[2] << 8) | msg[3];
	int in_err = (msg[4] << 24) | (msg[5] << 16) | (msg[6] << 8) | msg[7];

	//free
	free(msg);

	//return
	errno = in_err;
	return in_msg;
}
