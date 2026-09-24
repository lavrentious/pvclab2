#include "traffic.h"
#include "led.h"
#include "button.h"
#include "stm32f4xx_hal.h"

/* Фазы в порядке следования: после последней снова первая. */
typedef enum {
	PHASE_RED,
	PHASE_GREEN,
	PHASE_GREEN_BLINK,
	PHASE_YELLOW,
	PHASE_COUNT
} phase_t;

#define DEFAULT_RED_MS 12000
#define GREEN_BLINK_MS 2000
#define YELLOW_MS      1500
#define BLINK_HALF_MS  250 /* полпериода мигания зелёного */

static const char *const phase_names[PHASE_COUNT] = {
	[PHASE_RED]         = "red",
	[PHASE_GREEN]       = "green",
	[PHASE_GREEN_BLINK] = "blinking green",
	[PHASE_YELLOW]      = "yellow",
};

static uint32_t red_ms = DEFAULT_RED_MS;
static bool button_enabled = true;

static phase_t phase;
static uint32_t phase_start;
static uint32_t phase_len;   /* длительность текущей фазы, мс */
static bool green_requested; /* кнопку нажали до красного — ближайший красный короткий */

static uint32_t red_short_ms(void)
{
	return red_ms / 4;
}

static uint32_t phase_default_ms(phase_t p)
{
	switch (p) {
	case PHASE_RED:         return red_ms;
	case PHASE_GREEN:       return red_ms / 4; /* по заданию: в 4 раза меньше красного */
	case PHASE_GREEN_BLINK: return GREEN_BLINK_MS;
	default:                return YELLOW_MS;
	}
}

static void enter_phase(phase_t next, uint32_t now)
{
	phase = next;
	phase_start = now;
	phase_len = phase_default_ms(next);
	if (next == PHASE_RED && green_requested) {
		phase_len = red_short_ms();
		green_requested = false;
	}
}

static void on_button_press(uint32_t now)
{
	if (phase == PHASE_RED) {
		/* Осталось больше 1/4 периода — сокращаем остаток до 1/4, иначе ничего. */
		uint32_t elapsed = now - phase_start;
		if (elapsed + red_short_ms() < phase_len)
			phase_len = elapsed + red_short_ms();
	} else if (phase != PHASE_GREEN) {
		green_requested = true;
	}
}

static void show(uint32_t elapsed)
{
	bool blink_on = (elapsed / BLINK_HALF_MS) % 2 == 0;

	led_set(LED_RED, phase == PHASE_RED);
	led_set(LED_YELLOW, phase == PHASE_YELLOW);
	led_set(LED_GREEN, phase == PHASE_GREEN || (phase == PHASE_GREEN_BLINK && blink_on));
}

void traffic_init(void)
{
	green_requested = false;
	enter_phase(PHASE_RED, HAL_GetTick());
}

void traffic_update(void)
{
	uint32_t now = HAL_GetTick();

	/* Кнопку опрашиваем и в режиме 2, чтобы нажатие в нём не «всплыло» позже. */
	if (button_poll() == BUTTON_EVENT_PRESS && button_enabled)
		on_button_press(now);

	if (now - phase_start >= phase_len)
		enter_phase((phase + 1) % PHASE_COUNT, now);

	show(now - phase_start);
}

void traffic_set_button_enabled(bool enabled)
{
	button_enabled = enabled;
	if (!enabled)
		green_requested = false;
}

bool traffic_button_enabled(void)
{
	return button_enabled;
}

void traffic_set_red_ms(uint32_t ms)
{
	red_ms = ms; /* применяется со следующей фазы */
}

uint32_t traffic_red_ms(void)
{
	return red_ms;
}

const char *traffic_light_name(void)
{
	return phase_names[phase];
}
