/* Author: Jan Sucan */

#include <stdio.h>
#include <stdarg.h>

#include <diagnostic_module/debug.h>
#include <comm/usart.h>

/**
 * @brief Printf-like funkcia vypisujuca na seriovu linku.
 *
 * Velmi uzitocna pre debugovanie programov pre diagnosticky modul, kedze
 * ten nema ine indikacne prvky. Retazce maximalnej dlzky 1023 znakov
 * posiela cez seriovu linku a bluetooth modul na seriovy terminal.
 *
 * @param format  Formatovaci retezec vypisu. Vid printf().
 * @param[in] ... Argumenty pre formatovaci retazec. Vid printf().
 */
void
debug_usart_printf(const char* format, ...) {
	char buf[1024];

	va_list args;
	va_start(args, format);
	const int r = vsnprintf(buf, 1024, format, args);
	va_end(args);
	if (r >= 0) {
		for (size_t i = 0; buf[i] != '\0'; ++i) {
			usart_send_byte(buf[i]);
		}
	}
}