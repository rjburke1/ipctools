#ifndef __IPCTOOLS_ALLOCATOR_MALLOC_H__
#define __IPCTOOLS_ALLOCATOR_MALLOC_H__

#include "allocator.h"

/** 
 * \addtogroup Allocators
 * @{
 */

/** 
 * Create a heap allocator.
 *
 * @param[in] size The size of the requested allocator.
 *
 * @retval NULL  Failed to create the allocator.
 * @retval !NULL Pointer to successfully created allocator.
 */
ipt_allocator_t * ipt_allocator_malloc_create(size_t size);


/**
 * Attach to an existing heap allocator 
 *
 * @param[in] base_address The base address of an existing allocator 
 *
 * @retval NULL  Failed to create the allocator.
 * @retval !NULL Pointer to successfully created allocator.
 */
ipt_allocator_t * ipt_allocator_malloc_attach(void *base_address);

/** @{ */

#endif
