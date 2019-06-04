/* Author: Jan Sucan */

#ifndef SETJMP_BUFFERS_H_
#define SETJMP_BUFFERS_H_

#include <setjmp.h>

extern jmp_buf setjmp_buffer_cancel_operation;
extern jmp_buf setjmp_buffer_provide_data_to_operation;

#endif /* SETJMP_BUFFERS_H_ */