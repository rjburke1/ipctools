#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#include "handler.h"
#include "logger.h"
#include "process_monitor.h"
#include "reactor.h"

#define IPT_TEST_ALLOCATOR_SHM_KEY (5000)

ipt_logger_t *logger_ptr;
ipt_allocator_t *alloc_logs_ptr;

char *msg;

static void
help(void)
{
	fprintf(stderr,"usage : dump logs sent to the logger \n");
}

static int
parse_and_init_args(int argc, char *argv[])
{
	int c;
	while ((c = getopt (argc, argv, "m:?")) != -1)
         {
		switch (c)
        	{
			case '?':
				help();
				exit(1);
			break; 
		} 
	} 

	return 0;
}

int main(int argc, char *argv[])
{
 	parse_and_init_args(argc, argv);

	/* Attach to the allocators */
	alloc_logs_ptr = ipt_allocator_shm_attach(IPT_TEST_ALLOCATOR_SHM_KEY);

	if (!alloc_logs_ptr)
	{
		printf("Failed to attach to shared allocator.\n");
		return -1;
	}

	/* Attach to logger */
	logger_ptr = ipt_logger_attach("ipt_logger_t", alloc_logs_ptr);

	if (logger_ptr == NULL )
	{
		printf("Failed to attach to ipt_logger_t\n");
		return -1;
	}


	ipt_logger_message_t  *msg_ptr;
while(1) {
        if ( (msg_ptr = logger_ptr->dequeue(logger_ptr)) == NULL)
	{
		printf("Failed to dequeue message\n");
		return -1;
	}

	printf("%s : %s  : %s\n",msg_ptr->time, msg_ptr->originator,msg_ptr->message);

	logger_ptr->free(logger_ptr,(void*)msg_ptr);
	
}        	
	return 0;
}
