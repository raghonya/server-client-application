#ifndef SC_H
# define SC_H

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

#define PORT			7744
#define BUF_SIZE		100
#define INCREASE		1
#define DECREASE		0
#define TIMEOUT_SEC		3
#define MAX_CLI_COUNT	5

#define FAILED(cause)	"Error: "cause"\r\n"

typedef enum error_codes_t
{
	SUCCESS = 0,
	DISCONNECT,
	ERROR
} error_codes_t;

typedef struct data_t
{
	char	*request;
	char	*response;
} data_t;

typedef struct list_t
{
	int				content;
	struct list_t	*next;
}	list_t;

char	**split(char *, char);
void	free_2d_array(char **);
void	lstadd_back(list_t **, list_t *);
void	lstadd_front(list_t **, list_t *);
void	lstclear(list_t **);
void	lstdelone(list_t *);
int		lstsize(list_t *);
list_t	*lstnew(int);

#endif