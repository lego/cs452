#ifndef __ALLOC_H__
#define __ALLOC_H__

/*
 * Experimental heap style alloc
 *
 * Currently doesn't seem to work, but presumed a result of static variables
 * not working. This will also be relatively hard to do 'right' (free-ing, defraging)
 *
 * The best use case would be one off allocations with no expected free (end of life)
 */

/**
 * allocates a chunk of memory
 * @param  size bytes of memory
 * @return      status
 *              0 => OK
 */
void *alloc(unsigned int size);

/**
 * frees memory that was allocated
 * @return      status
 *              0 => OK
 *
 * Currently does nothing (not easy to implement)
 */
int free(void * ptr);

#endif
