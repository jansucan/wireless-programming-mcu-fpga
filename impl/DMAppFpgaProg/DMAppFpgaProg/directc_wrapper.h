/* Author: Jan Sucan */

#ifndef DIRECTC_WRAPPER_H_
#define DIRECTC_WRAPPER_H_

#include <DirectC/dpuser.h>

#include <stdlib.h>

/**
 * @brief Velkost bufferu pre ulozenie znakov textoveho vypisu o priebehu DirectC operacie.
 */
#define DCW_DISPLAY_BUFFER_SIZE  8192U // znakov

/* Interface to DirectC library */
int dcw_execute_operation(DPUCHAR opcode);
void dcw_get_displayed_text(const char ** const text, size_t * const chars);

/* Functions from dpuser.h */
void dcw_init(void);
void dcw_delay(DPULONG microseconds);
DPUCHAR dcw_jtag_inp(void);
void dcw_jtag_outp(DPUCHAR outdata);
void dcw_dp_display_text(const char *text);
void dcw_dp_display_value(DPULONG value,DPUINT descriptive);
void dcw_dp_display_array(DPUCHAR *value,DPUINT bytes, DPUINT descriptive);
void dcw_dp_report_progress(DPUCHAR value);

/* Functions from dpcom.h */
void dcw_get_page_data_from_external_storage(DPULONG image_requested_address, DPULONG * const return_bytes,
											 DPULONG * const start_page_address, DPULONG * const end_page_address);

#endif /* DIRECTC_WRAPPER_H_ */