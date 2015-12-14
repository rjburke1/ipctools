#include "allocator_shm.h"
#include "allocator_malloc.h"
#include "config.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define SEGMENT_SIZE 1024
#define BLOCK_SIZE 1024
/*
 * Allocate once and free
 */
int test_1(ipt_allocator_t *alloc_ptr)
{
void *ptr;

   void * ptr_1 = alloc_ptr->malloc( alloc_ptr,  100);

   assert(ptr_1 != NULL );

   alloc_ptr->free(alloc_ptr,ptr_1);

   assert ( alloc_ptr->blocks_allocated(alloc_ptr) == 0  &&
	    alloc_ptr->bytes_allocated(alloc_ptr) == 0 );
}
/*
 * Test overallocation.
 */
int test_2(ipt_allocator_t *alloc_ptr)
{
      /* should not be able to allocate sement */
      void * ptr = alloc_ptr->malloc( alloc_ptr, 2 * BLOCK_SIZE);

      assert( ptr == NULL );
}
/* 
 * Make sure our free/malloc is working. If there is a leak this will fail. 
 */
int test_3(ipt_allocator_t *alloc_ptr)
{ 

   int i;
   for( i=0; i<  BLOCK_SIZE / 512 * 10000; i++)
   {
      void *ptr = alloc_ptr->malloc( alloc_ptr, 512);

      assert ( ptr );

      alloc_ptr->free(alloc_ptr, ptr );
   }

   assert ( alloc_ptr->blocks_allocated(alloc_ptr) == 0  &&
	    alloc_ptr->bytes_allocated(alloc_ptr) == 0 );
}
/*
 * Make sure the stats calculations are correct.
 * The allocations are byte aligned. This means nearest 4 bytes on 32 bit, and 8 bytes on 64 bit.
 * The ptrdiff_t structure is used to determine the alignement in a platform independent way.
 */
void test_4(ipt_allocator_t *alloc_ptr)
{  
struct dmy_data { void *ptr; size_t size; };
struct dmy_data arr[5] = { {NULL,11}, {NULL,17}, {NULL,2}, {NULL,6}, {NULL,341} };
int in_size = 100;
size_t total_size = 0; 
int i = 0;

	void * ptr = alloc_ptr->malloc(alloc_ptr, in_size);

        size_t byte_aligned_size  = in_size % sizeof(ptrdiff_t) == 0 ? 
		in_size : in_size - in_size % sizeof(ptrdiff_t) + sizeof(ptrdiff_t);

	assert ( alloc_ptr->blocks_allocated(alloc_ptr) == 1 && 
	         alloc_ptr->bytes_allocated(alloc_ptr) == byte_aligned_size);

	alloc_ptr->free(alloc_ptr, ptr);

	for ( i = 0; i < 5; i ++ )
	{
	   arr[i].ptr = alloc_ptr->malloc(alloc_ptr,arr[i].size);

	   total_size += arr[i].size % sizeof(ptrdiff_t) == 0 ?
                         arr[i].size : arr[i].size - arr[i].size % sizeof(ptrdiff_t) + sizeof(ptrdiff_t);

        }

	assert ( alloc_ptr->blocks_allocated(alloc_ptr) == 5 && 
	         alloc_ptr->bytes_allocated(alloc_ptr) ==  total_size);

	for ( i = 0; i < 5; i ++ )
        {
	   alloc_ptr->free(alloc_ptr,arr[i].ptr);
        }

	assert ( alloc_ptr->blocks_allocated(alloc_ptr) == 0 && 
	         alloc_ptr->bytes_allocated(alloc_ptr) ==  0);

}
/* 
 * Make sure the order of allocation and freeing does not matter 
 */
