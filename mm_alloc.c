/*
*   linked list of blocks will be used for this memory allocator.
*
*  The pointers will be of size 4bytes


*  Must validate that a pointer always points to the correct block.
*
*  We must be able to find block of birtual memory in the heap.
*  Starting  at the base address of the heap if the found block fit our need we return its address if not, we continue until we find a 		fitting one
*
*  We need to be able to extend the heap because at times since we are not always guaranteed a fitting block
*
*  We must be able to split the block by just inserting a new block in the list
*
*  I need to write a function mm_malloc ,Our mm_malloc that first align the requested size
*  Then  search for a free block that is wide enough.
*  after finding the  block then Try to split the block, then mark it as used
*  if we cant find a block wide enough we extend the heap.

*
*  With The mm_free function we need to verify the pointer and get the corresponding block. We also need to try to release memory.
*  if we are at the end of the heap, we just have to put the
*  break at the block position with a call to brk().
*
*  If the pointer is valid the we get the block address ,we mark it free
*  If the previous block exists and is also free, we fusion the two blocks.
*  We are also going try fusion with then next block, if we're at the last block we release memory.
*  If there is no more block, we set heap_ptr to NULL.
*  If the pointer is not valid, we do nothing
*  
*  We need another function for resizing block. 
*  We allocate a new block of the given size using mm_malloc function
*  just copying data from the old one to the new one, Free the old block and return the new pointer.
*  If the size doesn't change, or the extra-available size is sufficient, we do nothing;
*  to shrink the block, we try a split;
*  If the next block is free and gives sufficient space, we fusion and try to split only if necessary.
*/

#include "mm_alloc.h"
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#define ASSERT assert
#define align4(x) (((((x)-1)>>2)<<2)+4)
#define S_BLOCK_SIZE sizeof ( struct s_block )

// mutex to protect allocation of memory
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
 
#define TRUE 1
#define FALSE 0


s_block_ptr heap_ptr = NULL; 
s_block_ptr last; 


s_block_ptr 
find_block (s_block_ptr *last , size_t size ){

	s_block_ptr p;
 pthread_mutex_lock( &lock );
    for(p =heap_ptr; p != NULL; p = p->next){
	 *last = p;
	 pthread_mutex_unlock( &lock );
       if(p && !(p->free && p->size >= size ))
	     return p; 
	 pthread_mutex_unlock( &lock );
	  }
	 pthread_mutex_unlock( &lock );
		return NULL;
    
}

 
s_block_ptr 
extend_heap (s_block_ptr last , size_t size){
 
s_block_ptr new_block = (s_block_ptr)sbrk(size); 
	 pthread_mutex_lock( &lock );
 if (sbrk( S_BLOCK_SIZE + size) < 0)
      return (NULL );
      
	 new_block->size = size;
	 new_block->next = NULL;
	 new_block->prev = last;
 	 new_block->ptr  = new_block->data;
         new_block->free = TRUE; 
	 pthread_mutex_unlock( &lock );
	  
      if (last){
	 last ->next = new_block; 
           new_block->free =FALSE;
	}
     pthread_mutex_unlock( &lock );
  
return new_block; 
}


s_block_ptr 
fusion_block( s_block_ptr blck){

        ASSERT(blck->free == TRUE);   
   
	 if (blck->next && blck->next ->free ){
	       blck->size =blck->size + S_BLOCK_SIZE + blck->next ->size;
	if (blck->next->next)
		blck->next->next->prev = blck;
		blck->next = blck->next->next;
	}
	return (blck);
}

