//
// Created by jiaxv on 2022/10/27.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "pkt.h"

#define update_crc(crc,c) crc= (crc>> 8)^crc16_ccitt_table[(crc^(c))&0xff];

#define append_char(head, offset, src) \
    *(head+offset) = src;\
    offset++;

#define append_str(head, offset, src, n) \
    memcpy(head+offset, src, n);\
    offset += n;


#define UPLINK_PRELEN (PREFIX_LEN + SOH_LEN + MODE_LEN + ARN_LEN + ACK_LEN + LABEL_LEN + BI_LEN + STX_LEN)
#define DOWNLINK_PRELEN (PREFIX_LEN + SOH_LEN + MODE_LEN + LABEL_LEN + ARN_LEN + BI_LEN + ACK_LEN + STX_LEN + SERIAL_LEN + FLIGHTID_LEN)
#define TAIL_LEN  (SUFFIX_LEN + BCS_LEN + BCSSUF_LEN)

static unsigned short crc16_ccitt_table[256] =
        {
                0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
                0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
                0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
                0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
                0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
                0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
                0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
                0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
                0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
                0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
                0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
                0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
                0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
                0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
                0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
                0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
                0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
                0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
                0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
                0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
                0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
                0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
                0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
                0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
                0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
                0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
                0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
                0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
                0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
                0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
                0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
                0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78
        };

void parity_str(uint8_t *dst, const uint8_t *src, int msg_len) ;

uint8_t parity_char(uint8_t value);

void get_crc(const uint8_t *lsb_msg, uint8_t *crc_res, int msg_len);

void lsb(uint8_t *dst, const uint8_t *src, int len);

at_error num2bits(int num, uint8_t *str, int bits, bool is_lsb);

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

    append_str(rawMsg, duplen, head, PREFIX_LEN);
    append_char(rawMsg, duplen, SOH);
    append_char(rawMsg, duplen, u->mode);
    append_str(rawMsg, duplen, u->arn, ARN_LEN);
    append_char(rawMsg, duplen, u->ack);
    append_str(rawMsg, duplen, u->label, LABEL_LEN);
    append_char(rawMsg, duplen, u->bi);

    if(!is_ack){
        append_char(rawMsg, duplen, STX);
        if (!u->is_uplink) {
            append_str(rawMsg, duplen, u->serial, SERIAL_LEN);
            append_str(rawMsg, duplen, u->flight, FLIGHTID_LEN);
        }
        append_str(rawMsg, duplen, u->text, u->text_len);
        append_char(rawMsg, duplen, u->suffix);

        fprintf(stderr, "The message you have sent is :\n");
        for(int i = 0; i < duplen; i++){
            fprintf(stderr, "%c", rawMsg[i]);
        }
        fprintf(stderr, "\n");

        parity_str(parityMsg, rawMsg, duplen);

        get_crc(parityMsg + PREFIX_LEN + SOH_LEN, u->crc, duplen - (PREFIX_LEN + SOH_LEN));
    }else{
        if(u->is_uplink == false){
            append_char(rawMsg, duplen, STX);
            append_str(rawMsg, duplen, u->serial, SERIAL_LEN);
            append_str(rawMsg, duplen, u->flight, FLIGHTID_LEN);
        }
        append_char(rawMsg, duplen, u->suffix);
        parity_str(parityMsg, rawMsg, duplen);
    }
    append_str(parityMsg, duplen, u->crc, CRC_LEN);

    append_char(parityMsg, duplen, DEL);

    lsb(u->lsb_with_crc_msg, parityMsg, u->total_bits);
    free(parityMsg);
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
