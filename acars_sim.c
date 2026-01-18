#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <getopt.h>
#include <regex.h>
#include "pkt.h"
#include "modulator.h"
#include "hackrf.h"
#include "acarstrans.h"
#include "util.h"


#define MANUAL 1000
#define FILE 1001
#define LIGHT_GREEN  "\033[1;32m"
#define LIGHT_RED    "\033[1;31m"
#define SUCCESS 0;
#define ERROR 1;

message_format *mf = NULL;
hackrf_devs *hd = NULL;

int tx_vga;
char *endptr = NULL;
int64_t freq_hz;

int parse_u32(char *s, uint32_t *const value) {
    uint_fast8_t base = 10;
    char *s_end;
    uint64_t ulong_value;

    if (strlen(s) > 2) {
        if (s[0] == '0') {
            if ((s[1] == 'x') || (s[1] == 'X')) {
                base = 16;
                s += 2;
            } else if ((s[1] == 'b') || (s[1] == 'B')) {
                base = 2;
                s += 2;
            }
        }
    }

    s_end = s;
    ulong_value = strtoul(s, &s_end, base);
    if ((s != s_end) && (*s_end == 0)) {
        *value = (uint32_t) ulong_value;
        return SUCCESS;
    } else {
        return ERROR;
    }
}

int parse_frequency_i64(char *optarg, char *endptr, int64_t *value) {
    *value = (int64_t) strtod(optarg, &endptr);
    if (optarg == endptr) {
        return ERROR;
    }
    return SUCCESS;
}

at_error regex_string(const char *regex, const char *text) {
    if (!text) {
        return AT_ERROR_NULL;
    }
    regex_t oregex;   // 编译后的结构体
    if (!regcomp(&oregex, regex, REG_EXTENDED | REG_NOSUB)) {
        if (!regexec(&oregex, text, 0, NULL, 0)) {
            regfree(&oregex);
            return AT_OK;
        }
        return AT_ERROR_INVALID;
    }
    return AT_ERROR_INTERNAL;
}

at_error regex_char(char *regex, char c) {
    char *temp = (char *) malloc(sizeof(char));
    *temp = c;

    const at_error ret = regex_string(regex, temp);
    free(temp);

    return ret;
}

at_error get_input(const char* prompt, const size_t max_size, char *output) {
    if (!output) {
        return AT_ERROR_NULL;
    }
    char input[MAX_BUFFER_LEN] = {0};

    printf("%s", prompt);
    if (fgets(input, (int)max_size + 1, stdin) != NULL) {
        size_t len = strlen(input);
        if (len > 0 && input[len-1] == '\n') {
            input[len-1] = '\0';
        } else if (len == max_size) {
            // 如果输入长度等于缓冲区大小，可能存在未读取的字符
            int ch;
            while ((ch = getchar()) != '\n' && ch != EOF);  // 清理剩余字符
        }

        memcpy(output, input, max_size);
        return AT_OK;
    }
    return AT_ERROR_INVALID;
}

