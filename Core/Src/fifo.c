#include "fifo.h"

void fifo_init(fifo_t *fifo, uint8_t *storage, uint16_t size)
{
	fifo->data = storage;
	fifo->size = size;
	fifo->head = 0;
	fifo->tail = 0;
}

bool fifo_push(fifo_t *fifo, uint8_t byte)
{
	uint16_t next = (fifo->head + 1) % fifo->size;

	if (next == fifo->tail)
		return false;
	fifo->data[fifo->head] = byte;
	fifo->head = next; /* сдвигаем голову только после записи данных */
	return true;
}

bool fifo_pop(fifo_t *fifo, uint8_t *byte)
{
	if (fifo_is_empty(fifo))
		return false;
	*byte = fifo->data[fifo->tail];
	fifo->tail = (fifo->tail + 1) % fifo->size;
	return true;
}

bool fifo_is_empty(const fifo_t *fifo)
{
	return fifo->head == fifo->tail;
}
