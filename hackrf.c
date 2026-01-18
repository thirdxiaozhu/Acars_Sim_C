//
// Created by jiaxv on 2022/8/19.
//

#include <stdio.h>
#include <stdlib.h>
#include <libhackrf/hackrf.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <math.h>
#include <signal.h>
#include "hackrf.h"
#include "pkt.h"

#define SAMP_RATE 1152000
#define HW_SYNC_MODE_OFF 0
int exit_code = EXIT_SUCCESS;
//struct message_format *messageFormat;
const char * iq_data;
static hackrf_device *device = NULL;
FILE *file = NULL;

struct timeval time_start;
struct timeval t_start;
struct timeval t_end;
volatile uint32_t byte_count = 0;
size_t bytes_to_xfer = 0;
bool isRepeat;
static volatile bool do_exit = false;
float time_diff;
uint64_t stream_amplitude = 0;    /* sum of magnitudes of all I&Q samples, reset on the periodic report */

bool hw_sync = false;
uint32_t hw_sync_enable = 0;
int64_t freq;

int vga_tx;

int mm = 0;

void sigint_callback_handler(int signum) {
    fprintf(stderr, "Caught signal %d\n", signum);
    do_exit = true;
}

int parseu32(char *s, uint32_t *const value) {
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
        return HACKRF_SUCCESS;
    } else {
        return HACKRF_ERROR_INVALID_PARAM;
    }
}

int parse_frequencyi64(char *optarg, char *endptr, int64_t *value) {
    *value = (int64_t) strtod(optarg, &endptr);
    if (optarg == endptr) {
        return HACKRF_ERROR_INVALID_PARAM;
    }
    return HACKRF_SUCCESS;
}

static float
TimevalDiff(const struct timeval *a, const struct timeval *b) {
    return (a->tv_sec - b->tv_sec) + 1e-6f * (a->tv_usec - b->tv_usec);
}

int initHackRF(const bool repeat, const char *serial_number, const char *path, const int vga_p, const int64_t freq_p, const char *data) {
    vga_tx = vga_p;
    freq = freq_p;
    isRepeat = repeat;
    iq_data = data;

    int result = hackrf_init();
    if (result != HACKRF_SUCCESS) {
        fprintf(stderr, "hackrf_init() failed: %s (%d)\n", hackrf_error_name(result), result);
        return EXIT_FAILURE;
    }
    result = hackrf_open_by_serial(serial_number, &device);
    if (result != HACKRF_SUCCESS) {
        fprintf(stderr, "hackrf_open() failed: %s (%d)\n", hackrf_error_name(result), result);
        return EXIT_FAILURE;
    }

    if (path != NULL) {
        file = fopen(path, "rb");
        if (file == NULL) {
            fprintf(stderr, "Failed to open file: %s\n", path);
            return EXIT_FAILURE;
        }
    }

    result = hackrf_set_sample_rate(device, SAMP_RATE);
    if (result != HACKRF_SUCCESS) {
        fprintf(stderr, "hackrf_set_sample_rate() failed: %s (%d)\n", hackrf_error_name(
                result), result);
        return EXIT_FAILURE;
    }

    result = hackrf_set_hw_sync_mode(device, HW_SYNC_MODE_OFF);
    if (result != HACKRF_SUCCESS) {
        fprintf(stderr, "hackrf_set_hw_sync_mode() failed: %s (%d)\n", hackrf_error_name(result), result);
        return EXIT_FAILURE;
    }

    result = hackrf_set_txvga_gain(device, vga_tx);
    if (result != HACKRF_SUCCESS) {
        fprintf(stderr, "hackrf_start_?x() failed: %s (%d)\n", hackrf_error_name(result), result);
        return EXIT_FAILURE;
    }


    result |= hackrf_start_tx(device, tx_callback, NULL);
    if (result != HACKRF_SUCCESS) {
        fprintf(stderr, "hackrf_start_?x() failed: %s (%d)\n", hackrf_error_name(result), result);
        return EXIT_FAILURE;
    }

    //result = parse_frequencyi64(freq_c, endptr, &freq_hz);
    result = hackrf_set_freq(device, freq);

    if (result != HACKRF_SUCCESS) {
        fprintf(stderr, "hackrf_set_freq() failed: %s (%d)\n", hackrf_error_name(result), result);
        return EXIT_FAILURE;
    }


    signal(SIGINT, &sigint_callback_handler);
    signal(SIGILL, &sigint_callback_handler);
    signal(SIGFPE, &sigint_callback_handler);
    signal(SIGSEGV, &sigint_callback_handler);
    signal(SIGTERM, &sigint_callback_handler);
    signal(SIGABRT, &sigint_callback_handler);


    return EXIT_SUCCESS;
}

