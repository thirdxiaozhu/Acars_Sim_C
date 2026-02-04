//
// Created by jiaxv on 2026/1/17.
//

#ifndef ACARS_SIM_C_UTIL_H
#define ACARS_SIM_C_UTIL_H
#include <stdint.h>

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


#endif //ACARS_SIM_C_UTIL_H