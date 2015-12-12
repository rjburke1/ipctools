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

char *log_file = NULL;
char *remote_syslog = NULL;
unsigned int log_size = (10*1024);
int sock = -1;
struct sockaddr_in remote_in_addr;
FILE *lf_fp ;

static void
help(void)
{
	fprintf(stderr,"usage : logger -r [remote syslog destination] -l [local file destination] -s [local file size]\n");
	fprintf(stderr,"remote syslog destination    : IPV4 address of a remote syslog server to which messages will be sent. (x.x.x.x)\n");
	fprintf(stderr,"local file destination       : Local file to which logs will be written. The default is stdout.\n");
        fprintf(stderr,"local storage limit (in MB ) : Maximum size allocated for storage of logs. The default is 10 MB.\n");
}

static int
parse_and_init_args(int argc, const char * const argv[])
{
	int c;
	while ((c = getopt (argc, (char * const *)argv, "l:s:r:?")) != -1)
         {
		switch (c)
        	{
			case 's':
				log_size = atol(optarg);
			break;

			case 'l':
				log_file = optarg;
			break;

			case 'r':
				remote_syslog = optarg;
			break;

			case '?':
			default:
				help();
				exit(1);
			break; 
		} 
	} 

	/* Create socket to remote syslog server */
	if ( remote_syslog ) 
	{
		if ( (sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP) ) < 0 )
		{
			return -1;
		}

		memset((char *) &remote_in_addr, 0 , sizeof(remote_in_addr));

		remote_in_addr.sin_family = AF_INET;
		remote_in_addr.sin_port = 514;

		if ( inet_aton(remote_syslog, &remote_in_addr.sin_addr) == 0 )
		{
			return -1;
		}
	}

	if ( log_file )
	{
		if ( ( lf_fp = fopen(log_file,"w")) == NULL )
		{
			fprintf(stderr,"failed to open log_file[%s]\n", log_file);
			exit(1);
		}
	}
	else
	{
		lf_fp = stdout;
	}

	return 0;
}

int main(int argc, const char * const argv[])
{
	const char *process_name = "logger";

	struct my_handler my_handler;

 	parse_and_init_args(argc, argv);

	if ( log_file )
	{
		if ( ( lf_fp = fopen(log_file,"w")) == NULL )
		{
			fprintf(stderr,"failed to open log_file[%s]\n", log_file);
			exit(1);
		}
	}
	else
	{
		lf_fp = stdout;
	}

	my_handler.eh.handle_input = handle_input;
	my_handler.eh.set_handle = set_handle;
	my_handler.eh.get_handle = get_handle;

	/* Attach to the allocators */
	alloc_logs_ptr = ipt_allocator_shm_attach(IPT_TEST_ALLOCATOR_SHM_KEY);
	ipt_allocator_t *alloc_se_ptr = ipt_allocator_shm_attach(IPT_TEST_ALLOCATOR_SHM_KEY);

	if ( alloc_se_ptr == NULL  || alloc_logs_ptr == NULL)
	{
		printf("Failed to attach to shared allocator.\n");
		return -1;
	}
        printf("stage 1\n");fflush(stdout);
	/* Attach to logger */
	logger_ptr = ipt_logger_attach("ipt_logger_t", alloc_logs_ptr);

	if  (logger_ptr == NULL )
	{
		printf("Failed to attach to ipt_logger_t\n");
		return -1;
	}

	/* Get handler */
	my_handler.eh.set_handle( &my_handler.eh, logger_ptr->get_fd(logger_ptr));
		
	/* Attach to process monitoring */
	ipt_process_monitor_t *pm_ptr =  ipt_process_monitor_attach("ipt_process_monitor_t",alloc_se_ptr);

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

	/* Register for log events */
	if ( reactor->register_handler(reactor,(ipt_event_handler_t *)&my_handler, EVENT_HANDLER_READ_MASK) < 0 )
	{
		printf("Failed to register ipt_logger_t handler.\n");
		return -1;
	}

	/* Create socket to remote syslog server */
	if ( remote_syslog ) 
	{
		if ( (sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP) ) < 0 )
		{
			return -1;
		}

		memset((char *) &remote_in_addr, 0 , sizeof(remote_in_addr));

		remote_in_addr.sin_family = AF_INET;
		remote_in_addr.sin_port = 514;

		if ( inet_aton(remote_syslog, &remote_in_addr.sin_addr) == 0 )
		{
			return -1;
		}
	}

	tv.tv_sec = 1;
	tv.tv_usec = 0;

	while (1) 
	{
		reactor->run_event_loop(reactor, &tv);
	}

	return 0;
}
