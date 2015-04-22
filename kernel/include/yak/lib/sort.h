#ifndef __YAK_SORT_H__
#define __YAK_SORT_H__

#include <yak/lib/types.h>

void qsort(void *data, size_t nitems, size_t size, int (*compare)(void *, void *));

/*
int cmp_int(void *a, void *b)
{
    return *(int *)a - *(int *)b;
}
*/

#endif
