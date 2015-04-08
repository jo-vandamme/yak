#ifndef __YAK_INITCALL_H__
#define __YAK_INITCALL_H__

#define INIT_CODE __attribute__((section(".init.text"), cold))
#define INIT_DATA __attribute__((section(".init.data")))

#endif
