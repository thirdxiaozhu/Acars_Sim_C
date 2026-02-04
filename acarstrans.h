//
// Created by jiaxv on 2026/1/17.
//

#ifndef ACARS_SIM_C_ACARSTRANS_H
#define ACARS_SIM_C_ACARSTRANS_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdint.h>
#include <getopt.h>
#include <regex.h>

#define VERSION 0.1

typedef enum {
    AT_OK = 0,
    AT_ERROR_INVALID_PARAM,
    AT_ERROR_INVALID_FILE,
    AT_ERROR_INTERNAL,
    AT_ERROR_INVALID,
    AT_ERROR_NULL,
} at_error;

typedef enum {
    HACKRF = 0,
    USRP
}DEVICE;

#endif //ACARS_SIM_C_ACARSTRANS_H