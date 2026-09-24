#include "uart.h"
#include "fifo.h"
#include "usart.h"
#include <string.h>

/* Отладочный UART стенда: USART6 (PC6/PC7) -> встроенный отладчик -> виртуальный COM-порт. */
#define UART_HANDLE huart6
#define UART_IRQ    USART6_IRQn

static uint8_t rx_storage[UART_RX_BUFFER_SIZE];
static uint8_t tx_storage[UART_TX_BUFFER_SIZE];
static fifo_t rx_fifo;
static fifo_t tx_fifo;

static volatile bool interrupts_enabled;
static volatile bool tx_busy; /* идёт отправка по прерыванию */
static uint8_t rx_byte;       /* сюда HAL принимает очередной байт */
static uint8_t tx_byte;       /* отсюда HAL отправляет очередной байт */

/* Вызывается из прерывания или при запрещённом прерывании UART. */
static void tx_start_next(void)
{
	tx_busy = fifo_pop(&tx_fifo, &tx_byte);
	if (tx_busy)
		HAL_UART_Transmit_IT(&UART_HANDLE, &tx_byte, 1);
}

static void rx_start(void)
{
	HAL_UART_Receive_IT(&UART_HANDLE, &rx_byte, 1);
}

void uart_init(bool use_interrupts)
{
	fifo_init(&rx_fifo, rx_storage, sizeof(rx_storage));
	fifo_init(&tx_fifo, tx_storage, sizeof(tx_storage));
	/* CubeMX включает прерывание USART6 в MX_USART6_UART_Init — начинаем с опроса. */
	HAL_NVIC_DisableIRQ(UART_IRQ);
	interrupts_enabled = false;
	uart_set_interrupts(use_interrupts);
}

void uart_set_interrupts(bool enabled)
{
	if (enabled == interrupts_enabled)
		return;

	if (enabled) {
		interrupts_enabled = true;
		HAL_NVIC_EnableIRQ(UART_IRQ);
		rx_start();
		return;
	}

	/* Иначе незавершённая операция _IT оставит драйвер HAL занятым навсегда
	 * (см. методичку, п. 1.4). Дожидаемся отправки очереди: это ограниченное
	 * по времени ожидание, и без него потерялся бы уже поставленный в очередь ответ. */
	while (tx_busy) {
	}
	HAL_UART_AbortReceive(&UART_HANDLE);
	HAL_NVIC_DisableIRQ(UART_IRQ);
	interrupts_enabled = false;
}

bool uart_interrupts_enabled(void)
{
	return interrupts_enabled;
}

bool uart_read_byte(uint8_t *byte)
{
	/* Сначала то, что успели принять по прерываниям до переключения в опрос. */
	if (fifo_pop(&rx_fifo, byte))
		return true;
	if (interrupts_enabled)
		return false;
	/* Таймаут 0: HAL проверяет флаг RXNE один раз и сразу возвращается. */
	return HAL_UART_Receive(&UART_HANDLE, byte, 1, 0) == HAL_OK;
}

void uart_write(const char *data, size_t len)
{
	if (!interrupts_enabled) {
		HAL_UART_Transmit(&UART_HANDLE, (uint8_t *)data, len, HAL_MAX_DELAY);
		return;
	}

	for (size_t i = 0; i < len; i++) {
		/* Очередь полна — ждём, пока прерывание её разгрузит, а не теряем данные. */
		while (!fifo_push(&tx_fifo, data[i])) {
		}
		HAL_NVIC_DisableIRQ(UART_IRQ);
		if (!tx_busy)
			tx_start_next();
		HAL_NVIC_EnableIRQ(UART_IRQ);
	}
}

void uart_print(const char *str)
{
	uart_write(str, strlen(str));
}

/* Колбэки HAL: их вызывает HAL_UART_IRQHandler из USART6_IRQHandler. */

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart != &UART_HANDLE)
		return;
	fifo_push(&rx_fifo, rx_byte); /* очередь полна — байт теряется */
	rx_start();
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart == &UART_HANDLE)
		tx_start_next();
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
	/* При переполнении (ORE) HAL прерывает приём — запускаем его снова. */
	if (huart == &UART_HANDLE && interrupts_enabled)
		rx_start();
}