void 
split_block( s_block_ptr blck, size_t new_size){

  s_block_ptr new_block;
 pthread_mutex_lock( &lock );
 if(new_size > 0){
     new_block = ( s_block_ptr )(blck->data + new_size);
        new_block->size = blck->size - new_size- S_BLOCK_SIZE ;
	new_block ->next = blck->next;
	new_block ->prev = blck;
	new_block ->free =TRUE;

	new_block->ptr = new_block ->data;
	blck->size = new_size;
	blck->next = new_block;
    pthread_mutex_lock( &lock );
    if (new_block->next)
 	  new_block->next ->prev = new_block;
	    blck->next = new_block;
	 pthread_mutex_lock( &lock );
      }
     pthread_mutex_lock( &lock );
 }

static void 
copy_block ( s_block_ptr blck , s_block_ptr data){
int *sd ,* ddata ;
size_t j;
	sd = blck ->ptr;
	ddata = data ->ptr;
 pthread_mutex_lock( &lock );
for (j=0; (j*4)<blck->size && (j*4)<data ->size; j++){
	ddata [j] = sd [j];
  pthread_mutex_unlock( &lock );
   }
 }


void * 
mm_malloc ( size_t size ){
s_block_ptr blck,last;
size_t s;

s = align4 (size );
   pthread_mutex_lock( &lock );
  if (heap_ptr == NULL) {
	last = heap_ptr;
	blck = find_block (&last ,s);
     pthread_mutex_unlock( &lock );
	if (blck) {

	if ((blck->size - s) >= ( S_BLOCK_SIZE + 4))
	   split_block (blck,s);
        pthread_mutex_unlock( &lock );
	   blck->free =FALSE;
	} else {

   	   blck = extend_heap (last ,s);
	  if (!blck)
	  return (NULL );
	}
     pthread_mutex_unlock( &lock );
     } 
      
 else {
	blck = extend_heap (NULL ,s);
  	if (!blck)
		return (NULL );
 		heap_ptr = blck;
  	    }
	return (blck->data);
}


s_block_ptr 
get_block (void *addr){

     char *temp;
       temp =(char*)addr;
         return (s_block_ptr)(temp -= S_BLOCK_SIZE );
}

void *base =NULL;
int is_valid_addr (void *addr){

if (base){
if (addr>base && addr< sbrk (0)){
 return (addr == ( get_block (addr))->ptr );
	}
     }
   return FALSE;
}

void * 
mm_realloc (void *addr, size_t size){
  size_t s;
 s_block_ptr pb, new;
 void *newp;
 
	if (!addr)
       pthread_mutex_lock( &lock );
	return ( mm_malloc (size ));
       pthread_mutex_unlock( &lock );
	if ( is_valid_addr (addr)){
	s = align4 (size );
	pb = get_block (addr);
	if (pb->size >= s){

	if (pb->size - s >= ( S_BLOCK_SIZE + 4))
	split_block (pb,s);
     }
else{


	if (pb->next && pb->next ->free
	&& (pb->size + S_BLOCK_SIZE + pb->next ->size) >= s){

	fusion_block(pb);
	if (pb->size - s >= ( S_BLOCK_SIZE + 4))
	split_block (pb,s);
	}
	else
	{
   pthread_mutex_lock( &lock );

	newp = mm_malloc (s);
   pthread_mutex_unlock( &lock );
	if (!newp)
		return (NULL );
	        new = get_block (newp );
           
		copy_block (pb,new );
		free(addr);
       		return (newp);
           }
       }
       return (addr);
    }
   return (NULL);
}

void 
mm_free(void *addr){
 s_block_ptr blck;

   pthread_mutex_lock(&lock);
 if(is_valid_addr (addr)){
       blck = get_block (addr);
       blck->free = TRUE;
   pthread_mutex_unlock( &lock );

	if(blck->prev && blck->prev ->free)
	   blck = fusion_block(blck->prev );
	   pthread_mutex_unlock( &lock );
	/* Also try fusion with the next block */
	if (blck->next)
	fusion_block(blck);
	 else{

	if (blck->prev)
	blck->prev->next = NULL;

	else
	blck->prev->next = NULL;
	brk(blck);
	}
     }
   pthread_mutex_unlock( &lock );
  }
