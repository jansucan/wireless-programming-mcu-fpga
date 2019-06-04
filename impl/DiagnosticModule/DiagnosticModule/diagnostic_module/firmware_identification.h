/* Author: Jan Sucan */

#ifndef FIRMWARE_IDENTIFICATION_H_
#define FIRMWARE_IDENTIFICATION_H_

int firmware_identification_string(char * dest, const char * app_name, const char * git_shorthash);

#endif /* FIRMWARE_IDENTIFICATION_H_ */