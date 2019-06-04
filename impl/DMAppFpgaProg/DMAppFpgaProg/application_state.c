/* Author: Jan Sucan */

#include <application_state.h>

application_state_t application_state;

void
application_state_init(void)
{
	application_state.state = APPLICATION_STATE_WAITING_FOR_OPERATION_CODE;
	application_state.directc_operation_code = APPLICATION_STATE_NO_DIRECTC_OPEARATION;
	application_state.directc_paging_image_offset = 0;
	application_state.directc_paging_bytes_requested = 0;
}