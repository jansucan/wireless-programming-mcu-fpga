/* Author: Jan Sucan */

#include <avr32/io.h>
#include <stdint.h>

#include <clock/pll.h>

static void pll_activate_xtal(void);
static void pll_activate_pll(void);

/**
 * @brief Len aktivuje krystalovy oscilator, neprepina nan ako na zdroj hodin.
 */
void
pll_activate_xtal(void)
{
	uint32_t tmpReg;

	/* Zapnutie externeho kr. oscilatora 0 */
	tmpReg = 0;
	tmpReg |= 3 << AVR32_SCIF_GAIN_OFFSET; /* Frekvencia krystalu > 16 MHz*/
	tmpReg |= AVR32_SCIF_OSCCTRL0_MODE_CRYSTAL << AVR32_SCIF_OSCCTRL_MODE_OFFSET; /* Pouzitie pinov krystalu 0 */
	tmpReg |= AVR32_SCIF_OSCCTRL_OSCEN_ENABLE << AVR32_SCIF_OSCCTRL_OSCEN_OFFSET; /* Aktivacia oscilatora 0 */
	/* Odomknutie registra OSCCTRL0 */
	AVR32_SCIF.unlock = (0xAA << 24) | AVR32_SCIF_OSCCTRL;
	/* Zapis do odomknuteho registru */
	AVR32_SCIF.oscctrl[0] = tmpReg;

	/* Cakanie na stabilizovanie krystalu */
	while ((AVR32_SCIF.pclksr & AVR32_SCIF_PCLKSR_OSC0RDY_MASK) == 0) {
		;
	}	
}

/**
 * @brief Len aktivuje PLL, neprepina nan ako na zdroj hodin.
 *
 *	xtalFreq  = 18.432 MHz
 *	pllDiv    = 2
 *	pllMul    = 13
 *	pllOutDiv = 1
 *	pllFreq   = 64.512 MHz
 *	pllVco    = 129.024 MHz
 */
void
pll_activate_pll(void)
{
	uint32_t tmpReg;
	
	/* Nastavenie zdroja pre PLL na Oscilator 0 */
	tmpReg = 0;
	/* Nastavenie nasobicky PLL */
	tmpReg |= 13 << AVR32_SCIF_PLL_PLLMUL_OFFSET;
	/* Nastavenie delicky PLL */
	tmpReg |= 2 << AVR32_SCIF_PLL_PLLDIV_OFFSET;
	/* Nastavenie rozsahu Vco */
	tmpReg |= 0 << AVR32_SCIF_PLLOPT_OFFSET;
	/* Nastavenie delenia frekvencie Vco */
	tmpReg |= 1 << (AVR32_SCIF_PLLOPT_OFFSET + 1);
	/* Povolenie PLL */
	tmpReg |= 1 << AVR32_SCIF_PLLEN_OFFSET;
	/* Odomknutie registra PLL0 */
	AVR32_SCIF.unlock = (0xAA << 24) | AVR32_SCIF_PLL;
	/* Zapis do odomknuteho registru */
	AVR32_SCIF.pll[0] = tmpReg;

	/* Cakanie na uzamknutie PLL0 aby sa dala pouzit ako zdroj hodin */
	while ((AVR32_SCIF.pclksr & AVR32_SCIF_PCLKSR_PLL0_LOCK_MASK) == 0) {
		;
	}
}

/**
 * @brief Prepne na PLL ako hlavny zdroj hodin na 64.512 MHz.
 */
void
pll_use_as_main_clock(void)
{
	// Nastavenie Wait-State pre FLASH na 1
	// Toto je nutne od frekvencie vyssej ako 33 MHz
	AVR32_FLASHC.fcr |= 1 << AVR32_FLASHC_FCR_FWS_OFFSET;
	
	pll_activate_xtal();
	pll_activate_pll();
	
	// Prepnutie na PLL0 ako hlavny zdroj hodin
	// Odomknutie registra PLL0
	AVR32_PM.unlock = (0xAA << 24) | AVR32_PM_MCCTRL;
	// Zapis do odomknuteho registra
	AVR32_PM.mcctrl = AVR32_PM_MCCTRL_MCSEL_PLL0 << AVR32_PM_MCCTRL_MCSEL_OFFSET;	
}