int startTransmit() {
    showEssence();
    fprintf(stderr, "Start transmitting......\n");
    fprintf(stderr, "Stop with Ctrl-C\n");
    gettimeofday(&t_start, NULL);
    gettimeofday(&time_start, NULL);

    while ((hackrf_is_streaming(device) == HACKRF_TRUE) &&
           (do_exit == false)) {

        uint32_t byte_count_now;
        struct timeval time_now;
        float time_difference, rate;
        uint64_t stream_amplitude_now;
        sleep(1);
        gettimeofday(&time_now, NULL);

        byte_count_now = byte_count;
        byte_count = 0;
        stream_amplitude_now = stream_amplitude;
        stream_amplitude = 0;
        if (byte_count_now < SAMP_RATE / 20)    // Don't report on very short frames
            stream_amplitude_now = 0;

        time_difference = TimevalDiff(&time_now, &time_start);
        rate = (float) byte_count_now / time_difference;
        if (byte_count_now == 0 && hw_sync == true && hw_sync_enable != 0) {
            fprintf(stderr, "Waiting for sync...\n");
        } else {
            // This is only an approximate measure, to assist getting receive levels right:
            double full_scale_ratio = ((double) stream_amplitude_now / (byte_count_now ? byte_count_now : 1)) / 128;
            double dB_full_scale_ratio = 10 * log10(full_scale_ratio);
            if (dB_full_scale_ratio > 1)
                dB_full_scale_ratio = NAN;    // Guard against ridiculous reports
        }

        time_start = time_now;

        if (byte_count_now == 0 && (hw_sync == false || hw_sync_enable == 0)) {
            exit_code = EXIT_FAILURE;
            fprintf(stderr, "\nCouldn't transfer any bytes for one second.\n");
            break;
        }
    }

    return EXIT_SUCCESS;
    //stopTransmit();
}

int stopTransmit() {
    int result;
    result = hackrf_is_streaming(device);
    if (do_exit) {
        fprintf(stderr, "\nExiting...\n");
    } else {
        fprintf(stderr, "\nExiting... hackrf_is_streaming() result: %s (%d)\n", hackrf_error_name(result), result);
    }
    gettimeofday(&t_end, NULL);
    time_diff = TimevalDiff(&t_end, &t_start);
    fprintf(stderr, "Total time: %5.5f s\n", time_diff);

    if (device != NULL) {
        result = hackrf_stop_tx(device);
        if (result != HACKRF_SUCCESS) {
            fprintf(stderr, "hackrf_stop_tx() failed: %s (%d)\n", hackrf_error_name(result), result);
        } else {
            fprintf(stderr, "hackrf_stop_tx() done\n");
        }

        result = hackrf_close(device);
        if (result != HACKRF_SUCCESS) {
            fprintf(stderr, "hackrf_close() failed: %s (%d)\n", hackrf_error_name(result), result);
        } else {
            fprintf(stderr, "hackrf_close() done\n");
        }

        //hackrf_exit();
        //fprintf(stderr, "hackrf_exit() done\n");
    }
    if (file != NULL) {
        if (file != stdin) {
            fflush(file);
        }
        if ((file != stdout) && (file != stdin)) {
            fclose(file);
            signal(SIGINT, &sigint_callback_handler);
            signal(SIGILL, &sigint_callback_handler);
            signal(SIGFPE, &sigint_callback_handler);
            signal(SIGSEGV, &sigint_callback_handler);
            signal(SIGTERM, &sigint_callback_handler);
            signal(SIGABRT, &sigint_callback_handler);

            file = NULL;
            fprintf(stderr, "fclose() done\n");
        }
    }
    fprintf(stderr, "exit\n");
    return exit_code;
}

