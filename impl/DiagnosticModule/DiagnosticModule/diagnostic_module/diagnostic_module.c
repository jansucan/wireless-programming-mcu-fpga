/* Author: Jan Sucan */

#include <diagnostic_module/diagnostic_module.h>

/**
 * @brief Inicializacia diagnostickeho modulu.
 *
 * Funkcia inicializuje casti diagnostickeho modulu pouzite v kazdej aplikacii.
 * Prepne hodiny na 64,512 MHz a nastavi komunikaciu cez bluetooth modul protokolom YUP.
 * Predvypocita sa tabulka hodnot pre CRC32-Q.
 */
void
diagnostic_module_init(void)
{
	pll_use_as_main_clock();
	crc32q_init();
	bt_init();
}