/* Author: Jan Sucan */

#include <avr32/io.h>

#include <diagnostic_module/diagnostic_module.h>
#include <protocol.h>
#include <fpga.h>

int main(void)
{
	diagnostic_module_init();
	fpga_init();

	protocol_loop();
	
	// Sem sa tok programu nikdy nedostane
	while (1) {
		;
	}
}