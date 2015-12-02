#ifndef __IPCTOOLS_OFFSET_PTR_H__
#define __IPCTOOLS_OFFSET_PTR_H__

#include <stdio.h>
#include <stddef.h>

/**
 * typedef for struct ipt_op_t
 */
typedef struct ipt_op_t ipt_op_t;

/**
 * @struct ipt_op_t
 *
 * @brief This structure is used to managed the offset of an address from a base address.
 *
 */

struct ipt_op_t
{
	/**
         * platform independent way of representing an offset in place of an absolute address.
         */
	ptrdiff_t offset;
};

static inline void
ipt_op_set(ipt_op_t *this, void *ptr)
{
	this->offset = (char *)this - (char *)ptr;
}

static inline void * 
ipt_op_drf(ipt_op_t *this)
{
	return (char *)this - this->offset;
} 

static inline  ptrdiff_t 
ipt_op_offset(ipt_op_t *this)
{
	return this->offset;
}

static inline char *
ipt_add_offset(char * ptr, ptrdiff_t offset)
{
	return ptr + offset;
}

static inline char *
ipt_sub_offset(char *ptr, ptrdiff_t offset)
{
	return ptr - offset;
}


#endif
