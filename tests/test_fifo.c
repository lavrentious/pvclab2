/* Проверка очереди на компьютере: cc -I../Core/Inc test_fifo.c ../Core/Src/fifo.c && ./a.out */
#include "fifo.h"
#include <assert.h>
#include <stdio.h>

int main(void)
{
	uint8_t storage[4];
	fifo_t f;
	uint8_t b;

	fifo_init(&f, storage, sizeof(storage));
	assert(fifo_is_empty(&f) && !fifo_pop(&f, &b));

	/* вмещает size - 1 */
	assert(fifo_push(&f, 1) && fifo_push(&f, 2) && fifo_push(&f, 3));
	assert(!fifo_push(&f, 4));

	/* порядок FIFO и переход через конец массива */
	for (uint8_t round = 0; round < 10; round++) {
		assert(fifo_pop(&f, &b) && b == (uint8_t)(round + 1));
		assert(fifo_push(&f, (uint8_t)(round + 4)));
	}
	assert(fifo_pop(&f, &b) && b == 11);
	assert(fifo_pop(&f, &b) && b == 12);
	assert(fifo_pop(&f, &b) && b == 13);
	assert(fifo_is_empty(&f));

	puts("fifo OK");
	return 0;
}