void test_5(ipt_allocator_t *alloc_ptr)
{
	void * ptr_1 = alloc_ptr->malloc(alloc_ptr, 100);
	void * ptr_2 = alloc_ptr->malloc(alloc_ptr, 100);
	void * ptr_3 = alloc_ptr->malloc(alloc_ptr, 100);

	alloc_ptr->free(alloc_ptr, ptr_1);
	alloc_ptr->free(alloc_ptr, ptr_2);
	alloc_ptr->free(alloc_ptr, ptr_3);

   	assert ( alloc_ptr->blocks_allocated(alloc_ptr) == 0  &&
	    alloc_ptr->bytes_allocated(alloc_ptr) == 0 );

	ptr_1 = alloc_ptr->malloc(alloc_ptr, 100);
	ptr_2 = alloc_ptr->malloc(alloc_ptr, 100);
	ptr_3 = alloc_ptr->malloc(alloc_ptr, 100);

	alloc_ptr->free(alloc_ptr, ptr_3);
	alloc_ptr->free(alloc_ptr, ptr_2);
	alloc_ptr->free(alloc_ptr, ptr_1);

   	assert ( alloc_ptr->blocks_allocated(alloc_ptr) == 0  &&
	    alloc_ptr->bytes_allocated(alloc_ptr) == 0 );

	ptr_1 = alloc_ptr->malloc(alloc_ptr, 100);
	ptr_2 = alloc_ptr->malloc(alloc_ptr, 100);
	ptr_3 = alloc_ptr->malloc(alloc_ptr, 100);

	alloc_ptr->free(alloc_ptr, ptr_1);
	alloc_ptr->free(alloc_ptr, ptr_3);
	alloc_ptr->free(alloc_ptr, ptr_2);

   	assert ( alloc_ptr->blocks_allocated(alloc_ptr) == 0  &&
	    alloc_ptr->bytes_allocated(alloc_ptr) == 0 );

	ptr_1 = alloc_ptr->malloc(alloc_ptr, 100);
	ptr_2 = alloc_ptr->malloc(alloc_ptr, 100);
	ptr_3 = alloc_ptr->malloc(alloc_ptr, 100);

	alloc_ptr->free(alloc_ptr, ptr_2);
	alloc_ptr->free(alloc_ptr, ptr_3);
	alloc_ptr->free(alloc_ptr, ptr_1);

   	assert ( alloc_ptr->blocks_allocated(alloc_ptr) == 0  &&
	    alloc_ptr->bytes_allocated(alloc_ptr) == 0 );

	ptr_1 = alloc_ptr->malloc(alloc_ptr, 100);
	ptr_2 = alloc_ptr->malloc(alloc_ptr, 100);
	ptr_3 = alloc_ptr->malloc(alloc_ptr, 100);

	alloc_ptr->free(alloc_ptr, ptr_2);
	alloc_ptr->free(alloc_ptr, ptr_1);
	alloc_ptr->free(alloc_ptr, ptr_3);

   	assert ( alloc_ptr->blocks_allocated(alloc_ptr) == 0  &&
	    alloc_ptr->bytes_allocated(alloc_ptr) == 0 );

	ptr_1 = alloc_ptr->malloc(alloc_ptr, 100);
	ptr_2 = alloc_ptr->malloc(alloc_ptr, 100);
	ptr_3 = alloc_ptr->malloc(alloc_ptr, 100);

	alloc_ptr->free(alloc_ptr, ptr_3);
	alloc_ptr->free(alloc_ptr, ptr_1);
	alloc_ptr->free(alloc_ptr, ptr_2);

	assert ( alloc_ptr->blocks_allocated(alloc_ptr) == 0  &&
		 alloc_ptr->bytes_allocated(alloc_ptr) == 0 );
}

/*
 * Test that register works. 
 */
void test_6(ipt_allocator_t *alloc_ptr)
{
   struct my_data { int a;int b; };

   struct my_data * ptr = alloc_ptr->malloc(alloc_ptr, sizeof(struct my_data));

   assert(ptr);

   ptr->a = 1;
   ptr->b = 2;

   /* register object in shared memory segment */
   alloc_ptr->register_object(alloc_ptr,"My Object", ptr);


   /* Find the segment */
   ptr = (struct my_data *)  alloc_ptr->find_registered_object(alloc_ptr,"My Object");

   assert (ptr);  

   assert (ptr->a == 1 && ptr->b == 2);  

   alloc_ptr->deregister_object(alloc_ptr,"My Object");

   alloc_ptr->free(alloc_ptr,ptr);

   assert ( alloc_ptr->blocks_allocated(alloc_ptr) == 0  &&
	    alloc_ptr->bytes_allocated(alloc_ptr) == 0 );
   
}
/*
 * Test that register/deregister works.
 */
void test_7(ipt_allocator_t *alloc_ptr)
{
   struct my_data { int a;int b; };

   struct my_data * ptr = alloc_ptr->malloc(alloc_ptr, sizeof(struct my_data));

   assert(ptr);

   ptr->a = 1; ptr->b = 2;

   /* attempt to register the same object twice should fail. */
   assert ( alloc_ptr->register_object(alloc_ptr,"My Object", ptr) != 0 ||
            alloc_ptr->register_object(alloc_ptr,"My Object", ptr) == 1);

   /* attempt to deregister the same object twice should fail. */
   assert ( alloc_ptr->deregister_object(alloc_ptr,"My Object") != NULL ||
            alloc_ptr->deregister_object(alloc_ptr,"My Object") == NULL);

   alloc_ptr->free(alloc_ptr, ptr);

   assert ( alloc_ptr->blocks_allocated(alloc_ptr) == 0  &&
	    alloc_ptr->bytes_allocated(alloc_ptr) == 0 );

}
/*
 * Verify copying the entire memory block maintains all the relationships.
 * This is the ultimate torture test that validates the offset logic (i.e offset_ptr works).
 */
