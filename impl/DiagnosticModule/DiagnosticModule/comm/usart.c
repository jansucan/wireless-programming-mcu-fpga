/* Author: Jan Sucan */

#include <avr32/io.h>
#include <stdlib.h>
#include <stdbool.h>

#include <comm/usart.h>
#include <clock/wait.h>
#include <diagnostic_module/parameters.h>

/**
 * @brief Maximalny pocet bajtov, co sa prijmu cez USART0 a zahodia.
 *
 * Tato hodnota sa pouziva pre vycistenie seriovej linky v smere prijmu. Experimentale bolo zistene, ze sa v prijimacom smere
 * pri prijme prvych bajtov objavuju bajty, ktore neodoslal PC. Bajty od PC nasledovali az za tymyto odpadovymi bajtmi.
 */
#define USART_MAX_RX_JUNK_BYTES  8

static bool usart_is_byte_transmitted(void);
static bool usart_is_byte_received(void);
static void usart_clear_error_flags(void);
static bool usart_is_error(void);
static bool usart_is_error_framing(void);
static bool usart_is_error_overflow(void);


/**
 * @brief Inicializacia rozhrania USART0, pouziteho pre komunikaciu s bluetooth modulom.
 *
 * Bluetooth modul je defaultne na 115 200 Bd. Format komunikacie je 1 stop bit, 8 datovych bitov,
 * bez parity.
 */
void
usart_init(void)
{	
	/* Nastavenie baudrate. Bit OVER = 0 a CD = 35. Vyjde presne 115 200 Bd pri 64,512 MHz */
	AVR32_USART0.brgr = 35;

	AVR32_USART0.mr &= ~AVR32_USART_MR_OVER_MASK;
	
	/* Nastav format UART */
	/* Jeden stop bit */
	AVR32_USART0.mr &= ~AVR32_USART_MR_NBSTOP_MASK;
	/* Bez parity */
	AVR32_USART0.mr &= ~AVR32_USART_MR_PAR_MASK;
	AVR32_USART0.mr |= AVR32_USART_MR_PAR_NONE << AVR32_USART_MR_PAR_OFFSET;
	/* 8 datovych bitov */
	AVR32_USART0.mr |= AVR32_USART_MR_CHRL_MASK;
	
	/* Normalny mod kanalu, ziadne echo ani loopback */
	AVR32_USART0.mr &= ~AVR32_USART_MR_CHMODE_MASK;
	/* Normalny mod */
	AVR32_USART0.mr &= ~AVR32_USART_MR_MODE_MASK;
	
	/* Zhodenie priznakov chyby */
	usart_clear_error_flags();
	
	/* Povolenia vysielania a prijmu */
	AVR32_USART0.cr &= ~AVR32_USART_CR_TXDIS_MASK;
	AVR32_USART0.cr |= AVR32_USART_CR_TXEN_MASK;
	
	AVR32_USART0.cr &= ~AVR32_USART_CR_RXDIS_MASK;
	AVR32_USART0.cr |= AVR32_USART_CR_RXEN_MASK;	
}

/**
 * @brief Prijem jedneho bajtu cez USART0.
 *
 * @param byte          Ukazovatel na buffer pre ulozenie jedneho bajtu.
 * @param time_deadline Ukazovatel na casovy deadline, dokedy najneskor sa ma byte prijat. Ak bude NULL, bude sa cakat na prijem donekonecna.
 *
 * @return 0 v pripade uspechu, -1 ak bol prekroceny casovy deadline @p time_deadline pre prijem.
 */
usart_retcode_t
usart_receive_byte(uint8_t *byte, const deadline_t * const time_deadline)
{
	usart_retcode_t r = USART_RECEIVE_BAD_ARGUMENT; // Defaultna navratova hodnota
	
	/* Priznaky chyby sa resetuju, aby nebranili nasledujucemu prijmu bajtov.
	 * Priznak chyby OVR (pretecenie) sa moze nastavit vtedy, ked sa diagnostickemu modulu niekto
	 * pokusi poslat viac ako 1 bajt vtedy, ked ich FW nie je pripraveny z prijimacieho
	 * bufferu precitat.
	 */
	usart_clear_error_flags();

	if (byte != NULL) {
		/* Caka sa, az sa prijme bajt */
		while ((AVR32_USART0.csr & AVR32_USART_CSR_RXRDY_MASK) == 0) {
			if (usart_is_error()) {
				/* Chyba prijmu */
				break;
			} else if (time_deadline == NULL) {
				/* Timeout sa nebude kontrolovat */
				continue;
			} else if (wait_has_deadline_expired(time_deadline)) {
				/* Timeout bol zadany a vyprsal */
				break;
			}
		}

		if (usart_is_byte_received()) {
			/* Prijal sa byte */
			*byte = AVR32_USART0.rhr;
			r =  USART_OK;
		} else if (usart_is_error()) {
			/* Framing error (nespravny stop bit) alebo pretecenie prijiamcieho bufferu */
			r = USART_RECEIVE_ERROR;
		} else {
			r = USART_RECEIVE_TIMEOUT;
		}
	}
	
	return r;
}

/**
 * @brief Odoslanie jedneho bajtu cez USART0.
 *
 * @param byte  Bajt pre odoslanie cez USART0.
 */
void
usart_send_byte(uint8_t byte)
{
	/* Pocka sa na uvolnenie vysielacieho bufferu */
	while (!usart_is_byte_transmitted());
	/* Bajt sa odosle */
	AVR32_USART0.thr = byte;
}

/**
 * @brief Zisti, ci je vysielaci buffer pripraveny pre zapis dalsieho bajtu pre odoslanie.
 *
 * @return true   Vysielaci buffer pripraveny.
 * @return false  Vysielaci buffer este nie je pripraveny.
 */
bool
usart_is_byte_transmitted(void)
{
	return ((AVR32_USART0.csr & AVR32_USART_CSR_TXRDY_MASK) != 0);
}

/**
 * @brief Zisti, ci je prijimaci buffer pripraveny pre precitanie prijateho bajtu.
 *
 * @return true   Prijimaci buffer pripraveny.
 * @return false  Prijimaci buffer este nie je pripraveny.
 */
bool
usart_is_byte_received(void)
{
	return ((AVR32_USART0.csr & AVR32_USART_CSR_RXRDY_MASK) != 0);
}

/**
 * @brief Zhodi priznaky chyb USART0.
 */
void
usart_clear_error_flags(void)
{
	AVR32_USART0.cr |= AVR32_USART_CR_RSTSTA_MASK;
}

/**
 * @brief Zisti, ci doslo k chybe USART0.
 *
 * @return true   Chyba protokolu seriovej linky, alebo pretecenie prijimacieho bfferu.
 * @return false  Vsetko v poriadku.
 */
bool
usart_is_error(void)
{
	return (usart_is_error_framing() || usart_is_error_overflow());
}

/**
 * @brief Zisti, ci doslo k chybe protokolu seriovej linky USART0.
 *
 * @return true   Chyba protokolu seriovej linky.
 * @return false  Vsetko v poriadku.
 */
bool
usart_is_error_framing(void)
{
	return ((AVR32_USART0.csr & AVR32_USART_CSR_FRAME_MASK) != 0);
}

/**
 * @brief Zisti, ci doslo k preteceniu prijimacieho bufferu USART0.
 *
 * @return true   Pretiekol prijimaci buffer.
 * @return false  Vsetko v poriadku.
 */
bool
usart_is_error_overflow(void)
{
	return ((AVR32_USART0.csr & AVR32_USART_CSR_OVRE_MASK) != 0);
}
