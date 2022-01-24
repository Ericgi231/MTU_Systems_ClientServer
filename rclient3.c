#include "r_client.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

int entry(int argc, char* argv[]){
	//open
	int local = open(argv[1], O_RDWR | O_CREAT | O_TRUNC, 0666);
	int remote = r_open(argv[2], O_RDWR | O_CREAT | O_TRUNC, 0666);
	int extra = r_open("extra.txt", O_RDWR | O_CREAT | O_TRUNC, 0666);

	//fill local file
	char *s1 = "This is a piece of text.\nIt has multiple lines.\nCats and dogs and rats and hogs.\nThe end!";
	write(local, s1, strlen(s1));

	//fill remote extra
	char *s2 = "This file has some text also.\nWowzers.";
	r_write(extra, s2, strlen(s2));
	r_lseek(extra, 5, SEEK_SET);
	r_write(extra, "debra", 5);

	//copy local into remote
	char buffer[100];
	int res;
	int remoteDup = 9;
	r_dup2(remote, remoteDup);
	lseek(local, 0, SEEK_SET);
	do {
		res = read(local, buffer, 100);
		r_write(remoteDup, buffer, res);
	} while (res != 0);

	//copy remote into local
	r_lseek(remote, 0, SEEK_SET);
	do {
		res = r_read(remote, buffer, 100);
		write(local, buffer, res);
	} while (res != 0);

	int pipeFd[2];
	char newBuf[5];
	r_pipe(pipeFd);
	r_write(pipeFd[1], "test\0", 5);
	r_read(pipeFd[0], newBuf, 5);
	printf("%s\n", newBuf);

	//close
	close(local);
	r_close(remote);
	r_close(extra);

	return 0;
}
