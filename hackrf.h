//
// Created by jiaxv on 2022/8/19.
//

#ifndef ACARS_SIM_C_HACKRF_H
#define ACARS_SIM_C_HACKRF_H

#include <libhackrf/hackrf.h>
#include <stdbool.h>
#include "pkt.h"

typedef struct hackrf_devs_s {
    bool is_repeat;
    char *serial_number;
    char *path;
    uint32_t vga_p;
    int64_t freq_p;
    char data[SAMPLE_MAX_LEN * 2];
}hackrf_args_t;

int hackrf_transmit(const hackrf_args_t *);

int initHackRF(bool repeat, const char *, const char *, int, int64_t, const char * data);

int startTransmit();

int stopTransmit();

void show_essence();

int tx_callback(hackrf_transfer *transfer);

void hackrf_transfer_data(hackrf_args_t *args, const message_format *mf);

#endif //ACARS_SIM_C_HACKRF_H
