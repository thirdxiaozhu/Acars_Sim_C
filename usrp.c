//
// Created by jiaxv on 2026/2/1.
//

#include <stdio.h>
#include <signal.h>
#include "uhd.h"

#define EXECUTE_OR_GOTO(label, ...) \
if (__VA_ARGS__) {              \
return_code = EXIT_FAILURE; \
goto label;                 \
}

bool stop_signal_called = false;

void sigint_handler(int code)
{
    (void)code;
    stop_signal_called = true;
}


void test() {
    int return_code          = EXIT_SUCCESS;
    char error_string[512];
    double freq = 131450000;
    size_t channel           = 0;
    double rate              = 1152000;
    double gain              = 40;


    uhd_usrp_handle usrp;
    fprintf(stderr, "Creating USRP with args \"%s\"...\n", "aa");
    EXECUTE_OR_GOTO(free_option_strings, uhd_usrp_make(&usrp, "type=b200,serial=3459F45"))

    // Create TX streamer
    uhd_tx_streamer_handle tx_streamer;
    EXECUTE_OR_GOTO(free_usrp, uhd_tx_streamer_make(&tx_streamer))


    // Create TX metadata
    uhd_tx_metadata_handle md;
    EXECUTE_OR_GOTO(
        free_tx_streamer, uhd_tx_metadata_make(&md, false, 0, 0.1, true, false))

    // Create other necessary structs
    uhd_tune_request_t tune_request = {
        .target_freq = freq,
        .rf_freq_policy = UHD_TUNE_REQUEST_POLICY_AUTO,
        .dsp_freq_policy = UHD_TUNE_REQUEST_POLICY_AUTO
    };
    uhd_tune_result_t tune_result;

    uhd_stream_args_t stream_args = {
        .cpu_format = "sc16",
        .otw_format = "sc16",
        .args = "",
        .channel_list = &channel,
        .n_channels = 1
    };

    size_t samps_per_buff;
    float *buff = NULL;
    const void **buffs_ptr = NULL;

    // Set rate
    fprintf(stderr, "Setting TX Rate: %f...\n", rate);
    EXECUTE_OR_GOTO(free_tx_metadata, uhd_usrp_set_tx_rate(usrp, rate, channel))

    // See what rate actually is
    EXECUTE_OR_GOTO(free_tx_metadata, uhd_usrp_get_tx_rate(usrp, channel, &rate))
    fprintf(stderr, "Actual TX Rate: %f...\n\n", rate);

    // Set gain
    fprintf(stderr, "Setting TX Gain: %f db...\n", gain);
    EXECUTE_OR_GOTO(free_tx_metadata, uhd_usrp_set_tx_gain(usrp, gain, 0, ""))

    // See what gain actually is
    EXECUTE_OR_GOTO(free_tx_metadata, uhd_usrp_get_tx_gain(usrp, channel, "", &gain))
    fprintf(stderr, "Actual TX Gain: %f...\n", gain);

    // Set frequency
    fprintf(stderr, "Setting TX frequency: %f MHz...\n", freq / 1e6);
    EXECUTE_OR_GOTO(free_tx_metadata,
        uhd_usrp_set_tx_freq(usrp, &tune_request, channel, &tune_result))

    // See what frequency actually is
    EXECUTE_OR_GOTO(free_tx_metadata, uhd_usrp_get_tx_freq(usrp, channel, &freq))
    fprintf(stderr, "Actual TX frequency: %f MHz...\n", freq / 1e6);

    // Set up streamer
    stream_args.channel_list = &channel;
    EXECUTE_OR_GOTO(
        free_tx_streamer, uhd_usrp_get_tx_stream(usrp, &stream_args, tx_streamer))

    // Set up buffer
    EXECUTE_OR_GOTO(
        free_tx_streamer, uhd_tx_streamer_max_num_samps(tx_streamer, &samps_per_buff))

    fprintf(stderr, "Buffer size in samples: %zu\n", samps_per_buff);
    buff = calloc(samps_per_buff * 2, sizeof(float));
    buffs_ptr = (const void **) &buff;
    size_t i = 0;
    for (i = 0; i < (samps_per_buff * 2); i += 2) {
        buff[i] = 0.1f;
        buff[i + 1] = 0;
    }

    // Ctrl+C will exit loop
    signal(SIGINT, &sigint_handler);
    fprintf(stderr, "Press Ctrl+C to stop streaming...\n");

    // Actual streaming
    uint64_t num_acc_samps = 0;
    size_t num_samps_sent  = 0;

    while (1) {
        uint64_t total_num_samps = 10000000;
        if (stop_signal_called)
            break;
        if (num_acc_samps >= total_num_samps)
            break;

        EXECUTE_OR_GOTO(free_buff,
            uhd_tx_streamer_send(
                tx_streamer, buffs_ptr, samps_per_buff, &md, 0.1, &num_samps_sent))

        num_acc_samps += num_samps_sent;

            fprintf(stderr, "Sent %zu samples\n", num_samps_sent);
    }

free_buff:
        free(buff);
free_tx_streamer:
    fprintf(stderr, "Cleaning up TX streamer.\n");
    uhd_tx_streamer_free(&tx_streamer);

free_tx_metadata:
    fprintf(stderr, "Cleaning up TX metadata.\n");
    uhd_tx_metadata_free(&md);
free_usrp:
    fprintf(stderr, "Cleaning up USRP.\n");
    if (return_code != EXIT_SUCCESS && usrp != NULL) {
        uhd_usrp_last_error(usrp, error_string, 512);
        fprintf(stderr, "USRP reported the following error: %s\n", error_string);
    }
    uhd_usrp_free(&usrp);
free_option_strings:
    return ;
}