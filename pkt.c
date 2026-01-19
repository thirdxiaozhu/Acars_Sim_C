//
// Created by jiaxv on 2022/10/27.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "pkt.h"

#define appendChar(dst, src) *(dst) = src;

void merge_elements(message_format *u) {

    const int pre_len = u->is_UP == 0 ? up_forelen : down_forelen;

    u->total_length = (prekey_len + pre_len + u->text_len + taillen) * 8;
    u->lsb_with_crc_msg = (uint8_t *) malloc(sizeof(uint8_t) * u->total_length);
    uint8_t * rawMsg = (uint8_t *) malloc(sizeof(uint8_t) * (pre_len + u->text_len + 1));
    uint8_t *parityMsg = (uint8_t *) malloc(sizeof(uint8_t) * (pre_len + u->text_len + taillen));

    int duplen = 0;
    char pro_suf = STX;
    bool isACK;
    if(u->ack != NAK_TAG){
        isACK = true;
        u->label[0] = 0x5F;
        u->label[1] = 0x7F;
        u->text[0] = 0x00;
        u->text_len = 0;
        // u->crc[0] = 0x22;
        // u->crc[1] = 0x33;

    }

    memcpy(rawMsg + duplen, head, PREFIX_LEN);
    duplen += PREFIX_LEN;
    appendChar(rawMsg + duplen, SOH);
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

    if(isACK == false){
        appendChar(rawMsg + duplen, pro_suf);
        duplen += STX_LEN;
        if (u->is_UP == 1) {
            memcpy(rawMsg + duplen, u->serial, serial_len);
            duplen += serial_len;
            memcpy(rawMsg + duplen, u->flight, flightid_len);
            duplen += flightid_len;
        }
        memcpy(rawMsg + duplen, u->text, u->text_len);
        duplen += u->text_len;
        appendChar(rawMsg + duplen, u->suffix);
        duplen += SUFFIX_len;

        fprintf(stderr, "The message you have sent is :\n");
        for(int i = 0; i < duplen; i++){
            fprintf(stderr, "%c", rawMsg[i]);
        }
        fprintf(stderr, "\n");

        parity(parityMsg, rawMsg, duplen);

        get_CRC(parityMsg + 5, u->crc, duplen - 5);

    }else{
        if(u->is_UP == 1){
            appendChar(rawMsg + duplen, pro_suf);
            duplen += STX_LEN;
            memcpy(rawMsg + duplen, u->serial, serial_len);
            duplen += serial_len;
            memcpy(rawMsg + duplen, u->flight, flightid_len);
            duplen += flightid_len;
        }
        appendChar(rawMsg + duplen, u->suffix);
        duplen += SUFFIX_len;
        parity(parityMsg, rawMsg, duplen);
    }
    unsignedMemcpy(parityMsg + duplen, u->crc, CRC_LEN);
    duplen += BCS_len;

    appendChar(parityMsg + duplen, DEL);
    duplen += BCSSUF_len;

    lsb(u->lsb_with_crc_msg, parityMsg, u->total_length);
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

void get_CRC(uint8_t *lsbMsg, uint8_t *crc_res, int msg_len) {
    uint8_t *crc16_string = (uint8_t *) malloc(sizeof(uint8_t) * 16);
    uint8_t *crc_1 = (uint8_t *) malloc(sizeof(uint8_t) * 8);
    uint8_t *crc_2 = (uint8_t *) malloc(sizeof(uint8_t) * 8);

    unsigned short crc = 0;
    while (msg_len-- > 0)
        update_crc(crc, *lsbMsg++);

    itoa((int) crc, crc16_string, 16, false); //转换为长度为16的二进制字符串

    memcpy(crc_1, crc16_string + 8, 8);
    memcpy(crc_2, crc16_string, 8);

    crc_res[0] = toInt(crc_1, 8);
    crc_res[1] = toInt(crc_2, 8);

    fprintf(stderr, "CRC:   %d %d\n", crc_res[0], crc_res[1]);

    free(crc16_string);
    free(crc_1);
    free(crc_2);
}

uint8_t toInt(const uint8_t *src, int len) {
    uint8_t res = 0;
    int index = 1;
    for (int i = len - 1; i >= 0; i--, index *= 2) {
        res += (src[i] - 48) * index;
    }

    return res;
}


void unsignedMemcpy(uint8_t *dst, uint8_t *src, int length) {
    while (length--) {
        *dst++ = *src++;
    }
}
void lsb(uint8_t *dst, const uint8_t *src, int len) {
    int i = 0;
    uint8_t *string = (uint8_t *) malloc(sizeof(uint8_t) * 8);
    uint8_t curr_char;
    for (; i < len / 8; i++) {
        if(i < prekey_len){
            curr_char = 0xFF;
        }else{
            curr_char = src[i-16];
        }
        itoa(curr_char, string, 8, true);
        unsignedMemcpy(dst + i * 8, string, 8);
    }
    //for (; i < len / 8; i++) {
    //    itoa(*(src + (i - 16)), string, 8, true);
    //    unsignedMemcpy_2(dst + i * 8, string, 8);
    //}
}

void itoa(int num, uint8_t *str, int bits, bool islsb) {/*索引表*/
    char index[] = "01";
    unsigned unum;
    int i = 0, j;
    unum = (unsigned) num;
    while (i < bits) {
        if (num) {
            str[i++] = index[unum % 2];
            unum /= 2;
        } else {
            str[i++] = 0;
        }
    }
    if (!islsb) {
        for (j = 0; j <= (i - 1) / 2; j++) {
            char temp;
            temp = str[j];
            str[j] = str[i - 1 - j];
            str[i - 1 - j] = temp;
        }
    }
}
