#include <yak/kernel.h>
#include <yak/initcall.h>
#include <yak/lib/string.h>
#include <yak/lib/utils.h>
#include <yak/arch/spinlock.h>
#include <yak/boot/multiboot.h>
#include <yak/cpu/percpu.h>
#include <yak/cpu/mp.h>
#include <yak/mem/vmm.h>
#include <yak/mem/mem.h>
#include <yak/mem/pmm.h>

#define LOG LOG_COLOR0 "pmm:\33r"

extern multiboot_info_t *mbi;

// Thoughts:
// instead of giving each core a local stack, is it possible to use a single stack
// for all processors and to modify/read it with atomic operations instead ?

// The memory is managed as a free block chain
// This structure is present at the beginning of every
// free frame used as a backing store for free frames pointers
// I call it a stack block - all addresses are physical
typedef struct
{
    uintptr_t next_block;
    uintptr_t this_frame; 
    unsigned long num_frames;
    uintptr_t frames[0];
} __attribute__((packed)) stack_block_t;

// This structure describes a frame stack
// It is used by each core and also by the global page stack
typedef struct
{
    unsigned long free_frames;
    stack_block_t *top;
} frame_stack_t;

// global stack - used as a backing store to service local stacks
static frame_stack_t global_frame_stack;

// local stacks - specific to each core
DEFINE_PERCPU(frame_stack_t, frame_stack);

enum { FRAMES_PER_BLOCK = 
    (PAGE_SIZE - (uintptr_t)&(((stack_block_t *)0)->frames)) / sizeof(uintptr_t) };

static uintptr_t frame_stack_base;
static unsigned long total_frames;

static spinlock_t lock;

// each stack uses 2 consecutive pages to map its content (list of pointers)
// The pages for the global stack come first, followed by the pages for the
// local stacks. This macro returns the first page for a specific core
// The PML4, PM3 and PML2 entries should already exist for these pages
#define get_stack_base(id) (frame_stack_base + ((id) + 1) * PAGE_SIZE * 2)

// this returns a full stack block from the global stack
static uintptr_t acquire_stack_block(void)
{
    //printk("acquire_stack_block()\n");
    frame_stack_t *stack = &global_frame_stack;

    if (stack->free_frames == 0) {
        // TODO we should try to release unused memory first
        // before calling panic
        panic("Out of memory\n");
    }

    // we return the currently mapped block to the calling cpu
    // for this, we get its address and then map the next block
    spin_lock(&lock);
    uintptr_t block = stack->top->this_frame;
    stack->top = (stack_block_t *)map(frame_stack_base, stack->top->next_block, 3);

    if (stack->free_frames > FRAMES_PER_BLOCK) {
        stack->free_frames -= FRAMES_PER_BLOCK;
    } else {
        // TODO we should try to release unused memory in a background thread
    }
    spin_unlock(&lock);

    return block;
}

// this returns a full stack block to the global stack
static void release_stack_block(uintptr_t frame)
{
    //printk("release_stack_block()\n");
    frame_stack_t *stack = &global_frame_stack;

    spin_lock(&lock);
    uintptr_t block = stack->top->this_frame;
    stack->top = (stack_block_t *)map(frame_stack_base, frame, 3);
    stack->top->next_block = block;

    stack->free_frames += FRAMES_PER_BLOCK;
    spin_unlock(&lock);
}

// allocate a physical frame from the current cpu local stack
uintptr_t alloc_frame(void)
{
    volatile frame_stack_t *stack = &percpu_get(frame_stack);
    stack_block_t *s = stack->top;
    unsigned int id = this_cpu_id;

    if (stack->free_frames > 0) 
        --stack->free_frames;

    if (s == 0) { 
        // the stack hasn't been initialized
        // get a full block and map it as the first block
        s = (stack_block_t *)map(get_stack_base(id), acquire_stack_block(), 3);
        stack->top = s;
        stack->free_frames = FRAMES_PER_BLOCK - 1; // -1 for the ongoing allocation
    } else if (s->num_frames == 0) { 
        // the current block is empty
        // if we are the second block, that means the first block is full, 
        // so change the stack pointer to the first block. 
        // Otherwise, we are the first block, so ask for a new block, 
        // in both cases, we return the empty block as the allocated frame.
        uintptr_t frame = s->this_frame;
        if ((uintptr_t)s == get_stack_base(id) + PAGE_SIZE) {
            s = (stack_block_t *)get_stack_base(id);
        } else {
            s = (stack_block_t *)map(get_stack_base(id), acquire_stack_block(), 3);
            stack->free_frames += FRAMES_PER_BLOCK;
        }
        stack->top = s;
        return frame;
    }
    return s->frames[--s->num_frames];
}

