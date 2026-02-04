
#include "pkt.h"
#include "modulator.h"
#include "hackrf.h"
#include "acarstrans.h"
#include "usrp.h"
#include "util.h"



void show_version() {
    printf("Acars Simulator version %.1f\n", VERSION);
    printf("Copyright (C) 2022-2026 Civil Aviation University of China (CAUC).\n");
    printf("This is free software, and you are welcome to redistribute it.\n");
}

void usage() {
    printf("Usage:\n");
    printf("\t-h # this help\n");
    printf("\t-v show software version.\n");
    printf("\t-d serial_number # Serial number of desired HackRF.\n");
    printf("\t-F <filename> # Transmit from data file (use '-' for manually input).\n");
    printf("\t-f freq_hz # Frequency in Hz [127.0MHz to 139.0MHz] (default is 131450000Hz / 131.450Hz).\n");
    printf("\t-x gain_db # TX VGA (IF) gain, 0-47dB, 1dB steps (default is 20).\n");
    printf("\t[-R] # Repeat TX mode (default is off).\n");
    printf("\t[-H] # Using HackRF (default)\n");
    printf("\t[-U] # Using USRP\n");
};

int main(int argc, char **argv) {
    // test();
    // exit(0);

    int mode = MANUAL_MODE;
    DEVICE dev = HACKRF;

    int64_t freq = 131450000;
    uint32_t gain = 30;
    message_format *mf = malloc(sizeof(message_format));
    char device_arg[32] = {0};
    char path[256] = {0};
    bool is_repeat = false;

    int c;
    while ((c = getopt(argc, argv, "d:f:x:F:RvhHU")) != EOF) {
        switch (c) {
            case 'H':
                dev = HACKRF;
                break;
            case 'U':
                dev = USRP;
                break;
            case 'd':
                // hackrf.serial_number = optarg;
                memcpy(device_arg, optarg, strlen(optarg));
                fprintf(stderr, "%s %s", device_arg, optarg);
                break;
            case 'F':
                memcpy(path, optarg, strlen(optarg));
                if(strcmp(path, "-") != 0){
                    mode = FILE_MODE;
                } else{
                    mode = MANUAL_MODE;
                }
                break;
            case 'f':
                parse_frequency_i64(optarg, NULL, &freq);
                break;
            case 'x':
                parse_u32(optarg, &gain);
                break;
            case 'R':
                is_repeat = true;
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

    if (mode == MANUAL_MODE) {
        generate_pkt(mf);
        merge_elements(mf);
        modulate(mf);
    }

    switch (dev) {
        case HACKRF: {
            hackrf_args_t hackrf = {0};
            hackrf.serial_number = device_arg;
            hackrf.path = path;
            hackrf.is_repeat = is_repeat;
            hackrf.gain = gain;
            hackrf.freq = freq;

            hackrf_transfer_data(&hackrf, mf);
            hackrf_transmit(&hackrf);
            break;
        }
        case USRP: {

            // 太大了，如果不放在栈上，就会报错
            usrp_args_t *usrp = malloc(sizeof(usrp_args_t));

            usrp->device_args = "type=b200,serial=192113";
            usrp->channel = 0;
            usrp->gain = gain;
            usrp->freq = freq;
            usrp->rate = SAMP_RATE;

            usrp->tune_request.target_freq = (double) freq;
            usrp->tune_request.rf_freq_policy = UHD_TUNE_REQUEST_POLICY_AUTO;
            usrp->tune_request.dsp_freq_policy = UHD_TUNE_REQUEST_POLICY_AUTO;

            usrp->stream_args.cpu_format = "fc32";
            usrp->stream_args.otw_format = "sc16";
            usrp->stream_args.args = "";
            usrp->stream_args.channel_list = &usrp->channel;
            usrp->stream_args.n_channels = 1;

            usrp_transfer_data(usrp, mf);
            usrp_transmit(usrp);

            break;
        }
    }

    free(mf);

    return 0;
}
