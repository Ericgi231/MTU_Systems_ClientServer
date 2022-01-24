/* Add any includes needed*/
#include "r_client.h"
#include <fcntl.h>
#include <unistd.h>

/* entry
 * This is essentially the "main" for any user program linked with
 * r_client. Main in r_client will establish the connection with the
 * server then call entry. From entry, we can implement the desired 
 * user program which may call any of the r_ RPC functions.
 *
 * rclient2 should open a local file as output and a remote file as input. 
 * It should seek the remote file to position 10 and copy the rest 
 * to the local file.
 */
int entry(int argc, char* argv[]){
	//open local file
	int local = open(argv[2], O_RDWR | O_CREAT | O_TRUNC, S_IWUSR | S_IRUSR);
	//open remote file
	int remote = r_open(argv[1], O_RDONLY | O_CREAT, S_IWUSR | S_IRUSR);

	//read chars 100 at a time
	char buffer[100];
	int res;
	r_lseek(remote, 10, SEEK_SET);
	do {
		res = r_read(remote, buffer, 100);
		write(local, buffer, res);
	} while (res != 0);

	//close local
	close(local);
	//close remote
	r_close(remote);

	return 0;
}
