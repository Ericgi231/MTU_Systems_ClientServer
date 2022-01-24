/* Add any includes needed*/
#include <fcntl.h>
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

/* In this project, we will develop a mini Remote Procedure Call (RPC) 
 * based system consisting of a em server and a client. Using the remote 
 * procedures supplied by the server our client program will be able to 
 * open files and perform computations on the server. 
 *
 * The server should open a socket and listen on an available port. The 
 * server program upon starting should print the port number it is using 
 * on the standard output 
 * (Print only the port number with no additional formatting. You may use printf \%d). 
 * This port number is then manually passed as a command line argument to 
 * the client to establish the connection. In order to implement the RPC, 
 * the server and the client should communicate through a TCP socket. It 
 * is allowed to fork a child for each open connection and delagate the 
 * handling to the child.
 */

/** Fill first 8 bytes of string with two integers
 * @param val value int
 * @param err error int
 * @param msg char array to fill
 * @return L value (8)
 */
int buildResponseBase(int val, int err, char *msg){
	int L = 0;

	msg[L++] =	(val >> 24) & 0xff;
	msg[L++] =	(val >> 16) & 0xff;
	msg[L++] =	(val >>  8) & 0xff;
	msg[L++] =	(val      ) & 0xff;
	
	msg[L++] =	(err >> 24) & 0xff;
	msg[L++] =	(err >> 16) & 0xff;
	msg[L++] =	(err >>  8) & 0xff;
	msg[L++] =	(err      ) & 0xff;

	return L;
}

/** Decode a single int and return value
 * @param conn connection pointer
 * @return value of decoded int
 */
int decodeInt(int conn){
	unsigned char buffer[4];
	int res = read(conn, buffer, 4);
	if (res == -1) { 
		perror("DECODE ERROR");
	}
	return (buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3];
}

/** Decode a set count of bytes
 * @param conn connection pointer
 * @param count count of bytes to decode
 * @return value of decoded bytes as char array
 */
char * decodeBytes(int conn, int count){
	char* outBuffer = malloc(count);
	if (read(conn, outBuffer, count) == -1) { 
		perror("DECODE ERROR"); 
	};
	return outBuffer;
}

/** Decode a length value and bytes of that length
 * @param conn connection pointer
 * @return value of decoded bytes as char array
 */
char * decodeString(int conn){
	unsigned char buffer[4];
	if(read(conn, buffer, 2) == -1) { 
		perror("DECODE ERROR"); 
	}
	int length = (buffer[0] << 8) | (buffer[1]);
	char* outBuffer = malloc(length);
	if (read(conn, outBuffer, length) == -1) { 
		perror("DECODE ERROR"); 
	};
	return outBuffer;
}

/** Decode, operate, and encode the open operation
 * @param conn connection pointer
 */
void s_open(int conn){
	//decode
	char *path = decodeString(conn);
	int flags = decodeInt(conn);
	int mode = decodeInt(conn);

	printf("post send\n");
	printf("flags: %d\n", flags);
	printf("mode: %d\n", mode);

	//operation
	int fd = open(path, flags, mode);
	if (fd == -1) { 
		perror("OPEN ERROR");
	}
	int error = errno;
	
	//encode
	char msg[8];
	buildResponseBase(fd, error, msg);

	//send
	write(conn, msg, 8);

	//free
	free(path);
}

/** Decode, operate, and encode the close operation
 * @param conn connection pointer
 */
void s_close(int conn){
	//decode
	int fd = decodeInt(conn);

	//operation
	int res = close(fd);
	if (res == -1) { 
		perror("CLOSE ERROR");
	}
	int error = errno;
	
	//encode
	char msg[8];
	buildResponseBase(res, error, msg);

	//send
	write(conn, msg, 8);
}

/** Decode, operate, and encode the read operation
 * @param conn connection pointer
 */
void s_read(int conn){
	//decode
	int fd = decodeInt(conn);
	int count = decodeInt(conn);

	//operation
	char *buffer = malloc(count);
	int res = read(fd, buffer, count);
	if (res == -1) {
		perror("READ ERROR");
	}
	int error = errno;
	
	//encode
	char *msg = malloc(8 + res);
	int L = buildResponseBase(res, error, msg);
	for (int i = 0; i < res; i++) {
		msg[L++] = buffer[i];
	}

	//send
	write(conn, msg, 8+res);

	//free
	free(buffer);
	free(msg);
}

