#ifndef __IPCTOOLS_ALLOCATOR_SHM_H__
#define __IPCTOOLS_ALLOCATOR_SHM_H__

#include "allocator.h"

/** \addtogroup Allocators
 * @{
 */

/** typedef for the shared memory key */
typedef unsigned int ipt_allocator_shm_key_t;

/** 
 * Create a Shared Memory Allocator.
 *
 * @param[in] size The size of the requested allocator.
 * @param[in] key  The shared memory key.
 *
 * @retval NULL Failed to create the allocator.
 * @retval !NULL  Pointer to successfully created allocator.
 */
ipt_allocator_t * ipt_allocator_shm_create(size_t size, ipt_allocator_shm_key_t key);

/**
 * Destroy the shared allocator. All memory will be released.
 *
 * @param[in] key Shared Memory key.
 */
ipt_allocator_t * ipt_allocator_shm_attach(ipt_allocator_shm_key_t key);

/** @} */

#endif
