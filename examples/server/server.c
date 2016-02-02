#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "handler.h"
#include "acceptor_handler.h"
#include "shared_in_list.h"


#include "logger.h"
#include "process_monitor.h"
#include "reactor.h"

#define IPT_ALLOCATOR_SHM_KEY (5000)

/* Global */
ipt_allocator_t *alloc_ptr;

static void
help(void)
{
	fprintf(stderr,"usage : server\n");
        fprintf(stderr,"This application reads and writes barcodes from the HID I/O channels.\n");
}

static int
parse_and_init_args(int argc, const char * const argv[])
{
	int c;
	while ((c = getopt (argc, (char * const *)argv, "?")) != -1)
	{
		switch (c)
       		{
			case '?':
			default:
				help();
				exit(1);
			break; 
		} 
	} 
	return 0;
}

int main(int argc, const char * const argv[])
{
	int fd;
	int option=1;
	const char *process_name = "msg";
	ipt_acceptor_handler_t *ah_ptr ;

 	if( parse_and_init_args(argc, argv) )
	{
		help();
		return 1;
	}

	/* Attach to the allocators */
	if ( (alloc_ptr = ipt_allocator_shm_attach(IPT_ALLOCATOR_SHM_KEY) ) == NULL )
	{
		printf("failed to create allocator\n");
		return 1;
	}

	/* Attach to process monitoring */
	ipt_process_monitor_t *pm_ptr =  ipt_process_monitor_attach("ipt_process_monitor_t",alloc_ptr);

	if ( pm_ptr == NULL )
	{
		printf("Failed to attach to ipt_process_monitor_t.\n");
		return -1;
	}	

	/* Create reactor */
	ipt_reactor_t *reactor = ipt_reactor_create();

	if ( reactor == NULL )
	{
		printf("Failed to create reactor.\n");
		return -1;
	}

	/* Create acceptor */
	if ( (ah_ptr = ipt_acceptor_handler_create(reactor, ipt_msg_handler_create))==NULL )
	{
		printf("Failed to create the ipt_acceptor_handler_t\n");
		return 1;
	}

	/* Register this process with process manager */
	if ( pm_ptr->register_process(pm_ptr, process_name, argv[0], argv) < 0 )
	{
		printf("Failed to register process with process management.\n");
		return -1;
	}

	ipt_time_value_t tv = { 1, 0 };

	/* Schedule process monitoring heartbeat */
	if ( reactor->schedule_timer(reactor, (ipt_event_handler_t *) pm_ptr, NULL, &tv,process_name) < 0 )
	{
		printf("Failed to schecule process monitoring timer.\n");
		return -1;
	}

	tv.tv_sec = 1;
	tv.tv_usec = 0;

	int sockfd;
	struct sockaddr_in serv_addr, cli_addr;

	/* Create socket to listen on */
 	if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		printf("Failed to create socket\n");
		return 1;
	}

     	bzero((char *) &serv_addr, sizeof(serv_addr));

     	serv_addr.sin_family = AF_INET;

     	serv_addr.sin_addr.s_addr = INADDR_ANY;
     	serv_addr.sin_port = htons(9000);

     	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
	{
		printf("Failed to bind socket\n");
		return 1;
	}

	if(setsockopt(sockfd,SOL_SOCKET,(SO_REUSEPORT | SO_REUSEADDR),(char*)&option,sizeof(option)) < 0)
	{

    		printf("setsockopt failed\n");
    		close(sockfd);
		return 1;
	}

     	if ( listen(sockfd,5) < 0 )
	{
		printf("Failed to listen on socket\n");	
		return 1;
	}

	ah_ptr->eh.set_handle(&ah_ptr->eh, sockfd);

	if ( reactor->register_handler(reactor, &ah_ptr->eh, EVENT_HANDLER_READ_MASK) < 0 )
        {
        	printf("failed to register handler\n");
        	return 1;
        }

	while (1) 
	{
		reactor->run_event_loop(reactor, &tv);
	}

	return 0;
}
