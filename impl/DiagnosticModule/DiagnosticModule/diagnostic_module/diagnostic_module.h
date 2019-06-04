/* Author: Jan Sucan */

#ifndef DIAGNOSTIC_MODULE_H_
#define DIAGNOSTIC_MODULE_H_

#include <diagnostic_module/firmware_identification.h>

#include <clock/pll.h>
#include <clock/wait.h>

#include <comm/bt.h>

#include <flash/exec.h>
#include <flash/flash.h>

#include <protocol/data.h>

#include <utils/byte_buffer.h>
#include <utils/crc32q.h>
#include <utils/system_registers.h>

void diagnostic_module_init(void);

#endif /* DIAGNOSTIC_MODULE_H_ */