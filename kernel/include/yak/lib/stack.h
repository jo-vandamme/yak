#ifndef __YAK_STACK_H__
#define __YAK_STACK_H__

#include <yak/kernel.h>
#include <yak/lib/atomic.h>

struct stack_node
{
    struct stack_node *next;
};

union stack_head
{
    uint128_t data;
    struct {
        struct stack_node *node;
        uint64_t aba;
    } __attribute__((aligned(16), packed));
};

// the size field isn't updated atomically with the head and should
// not be used as an index into node_buffer
typedef struct
{
    struct stack_node *node_buffer;
    union stack_head head;
    size_t size;
    size_t max_size;
    unsigned node_size;
} stack_t;

void stack_init(stack_t *stack, struct stack_node *buffer, size_t size, unsigned node_size);
struct stack_node *stack_alloc(stack_t *stack);
void stack_free(stack_t *stack, struct stack_node *node);

#endif
