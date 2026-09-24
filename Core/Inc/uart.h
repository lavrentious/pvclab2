#ifndef UART_H
#define UART_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Размеры очередей режима прерываний; можно переопределить при сборке (-D...). */
#ifndef UART_RX_BUFFER_SIZE
#define UART_RX_BUFFER_SIZE 64
#endif
#ifndef UART_TX_BUFFER_SIZE
#define UART_TX_BUFFER_SIZE 256
#endif

void uart_init(bool use_interrupts);

/* Переключение режима: прерывания (буферизация, неблокирующий обмен)
 * или опрос (прерывания UART запрещены). */
void uart_set_interrupts(bool enabled);
bool uart_interrupts_enabled(void);

/* Не блокирует: false, если принятых данных нет. */
bool uart_read_byte(uint8_t *byte);

/* Режим прерываний: кладёт данные в очередь и сразу возвращается
 * (ждёт, только если очередь переполнена).
 * Режим опроса: отправляет данные и возвращается после отправки. */
void uart_write(const char *data, size_t len);
void uart_print(const char *str);

#endif
