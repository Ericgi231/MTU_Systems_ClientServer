#include "r_client.h"
#include <fcntl.h>
#include <unistd.h>

/* Add any includes needed*/

/* entry
 * This is essentially the "main" for any user program linked with
 * r_client. Main in r_client will establish the connection with the
 * server then call entry. From entry, we can implement the desired 
 * user program which may call any of the r_ RPC functions.
 *
 * rclient1 remotely opens an output file, locally opens an input file, 
 * copies the input file to the output file and closes both files.
 */
int entry(int argc, char* argv[]){
	//open local file
	int local = open(argv[1],  O_RDONLY| O_CREAT, 0666);
	//open remote file
	int remote = r_open(argv[2], O_RDWR | O_CREAT, 0666);

	//read chars 100 at a time
	char buffer[100];
	int res;
	do {
		res = read(local, buffer, 100);
		r_write(remote, buffer, res);
	} while (res != 0);

	//close local
	close(local);
	//close remote
	r_close(remote);

	return 0;
}