int generate_pkt(struct message_format * mf) {
    mf->crc = (uint8_t *) malloc(sizeof(uint8_t) * BCS_len);

    while (true) {
        char isUplink[1] = {0};
        get_input(LIGHT_GREEN"Is Uplink?  [Y/N]: ", 1, isUplink);

        if (regex_char("[YyNn]", *isUplink) == AT_OK) {
            mf->isUp = (*isUplink == 'Y' || *isUplink == 'y') ? 0 : 1;
            break;
        }
    }
    while (true) {
        char mode[1] = {0};
        get_input(LIGHT_GREEN"Mode: ", 1, mode);
        mf->mode = *mode;
        if (mf->mode == '2') {
            break;
        }
        printf(LIGHT_RED"Only support Mode A, please input '2' !\n");
    }
    while (true) {
        mf->arn = (uint8_t *) malloc(sizeof(uint8_t) * arn_len);
        uint8_t *temparn = (uint8_t *) malloc(sizeof(uint8_t) * arn_len);
        for (int r = 0; r < 7; r++) {
            *(mf->arn + r) = '.';
        }

        printf(LIGHT_GREEN"Address: ");
        scanf("%7s", temparn);

        if (!regex_string("[A-Z0-9.-]", temparn)) {
            int i = 0;
            for (; i < 7; i++) {
                if (*(temparn + i) == '\0') {
                    break;
                }
            }
            memcpy(mf->arn + 7 - i, temparn, i);
            break;
        } else {
            printf(LIGHT_RED"Valid characters are {A-Z} {0-9} {.} and {-} !\n");
        }
    }
    while (true) {
        printf(LIGHT_GREEN"ACK (Enter for NAK): ");
        getchar();
        mf->ack = getchar();
        if (mf->isUp == 0) {
            if (mf->ack == '\n') {
                mf->ack = 21;
                mf->crc[0] = 0x22;
                mf->crc[1] = 0x33;
                break;
            }else if (!regex_char("[0-9]", mf->ack)) {
                break;
            }else {
                printf(LIGHT_RED"Uplink technical acknowledgement must be a character between 0 to 9 or a NAK !\n");
            }
        } else if (mf->isUp == 1) {
            if (mf->ack == '\n') {
                mf->ack = 21;
                mf->crc[0] = 0x22;
                mf->crc[1] = 0x33;
                break;
            }else if (!regex_char("[A-Za-z]", mf->ack)) {
                break;
            }else {
                printf(LIGHT_RED"Downlink technical acknowledgement must be a character between A to Z / a to z / a NAK !\n");
            }
        }
    }
    while (true) {
        mf->label = (uint8_t *) malloc(sizeof(uint8_t) * 2);
        printf(LIGHT_GREEN"Label: ");
        scanf("%2s", mf->label);
        if (!regex_string("[A-Z0-9]{2}", mf->label)) {
            break;
        } else {
            printf(LIGHT_RED"Valid characters are {A-Z} and {0-9} and the length is 2!\n");
        }
    }

    while (true) {
        if (mf->isUp == 0) {
            printf(LIGHT_GREEN"UBI (Enter for NAK): ");
            getchar();
            mf->udbi = getchar();
            if (mf->udbi == '\n') {
                mf->udbi = 21;
                break;
            }else if (!regex_char("[A-Za-z]", mf->udbi)) {
                break;
            } else {
                printf(LIGHT_RED"Uplink technical acknowledgement must be a character between A to Z / a to z / a NAK !\n");
            }
        } else if (mf->isUp == 1) {
            printf(LIGHT_GREEN"DBI : ");
            getchar();
            mf->udbi = getchar();
            if (!regex_char("[0-9]", mf->udbi)) {
                break;
            } else {
                printf(LIGHT_RED"Downlink technical acknowledgement must be a character between 0-9 or a NAK !\n");
            }
        }
    }
    while (true) {
        int i;
        mf->text = (uint8_t *) malloc(sizeof(uint8_t) * 220);
        printf("Text :");
        scanf("%220s", mf->text);
        for (i = 0; i < 220; i++) {
            if (*(mf->text + i) == '\0') {
                mf->text_len = i;
                break;
            }
        }
        if (i == 220) {
            printf(LIGHT_RED"The Length of Text must shorter than 220 characters!\n");
        } else {
            break;
        }
    }
    if (mf->isUp == 1) {
        while (true) {
            mf->flight = (uint8_t *) malloc(sizeof(uint8_t) * flightid_len);
            printf("Flight ID :");
            scanf("%6s", mf->flight);
            if (!regex_string("[A-Z0-9]{6}", mf->flight)) {
                break;
            } else {
                printf(LIGHT_RED"The Length of flight id must be 6 and only {A-Z}/{0-9} are avilable!\n");
            }
        }
        mf->serial = (uint8_t *) malloc(sizeof(uint8_t) * 4);
        uint8_t serial[4] = "M01A";
        memcpy(mf->serial, serial, serial_len);
    }

    mf->suffix = 0x03;

    mf->complex_i8 = (char* ) malloc(sizeof (char ) * BAUD * F_S * F_S_2 * RESAMPLE*2);
}

void show_version() {
    printf("Acars Simulator version %.1f\n", VERSION);
    printf("Copyright (C) 2022 Civil Aviation University of China (CAUC).\n");
    printf("This is free software, and you are welcome to redistribute it.\n");
}

void usage() {
    printf("Usage:\n");
    printf("\t-h # this help\n");
    printf("\t-v show software version.\n");
    printf("\t-d serial_number # Serial number of desired HackRF.\n");
    printf("\t-t <filename> # Transmit from data file (use '-' for manually input).\n");
    printf("\t-f freq_hz # Frequency in Hz [127.0MHz to 139.0MHz] (default is 131450000Hz / 131.450Hz).\n");
    printf("\t-x gain_db # TX VGA (IF) gain, 0-47dB, 1dB steps (default is 20).\n");
    printf("\t[-R] # Repeat TX mode (default is off).\n");
};

int main(int argc, char **argv) {
    int c;
    int mode = MANUAL;
    char *freq_c = "131450000";
    char *vga_c = "20";
    hd = (struct hackrf_devs *) malloc(sizeof (struct hackrf_devs));
    mf = (struct message_format *) malloc(sizeof(struct message_format));

    while ((c = getopt(argc, argv, "d:f:x:t:Rvh")) != EOF) {
        switch (c) {
            case 'd':
                hd->serial_number = optarg;
                break;
            case 't':
                hd->path = optarg;
                if(strcmp(hd->path, "-") != 0){
                    mode = FILE;
                } else{
                    mode = MANUAL;
                }
                break;
            case 'f':
                freq_c = optarg;
                break;
            case 'x':
                vga_c = optarg;
                break;
            case 'R':
                hd->is_repeat = true;
                break;
            case 'v':
                show_version();
                return 0;
            case 'h':
                usage();
                return 0;
            default:
                show_version();
                usage();
                return 0;
        }
    }

    parse_u32(vga_c, &hd->vga_p);
    parse_frequency_i64(freq_c, endptr, &hd->freq_p);

    if (mode == MANUAL) {
        generate_pkt(mf);
        merge_elements(mf);
        modulate(mf);
    }
    hd->data = mf->complex_i8;

    transmit(hd);
    return 0;
}
