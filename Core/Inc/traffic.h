#ifndef TRAFFIC_H
#define TRAFFIC_H

#include <stdbool.h>
#include <stdint.h>

/* Пешеходный светофор (вариант 1). */
void traffic_init(void);
void traffic_update(void); /* вызывать в главном цикле, не блокирует */

/* false — нажатия кнопки игнорируются. */
void traffic_set_button_enabled(bool enabled);
bool traffic_button_enabled(void);

/* Длительность красного; зелёный горит 1/4 от неё. */
void traffic_set_red_ms(uint32_t ms);
uint32_t traffic_red_ms(void);

/* Что сейчас горит: "red", "green", "blinking green", "yellow". */
const char *traffic_light_name(void);

#endif
