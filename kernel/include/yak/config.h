#ifndef __CONFIG_H__
#define __CONFIG_H__

#define PHYSICAL_BASE   0x0000000000100000
#define VIRTUAL_BASE    0xffffffff80000000
#define LFB_BASE        0xfffffeffc0000000
#define PAGE_SIZE       0x1000
#define STACK_SIZE      0x4000

#define SYSCALL_VECTOR  128

#define NUM_TERM        5

#define TIMER_FREQ      1000

#define REP0(x)
#define REP1(x) x
#define REP2(x) REP1(x) REP1(x)
#define REP3(x) REP2(x) REP1(x)
#define REP4(x) REP3(x) REP1(x)
#define REP5(x) REP4(x) REP1(x)
#define REP6(x) REP5(x) REP1(x)
#define REP7(x) REP6(x) REP1(x)
#define REP8(x) REP7(x) REP1(x)
#define REP9(x) REP8(x) REP1(x)
#define REP10(x) REP9(x) REP1(x)
#define REP11(x) REP10(x) REP1(x)

#define REP(x, n) REP ## n(x)
#define XREP(x, n) REP(x, n)

#define LOG_PREFIX(str, blanks) XREP(" ", blanks) "\33\x08\x88" str " -\33\x0f\xff "

#endif
