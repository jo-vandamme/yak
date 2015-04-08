#ifndef __YAK_MP_H__
#define __YAK_MP_H__

#define TRAMPOLINE_START    0x7000
#define TRAMPOLINE_PARAMS   0x7c00
#define TRAMPOLINE_END      0x9000

#define AP_STATUS_FLAG      (TRAMPOLINE_END - 8)
#define AP_SLEEP            0
#define AP_STARTED          1
#define AP_CONTINUE         2
#define AP_READY            3

#define CR3_OFFSET          0x00
#define GDT_OFFSET          (CR3_OFFSET + 8)
#define IDT_OFFSET          (GDT_OFFSET + 10)
#define STACK_OFFSET        (IDT_OFFSET + 10)
#define PERCPU_OFFSET       (STACK_OFFSET + 8)
#define ID_OFFSET           (PERCPU_OFFSET + 8)

#ifndef __ASSEMBLER__

#include <yak/lib/types.h>

void mp_init(uintptr_t madt);
unsigned int num_running_cores(void);

#endif

#endif
