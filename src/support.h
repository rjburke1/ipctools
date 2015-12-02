#ifndef __IPCTOOLS_SUPPORT_H__
#define __IPCTOOLS_SUPPORT_H__

#include "event_handler.h"

int handle_is_write_ready(ipt_handle_t h, ipt_time_value_t *tv);

int handle_is_read_ready(ipt_handle_t h, ipt_time_value_t *tv);

int recv_n(int fd, void * buffer, size_t size, int *bt, ipt_time_value_t *tv);

int send_n(int fd, void * buffer, size_t size, int *bt, ipt_time_value_t *tv);

void timespec_print(struct timespec tp);

int timespec_compare(struct timespec a, struct timespec b);

struct timespec timespec_max(struct timespec a, struct timespec b);

struct timespec timespec_min(struct timespec a, struct timespec b);

struct timespec timespec_diff(struct timespec a, struct timespec b);

struct timespec ipt_time_value_to_timespec(ipt_time_value_t tv);

#endif
