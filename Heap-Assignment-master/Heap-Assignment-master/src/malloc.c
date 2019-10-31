#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

/*
  Gregory Ferguson
  1001524421

*/



#define ALIGN4(s)         (((((s) - 1) >> 2) << 2) + 4)
#define BLOCK_DATA(b)      ((b) + 1)
#define BLOCK_HEADER(ptr)   ((struct _block *)(ptr) - 1)


static int atexit_registered = 0;
static int num_mallocs       = 0;
static int num_frees         = 0;
static int num_reuses        = 0;
static int num_grows         = 0;
static int num_splits        = 0;
static int num_coalesces     = 0;
static int num_blocks        = 0;
static int num_requested     = 0;
static int max_heap          = 0;

/*
 *  \brief printStatistics
 *
 *  \param none
 *
 *  Prints the heap statistics upon process exit.  Registered
 *  via atexit()
 *
 *  \return none
 */
void printStatistics( void )
{
  printf("\nheap management statistics\n");
  printf("mallocs:\t%d\n", num_mallocs );
  printf("frees:\t\t%d\n", num_frees );
  printf("reuses:\t\t%d\n", num_reuses );
  printf("grows:\t\t%d\n", num_grows );
  printf("splits:\t\t%d\n", num_splits );
  printf("coalesces:\t%d\n", num_coalesces );
  printf("blocks:\t\t%d\n", num_blocks );
  printf("requested:\t%d\n", num_requested );
  printf("max heap:\t%d\n", max_heap );
}

struct _block
{
   size_t  size;         /* Size of the allocated _block of memory in bytes */
   struct _block *prev;  /* Pointer to the previous _block of allcated memory   */
   struct _block *next;  /* Pointer to the next _block of allcated memory   */
   bool   free;          /* Is this _block free?                     */
   char   padding[3];
};


struct _block *freeList = NULL; /* Free list to track the _blocks available */
struct _block *curr_next_fit = NULL;

/*
 * \brief findFreeBlock
 *
 * \param last pointer to the linked list of free _blocks
 * \param size size of the _block needed in bytes
 *
 * \return a _block that fits the request or NULL if no free _block matches
 *
 * \TODO Implement Next Fit
 * \TODO Implement Best Fit
 * \TODO Implement Worst Fit
 */
struct _block *findFreeBlock(struct _block **last, size_t size)
{
   struct _block *curr = freeList;
   struct _block *temp = NULL;



#if defined FIT && FIT == 0
   /* First fit */
   while (curr && !(curr->free && curr->size >= size))
   {
      *last = curr;
      curr  = curr->next;
   }
#endif

#if defined BEST && BEST == 0

/* Go through list and find the memory block with the least amount of space left over while still being bigger than the requested size */

   while (curr != NULL)
   {

    //Set temp value eqaul to the first block that meets the size requirements and is available
     if(curr->free && curr->size >= size && temp == NULL)
     {
       temp = curr;
     }

     //Checks to see if block is smaller than currently selected block and checks to make sure to see if size is equal or larger than request size.
     //i.e. find minimum value.
     if(curr->free && curr->size >= size)
     {
       if (temp->size > curr->size)
       {
         temp = curr;
       }
     }
     *last = curr;
     curr = curr->next;
   }

//Set curr eqaul to the smallest available block within the reqeusted size
curr = temp;

#endif

#if defined WORST && WORST == 0

/* Go through list and find the memory block with the most amount of space left over */
while (curr != NULL)
{
  //Set temp value eqaul to the first block that meets the size requirements and is available
  if(curr->free && curr->size >= size && temp == NULL)
  {
    temp = curr;
  }

  //Finds the largest block available
  if(curr->free && curr->size >= size)
  {
    if (temp->size < curr->size)
    {
      temp = curr;
    }
  }
  *last = curr;
  curr = curr->next;
}

//Set curr eqaul to the largest available block
curr = temp;

#endif

#if defined NEXT && NEXT == 0

     int loop;


  /*
  Next fit is a modified version of ‘first fit’.
  It begins as the first fit to find a free partition but when called next time it starts searching from where it left off, not from the beginning.
  */


  //If curr_next_fit is equal to NULL, next fit will behave as first fit.
  if(curr_next_fit == NULL)
  {
    while (curr && !(curr->free && curr->size >= size))
    {
       *last = curr;
       curr  = curr->next;
       curr_next_fit = curr;
    }
  }

  //If curr_next_fit is not equal to NULL, set curr eqaul to curr_next_fit (global pointer to keep track of position in freeList)
  else
  {
    curr = curr_next_fit;

    while (curr && !(curr->free && curr->size >= size))
    {
       *last = curr;
       curr  = curr->next;

       //If end of freeList is reached, will loop back to beginning of list to go through again.
       //Will only repeat once
       if(!curr && loop == 0)
       {
         loop = 1;
         //Start at beginning
         curr = freeList;
       }
    }
  }

  //Couldn't find space
  if (loop == 1)
  {
    return NULL;
  }

  loop = 0;

  //Restart loop, i.e. will act as first fit in next loop
  if(curr->next == NULL)
  {
    curr_next_fit == NULL;
  }


#endif

   return curr;
}

