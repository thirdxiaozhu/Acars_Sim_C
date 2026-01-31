//
// Created by jiaxv on 2022/10/27.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "pkt.h"


#define append_char(head, offset, src) \
    *(head+offset) = src;\
    offset++;

#define append_char_parity(head, offset, src) \
    *(head+offset) = parity_char(src);\
    offset++;

#define append_str(head, offset, src, n) \
    memcpy(head+offset, src, n);\
    offset += n;

#define append_str_parity(head, offset, src, n){\
    uint8_t tmp[256] = {0};\
    parity_str(tmp, src, n);\
    memcpy(head+offset, tmp, n);\
    offset += n;\
    } \


#define UPLINK_PRELEN (PREKEY_LEN + SYNC_LEN + SOH_LEN + MODE_LEN + ARN_LEN + ACK_LEN + LABEL_LEN + BI_LEN + STX_LEN)
#define DOWNLINK_PRELEN (PREKEY_LEN + SYNC_LEN + SOH_LEN + MODE_LEN + LABEL_LEN + ARN_LEN + BI_LEN + ACK_LEN + STX_LEN + SERIAL_LEN + FLIGHT_ID_LEN)
#define TAIL_LEN  (SUFFIX_LEN + BCS_LEN + BCS_SUF_LEN)



void parity_str(uint8_t *dst, const uint8_t *src, int msg_len) ;

uint8_t parity_char(uint8_t value);

void get_crc(const uint8_t *lsb_msg, uint8_t *crc_res, int msg_len);

void lsb(uint8_t *dst, const uint8_t *src, size_t len);

at_error num2bits(int num, uint8_t *str, int bits, bool is_lsb);

void merge_elements(message_format *u) {

    const int pre_len = u->is_uplink == true ? UPLINK_PRELEN : DOWNLINK_PRELEN;

    u->total_bits = (pre_len + u->text_len + TAIL_LEN) * 8;
    uint8_t *rawMsg = malloc(sizeof(uint8_t) * (pre_len + u->text_len + TAIL_LEN));

    int duplen = 0;
    bool is_ack = false;
    if(u->ack != NAK_TAG){
        is_ack = true;
        u->label[0] = 0x5F;
        u->label[1] = 0x7F;
        u->text[0] = 0x00;
        u->text_len = 0;
        u->crc[0] = 0x22;
        u->crc[1] = 0x33;
    }

    append_str(rawMsg, duplen, prekey, PREKEY_LEN);
    append_str_parity(rawMsg, duplen, sync_seq, SYNC_LEN);
    append_char_parity(rawMsg, duplen, SOH);
    append_char_parity(rawMsg, duplen, u->mode);
    append_str_parity(rawMsg, duplen, u->arn, ARN_LEN);
    append_char_parity(rawMsg, duplen, u->ack);
    append_str_parity(rawMsg, duplen, u->label, LABEL_LEN);
    append_char_parity(rawMsg, duplen, u->bi);

    if(!is_ack){
        append_char_parity(rawMsg, duplen, STX);
        if (!u->is_uplink) {
            append_str_parity(rawMsg, duplen, u->serial, SERIAL_LEN);
            append_str_parity(rawMsg, duplen, u->flight, FLIGHT_ID_LEN);
        }
        append_str_parity(rawMsg, duplen, u->text, u->text_len);
        append_char_parity(rawMsg, duplen, u->suffix);

        get_crc(rawMsg + PREKEY_LEN + SYNC_LEN + SOH_LEN, u->crc, duplen - (PREKEY_LEN + SYNC_LEN + SOH_LEN));
    }else{
        if(u->is_uplink == false){
            append_char_parity(rawMsg, duplen, STX);
            append_str_parity(rawMsg, duplen, u->serial, SERIAL_LEN);
            append_str_parity(rawMsg, duplen, u->flight, FLIGHT_ID_LEN);
        }
        append_char_parity(rawMsg, duplen, u->suffix);
    }
    append_str(rawMsg, duplen, u->crc, CRC_LEN);
    append_char_parity(rawMsg, duplen, DEL);

    lsb(u->lsb_with_crc_msg, rawMsg, u->total_bits);
    free(rawMsg);
}

void parity_str(uint8_t *dst, const uint8_t *src, int msg_len) {
    while (msg_len--) {
        *dst++ = parity_char(*src++);
    }
}

uint8_t parity_char(uint8_t value) {
    uint8_t res = 0;
    int count = 0;
    for (int i = 0; (value >> i) != 0; i++) {
        if ((value >> i) & 0x01) {
            count++;
        }
    }
    if (count % 2 == 0) { //1的个数为偶数
        res = (uint8_t) value | 0x80;
    } else {    //1的个数为奇数
        res = (uint8_t) value;
    }
    return res;
}

void lsb(uint8_t *dst, const uint8_t *src, const size_t len) {
    for (int i = 0; i < len / BITS_PER_BYTE; i++) {
        uint8_t out[BITS_PER_BYTE];
        num2bits(src[i], out, BITS_PER_BYTE, true);
        memcpy(dst + i * BITS_PER_BYTE, out, 8);
    }
}

at_error num2bits(const int num, uint8_t *str, const int bits, const bool is_lsb) {/*索引表*/
    if (!str) {
        return AT_ERROR_NULL;
    }

    int i = 0, j;
    unsigned unum = (unsigned) num;
    while (i < bits) {
        if (num) {
            char index[] = "01";
            str[i++] = index[unum % 2];
            unum /= 2;
        } else {
            str[i++] = 0;
        }
    }
    if (!is_lsb) {
        for (j = 0; j <= (i - 1) / 2; j++) {
            char temp = str[j];
            str[j] = str[i - 1 - j];
            str[i - 1 - j] = temp;
        }
    }
    return AT_OK;
}
