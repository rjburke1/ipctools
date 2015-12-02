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
char *orig;
static void
help(void)
{
	fprintf(stderr,"usage : enqueue_logmsg -m [message] -o [originator]\n");
        fprintf(stderr,"message    : The message to be generated.\n");
        fprintf(stderr,"originator : The originator of the message.\n");
}

static int
parse_and_init_args(int argc, char *argv[])
{
	int c;
	while ((c = getopt (argc, argv, "m:o:?")) != -1)
         {
		switch (c)
        	{
			case 'm':
				msg = optarg;
			break;

			case 'o':
				orig = optarg;
			break;

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

	if ( !msg || !orig)
	{
		help();
		return -1;
	}

	/* Attach to the allocators */
	alloc_logs_ptr = ipt_allocator_shm_attach(IPT_TEST_ALLOCATOR_SHM_KEY);


	if (alloc_logs_ptr == NULL)
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

        logger_ptr->set_category(logger_ptr,  IPT_MODULE_ALL  );
        logger_ptr->set_level(logger_ptr,  IPT_LEVEL_ALL  );

        if ( logger_ptr->enqueue(logger_ptr,  IPT_MODULE_ALL  , IPT_LEVEL_ALL,orig, "%s", msg) < 0 )
	{
		printf("failed to enqueue the message\n");
		return -1;
        }

	return 0;
}
