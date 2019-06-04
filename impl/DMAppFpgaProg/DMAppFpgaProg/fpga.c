/* Author: Jan Sucan */

#include <avr32/io.h>
#include <fpga.h>

/**
 * @brief Inicializacia I/O pre ovladanie FPGA.
 *
 * Nastavi sa vystup PD30 pre ovladanie signalu nPRG. Predvoleny stav je vypnute napajanie.
 */
void
fpga_init(void)
{
    /* PD30 = nPRG = GPIO number 126 = port 3 pin 30 */
    
    /* PD30 je ovladany GPIO, nie perifernou funkciou */
    AVR32_GPIO.port[3].gpers = 1 << 30;
    /* Aktivacia driveru pinu */
    AVR32_GPIO.port[3].oders = 1 << 30;
    /* Diagnosticky modul uz ma pripojeny pull-up, vypne sa pull-up v MCU */
    AVR32_GPIO.port[3].puerc = 1 << 30;
    /* Vypne sa pull-down */
    AVR32_GPIO.port[3].pderc = 1 << 30;
    /* Predvoleny stav napajania */
    fpga_power_supply_off();
}

/**
 * @brief Zapnutie napajania FPGA pomocou signalu nPRG.
 */
void
fpga_power_supply_on(void)
{
   AVR32_GPIO.port[3].ovrc = 1 << 30;
}

/**
 * @brief Vypnutie napajania FPGA pomocou signalu nPRG.
 */
void
fpga_power_supply_off(void)
{
   AVR32_GPIO.port[3].ovrs = 1 << 30;
}