/*
 * \brief growheap
 *
 * Given a requested size of memory, use sbrk() to dynamically
 * increase the data segment of the calling process.  Updates
 * the free list with the newly allocated memory.
 *
 * \param last tail of the free _block list
 * \param size size in bytes to request from the OS
 *
 * \return returns the newly allocated _block of NULL if failed
 */
struct _block *growHeap(struct _block *last, size_t size)
{

    //Increment number of grows, since the program is growing the heap.
    num_grows++;

   /* Request more space from OS */
   struct _block *curr = (struct _block *)sbrk(0);
   struct _block *prev = (struct _block *)sbrk(sizeof(struct _block) + size);

   assert(curr == prev);

   /* OS allocation failed */
   if (curr == (struct _block *)-1)
   {
      return NULL;
   }

   /* Update freeList if not set */
   if (freeList == NULL)
   {
      num_blocks++;
      freeList = curr;
   }

   /* Attach new _block to prev _block */
   if (last)
   {
      num_blocks++;
      last->next = curr;
      curr->prev = last;
   }



   /* Update _block metadata */
   curr->size = size;
   curr->next = NULL;
   curr->free = false;


   max_heap += size;
   return curr;
}

/*
 * \brief malloc
 *
 * finds a free _block of heap memory for the calling process.
 * if there is no free _block that satisfies the request then grows the
 * heap and returns a new _block
 *
 * \param size size of the requested memory in bytes
 *
 * \return returns the requested memory allocation to the calling process
 * or NULL if failed
 */
void *malloc(size_t size)
{

  //Space requested
  num_requested += size;

   if( atexit_registered == 0 )
   {
      atexit_registered = 1;
      atexit( printStatistics );
   }

   /* Align to multiple of 4 */
   size = ALIGN4(size);

   /* Handle 0 size */
   if (size == 0)
   {
      return NULL;
   }

   /* Look for free _block */
   struct _block *last = freeList;
   struct _block *next = findFreeBlock(&last, size);

   /* */


   /*

   Could not get split to work (I would love to come talk about why this is not working)

   The basic logic here is to set next to the size its needs, and create a new node in the list.

   if(next->size >size)
   {

   num_splits++;
   num_blocks++;

    //Seting size of curr block to the request size.
    next->size = size;

    //Move pointer to the address to begin splitting
    temp = (int*)next + sizeof(struct _block) + size;

    temp->next = next->next;

    //Setting size of new block (struct block is for metadata)
    temp->size = (next->size - size) - sizeof(struct _block);

    //Mark block as available
    temp->next->free = true;

    //Update list
    next->next = &temp;

    }

   */

   /* Could not find free _block, so grow heap */
   if (next == NULL)
   {
      next = growHeap(last, size);
   }

   //If Heap did not grow, reusing block
   else
   {
     num_reuses++;
   }

   /* Could not find free _block or grow heap, so just return NULL */
   if (next == NULL)
   {
      return NULL;
   }

   //Called malloc successfully
   num_mallocs++;

   /* Mark _block as in use */
   next->free = false;

   /* Return data address associated with _block */
   return BLOCK_DATA(next);
}

/*
 * \brief free
 *
 * frees the memory _block pointed to by pointer. if the _block is adjacent
 * to another _block then coalesces (combines) them
 *
 * \param ptr the heap memory to free
 *
 * \return none
 */

void free(void *ptr)
{
   if (ptr == NULL)
   {
      return;
   }

   //Called free successfully
   num_frees++;

   /* Make _block as free */
   struct _block *curr = BLOCK_HEADER(ptr);
   assert(curr->free == 0);
   curr->free = true;

   /* TODO: Coalesce free _blocks if needed */

   /*
   Could never get Coalesce to work.
   My basic logic was to see if prev or next block was also free, and then combine their sizes.
   Once their size was combined, would update  freeList accordingly
   */


    /*
     struct _block *temp;
     struct _block *temp2;

     if(curr->prev != NULL)
     {
        temp = curr->prev;
            // Just need to add size of blocks, and adjust linked list.
         if(temp->free)
         {
           //Combine previous block with current block
           temp->size += curr->size;

           curr = curr->prev;
           curr->size = temp->size;
           curr->next = curr->next->next;

         }
      }

     else if(curr->next != NULL)
        temp2 = curr->next;

        if(temp2->free)
        {
        curr->size += temp->size;
        curr->next = curr->next->next;
        }
        */
}

/* Calloc zero-initalizes the buffer */
void *calloc(size_t nmeb, size_t size)
{

  //Use malloc to find new block of requested size
  void *new = malloc(size);

  //Memset initalizes the data in nmeb to zero
  memset(new,0,nmeb);

  return new;

}

/* Finds a new block of the desired size*/
void *realloc(void *ptr, size_t size)
{

  //Get address of specified pointer
  struct _block *curr = BLOCK_HEADER(ptr);

  //Use malloc to find new block of requested size
  void *new = malloc(size);

  //Copy data from old pointer to the new pointer
  memcpy(new,ptr,curr->size);

  //Free old pointer
  free(ptr);
  return new;
}

/* vim: set expandtab sts=3 sw=3 ts=6 ft=cpp: --------------------------------*/