/** Decode, operate, and encode the write operation
 * @param conn connection pointer
 */
void s_write(int conn){
	//decode
	int fd = decodeInt(conn);
	int count = decodeInt(conn);
	char *data = decodeBytes(conn, count);

	//operation
	int res = write(fd, data, count);
	if (res == -1) { 
		perror("WRITE ERROR");
	}
	int error = errno;

	//encode
	char msg[8];
	buildResponseBase(res, error, msg);

	//send
	write(conn, msg, 8);
}

/** Decode, operate, and encode the seek operation
 * @param conn connection pointer
 */
void s_seek(int conn){
	//decode
	int fd = decodeInt(conn);
	int offset = decodeInt(conn);
	int whence = decodeInt(conn);

	// printf("fd: %d\n", fd);
	// printf("offset: %d\n", offset);
	// printf("Whence: %d\n", whence);

	//operation
	int res = lseek(fd, offset, whence);
	if (res == -1) { 
		perror("SEEK ERROR");
	}
	int error = errno;

	//encode
	char msg[8];
	buildResponseBase(res, error, msg);

	//send
	write(conn, msg, 8);
}

/** Decode, operate, and encode the pipe operation
 * @param conn connection pointer
 */
void s_pipe(int conn){
	//operation
	int pipedes[2];
	int res = pipe(pipedes);
	if (res == -1) { 
		perror("PIPE ERROR");
	}
	int error = errno;
	
	//encode
	char msg[16];
	int L = buildResponseBase(res, error, msg);
	
	msg[L++] = (pipedes[0] >> 24) & 0xff;
	msg[L++] = (pipedes[0] >> 16) & 0xff;
	msg[L++] = (pipedes[0] >>  8) & 0xff;
	msg[L++] = (pipedes[0]    ) & 0xff;

	msg[L++] = (pipedes[1] >> 24) & 0xff;
	msg[L++] = (pipedes[1] >> 16) & 0xff;
	msg[L++] = (pipedes[1] >>  8) & 0xff;
	msg[L++] = (pipedes[1]      ) & 0xff;

	//send
	write(conn, msg, 16);
}

/** Decode, operate, and encode the dup2 operation
 * @param conn connection pointer
 */
void s_dup2(int conn){
	//decode
	int oFd = decodeInt(conn);
	int nFd = decodeInt(conn);

	// printf("old: %d\n", oFd);
	// printf("new: %d\n", nFd);

	//operation
	int res = dup2(oFd, nFd);
	if (res == -1) { 
		perror("DUP2 ERROR\n");
	}
	int error = errno;

	//encode
	char msg[8];
	buildResponseBase(res, error, msg);

	//send
	write(conn, msg, 8);
}

/* main - server implementation
 */
int main(int argc, char *argv[]){
	int listener, conn, length;
	struct sockaddr_in s1, s2;
	listener = socket( AF_INET, SOCK_STREAM, 0 ); //1
	bzero((char *) &s1, sizeof(s1));
	s1.sin_family = AF_INET;
	s1.sin_addr.s_addr = htonl(INADDR_ANY); /* Any of this host’s interfaces is OK. */
	s1.sin_port = htons(0); /* bind() will gimme unique port. */
	bind(listener, (struct sockaddr *)&s1, sizeof(s1)); //2
	length = sizeof(s1);
	getsockname(listener, (struct sockaddr *)&s1, (socklen_t* restrict) &length); /* Find out port number */
	printf("%d\n", ntohs(s1.sin_port));
	listen(listener,10); //3
	length = sizeof(s2);

	for (;;) {
		conn=accept(listener, (struct sockaddr *)&s2, (socklen_t* restrict) &length);
		if (fork() == 0) {
			close(listener);
			char op;
			do {
				read(conn, &op, 1);
				switch (op)
				{
					case open_call:
						s_open(conn);
						break;
					case close_call:
						s_close(conn);
						break;
					case read_call:
						s_read(conn);
						break;
					case write_call:
						s_write(conn);
						break;
					case seek_call:
						s_seek(conn);
						break;
					case pipe_call:
						s_pipe(conn);
						break;
					case dup2_call:
						s_dup2(conn);
						break;
					default:
						break;
				}
			} while (op != EOF);
			exit(1);
		}
		close(conn);
	}
	return 0;
}
