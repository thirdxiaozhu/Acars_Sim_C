//
// Created by jiaxv on 2022/10/27.
//

#ifndef ACARS_SIM_C_GENMSG_H
#define ACARS_SIM_C_GENMSG_H

#include <stdbool.h>
#include <stdint.h>
#include "acarstrans.h"
#include "util.h"

const static char prekey[16] = {0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                                0x01, 0x01};
const static uint8_t sync_seq[4] = {0x2b, 0x2a, 0x16, 0x16};
const static char SOH = 0x01;
const static char STX = 0x02;
const static char ETX = 0x03;
const static char ETB = 0x17;
const static char DEL = 0X7F;
#define NAK_TAG 0x15

#define PREKEY_LEN 16
#define SYNC_LEN  4
#define SOH_LEN  1
#define MODE_LEN 1
#define ARN_LEN  7
#define ACK_LEN  1
#define LABEL_LEN 2
#define BI_LEN  1
#define STX_LEN  1
#define SERIAL_LEN  4
#define FLIGHTID_LEN  6
#define SUFFIX_LEN 1
#define BCS_LEN 2
#define BCSSUF_LEN 1
#define TEXT_MAX_LEN 220
#define CRC_LEN 2


typedef struct message_format {
    bool is_uplink;
    uint8_t mode;
    uint8_t arn[ARN_LEN + 1];
    uint8_t ack;
    uint8_t label[LABEL_LEN + 1];
    uint8_t bi;
    uint8_t serial[SERIAL_LEN + 1];
    uint8_t flight[FLIGHTID_LEN + 1];
    uint8_t text[TEXT_MAX_LEN + 1];
    uint8_t crc[CRC_LEN];
    uint8_t suffix;
    size_t text_len;
    uint8_t *lsb_with_crc_msg;
    size_t total_bits;
    int complex_length;
    float * cpfsk;
    char complex_i8[BAUD * F_S * F_S_2 * RESAMPLE*2]; //signed char  <--> __int_8
} message_format;


void merge_elements(message_format *);


#endif //ACARS_SIM_C_GENMSG_H
