#ifndef FIFO_H
#define FIFO_H

#include <stdbool.h>
#include <stdint.h>

/* Кольцевой буфер байтов (очередь). Безопасен без запрета прерываний, если
 * пишет только одна сторона (например, обработчик прерывания), а читает
 * только другая (главный цикл). Вмещает size - 1 байт. */
typedef struct {
	volatile uint8_t *data;
	uint16_t size;
	volatile uint16_t head; /* куда писать следующий байт */
	volatile uint16_t tail; /* откуда читать следующий байт */
} fifo_t;

void fifo_init(fifo_t *fifo, uint8_t *storage, uint16_t size);
bool fifo_push(fifo_t *fifo, uint8_t byte); /* false — буфер полон */
bool fifo_pop(fifo_t *fifo, uint8_t *byte); /* false — буфер пуст */
bool fifo_is_empty(const fifo_t *fifo);

#endif
