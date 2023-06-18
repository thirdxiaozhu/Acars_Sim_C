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
    int vga_p;
    int64_t freq_p;
    char * data;
}hackrf_devs;

int Transmit(struct hackrf_devs *);

int initHackRF(bool repeat, const char *, const char *, const int *, const int64_t *, const char * data);

int startTransmit();

int stopTransmit();

void showEssence();

int tx_callback(hackrf_transfer *transfer);

#endif //ACARS_SIM_C_HACKRF_H