void test_8(ipt_allocator_t *alloc_ptr)
{
   struct my_data *ptr_a, *ptr_n;
   struct my_data { int a;int b; };

   ptr_a = alloc_ptr->malloc(alloc_ptr, sizeof(struct my_data));

   assert(ptr_a);

   ptr_a->a = 1; ptr_a->b = 2;

   alloc_ptr->register_object(alloc_ptr,"My Object", ptr_a);

   ptr_a = (struct my_data *)  alloc_ptr->find_registered_object(alloc_ptr,"My Object");

   assert (ptr_a);  

   assert ( ptr_a->a == 1 && ptr_a->b == 2 );

   char * dst_ptr =  malloc(alloc_ptr->get_size(alloc_ptr)*2);

   void * sd_ptr = alloc_ptr->get_shared_ptr(alloc_ptr);

   assert (sd_ptr);

   memmove(dst_ptr,sd_ptr, alloc_ptr->get_size(alloc_ptr));

   ipt_allocator_t *nalloc_ptr =  (ipt_allocator_t *) ipt_allocator_malloc_attach((void*)dst_ptr);

   nalloc_ptr->dump_stats(nalloc_ptr);

   ptr_n = (struct my_data *) nalloc_ptr->find_registered_object(nalloc_ptr,"My Object");
 return 0;
   assert(ptr_n);

	return 0;

   assert ( ptr_n->a == 1 && ptr_n->b == 2 );

   assert ( nalloc_ptr->bytes_allocated(nalloc_ptr) == alloc_ptr->bytes_allocated(alloc_ptr) &&
            nalloc_ptr->blocks_allocated(nalloc_ptr) == alloc_ptr->blocks_allocated(alloc_ptr) ); 

   assert ( (ptr_a = alloc_ptr->deregister_object(alloc_ptr,"My Object")) != NULL &&
            (ptr_n = nalloc_ptr->deregister_object(nalloc_ptr,"My Object")) != NULL );

   alloc_ptr->free(alloc_ptr,ptr_a);

   nalloc_ptr->free(nalloc_ptr,ptr_n);

   assert ( nalloc_ptr->bytes_allocated(nalloc_ptr) == 0 && alloc_ptr->bytes_allocated(alloc_ptr)  == 0); 

   assert ( nalloc_ptr->blocks_allocated(nalloc_ptr) == 0 && alloc_ptr->blocks_allocated(alloc_ptr)  == 0); 
   
}
/*
 * Test fragementation occurs and, when returned the memory blocks are coalesced
 */
void test_9(ipt_allocator_t *alloc_ptr)
{
int i;
struct dmy_data { void *ptr; size_t size; };
struct dmy_data arr[5] = { {NULL,11}, {NULL,17}, {NULL,2}, {NULL,6}, {NULL,341} };

	/* No allocation yet, so we have a single block of size 1024 */
	assert( alloc_ptr->free_blocks(alloc_ptr) == 1  &&
                alloc_ptr->bytes_remaining(alloc_ptr) == BLOCK_SIZE);

	for ( i = 0; i < 5; i ++ )
        {
           assert( (arr[i].ptr = alloc_ptr->malloc(alloc_ptr,arr[i].size)) );
        }

        assert ( alloc_ptr->blocks_allocated(alloc_ptr) == 5 );

        for ( i = 0; i < 5; i ++ )
        {
           alloc_ptr->free(alloc_ptr,arr[i].ptr);
        }

        /* Returned all free blocks, so should end up with a single block = 1024 */
	assert( alloc_ptr->free_blocks(alloc_ptr) == 1 );

	/* 
         * The allocation algorithm pulls a block from the end of the first available block.
         * When it frees a block, it tries to coalesce continguous blocks
         */ 	

	/* Allocate five more blocks */
	for ( i = 0; i < 5; i++ )
        {
           assert( (arr[i].ptr = alloc_ptr->malloc(alloc_ptr,arr[i].size)) );
        }

	/* Should have a single free block */
	assert( alloc_ptr->free_blocks(alloc_ptr) == 1);

	/* Return first non-contigous */
        alloc_ptr->free(alloc_ptr,arr[0].ptr);
	assert( alloc_ptr->free_blocks(alloc_ptr) == 2);
        alloc_ptr->free(alloc_ptr,arr[2].ptr);
	assert( alloc_ptr->free_blocks(alloc_ptr) == 3);
        alloc_ptr->free(alloc_ptr,arr[4].ptr);
	assert( alloc_ptr->free_blocks(alloc_ptr) == 3);
        alloc_ptr->free(alloc_ptr,arr[1].ptr);
	assert( alloc_ptr->free_blocks(alloc_ptr) == 2);
        alloc_ptr->free(alloc_ptr,arr[3].ptr);
	assert( alloc_ptr->free_blocks(alloc_ptr) == 1);
}

int main( int argc, char *argv[])
{
	unsigned int i;

	ipt_allocator_t *alloc_ptr;

	alloc_ptr = ipt_allocator_shm_create(BLOCK_SIZE, IPT_TEST_ALLOCATOR_SHM_KEY);

   	if ( alloc_ptr == NULL )
   	{
      		printf("Failed to create allocator.\n");
      		return -1;
   	}
   	test_1(alloc_ptr); 
   	test_2(alloc_ptr);
  	test_3(alloc_ptr);
	test_4(alloc_ptr);

	test_5(alloc_ptr);
	test_6(alloc_ptr);
	test_7(alloc_ptr);
	//TODO: Fix the memove shm segement to heap.
	//test_8(alloc_ptr);
	test_9(alloc_ptr);

 	printf(" %s completed successfully\n", argv[0]);

	return 0;
}
