//
// Created by jiaxv on 2022/8/14.
//

#ifndef ACARS_SIM_C_MODULATOR_H
#define ACARS_SIM_C_MODULATOR_H

#include <stdbool.h>
#include "pkt.h"


#define F_S 20
#define Ac  64.0
#define F_S_2  2
#define BAUD 2400
#define PI 3.1415926
#define EPSILON 0.000001
#define RESAMPLE 12

//extern "C" void modulatorTest();
//extern "C" void modulate(message_format *);
void modulate(struct message_format *);

void MSK(struct message_format *, float *);

void AM(struct message_format *, float *);

void diff_code(struct message_format *, float *);

void getIp(float *, int);

void getFmR(const float *, float *, int);

void getTsR(float *, int);

void filter(const float *, const float *, const float *, float *, int, int);

void getThetaR(const float *, float *, int);

void getCpfsk(float *, const float *, const float *, const float *, int);

void getCpfskR(float *, const float *, int);

void getT(float *, const float *, int);

void getAM(float *, const float *, const float *, int);

void getCfAm(float *, const float *, int, int);

void convert_sse2(const float *inbuf, int8_t *outbuf, const unsigned int count);


int resample(float *Input_real, float *Input_image, int len_Input, int p, int q, float *Output_real,
             float *Output_image, int out_cnt);

void Firls_LP(float *h_firls, int N, float *F, int *M);   //计算FIR滤波器抽头系数
bool Constant_Diff(float *);             //判断频率范围是否合理
float Sinc(float);                      //辛格函数
float *Divide_Matrix(float **, float *, int);  //矩阵左除
void Inverse_Matrix(float **, int);      //求逆矩阵
float Besseli(int n, float x);

int Kaiser(float *Wn_Kaiser, int L, float beta);

static void upfirdnmex(
        float y[], unsigned int Ly, unsigned int ky,
        float x[], unsigned int Lx, unsigned int kx,
        float h[], unsigned int Lh, unsigned int kh,
        int p,
        int q
);

float *upfirdn(
        float *Input,
        int len_Input,
        float *h,
        int len_h,
        int p,
        int q,
        float *Output_initial
);

#endif //ACARS_SIM_C_MODULATOR_H
