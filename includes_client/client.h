#ifndef CLIENT_H
# define CLIENT_H

#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <strings.h>
#include <pthread.h>
#include <sys/time.h> 
#include <sys/wait.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/socketvar.h>

#define REQ_CAPACITY	2048
#define RESP_CHUNK		512
#define DELIM			"\r\n\t\f\v "

#define FAILED(cause)	"Error: "cause"\r\n"

char	**split(char *, char);
void	free_2d_array(char **);

#endif