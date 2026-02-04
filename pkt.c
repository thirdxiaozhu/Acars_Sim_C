//
// Created by jiaxv on 2022/10/27.
//

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


int generate_pkt(message_format *mf) {
    while (true) {
        uint8_t isUplink[1] = {0};
        get_input(LIGHT_GREEN"Is Uplink?  [Y/N]: "RESET_COLOR, 1, isUplink);

        if (regex_char("[YyNn]", *isUplink) == AT_OK) {
            mf->is_uplink = (*isUplink == 'Y' || *isUplink == 'y') ? true : false;
            break;
        }
    }
    while (true) {
        uint8_t mode[MODE_LEN] = {0};
        get_input(LIGHT_GREEN"Mode: "RESET_COLOR, MODE_LEN, mode);
        if (*mode == '2') {
            mf->mode = *mode;
            break;
        }
        printf(LIGHT_RED"Only support Mode A, please input '2' !\n"RESET_COLOR);
    }
    while (true) {
        uint8_t temp_arn[ARN_LEN] = {0};
        for (int r = 0; r < ARN_LEN; r++) {
            *(mf->arn + r) = '.';
        }

        get_input(LIGHT_GREEN"Address: "RESET_COLOR, ARN_LEN, temp_arn);

        if (!regex_string("[A-Z0-9.-]", temp_arn)) {
            memcpy(mf->arn + ARN_LEN - strlen(temp_arn), temp_arn, strlen(temp_arn));
            break;
        }
        printf(LIGHT_RED"Valid characters are {A-Z} {0-9} {.} and {-} !\n"RESET_COLOR);
    }
    while (true) {
        uint8_t ack[ACK_LEN] = {0};
        get_input(LIGHT_GREEN"ACK (Enter for NAK): "RESET_COLOR, ACK_LEN, ack);

        if (!strlen(ack)) {
            mf->ack = NAK_TAG;
            break;
        }

        if (mf->is_uplink == true) {
            if (!regex_char("[0-9]", *ack)) {
                mf->ack = *ack;
                break;
            }
            printf(LIGHT_RED"Uplink technical acknowledgement must be a character between 0 to 9 or a NAK !\n"RESET_COLOR);
        } else {
            if (!regex_char("[A-Za-z]", *ack)) {
                mf->ack = *ack;
                break;
            }
            printf(LIGHT_RED"Downlink technical acknowledgement must be a character between A to Z / a to z / a NAK !\n"RESET_COLOR);
        }
    }
    while (true) {
        get_input(LIGHT_GREEN"Label: "RESET_COLOR, LABEL_LEN, mf->label);

        if (!regex_string("[A-Z0-9]{2}", mf->label)) {
            break;
        }
        printf(LIGHT_RED"Valid characters are {A-Z} and {0-9} and the length is 2!\n"RESET_COLOR);
    }

    while (true) {
        uint8_t bi[BI_LEN] = {0};
        if (mf->is_uplink == true) {
            get_input(LIGHT_GREEN"UBI (Enter for NAK): "RESET_COLOR, BI_LEN, bi);
            if (!strlen(bi)) {
                mf->bi = NAK_TAG;
                break;
            }
            if (!regex_char("[A-Za-z]", *bi)) {
                mf->bi = *bi;
                break;
            }
            printf(LIGHT_RED"Uplink technical acknowledgement must be a character between A to Z / a to z / a NAK !\n"RESET_COLOR);
        } else if (mf->is_uplink == false) {
            get_input(LIGHT_GREEN"DBI: ", BI_LEN, bi);
            if (!strlen(bi)) {
                mf->bi = NAK_TAG;
                break;
            }
            if (!regex_char("[0-9]", *bi)) {
                mf->bi = *bi;
                break;
            }
            printf(LIGHT_RED"Downlink technical acknowledgement must be a character between 0-9 or a NAK !\n"RESET_COLOR);
        }
    }
    while (true) {
        get_input(LIGHT_GREEN"TEXT: "RESET_COLOR, TEXT_MAX_LEN, mf->text);
        mf->text_len = strlen(mf->text);
        break;
    }
    if (mf->is_uplink == false) {
        while (true) {
            get_input(LIGHT_GREEN"Flight ID: "RESET_COLOR, FLIGHT_ID_LEN, mf->flight);
            if (!regex_string("[A-Z0-9]{6}", mf->flight)) {
                break;
            }
            printf(LIGHT_RED"The Length of flight id must be 6 and only {A-Z}/{0-9} are avilable!\n"RESET_COLOR);
        }
        memcpy(mf->serial, "M01A", SERIAL_LEN);
    }

    mf->suffix = ETX;
}