// release a physical frame to the current cpu local stack
void free_frame(uintptr_t frame)
{
    volatile frame_stack_t *stack = &percpu_get(frame_stack);
    stack_block_t *s = stack->top;
    unsigned int id = this_cpu_id;
    
    if (s == 0) { 
        // the stack hasn't been initialized
        // get a full block and map it as the first block
        uintptr_t block = acquire_stack_block();
        map(get_stack_base(id), block, 3);
        stack->free_frames = FRAMES_PER_BLOCK; 
        // map the free frame as the second block (empty)
        s = (stack_block_t *)map(get_stack_base(id) + PAGE_SIZE, frame, 3);
        s->this_frame = frame;
        s->num_frames = 0;
        stack->top = s;
    } else if (s->num_frames == FRAMES_PER_BLOCK) { 
        // the current block is full
        // if we are the second block, free us first 
        if ((uintptr_t)s == get_stack_base(id) + PAGE_SIZE) {
            release_stack_block(s->this_frame);
            stack->free_frames -= FRAMES_PER_BLOCK;
        }
        // map the free frame as the second block (empty)
        s = (stack_block_t *)map(get_stack_base(id) + PAGE_SIZE, frame, 3);
        s->num_frames = 0;
        s->this_frame = frame;
        stack->top = s;
    } else {
        s->frames[s->num_frames++] = frame;
    }
    ++stack->free_frames;
}

static void print_mem_stat(frame_stack_t *stack)
{
    unsigned long total_free = stack->free_frames * PAGE_SIZE;

    printk(LOG " %uGiB-%uMiB-%uKiB available (%u frames)\n", 
            byte2gb(total_free), byte2mb(total_free), byte2kb(total_free), stack->free_frames);
}

inline void print_mem_stat_local(void)
{
    print_mem_stat(&percpu_get(frame_stack));
}

inline void print_mem_stat_global(void)
{
    print_mem_stat(&global_frame_stack);
}

INIT_CODE void pmm_init(uintptr_t kstart, uintptr_t kstop)
{
    if (!(mbi->flags & MBOOT_MMAP)) {
        panic("<mem> No multiboot memory map found!\n");
    }
    
    kstart = align_down(kstart, PAGE_SIZE);
    kstop = align_up(kstop, PAGE_SIZE);
    frame_stack_base = kstop + VIRTUAL_BASE;

    frame_stack_t *stack = &global_frame_stack;
    stack_block_t *s = stack->top;

    printk(LOG " initializing memory");

    uintptr_t mmap_stop = VMM_P2V(mbi->mmap_addr) + mbi->mmap_length;
    uintptr_t frame, last_frame;
    multiboot_mmap_entry_t *mmap = (multiboot_mmap_entry_t *)VMM_P2V(mbi->mmap_addr); 
    for (; (uintptr_t)mmap < mmap_stop; mmap = (multiboot_mmap_entry_t *)
            ((uintptr_t)mmap + mmap->size + sizeof(mmap->size))) {

        if (mmap->type != 1)
            continue;

        frame = align_up(mmap->addr, PAGE_SIZE);
        last_frame = align_down(frame + mmap->length, PAGE_SIZE);
        if (frame == 0)
            frame += PAGE_SIZE;

        //printk("%s freeing %016x:%016x - %uMB-%uKB = %u frames ", 
        //    logid, frame, last_frame, 
        //    byte2mb(last_frame - frame), byte2kb(last_frame - frame), 
        //    (last_frame - frame) / PAGE_SIZE);

        unsigned long i = 0;
        for (; frame + PAGE_SIZE <= last_frame; frame += PAGE_SIZE) {

            if ((frame >= TRAMPOLINE_START && frame < TRAMPOLINE_END) ||
                (frame >= kstart && frame < kstop))
                continue;

            if (s == 0) {
                s = (stack_block_t *)map(frame_stack_base, frame, 3);
                s->next_block = 0;
                s->this_frame = frame;
                s->num_frames = 0;
                stack->top = s;
            } else if (s->num_frames == FRAMES_PER_BLOCK) {
                uintptr_t block = s->this_frame;
                s = (stack_block_t *)map(frame_stack_base, frame, 3);
                s->next_block = block;
                s->this_frame = frame;
                s->num_frames = 0;
                stack->top = s;
            } else {
                s->frames[s->num_frames++] = frame;
            }
            ++stack->free_frames;
            if (++i % 50000 == 0) printk(".");
        }
        //printk("\n");
    }
    printk("\n");
    total_frames = global_frame_stack.free_frames;
    print_mem_stat_global();

    /*
    // unmap the identity mapping of the first 4MB but do not reclaim the frames yet
    // they will be freed in batch later
    unsigned long *pd = (unsigned long *)PD_BASE;
    pd[0] = 0;

    // unmap unecessary pages after the early allocations and the global and local stacks mapping
    kstop += PAGE_SIZE + ncpus * 2 * PAGE_SIZE;
    unsigned long pt_idx = PT_IDX(kstop + VIRTUAL_BASE);
    unsigned long *pt = ((unsigned long *)PT_BASE) + (1024 * PD_IDX(kstop + VIRTUAL_BASE));
    for (unsigned i = pt_idx + 1; i < 1024; ++i)
        pt[i] = 0;

    // set bit write protection bit (16 WP) of CR0
    // so kernel-mode code can't write to read-only pages
    asm volatile("movl %%cr0, %%eax\n"
                 "orl $0x00010000, %%eax\n"
                 "movl %%eax, %%cr0" 
                 : : : "cc");
    // reload cr3
    asm volatile("movl %cr3, %eax; movl %eax, %cr3");
    */
}
