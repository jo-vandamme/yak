#include <yak/kernel.h>
#include <yak/lib/types.h>
#include <yak/lib/sort.h>

static void swap(void *a, void *b, size_t size)
{
    char c;
    char *x = (char *)a, *y = (char *)b;
    while (size--) {
        c = *x;
        *x++ = *y;
        *y++ = c;
    }
}

static void _qsort(void *array, size_t size, int (*cmp)(void *, void *), int low, int high)
{
    if (high > low) {
        int pivot_idx = ((low + high) / 2) / size * size;
        swap(array + low, array + pivot_idx, size); // place pivot at the beginning
        void *pivot = array + low;
        int l = low + size; // order from second element to end of partition
        int r = high;
        while (l < r) {
            if (cmp(array + l, pivot) <= 0) {
                l += size;
            } else {
                for (r -= size; l < r && cmp(array + r, pivot) >= 0; r -= size) { }
                swap(array + l, array + r, size);
            }
        }
        l -= size;
        swap(array + low, array + l, size);
        _qsort(array, size, cmp, low, l);
        _qsort(array, size, cmp, r, high);
    }
}

void qsort(void *array, size_t nitems, size_t size, int (*cmp)(void *, void *))
{
    _qsort(array, size, cmp, 0, nitems * size);
}

