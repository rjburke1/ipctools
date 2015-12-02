#ifndef __IPCTOOLS_ALLOCATOR_H__
#define __IPCTOOLS_ALLOCATOR_H__
#include <stddef.h>

/** typedef for struct ipt_allocator_t */
typedef struct ipt_allocator_t ipt_allocator_t;

/** \defgroup Allocators The collection of allocators. 
 * The allocators manage memory by allocating a contigous block of memory, and each 
 * call to malloc returns byte aligned blocks. As blocks are returned, the memory is 
 * coelesced in order to eliminate fragmentation. The supported allocators are:
 *
 * heap allocator         : ipt_allocator_malloc_t
 * shared memory allocator: ipt_allocator_shm_t
 *
 * @{ 
 */

/**
 * @struct ipt_allocator_t
 *
 * @brief The abstract class for memory allocators.
 *
 * The abstract allocator class. Memory allocated is byte alligned, and subtracted from the contiguous
 * block of memory. When the memory is returned, it is coalesced with adjacent free blocks.
 * 
 * The allocation scheme uses an intrusive list so there is overhead associated with each allocation equal
 * to the size of the linkage structure per allocated block. 
 */
struct ipt_allocator_t
{
	/**
	 * Allocate a block of memory.
	 *
	 * @param[in] this The allocator's this pointer.
	 * @param[in] size The size of block to be allocated.
	 *
	 * @retval NULL  Successfully allocated block.
	 * @retval !NULL Failed to allocate block.
	 */
	void * (*malloc)(ipt_allocator_t *this, size_t size);

	/**
	 * Free a block of memory.
	 *
	 * @param[in] this The allocator's this pointer.
	 * @param[in] ptr  The pointer to block of memory previously allocated.
	 *
	 */
	void (*free)(ipt_allocator_t *this, void *ptr);

	/**
	 * Register an allocated block of memory with a well-known name. The memory must have been allocated
         * by this allocator. This is used to find * objects such as shared lists, etc by multiple processes.
	 *
	 * @param[in] this The allocator's this pointer.
	 * @param[in] name The well-known name of the object.
	 * @param[in] ptr  The pointer to block of memory previously allocated
	 *                 by this allocator.
	 *
	 * @retval 0 Successfully allocated block.
	 * @retval 1 Failed to allocate block.
	 */
	int (*register_object)(ipt_allocator_t *this, const char *name, void *ptr);

	/**
	 * Deregister a previously allocated object.
	 *
	 * @param[in] this The allocator's this pointer.
	 * @param[in] name The well-known name used to register the object.
	 *
	 * @retval 0 Successfully allocated block.
	 * @retval 1 Failed to allocate block.
	 */
	void * (*deregister_object)(ipt_allocator_t *this, const char *name);

	/**
	 * Find a registered allocated object.
	 *
	 * @param[in] this The allocator's this pointer.
	 * @param[in] name The name of the registered object.
	 *
	 * @retval NULL  Successfully allocated block.
	 * @retval !NULL Failed to allocate block.
	 */
	void * (*find_registered_object)(ipt_allocator_t *this, const char *name);
	
	/**
	 * Dump the basic allocator statistics.
	 *
	 * @param[in] this The allocator's this pointer.
	 *
	 */
	void (*dump_stats)(ipt_allocator_t *this);

	/**
	 * Get the size of the allocator.
	 *
	 * @param[in] this The allocator's this pointer.
	 *
	 * @retval size_t Size of the block of memory allocated.
	 */
	size_t (*get_size)(ipt_allocator_t *this);

	/**
	 * Destroy the allocator. All memory will be free'd
	 *
	 * @param[in] this the allocator's this pointer.
	 */
	void (*destroy)(ipt_allocator_t *this);

	/**
	 * Blocks allocated.
	 *
	 * @param[in] this The allocator's this pointer.
	 *
	 * @retval size_t  The number of blocks malloc'd. This is essentially 
         *                 the number of times malloc has been called.
	 */
	size_t (*blocks_allocated)(ipt_allocator_t *this);

	/**
	 * Return the bytes malloc'd by this allocator. 
	 *
	 * @param[in] this The allocator's this pointer.
	 *
	 * @retval size_t  The total number of bytes malloc'd.
	 */
	size_t (*bytes_allocated)(ipt_allocator_t *this);

        /**
         * Access the contiguous memory block.
         *
         * @param[in] this The allocator's this pointer
	 *
         * @retval void* The pointer to internal control structure that manages the 
         *               contiguous block of allocated memory.
         *
         */
        void * (*get_shared_ptr)(ipt_allocator_t *this);

};

/**
 * Allocator destructor
 * 
 * @param[in] this The allocator's this pointer.
 *
 * @retval -1 failed to destroy the object.
 * @retval 0 successfully destroyed the object.
 * 
 */
int ipt_allocator_destroy(ipt_allocator_t *this);

/** @} */

#endif
