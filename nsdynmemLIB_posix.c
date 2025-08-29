// nsdynmemLIB_posix.c
// Standard malloc/free wrappers for ns_dyn_mem_* functions on POSIX/Linux

#include <stdlib.h>
#include "nsdynmemLIB.h"

void *ns_dyn_mem_alloc(size_t size) {
    return malloc(size);
}

void ns_dyn_mem_free(void *ptr) {
    free(ptr);
}

void *ns_dyn_mem_realloc(void *ptr, size_t size) {
    return realloc(ptr, size);
}

void *ns_dyn_mem_temporary_alloc(size_t size) {
    return malloc(size);
}

// Add more wrappers if needed
