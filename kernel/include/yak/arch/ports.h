#ifndef __YAK_PORTS_H__
#define __YAK_PORTS_H__

void outb(short port, char data);
void outw(short port, short data);
void outl(short port, int data);
char inb(short port);
short inw(short port);
int inl(short port);
void io_wait(void);

#endif
