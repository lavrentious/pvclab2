#ifndef CONSOLE_H
#define CONSOLE_H

/* Командная консоль светофора через UART. */
void console_init(void);
void console_update(void); /* вызывать в главном цикле, не блокирует */

#endif
