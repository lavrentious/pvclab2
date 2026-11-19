#include "uart.h"
#include "fifo.h"
#include "usart_drv.h"
#include <string.h>

/* Отладочный UART стенда: USART6 (PC6/PC7) -> встроенный отладчик -> виртуальный COM-порт. */

static uint8_t rx_storage[UART_RX_BUFFER_SIZE];
static uint8_t tx_storage[UART_TX_BUFFER_SIZE];
static fifo_t rx_fifo;
static fifo_t tx_fifo;

static volatile bool interrupts_enabled;
static volatile bool tx_busy; /* идёт отправка по прерыванию */

/* Вызывается из прерывания или при запрещённом прерывании UART. */
static void tx_start_next(void)
{
	uint8_t byte;

	if (usart_drv_tx_ready() && fifo_pop(&tx_fifo, &byte)) {
		tx_busy = true;
		(void)usart_drv_write(byte);
	} else {
		tx_busy = false;
	}
	usart_drv_enable_tx_irq(tx_busy || !fifo_is_empty(&tx_fifo));
}

void uart_init(bool use_interrupts)
{
	fifo_init(&rx_fifo, rx_storage, sizeof(rx_storage));
	fifo_init(&tx_fifo, tx_storage, sizeof(tx_storage));
	usart_drv_init(USART_DRV_POLLING, 57600);
	interrupts_enabled = false;
	uart_set_interrupts(use_interrupts);
}

void uart_set_interrupts(bool enabled)
{
	if (enabled == interrupts_enabled)
		return;

	if (enabled) {
		interrupts_enabled = true;
		usart_drv_set_mode(USART_DRV_INTERRUPTS);
		return;
	}

	while (tx_busy || !fifo_is_empty(&tx_fifo)) {
		usart_drv_irq_disable();
		if (!tx_busy)
			tx_start_next();
		usart_drv_irq_enable();
	}
	usart_drv_flush();
	usart_drv_set_mode(USART_DRV_POLLING);
	interrupts_enabled = false;
}

bool uart_interrupts_enabled(void)
{
	return interrupts_enabled;
}

bool uart_read_byte(uint8_t *byte)
{
	if (fifo_pop(&rx_fifo, byte))
		return true;
	if (interrupts_enabled)
		return false;
	return usart_drv_read(byte) == USART_DRV_OK;
}

void uart_write(const char *data, size_t len)
{
	if (!interrupts_enabled) {
		for (size_t i = 0; i < len; i++)
			usart_drv_write_blocking((uint8_t)data[i]);
		usart_drv_flush();
		return;
	}

	for (size_t i = 0; i < len; i++) {
		/* Очередь полна — ждём, пока прерывание её разгрузит, а не теряем данные. */
		while (!fifo_push(&tx_fifo, (uint8_t)data[i])) {
		}
		usart_drv_irq_disable();
		if (!tx_busy)
			tx_start_next();
		usart_drv_irq_enable();
	}
}

void uart_print(const char *str)
{
	uart_write(str, strlen(str));
}

void usart_drv_on_rx(uint8_t byte)
{
	fifo_push(&rx_fifo, byte); /* очередь полна — байт теряется */
}

void usart_drv_on_tx_ready(void)
{
	tx_start_next();
}
