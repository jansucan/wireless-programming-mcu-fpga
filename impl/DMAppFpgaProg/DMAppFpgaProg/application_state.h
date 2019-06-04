/* Author: Jan Sucan */

#ifndef APPLICATION_STATE_H_
#define APPLICATION_STATE_H_

#include <stdint.h>

#define APPLICATION_STATE_NO_DIRECTC_OPEARATION  UINT8_MAX

enum application_state_codes {
	APPLICATION_STATE_WAITING_FOR_OPERATION_CODE = 0,
	APPLICATION_STATE_OPERATION_WAITING_FOR_DATA
};

typedef struct application_state_s {
	uint8_t  state;                           /**< Stav aplikacie. */
	uint8_t  directc_operation_code;          /**< Kod aktualne spustenej DirectC operacie. */
	uint32_t directc_paging_image_offset;     /**< Offset stranky z .DAT suboru, ak je aplikacia v stave, kedy DirectC operacia caka na data. */
	uint32_t directc_paging_bytes_requested;  /**< Pocet bajtov stranky z .DAT suboru, ak je aplikacia v stave, kedy DirectC operacia caka na data.*/
} application_state_t;

extern application_state_t application_state;

void application_state_init(void);

#endif /* APPLICATION_STATE_H_ */