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

#define MAX 100
#define PORT 7747
// #define SA struct sockaddr

char	**split(char *, char);
void	free_2d_array(char **);

void communication( void )
{
	int		sockfd;
	char    buff[MAX];
	int     n;
	char    **splitted;

	while (1) {
		bzero(buff, sizeof(buff));
		printf("Enter the string : ");
		n = 0;
		while (n < MAX - 1 && (buff[n] = getchar()) != '\n')
			n++;
		buff[n] = 0;
		// char	*pbuff = buff;
		// while (*p && isspace(*p))
		// 	pbuff++;
		//////// i forget what to do
		splitted = split(buff, ' ');
		if (!splitted)
			continue ;
		if (splitted[0] && strcmp(splitted[0], "connect") == 0)
		{
			struct sockaddr_in	servaddr;
			char				tmp_buf[30];

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
				printf("socket creation failed\n");
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
			printf ("ERRNO: %d\n", errno);
		}
		else if (splitted[0] && \
			(strcmp(splitted[0], "disconnect") == 0 || strcmp(splitted[0], "shell") == 0))
		{
			printf ("sendbuf: '%s'\n", buff);
			if (send(sockfd, buff, strlen(buff), MSG_NOSIGNAL) < 0)
			{
				printf ("Youre not connected\n");
				// continue ;
			}
			else
			{
				bzero(buff, sizeof(buff));
				int ret_recv = recv(sockfd, buff, sizeof(buff), MSG_NOSIGNAL);
				printf ("sendaaaaa: '%d'\n", ret_recv);
				if (ret_recv < 0)
				{
					printf ("Connection lost\n");
					// continue ;
				}
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