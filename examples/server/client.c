/* 
 * tcpclient.c - A simple TCP client
 * usage: tcpclient <host> <port>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h> 

#define BUFSIZE 1024

void help(void)
{
        fprintf(stderr,"usage      : tcp_client [ip address] [port] [message] [num_msgs]\n");
        fprintf(stderr,"ipaddr     : Dotted decimal ip address.\n");
        fprintf(stderr,"port       : Port number.\n");
        fprintf(stderr,"msg        : The message to be sent.\n");
        fprintf(stderr,"num_msgs   : Send the message multiple times.\n");
}
int main(int argc, char **argv) {
    int num_msgs;
    int length;
    int sockfd, port, n;
    struct sockaddr_in serveraddr;
    struct hostent *server;
    char *server_ip;
    char *msg;

    if ( argc != 5) 
    {
	help();
	return 1;
    } 

    server_ip = argv[1];
    port = atoi(argv[2]);
    msg  = argv[3]; 
    num_msgs  = atoi(argv[4]); 

    /* socket: create the socket */
    if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0) ) <=0 )
    {
	printf("Failed to open socket\n");
 	return 1;
    }	

    /* build the server's Internet address */
    memset(&serveraddr, 0, sizeof(serveraddr));    
    serveraddr.sin_family      = AF_INET;         
    serveraddr.sin_addr.s_addr = inet_addr(server_ip);   
    serveraddr.sin_port        = htons(port); 

    /* connect: create a connection with the server */
    if (connect(sockfd, (const struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) 
    {
	printf("Failed to connect\n");
	return 0;
    }

    int count = 0;
    length = strlen(msg) + 1;
    while ( count < num_msgs )
    {
	/* Write the length */
	if ( write(sockfd,&length,sizeof(length) ) != sizeof(length) ||
    	     write(sockfd,msg,length) != length  )
    	{
		printf("Failed to send message\n");
        	return 1;
    	}
	count++;
     }

    close(sockfd);

    return 0;
}
