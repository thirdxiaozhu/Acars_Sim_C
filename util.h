//
// Created by jiaxv on 2026/1/17.
//

#ifndef ACARS_SIM_C_UTIL_H
#define ACARS_SIM_C_UTIL_H
#include <stdint.h>
#include "acarstrans.h"

#define MANUAL_MODE 1000
#define FILE_MODE 1001
#define LIGHT_GREEN  "\033[1;32m"
#define LIGHT_RED    "\033[1;31m"
#define RESET_COLOR "\033[0m"
#define SUCCESS 0;
#define ERROR 1;

#define MAX_BUFFER_LEN 256
#define BITS_PER_BYTE 8

#define F_S 20
#define Ac  64.0
#define F_S_2  2
#define BAUD 2400
#define PI 3.1415926
#define EPSILON 0.000001
#define RESAMPLE 12

#define SAMP_RATE 1152000

void get_crc(const uint8_t *lsb_msg, uint8_t *crc_res, int msg_len);

int parse_u32(char *s, uint32_t *const value);

int parse_frequency_i64(char *optarg, char *endptr, int64_t *value);

at_error regex_string(const char *regex, const char *text);

at_error regex_char(const char *regex, const char c);

at_error get_input(const char* prompt, const size_t max_size, uint8_t *output);


#endif //ACARS_SIM_C_UTIL_H