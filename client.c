#include <arpa/inet.h> // inet_addr()
#include <netdb.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h> // bzero()
#include <unistd.h> // read(), write(), close()
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/socketvar.h>

#define BUF_CAPACITY 1000
#define PORT 7747
// #define SA struct sockaddr

char	**split(char *, char);
void	free_2d_array(char **);

int	check_socket_connection(int sockfd)
{
	int			error;
	socklen_t	len;
	int			ret_code;
	int			retval;
	
	error = 1;
	ret_code = 0;
	len = sizeof(error);
	retval = getsockopt (sockfd, SOL_SOCKET, SO_ERROR, &error, &len);
	// if (retval != 0)
	// {
	// 	printf("Error getting socket error code: %s\n", strerror(retval));
	// 	ret_code = 1;
	// }
	if (error != 0)
	{
		printf("Socket disconnected: %s\n", strerror(error));
		ret_code = 1;
	}
	return (ret_code);
}

void communication( void )
{
	int		sockfd;
	char    buff[BUF_CAPACITY];
	int     n;
	char    **splitted;

	sockfd = 0;
	while (1) {
		bzero(buff, sizeof(buff));
		printf("Enter the string : ");
		n = 0;
		while (n < BUF_CAPACITY - 1 && (buff[n] = getchar()) != '\n')
			n++;
		buff[n] = 0;
		splitted = split(buff, ' ');
		if (!splitted)
		{
			printf ("Allocation failure\n");
			continue ;
		}
		if (splitted[0] && strcmp(splitted[0], "connect") == 0)
		{
			struct sockaddr_in	servaddr;
			char				tmp_buf[30];

			// if (check_socket_connection(sockfd) == 0)
			if (send(sockfd, NULL, 0, MSG_NOSIGNAL) >= 0)
			{
				printf ("Already connected to the server\n");
				free_2d_array(splitted);
				continue ;
			}
			if (!splitted[1] || !splitted[2] || inet_pton(AF_INET, splitted[1], tmp_buf) == 0)
			{
				printf ("Invalid arguments\n");
				free_2d_array(splitted);
				continue ;
			}
			int	port = atoi(splitted[2]);
			// socket create and verification
			sockfd = socket(AF_INET, SOCK_STREAM, 0);
			if (sockfd == -1) {
				printf("Socket creation failed\n");
				free_2d_array(splitted);
				continue ;
			}
			else printf("Socket successfully created\n");
			// fcntl(sockfd, F_SETFL, O_NONBLOCK);
			bzero(&servaddr, sizeof(servaddr));
			
			// assign IP, PORT
			servaddr.sin_family = AF_INET;
			servaddr.sin_addr.s_addr = inet_addr(splitted[1]);
			servaddr.sin_port = htons(port);
			
			// connect the client socket to server socket
			if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr))	< 0)
			{
				printf("Connection with the server failed\n");
				free_2d_array(splitted);
				close(sockfd);
				continue ;
			}
			else printf("Connected to the server\n");
		}
		else if (splitted[0] && \
			(strcmp(splitted[0], "disconnect") == 0 || strcmp(splitted[0], "shell") == 0))
		{
			if (send(sockfd, buff, strlen(buff), MSG_NOSIGNAL) < 0)
				printf ("Youre not connected\n");
			else
			{
				bzero(buff, sizeof(buff));
				int recv_ret = recv(sockfd, buff, BUF_CAPACITY, MSG_NOSIGNAL);
				// printf ("%d\n", recv_ret);
				if (recv_ret <= 0)
				{
					printf ("Connection lost\n");
					close(sockfd);
				}
				else
					buff[recv_ret - 1] = 0;
				printf("From Server` \n%s\n", buff);
			}
		}
		else
			printf ("Unidentified command\n");
		free_2d_array(splitted);
	}
}

int main()
{
	communication();
}