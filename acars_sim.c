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
#define RESET_COLOR "\033[0m"
#define SUCCESS 0;
#define ERROR 1;

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

at_error get_input(const char* prompt, const size_t max_size, uint8_t *output) {
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
    // uint8_t ss[2] = {0xCB, 0x37};
    // uint8_t rr[2] = {0};
    // get_crc(ss, rr, 2);

    int c;
    int mode = MANUAL;
    char *freq_c = "131450000";
    char *vga_c = "20";
    message_format mf = {0};
    hd = (struct hackrf_devs *) malloc(sizeof (struct hackrf_devs));
    // mf = (struct message_format *) malloc(sizeof(struct message_format));

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
        generate_pkt(&mf);
        merge_elements(&mf);
        modulate(&mf);
    }
    hd->data = mf.complex_i8;

    transmit(hd);
    return 0;
}
