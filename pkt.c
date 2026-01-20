//
// Created by jiaxv on 2022/10/27.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "pkt.h"

#define appendChar(head, offset, src) \
    *(head+offset) = src;\
    offset++;

#define append_string(dst, src, n)

void merge_elements(message_format *u) {

    const int pre_len = u->is_uplink == true ? UPLINK_PRELEN : DOWNLINK_PRELEN;

    u->total_bits = (PREKEY_LEN + pre_len + u->text_len + TAIL_LEN) * 8;
    u->lsb_with_crc_msg = (uint8_t *) malloc(sizeof(uint8_t) * u->total_bits);
    uint8_t *rawMsg = malloc(sizeof(uint8_t) * (pre_len + u->text_len + 1));
    uint8_t *parityMsg = malloc(sizeof(uint8_t) * (pre_len + u->text_len + TAIL_LEN));

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

    memcpy(rawMsg + duplen, head, PREFIX_LEN);
    duplen += PREFIX_LEN;
    appendChar(rawMsg, duplen, SOH);
    duplen += SOH_LEN;
    appendChar(rawMsg + duplen, u->mode);
    duplen += MODE_LEN;
    memcpy(rawMsg + duplen, u->arn, ARN_LEN);
    duplen += ARN_LEN;
    appendChar(rawMsg + duplen, u->ack);
    duplen += ACK_LEN;
    memcpy(rawMsg + duplen, u->label, LABEL_LEN);
    duplen += LABEL_LEN;
    appendChar(rawMsg + duplen, u->bi);
    duplen += BI_LEN;

    if(!is_ack){
        appendChar(rawMsg + duplen, STX);
        duplen += STX_LEN;
        if (!u->is_uplink) {
            memcpy(rawMsg + duplen, u->serial, SERIAL_LEN);
            duplen += SERIAL_LEN;
            memcpy(rawMsg + duplen, u->flight, FLIGHTID_LEN);
            duplen += FLIGHTID_LEN;
        }
        memcpy(rawMsg + duplen, u->text, u->text_len);
        duplen += u->text_len;
        appendChar(rawMsg + duplen, u->suffix);
        duplen += SUFFIX_LEN;

        fprintf(stderr, "The message you have sent is :\n");
        for(int i = 0; i < duplen; i++){
            fprintf(stderr, "%c", rawMsg[i]);
        }
        fprintf(stderr, "\n");

        parity(parityMsg, rawMsg, duplen);

        get_crc(parityMsg + PREFIX_LEN + SOH_LEN, u->crc, duplen - (PREFIX_LEN + SOH_LEN));
    }else{
        if(u->is_uplink == false){
            appendChar(rawMsg + duplen, STX);
            duplen += STX_LEN;
            memcpy(rawMsg + duplen, u->serial, SERIAL_LEN);
            duplen += SERIAL_LEN;
            memcpy(rawMsg + duplen, u->flight, FLIGHTID_LEN);
            duplen += FLIGHTID_LEN;
        }
        appendChar(rawMsg + duplen, u->suffix);
        duplen += SUFFIX_LEN;
        parity(parityMsg, rawMsg, duplen);
    }
    // unsignedMemcpy(parityMsg + duplen, u->crc, CRC_LEN);
    memcpy(parityMsg + duplen, u->crc, CRC_LEN);
    duplen += BCS_LEN;

    appendChar(parityMsg + duplen, DEL);
    duplen += BCSSUF_LEN;

    lsb(u->lsb_with_crc_msg, parityMsg, u->total_bits);
    free(parityMsg);
    //free(crc);

}

void parity(uint8_t *dst, const uint8_t *src, int msg_len) {
    for (int i = 0; i < msg_len; i++) {
        dst[i] = parityCheck(src[i]);
    }
}

uint8_t parityCheck(uint8_t value) {
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

void get_crc(const uint8_t *lsb_msg, uint8_t *crc_res, int msg_len) {
    unsigned short crc = 0;
    while (msg_len-- > 0)
        update_crc(crc, *lsb_msg++);

    crc_res[0] = (uint8_t)(crc & 0xFF);
    crc_res[1] = (uint8_t)((crc >> 8) & 0xFF);

    fprintf(stderr, "CRC:   %d %d\n", crc_res[0], crc_res[1]);

}

void lsb(uint8_t *dst, const uint8_t *src, int len) {
    int i = 0;
    uint8_t *string = (uint8_t *) malloc(sizeof(uint8_t) * 8);
    uint8_t curr_char;
    for (; i < len / 8; i++) {
        if(i < PREKEY_LEN){
            curr_char = 0xFF;
        }else{
            curr_char = src[i-16];
        }
        num2bits(curr_char, string, 8, true);
        memcpy(dst + i * 8, string, 8);
    }
    //for (; i < len / 8; i++) {
    //    itoa(*(src + (i - 16)), string, 8, true);
    //    unsignedMemcpy_2(dst + i * 8, string, 8);
    //}
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
