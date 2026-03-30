/*
 * Some Zephyr / ZMK build configurations leave calls to gpio_is_valid_dt() in
 * object files without linking the TU that defines it → undefined reference.
 *
 * Implements the usual rule: valid when @p spec is non-NULL and spec->port is non-NULL.
 *
 * Avoids including <zephyr/drivers/gpio.h>: on trees that use static inline
 * gpio_is_valid_dt there, a second definition in this TU would conflict.
 */
#include <stdbool.h>
#include <stdint.h>

struct device;

/* Layout must match include/zephyr/drivers/gpio.h (gpio_pin_t / gpio_dt_flags_t size). */
struct gpio_dt_spec {
	const struct device *port;
	uint8_t pin;
	uint16_t dt_flags;
};

__attribute__((weak)) bool gpio_is_valid_dt(const struct gpio_dt_spec *spec)
{
	return spec != NULL && spec->port != NULL;
}
