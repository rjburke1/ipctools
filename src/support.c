#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>


#include "support.h"

int handle_is_write_ready(ipt_handle_t h, ipt_time_value_t *tv)
{
   struct timespec tp = ipt_time_value_to_timespec(*tv);

   fd_set write_fds;

   FD_ZERO(&write_fds);

   FD_SET(h, &write_fds);

   int rtn =  pselect(h + 1, NULL, &write_fds, NULL, &tp, NULL);

   if ( rtn <= 0 )
   {
      return -1;
   }
   
   return 0;
}

int handle_is_read_ready(ipt_handle_t h, ipt_time_value_t *tv)
{
   struct timespec tp = ipt_time_value_to_timespec(*tv);

   fd_set read_fds;

   FD_ZERO(&read_fds);

   FD_SET(h, &read_fds);

   int rtn =  pselect(h + 1, &read_fds, NULL, NULL, &tp, NULL);

   if ( rtn <= 0 )
   {
      return -1;
   }
   
   return 0;
}

void
timespec_print(struct timespec tp)
{
   printf("Timespec[%lu,%lu]\n",tp.tv_sec, tp.tv_nsec);
}

int
timespec_compare(struct timespec a, struct timespec b)
{
 
  if ( a.tv_sec < b.tv_sec || (a.tv_sec == b.tv_sec && a.tv_nsec < b.tv_nsec ) )
  {
     return -1;
  }

  if ( a.tv_sec > b.tv_sec || ( a.tv_sec == b.tv_sec && a.tv_nsec > b.tv_nsec) )
  {
     return 1;
  }

  return 0;
}

struct timespec
timespec_max(struct timespec a, struct timespec b)
{
   return timespec_compare(a,b) < 0 ? b : a;
}

struct timespec
timespec_min(struct timespec a, struct timespec b)
{
   return timespec_compare(a,b) < 0 ? a : b;
}

/* Assumes a > b */
struct timespec
timespec_diff(struct timespec a, struct timespec b)
{

   long sec = a.tv_sec - b.tv_sec;
   long nsec = a.tv_nsec - b.tv_nsec;

   if ( nsec < 0 )
   {
      nsec = 1000000000 + nsec;
      sec--;
   }

   struct timespec tmp = {sec, nsec};

   return tmp;
}

struct timespec
ipt_time_value_to_timespec(ipt_time_value_t tv)
{
   /* convert and correct */
   struct timespec tmp = { tv.tv_sec + tv.tv_usec / 1000000, tv.tv_usec % 1000000 * 1000 };

   return tmp;
}

int
record_and_set_non_blocking_mode (ipt_handle_t handle, int *val)
{
  
  	*val = fcntl(handle, F_GETFL,0);

	if ( val < 0 )
	{
		return -1;
	}


	if ( *val & O_NONBLOCK )
	{
		return 0;
	}

	if ( fcntl(handle, F_SETFL, *val | O_NONBLOCK) < 0 )
	{
		return -1;
	}

	return 0;
}

int
restore_mode(ipt_handle_t handle, int val)
{
	return fcntl(handle,F_SETFL,val);
}

int recv_n(int handle, void *buffer, size_t len, int *bt, ipt_time_value_t *tv)
{
	int bytes_transferred;
	int n;

	int val;

	if ( record_and_set_non_blocking_mode(handle, &val) < 0 )
	{
		return -1;
	}

	for( bytes_transferred = 0; bytes_transferred < len; bytes_transferred += n)
	{
		n = read( handle, buffer + bytes_transferred, len - bytes_transferred);

		if ( n == 0 ) 
		{
			restore_mode(handle,val);
			return 0;
		}

		if ( errno == EAGAIN )
		{
			int result = handle_is_read_ready(handle, tv);

			if ( result != -1 )
			{
				n = 0;
				continue;
			}
		}

		if ( n < 0 )
		{
			return -1;
		}
	}

	restore_mode(handle,val);

	return bytes_transferred;
}

int send_n(int handle, void *buffer, size_t len, int *bt, ipt_time_value_t *tv)
{
	int bytes_transferred = 0;
	int val;
	int n = 0;

	if ( record_and_set_non_blocking_mode(handle, &val) < 0 )
	{
		return -1;
	}

	for( bytes_transferred = 0; bytes_transferred < len; bytes_transferred += n)
	{
		n = write( handle, buffer + bytes_transferred, len - bytes_transferred);

		if ( n == 0 ) 
		{
			return 0;
		}

		if ( errno == EAGAIN )
		{
			int result = handle_is_write_ready(handle, tv);

			if ( result != -1 )
			{
				n = 0;
				continue;
			}
		}

		if ( n < 0 )
		{
			restore_mode(handle,val);
			return -1;
		}
	}

	restore_mode(handle,val);
	return bytes_transferred;
}