void showEssence(){
    fprintf(stderr,"HackRF Settings:\n");
    fprintf(stderr,"Frequency: %.2f\n", freq/1e6);
    fprintf(stderr,"TX VGA: %d\n", vga_tx);
    fprintf(stderr,"Repeat: %d\n", isRepeat);
    fprintf(stderr,"\n");
}

int transmit(const hackrf_devs * hd){
    if(initHackRF(hd->is_repeat, hd->serial_number, hd->path, hd->vga_p, hd->freq_p, hd->data) != EXIT_SUCCESS){
        fprintf(stderr, "Init HackRF Failed!");
        return EXIT_FAILURE;
    }
    if(startTransmit()!= EXIT_SUCCESS){
        fprintf(stderr, "Start Trans Failed!");
        return EXIT_FAILURE;
    }
    usleep(500000);
    if(stopTransmit() != EXIT_SUCCESS){
        fprintf(stderr, "Stop Trans Failed!");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int tx_callback(hackrf_transfer *transfer) {
    size_t bytes_to_read;
    size_t remain;
    size_t bytes_read;
    char * p = NULL;
    /* 如果文件路径不为空，则为文件模式(尚未做完), 否则为数据模式  */
    if (file != NULL) {
        byte_count += transfer->valid_length;
        bytes_to_read = transfer->valid_length;
        bytes_read = fread(transfer->buffer, 1, bytes_to_read, file);
        if (bytes_read != bytes_to_read) {
            if (isRepeat) {
                fprintf(stderr, "Transmitting one messages.\n");
                rewind(file);
                fread(transfer->buffer + bytes_read, 1, bytes_to_read - bytes_read, file);
                return 0;
            } else {
                return -1; /* not repeat mode, end of file */
            }
        } else {
            return 0;
        }
    } else if (iq_data != NULL) {
        remain = SAMP_RATE * 2 - byte_count;
        bytes_to_read = (remain > transfer->valid_length) ? transfer->valid_length : remain;
        memcpy(transfer->buffer, iq_data + byte_count, bytes_to_read);
        byte_count += bytes_to_read;
        //判断是否为循环模式
        if(isRepeat){
            if (bytes_to_read < transfer->valid_length) {
                fprintf(stderr, "Transmitting one messages.\n");
                memcpy(transfer->buffer + bytes_to_read, iq_data,
                       transfer->valid_length - bytes_to_read);
                byte_count = transfer->valid_length - bytes_to_read;
                return 0;
            }else{
                return 0;
            }
        }else{
            //acars_security项目调用的是该模式
            bool isend = false;

            /*
             * 判断是否到文件结尾，目前最优判断方法，未来可以改进，方法如下：
             * hackrf所使用的IQ数据，一个信号由两个有符号8bit数据组成，经过调制后的信号，虚部一定为0，且每段信号前一部分必定存在一段大于0的数据
             * 据此可以判断一段数据是否为报文开头
             */
            p = memchr(transfer->buffer, 0, bytes_to_read);
            if(p){
                if(*p >= 0 && *(p + 1) >= 0 && mm > 0){
                    isend = true;
                }else{
                    mm++;
                }
            }
            if(isend){
                fprintf(stderr, "Transmit one message.\n");
                return -1; /* not repeat mode, end of file */
            }else{
                return 0;
            }
        }

    } else {
        return -1;
    }
}
