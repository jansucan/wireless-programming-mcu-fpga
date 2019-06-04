/* Author: Jan Sucan */

#include <avr32/io.h>
#include <protocol.h>
#include <diagnostic_module/diagnostic_module.h>

int
main(void)
{	
 	diagnostic_module_init();
	// Odteraz su hlavne hodiny na 64,512 MHz

	// Prijem a spracovavanie prikazov
	protocol_loop();
	// Sem uz sa tok programu nikdy nedostane, pretoze sa bud opakovane spracovavaju
	// prikazu pre zapis, mazanie a citanie stranok, alebo sa vykona prikaz pre 
	// spustenie aplikacneho firmware a ten prevezme riadenie namiesto Bootloaderu
	while (1) {
		;
	}
}
