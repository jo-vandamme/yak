#include <yak/kernel.h>
#include <yak/lib/atomic.h>
#include <yak/lib/stack.h>

void stack_init(stack_t *stack, struct stack_node *buffer, size_t size, unsigned node_size)
{
    stack->node_buffer = buffer;
    stack->head.node = buffer;
    stack->head.aba = 0;
    stack->size = size;
    stack->max_size = size;
    stack->node_size = node_size;
}

void stack_free(stack_t *stack, struct stack_node *node)
{
    union stack_head orig_head;
    union stack_head new_head;

    if (node < stack->node_buffer || node > stack->node_buffer + stack->max_size * stack->node_size)
        return;

    // copy the head
    orig_head.data = atomic_load16(&stack->head.data);

    do {
        // link node to the head
        node->next = orig_head.node;

        // create a new head
        new_head.aba = orig_head.aba + 1;
        new_head.node = node;

        // try to update the head, if it fails, orig_head is updated
    } while (!cas16(&stack->head.data, &orig_head.data, new_head.data));

    atomic_add(&stack->size, 1);
}

struct stack_node *stack_alloc(stack_t *stack)
{
    struct stack_node *node;
    union stack_head orig_head;
    union stack_head new_head;

    // copy the head
    orig_head.data = atomic_load16(&stack->head.data);

    do {
        // check if the stack is empty
        if (orig_head.node == 0)
            return 0;

        // get the node pointed to by the head
        node = (struct stack_node *)orig_head.node;

        // create a new head
        new_head.aba = orig_head.aba + 1;
        new_head.node = node->next == 0 ? 
            (struct stack_node *)((uintptr_t)node + stack->node_size) : node->next;

        // try to update the head, if it fails, orig_head is updated
    } while (!cas16(&stack->head.data, &orig_head.data, new_head.data));

    atomic_sub(&stack->size, 1);
    return node;
}
