//
// Created by jiaxv on 2026/2/1.
//

#ifndef ACARSTRANS_USRP_H
#define ACARSTRANS_USRP_H
#include <uhd.h>
#include "pkt.h"

typedef struct {
    char *device_args;
    bool is_repeat;
    uint32_t gain;
    int64_t freq;
    double rate;

    float data[SAMPLE_MAX_LEN * 2];

    size_t channel;
    uhd_usrp_handle usrp;
    uhd_tx_streamer_handle tx_streamer;
    uhd_tx_metadata_handle md;
    uhd_tune_request_t tune_request;
    uhd_stream_args_t stream_args;
    uhd_tune_result_t tune_result;
}usrp_args_t;

void usrp_transfer_data(usrp_args_t *args, const message_format *mf);

at_error usrp_transmit(usrp_args_t *args);

void test();


#endif //ACARSTRANS_USRP_H