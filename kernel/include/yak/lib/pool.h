#ifndef __YAK_POOL_H__
#define __YAK_POOL_H__

#include <yak/lib/stack.h>

#define POOL_DECLARE(name, type, size) \
    struct __object_##name { \
        struct stack_node node; \
        type data; \
    }; \
    static struct __object_##name __pool_##name[(size)] __attribute__((section("aod"))); \
    static stack_t __stack_##name

#define POOL_INIT(name) \
    stack_init(&__stack_##name, \
            (struct stack_node *)__pool_##name, \
            sizeof(__pool_##name) / sizeof(struct __object_##name), \
            sizeof(struct __object_##name))
     
#define POOL_ALLOC(name) (typeof(((struct __object_##name *)0)->data) *)stack_alloc(&__stack_##name)

#define POOL_FREE(name, o) stack_free(&__stack_##name, (struct stack_node *)o)

#endif
