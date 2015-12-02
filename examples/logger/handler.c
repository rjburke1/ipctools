#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#include "handler.h"
#include "logger.h"

extern ipt_allocator_t *alloc_logs_ptr;
extern ipt_logger_t *logger_ptr;
extern char *log_file;
extern char *remote_syslog;
extern struct sockaddr_in remote_in_addr;
extern unsigned int log_size;
extern int sock;
extern FILE *lf_fp;


int handle_input(ipt_event_handler_t *eh_ptr, ipt_handle_t h)
{

	ipt_logger_message_t *msg_ptr = logger_ptr->dequeue(logger_ptr);

	if ( msg_ptr == NULL )
	{
		return 0;
	}	

	/* Log info to remote address */
	if ( remote_syslog != NULL && sock > 0 )
	{
		char buf[1024];
		char time[64];
		char hostname[64];

		memset(buf, 0 , sizeof(buf));
		memset(time, 0 , sizeof(time));
		memset(hostname, 0 , sizeof(hostname));

		/* Rules for generating message 
		 *
		 * The following is based on RFC 3164
		 *
		 * 1. All Compiled Networks Messages use a Faclity Code of ( 1 user-level messages )
		 * 2. The Compiled Networks Messages use Severity codes defined in the RFC.)
		 */
		
		/* 
		 * Strip the millisecond portion off of timestamp so we conform to RFC 3164 
		 *
		 *  Compiled Networks : MM-DD H:M:S:ML
		 *  Syslog            : MM-DD H:M:S 
		 *
		 * where ML is milliseconds.
		 */

		strcpy(time, msg_ptr->time);

		int index = strlen(time);

		while ( time[index] != ':' && index) index--;

		time[index] = '\0';

		gethostname(hostname, sizeof(hostname) - 1);

		snprintf(buf,1024 - 1 , "<%d>%s %s %s %s", logger_ptr->syslog_priority(logger_ptr, msg_ptr), time, 
			hostname, msg_ptr->originator, msg_ptr->message);
		
		sendto(sock, buf, strlen(buf) + 1, 0, (struct sockaddr *) &remote_in_addr, sizeof(remote_in_addr)); 
		
	}

	/* Log info to local file */
	if ( lf_fp != NULL)
	{
		fprintf(lf_fp, "%s %s %s\n",msg_ptr->time,msg_ptr->originator, msg_ptr->message);
		fflush(lf_fp);
	}

	logger_ptr->free(logger_ptr, msg_ptr);

	return 0;
}

int set_handle(ipt_event_handler_t *eh_ptr, ipt_handle_t h)
{
	eh_ptr->_h = h;
}

ipt_handle_t get_handle(ipt_event_handler_t *eh_ptr)
{
	return eh_ptr->_h;
}
