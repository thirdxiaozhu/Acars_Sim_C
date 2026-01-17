//
// Created by jiaxv on 2022/8/19.
//

#ifndef ACARS_SIM_C_HACKRF_H
#define ACARS_SIM_C_HACKRF_H

#include <libhackrf/hackrf.h>
#include <stdbool.h>
#include "GenMsg.h"


typedef struct hackrf_devs {
    bool is_repeat;
    char * serial_number;
    char * path;
    uint32_t vga_p;
    int64_t freq_p;
    char * data;
}hackrf_devs;

int transmit(const hackrf_devs *);

int initHackRF(bool repeat, const char *, const char *, int, int64_t, const char * data);

int startTransmit();

int stopTransmit();

void showEssence();

int tx_callback(hackrf_transfer *transfer);

#endif //ACARS_SIM_C_HACKRF_H
