#include <os/mm.h>

ptr_t memCurr = FREEMEM;
ptr_t heapCurr = HEAP;
ptr_t allocPage(int numPage)
{
    // align PAGE_SIZE
    ptr_t ret = ROUND(memCurr, PAGE_SIZE);
    memCurr = ret + numPage * PAGE_SIZE;
    return memCurr;
}

void* kmalloc(size_t size)
{
    ptr_t ret = ROUND(heapCurr, 4);
    heapCurr = ret + size;
    return (void*)ret;
}
