//
// Created by jiaxv on 2022/8/14.
//
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include "modulator.h"
#include <emmintrin.h>
#include "pkt.h"

#ifndef max
#define    max(A, B) ((A)>(B) ? (A) : (B))
#endif
#ifndef min
#define    min(A, B) ((A)<(B) ? (A) : (B))
#endif

const float msk_b[] = {0.0, 1.0};
const float msk_a[] = {1.0, -1.0};
const float am_b[] = {0.00013651, 0.00027302, 0.00013651};
const float am_a[] = {1.0, -1.96668139, 0.96722743};

at_error MSK(struct message_format *, float *);

at_error modulate(message_format *mf) {
    mf->cpfsk =  (float *) malloc(sizeof(float) * mf->total_bits * F_S);
    float *cpfsk = mf->cpfsk;
    MSK(mf, cpfsk);
    AM(mf, cpfsk);
    free(cpfsk);

    return AT_OK;
}

at_error MSK(struct message_format *mf, float *cpfsk) {
    const size_t length = mf->total_bits;
    float *ip = (float *) malloc(sizeof(float) * length);
    float *fmR = (float *) malloc(sizeof(float) * length * F_S);
    float *tsR = (float *) malloc(sizeof(float) * length * F_S);
    float *fil = (float *) malloc(sizeof(float) * length);
    float *thetaR = (float *) malloc(sizeof(float) * length * F_S);

    if (!ip || !fmR || !tsR || !fil || !thetaR) {
        if (ip)     free(ip);
        if (fmR)    free(fmR);
        if (tsR)    free(tsR);
        if (fil)    free(fil);
        if (thetaR) free(thetaR);
        fprintf(stderr, "Invalid init");
        return AT_ERROR_NULL;
    }

    diff_code(mf, ip);
    getIp(ip, length);
    getFmR(ip, fmR, length);
    getTsR(tsR, length);

    filter(msk_b, msk_a, ip, fil, length, 2);
    getThetaR(fil, thetaR, length);
    getCpfsk(cpfsk, fmR, tsR, thetaR, length * F_S);

    free(ip);
    free(fmR);
    free(tsR);
    free(fil);
    free(thetaR);

    return AT_OK;
}

void AM(message_format *mf, const float *cpfsk) {
    const int valid_length = mf->total_bits * F_S * F_S_2;
    const int total_length = BAUD * F_S * F_S_2;
    const int valid_com_length = valid_length * RESAMPLE;
    mf->complex_length = total_length * RESAMPLE;
    float *cpfskR = (float *) malloc(sizeof(float) * valid_length);
    float *t = (float *) malloc(sizeof(float) * valid_length);
    float *am = (float *) malloc(sizeof(float) * valid_length);
    float *cf_am = (float *) malloc(sizeof(float) * total_length);
    float *input_r = (float *) malloc(sizeof(float) * total_length);
    float *input_i = (float *) malloc(sizeof(float) * total_length);
    float *output_r = (float *) malloc(sizeof(float) * valid_com_length);
    float *output_i = (float *) malloc(sizeof(float) * valid_com_length);
    //mf->complex_i8 = (char *) malloc(sizeof(char) * mf->complex_length * 2);

    getCpfskR(cpfskR, cpfsk, mf->total_bits * F_S);
    getT(t, cpfskR, valid_length);
    getAM(am, cpfskR, t, valid_length);
    getCfAm(cf_am, am, valid_length, total_length);

    filter(am_b, am_a, cf_am, input_r, total_length, 3);
    for (int i = 0; i < total_length; i++) {
        *(input_i + i) = 0.0;
    }

    resample(input_r, input_i, total_length, RESAMPLE, 1, output_r, output_i, valid_com_length);

    for (int i = 0; i < valid_com_length; i++) {
        *(mf->complex_i8 + i * 2) = (int8_t) *(output_r + i);
        *(mf->complex_i8 + i * 2 + 1) = (int8_t) *(output_i + i);
    }

    const int remain_len = (mf->complex_length - valid_com_length) * 2;
    int *zero_pendding = (int *) calloc (remain_len, sizeof (int));
    memcpy(mf->complex_i8 + (valid_com_length * 2), zero_pendding, remain_len);


    FILE *_outfile;
    {
        if ((_outfile = fopen("complex_i8.txt", "wt+")) == NULL) {
            puts("Open IQ file failed!");
            exit(0);
        }

        for (int i = 0; i < mf->complex_length * 2; i++) {
            fputc(*(mf->complex_i8 + i), _outfile);
        }
        fclose(_outfile);
    }
    {
        if ((_outfile = fopen("cfam.txt", "wt+")) == NULL) {
            puts("Open IQ file failed!");
            exit(0);
        }

        for (int i = 0; i < valid_length; i++) {
            fputc(cf_am[i], _outfile);
        }
        fclose(_outfile);
    }
    {
        if ((_outfile = fopen("inputr.txt", "wt+")) == NULL) {
            puts("Open IQ file failed!");
            exit(0);
        }

        for (int i = 0; i < valid_length; i++) {
            fputc(input_r[i], _outfile);
        }
        fclose(_outfile);
    }

    free(input_r);
    free(input_i);
    free(output_r);
    free(output_i);
    free(zero_pendding);
    free(cpfskR);
    free(t);
    free(am);
    free(cf_am);

}

void diff_code(message_format *mf, float *diff) {
    *diff = 1.0;
    for (int i = 1; i < mf->total_bits; i++) {
        const int last_bit = *(mf->lsb_with_crc_msg + i - 1);
        if (*(mf->lsb_with_crc_msg + i) == last_bit)
            *(diff + i) = 1.0;
        else
            *(diff + i) = 0.0;
    }
}

void getIp(float *ip, const size_t length) {
    for (int i = 0; i < length; i++)
        *(ip + i) = *(ip + i) * 2.0 - 2.0;
}

void getFmR(const float *ip, float *fmR, const size_t length) {
    for (int i = 0; i < length; i++) {
        float temp = *(ip + i) / 4;
        for (int r = 0; r < F_S; r++)
            *(fmR + i * F_S + r) = temp;
    }
}

void getTsR(float *tsR, const size_t length) {
    float *ts = malloc(sizeof(float) * F_S);
    for (int i = 0; i < F_S; i++)
        *(ts + i) = 1.0 * i / F_S;

    for (int i = 0; i < length; i++)
        memcpy(tsR + i * F_S, ts, F_S * sizeof(float));

    free(ts);
}

void filter(const float *b, const float *a, const float *x, float *y, int length, int b_len) {
    *y = (*b) * (*x);
    for (int i = 1; i < length; i++) {
        *(y+i) = 0;
        for (int j = 0; j < b_len; j++)
            if (i >= j)
                *(y + i) = *(y + i) + (*(b + j)) * (*(x + i - j));

        for (int l = 0; l < b_len - 1; l++)
            if (i > l)
                *(y + i) = *(y + i) - (*(a + l + 1)) * (*(y + i - l - 1));
    }
}

void getThetaR(const float *filter, float *thetaR, int times) {
    for (int i = 0; i < times; i++) {
        float temp = *(filter + i) * M_PI_2;

        for (int r = 0; r < F_S; r++) {
            *(thetaR + i * F_S + r) = temp;
        }
    }
}

void getCpfsk(float *cpfsk, const float *fmR, const float *tsR, const float *thetaR, int length) {
    for (int i = 0; i < length; i++) {
        *(cpfsk + i) = cos(2 * M_PI * (1 + *(fmR + i)) * (*(tsR + i)) + *(thetaR + i));
    }
}

void getCpfskR(float *cpfskR, const float *cpfsk, int length) {
    for (int i = 0; i < length; i += 1) {
        for (int r = 0; r < F_S_2; r++) {
            *(cpfskR + i * F_S_2 + r) = *(cpfsk + i);
        }
    }
}

void getT(float *t, const float *cpfskR, int length) {
    int i = 0;
    for (float f = 0.0; f < (length - 1.0) / F_S_2 + 1.0 / F_S_2; f += 1.0 / F_S_2, i++) {
        *(t + i) = f;
    }
}

void getAM(float *am, const float *cpfskR, const float *t, int length) {
    for (int i = 0; i < length; i++) {
        float temp = cos(2.0 * M_PI * (*(t + i))) * Ac;
        *(am + i) = temp * (1.0 + *(cpfskR + i));
    }
}


void getCfAm(float *cf_am, const float *am, int valid, int total) {
    int i = 0;
    for (; i < valid; i++) {
        *(cf_am + i) = Ac * (*(am + i)) - Ac;
    }

    for (; i < total; i++) {
        *(cf_am + i) = 0;
    }
}


int resample(float *Input_real, float *Input_image, int len_Input, int p, int q, float *Output_real,
             float *Output_image, int out_cnt) {
    if (Input_real == NULL || Input_image == NULL)
        return 1;              //输入数据不能为空
    if(len_Input <= 0)
        return 2;              //数组元素个数须大于零
    if(p <= 0 || q <= 0)
        return 3;              //p、q须为非负整数
    if(p == q)
    {
        for (int i = 0; i<len_Input; i++)
        {
            Output_real[i]  = Input_real[i];
            Output_image[i] = Input_image[i];
        }
        //printf("%d   %d",len_Input,*out_cnt);
        return 4;              //p、q须不相等
    }

    int pqmax = 0, i, L, n, len_h_resample, Lhalf, delay = 0;
    float fc = 0.0;
    n = len_Input;

    if (p > q)
        pqmax = p;
    else
        pqmax = q;


    /***********定值参数********************/
    int M[4] = {1, 1, 0, 0};
    int N = 10;
    float beta = 5;
    fc = 1.0 / 2.0 / pqmax;
    float F[4] = {0, 2 * fc, 2 * fc, 1};

    L = 2 * N * pqmax + 1;

    float *h_resample, *h_firls, *Wn_kaiser;

    h_firls = (float *) malloc(sizeof(float) * L);
    Wn_kaiser = (float *) malloc(sizeof(float) * L);

    Firls_LP(h_firls, L - 1, F, M);
    Kaiser(Wn_kaiser, L, beta);

    h_resample = (float *) malloc(sizeof(float) * L);
    for (int i = 0; i < L; i++) {
        h_resample[i] = p * h_firls[i] * Wn_kaiser[i];
    }

    free(h_firls);
    free(Wn_kaiser);
    h_firls = Wn_kaiser = NULL;

    len_h_resample = L;

    Lhalf = (L - 1) / 2;
    int nz;
    nz = q - Lhalf % q;

    Lhalf = Lhalf + nz;
    //delay = floor(Lhalf / float(q));
    delay = floorf((float) Lhalf / (float) q);

    int nz1 = 0, nz2;
    nz2 = (n - 1) * p + nz + len_h_resample + nz1;
    //while (ceil(nz2 / float(q)) - delay < ceil(n * p / float(q))) {
    while (ceil(nz2 / (float) q) - delay < ceil(n * p / (float) q)) {
        nz1 = nz1 + 1;
        nz2 = (n - 1) * p + nz + len_h_resample + nz1;
    }

    float *h;
    h = (float *) malloc((nz + len_h_resample + nz1) * sizeof(float));
    for (i = 0; i < (nz + len_h_resample + nz1); i++) {
        h[i] = 0;
    }
    for (i = 0; i < len_h_resample; i++) {
        h[nz + i] = h_resample[i];
    }


    /*float *y_real, *y_image;*/
    int len_h;
    len_h = nz + len_h_resample + nz1;

    int len_pw = len_Input + len_h - 1;
    float *Output_real_initial, *Output_image_initial;
    Output_real_initial = (float *) malloc(sizeof(float) * len_pw * p);
    Output_image_initial = (float *) malloc(sizeof(float) * len_pw * p);

    upfirdn(Input_real, len_Input, h, len_h, p, q, Output_real_initial);
    upfirdn(Input_image, len_Input, h, len_h, p, q, Output_image_initial);

    int len_Output = out_cnt;
    //len_Output = ceil(len_Input * p / float(q));

    float *yro, *yio, *yroi, *yioi;
    yro = Output_real;
    yio = Output_image;
    yroi = Output_real_initial;
    yioi = Output_image_initial;

    for (int i = 0; i < len_Output; i++) {
        yro = Output_real + i;
        yroi = Output_real_initial + i + delay;
        *yro = *yroi;

        yio = Output_image + i;
        yioi = Output_image_initial + i + delay;
        *yio = *yioi;
    }

    free(Output_real_initial);
    free(Output_image_initial);
    free(h);
    free(h_resample);
    Output_real_initial = Output_image_initial = h = h_resample = NULL;
    return 0;
}


/*
* =================================================
*/
static void upfirdnmex(
        float y[], unsigned int Ly, unsigned int ky,
        float x[], unsigned int Lx, unsigned int kx,
        float h[], unsigned int Lh, unsigned int kh,
        int p,
        int q
) {
    int r, rpq_offset, k, Lg;
    int iv, ig, igv, iw;
    float *pw;
    float *pv, *pvend;
    float *pvhi, *pvlo, *pvt;
    float *pg, *pgend;
    float *pghi, *pglo, *pgt;
    int kmax = max(kh, kx);

    for (int i = 0; i < (Lx + Lh - 1) * p; i++) {
        y[i] = 0.0;
    }

    iv = q;                 //iv = 1
    ig = iw = p;               //ig = iw = 3
    igv = p * q;               //igv = 3

    for (k = 0; k < kmax; k++) {  //kmax = 1;
        pvend = x + Lx;
        //pvend???x?????????
        pgend = h + Lh;

        for (r = 0; r < p; r++) {
            pw = y + r;
            pg = h + ((r * q) % p);
            Lg = pgend - pg;
            Lg = (Lg % p) ? Lg / p + 1 : Lg / p;
            rpq_offset = (r * q) / p;
            pv = x + rpq_offset;

            /*
            * PSEUDO-CODE for CONVOLUTION with GENERAL INCREMENTS:
            *
            *   w[n] = v[n] * g[n]
            *
            * Given:
            *   pointers:   pg, pv, and pw
            *   or arrays:  g[ ], v[ ], and w[ ]
            *   increments: ig, iv, and iw
            *   end points: h+Lh, x+Lx
            */

            /*
            * Region #1 (running onto the data):
            */
            pglo = pg;
            pghi = pg + p * rpq_offset;
            pvlo = x;
            pvhi = pv;
            while ((pvhi < pvend) && (pghi < pgend)) {
                float acc = 0.0;
                pvt = pvhi;
                pgt = pglo;
                while (pgt <= pghi) {
                    acc += (*pgt) * (*pvt--);
                    pgt += ig;
                }
                *pw += acc;

                pw += iw;
                pvhi += iv;
                pghi += igv;
            }

            /*
            * Do we need to drain rest of signal?
            */
            if (pvhi < pvend) {
                /*
                * Region #2 (complete overlap):
                */
                while (pghi >= pgend) {
                    pghi -= ig;
                }
                while (pvhi < pvend) {
                    float acc = 0.0;
                    pvt = pvhi;
                    pgt = pglo;
                    while (pgt <= pghi) {
                        acc += (*pgt) * (*pvt--);
                        pgt += ig;
                    }
                    *pw += acc;
                    pw += iw;
                    pvhi += iv;
                }

            } else if (pghi < pgend) {
                /*
                * Region #2a (drain out the filter):
                */
                while (pghi < pgend) {
                    float acc = 0.0;
                    pvt = pvlo;     /* pvlo is still equal to x */
                    pgt = pghi;
                    while (pvt < pvend) {
                        acc += (*pgt) * (*pvt++);
                        pgt -= ig;
                    }
                    *pw += acc;
                    pw += iw;
                    pghi += igv;
                    pvhi += iv;
                }
            }

            while (pghi >= pgend) {
                pghi -= ig;
            }
            pvlo = pvhi - Lg + 1;


            while (pvlo < pvend) {
                /*
                *  Region #3 (running off the data):
                */
                float acc = 0.0;
                pvt = pvlo;
                pgt = pghi;
                while (pvt < pvend) {
                    acc += (*pgt) * (*pvt++);
                    pgt -= ig;
                }
                *pw += acc;
                pw += iw;
                pvlo += iv;
            }

        } /* end of r loop */


    }
}

float *upfirdn(
        float *Input,
        int len_Input,
        float *h,
        int len_h,
        int p,
        int q,
        float *Output_initial
) {

    int pp = 0, qq, x_is_row;
    int Lx, Lh, Ly,    /* Lengths           */
    kx, kh, ky,    /* number of signals */
    mx, mh, my,    /* # of rows         */
    nx, nh, ny;    /* # of columns      */

    mx = 1;
    nx = len_Input;
    x_is_row = (mx == 1);
    Lx = x_is_row ? nx : mx;
    kx = x_is_row ? mx : nx;

    mh = 1;
    nh = len_h;
    Lh = (mh == 1) ? nh : mh;
    kh = (mh == 1) ? mh : nh;

    ky = max(kx, kh);
    Ly = pp * (Lx - 1) + Lh;
    pp = p;
    qq = q;
    Ly = (Ly % qq) ? Ly / qq + 1 : Ly / qq;

    my = 1;
    ny = Ly;


    upfirdnmex(Output_initial, Ly, ky, Input, Lx, kx, h, Lh, kh, pp, qq);

    return 0;
}

void Firls_LP(float *h_firls, int N, float *F, int *M) {
    int i, j, fullband, constant_weight, Nodd, L, need_matrix, s;
    constant_weight = 1;               //类型I和II滤波器，constant_weight值恒定，由w矩阵计算而来
    int W[2] = {1, 1};                //类型I和II滤波器，w矩阵值恒定

    float *m, *k_Odd, *k_Even, *b_Even, *b_Odd, *bNew, *a;
    float **I1, **I2, **G;
    float b0, m0, b1;

    N = N + 1;                        //N阶滤波器具有N+1个系数
    Nodd = N%2;                       //判断序列长度奇偶性
    L = (N-1)/2;                      //中心对称点
    m = (float *) malloc(sizeof(float) * (L + 1));


    for (i=0; i<4; i++)
        F[i] /= 2;		              //频率归一化

    if (Constant_Diff(F))             //判断频率是否恒定
        fullband = 1;
    else
        fullband = 0;

    if (!Nodd)
    {
        //序列长为奇数
        for (i=0; i<=L; i++)
            m[i] = i + 0.5;
    }
    else   //序列长为偶数
    {
        for (i=0; i<=L; i++)
            m[i] = i;
    }
    need_matrix = (!fullband)||(!constant_weight);    //参数

    if (need_matrix)
    {
        //定义(L+1)*(L+1)大小的方阵，其值等于m方阵+m方阵的转置矩阵
        I1 = (float **) malloc(sizeof(float *) * (L + 1));
        for (i = 0; i <= L; i++)
            *(I1 + i) = (float *) malloc(sizeof(float) * (L + 1));

        for (i = 0; i <= L; i++)
            for (j = 0; j <= L; j++)
                I1[i][j] = m[i] + m[j];

        I2 = (float **) malloc(sizeof(float *) * (L + 1));
        for (i = 0; i <= L; i++)
            *(I2 + i) = (float *) malloc(sizeof(float) * (L + 1));

        for (i = 0; i <= L; i++)
            for (j = 0; j <= L; j++)
                I2[i][j] = m[i] - m[j];

        G = (float **) malloc(sizeof(float *) * (L + 1));
        for (i = 0; i <= L; i++)
            *(G + i) = (float *) malloc(sizeof(float) * (L + 1));

        for (i = 0; i <= L; i++)
            for (j = 0; j <= L; j++)
                G[i][j] = 0;
    }
    k_Even = (float *)malloc(sizeof(float) * L);       //偶阶次的k序列
    k_Odd = (float *)malloc(sizeof(float) * (L+1));    //奇阶次序列的k序列
    bNew = (float *)malloc(sizeof(float) * (L+1));     //偶阶次序列的b序列，其中b[0] = b0
    b_Even = (float *)malloc(sizeof(float) * L);       //偶阶次序列的原始b序列
    b_Odd = (float *)malloc(sizeof(float) * (L+1));    //偶阶次序列的b序列

    if (Nodd)                                            //偶阶次，b初始化
    {

        for(i=0; i<L; i++)                               //k=k(2:length(k))
        {
            k_Even[i] = m[i] + 1;
            b_Even[i] = 0;
        }
        b0 = 0;
    }
    else                                                // 奇阶次，k=m'，b初始化
    {
        k_Odd = m;
        for (i=0; i<=L; i++)
            b_Odd[i] = 0;

    }


    for (s=0; s<4; s+=2)                               //k为奇数，f(k),f(k+1)之间的理想幅值响应应有点f(k),a(k)和f(k+1),a(k+1)的连线决定
    {		                                           //k为偶数，f(k),f(k+1)之间的理想幅值被视为过渡频带
        m0 = (M[s+1] - M[s])/(F[s+1] - F[s]);		   //计算斜率
        b1 = M[s] - m0 * F[s];                         //计算截距

        if (Nodd)
        {
            b0 = b0 + (b1 * (F[s+1] - F[s]) + (m0 / 2) * (F[s+1] * F[s+1] - F[s] * F[s])) * abs(W[(s+1)/2] * W[(s+1)/2]);

            for (i=0; i<L; i++)                        //计算偶阶次的b序列，长度为L
            {
                b_Even[i] = b_Even[i] + (m0/(4 * PI * PI) * (cos(2 * PI * k_Even[i] * F[s+1]) - cos(2 * PI * k_Even[i] * F[s]))/(k_Even[i] * k_Even[i])) * abs(W[(s+1)/2] * W[(s+1)/2]);
                b_Even[i] = b_Even[i] + (F[s+1] * (m0 * F[s+1] + b1) * Sinc(2 * k_Even[i] * F[s+1]) - F[s] * (m0 * F[s] + b1) * Sinc(2 * k_Even[i] * F[s])) * abs(W[(s+1)/2] * W[(s+1)/2]);
            }
        }
        else                                          //计算奇阶次的b序列，长度为L+1
            for (i=0; i<=L; i++)
            {
                b_Odd[i] = b_Odd[i] + (m0/(4 * PI * PI) * (cos(2 * PI * k_Odd[i] * F[s+1]) - cos(2 * PI * k_Odd[i] * F[s]))/(k_Odd[i] * k_Odd[i])) * abs(W[(s+1)/2] * W[(s+1)/2]);
                b_Odd[i] = b_Odd[i] + (F[s+1] * (m0 * F[s+1] + b1) * Sinc(2 * k_Odd[i] * F[s+1]) - F[s] * (m0 * F[s] + b1) * Sinc(2 * k_Odd[i] * F[s])) * abs(W[(s+1)/2] * W[(s+1)/2]);
            }

        if (need_matrix)                          //计算G矩阵，大小为L+1 * L+1
        {
            for(i=0; i<=L; i++)
                for (j=0; j<=L; j++)
                    G[i][j] += (0.5 * F[s+1] * (Sinc(2 * I1[i][j] * F[s+1]) + Sinc(2 * I2[i][j] * F[s+1])) - 0.5 * F[s] * (Sinc(2 * I1[i][j] * F[s]) + Sinc(2 * I2[i][j] * F[s]))) * abs(W[(s+1)/2] * W[(s+1)/2]);
            for (i=0; i<=L; i++)
            {
                free(I1[i]);
                free(I2[i]);
                I1[i] = I2[i] = NULL;
            }
            free(I1);
            free(I2);
            I1 = I2 = NULL;
        }
    }


    if (Nodd)                                      //重新构造偶阶次的b序列，b=[b0;b]
    {
        bNew[0] = b0;

        for (i = 1; i <= L; i++) {
            bNew[i] = b_Even[i - 1];
        }
        free(b_Even);
        b_Even = NULL;
    }

    if (need_matrix)                                //a = G\b
    {
        if (Nodd)
            a = Divide_Matrix(G, bNew, L);
        else
            a = Divide_Matrix(G, b_Odd, L);
        free(G);
        G = NULL;
    } else {
        a = (float *) malloc(sizeof(float) * (L + 1));
        if (Nodd) {
            for (i = 0; i <= L; i++)
                a[i] = W[0] * W[0] * 4 * bNew[i];
            a[0] /= 2;
        } else {
            for (i = 0; i <= L; i++)
                a[i] = W[0] * W[0] * 4 * b_Odd[i];
        }
    }


    if (Nodd)                                   //输出偶阶次FIR滤波器的系数，h=[a(L+1:-1:2)/2; a(1); a(2:L+1)/2].'
    {
        //h = (float *)malloc(sizeof(float) * (2 * L + 1));

        for (i = 1; i <= L; i++)
            h_firls[i - 1] = a[L - i + 1] / 2;

        h_firls[L] = a[0];

        for (i = L + 1; i <= 2 * L; i++)
            h_firls[i] = a[i - L] / 2;

// 		for (i=0; i<=2*L; i++)
// 		{
// 		if (i%4 == 0)
// 		printf("\n");
// 		printf("h[%d]:%f       ", i+1, h[i]);
// 		}
// 		printf("\n");
    }else                                         //输出奇阶次FIR滤波器的系数，h=.5*[flipud(a); a].'
    {
        //h = (float *)malloc(sizeof(float) * (2 * L + 1));
        for (i = 0; i <= L; i++)
            h_firls[i] = a[L - i] / 2;

        for (i = L + 1; i <= 2 * L + 1; i++)
            h_firls[i] = a[i - L - 1] / 2;

// 		for (i=0; i<=2*L+1; i++)
// 		{
// 			if(i%4 == 0)
// 				printf("\n");
// 			printf("h[%d]:%f       ", i+1, h[i]);
// 		}
// 		printf("\n");
    }
    free(b_Even);
    free(b_Odd);
    free(bNew);
    free(k_Even);
    free(k_Odd);
    free(a);
    free(m);
    m = NULL;
    b_Even = b_Odd = bNew = k_Even = k_Odd = a = NULL;
    return;
}

bool Constant_Diff(float * F)  //判断F范围是否合理
{
    int i;
    float diff[3];

    for (i = 0; i < 2; i++)
        diff[i] = F[i + 1] - F[i];

    if (diff[1] == 0)
        return true;
    else
        return false;
}

float Sinc(float x) {
    float y;

    if (fabs(x) <= EPSILON)
        y = 1;
    else
        y = sin(PI * fabs(x)) / (PI * fabs(x));

    return y;
}

float *Divide_Matrix(float **G, float *b, int L) {
    int i, j;
    float *a;
    a = (float *) malloc(sizeof(float) * (L + 1));

    for (i = 0; i <= L; i++)
        a[i] = 0;

    Inverse_Matrix(G, L);

    for (i = 0; i <= L; i++)
        for (j = 0; j <= L; j++)
            a[i] += G[i][j] * b[j];          //矩阵相乘

    return a;
}

void Inverse_Matrix(float ** G, int L)  //采用初等矩阵变换
{
    int i, j, k;
    float temp;
    float w = 1;                //系数
    float **G_inverse;
    G_inverse = (float **) malloc(sizeof(float *) * (L + 1));

    for (i = 0; i <= L; i++)
        *(G_inverse + i) = (float *) malloc(sizeof(float) * (2 * (L + 1)));

    //构造增广矩阵
    for (i = 0; i <= L; i++) {
        for (j = 0; j <= L; j++)
            G_inverse[i][j] = G[i][j];
    }

    for (i = 0; i <= L; i++) {
        for (j = L + 1; j <= 2 * L + 1; j++)
            G_inverse[i][j] = 0;
        G_inverse[i][i + L + 1] = 1;
    }

    for (i = 0; i <= L; i++) {
        if (fabs(G_inverse[i][i]) <= EPSILON) {
            if (i == L) { //矩阵不可逆
                exit(-1);
            }
            for (j = i; j <= L; j++) {
                if (fabs(G_inverse[j][i]) > EPSILON) {
                    k = j;
                    break;
                }
            }
            //交换两行
            for (j = 0; j <= 2 * L + 1; j++) {
                temp = G_inverse[i][j];
                G_inverse[i][j] = G_inverse[k][j];
                G_inverse[k][j] = temp;
            }
        }

        //消元,除去本行，其他行该列都要进行运算
        for (j = 0; j <= L; j++) {
            if (j != i) {
                if (fabs(G_inverse[j][i]) > EPSILON) {

                    w = G_inverse[j][i] / G_inverse[i][i];

                    for (k = i; k <= 2 * L + 1; k++)
                        G_inverse[j][k] -= G_inverse[i][k] * w;
                }
            }
        }

        //将本行所有元素都除以对角线的元素
        w = G_inverse[i][i];
        for (k = 0; k <= 2 * L + 1; k++) {
            if (fabs(w) > EPSILON)
                G_inverse[i][k] /= w;
        }
    }

    for (int m = 0; m <= L; m++) {
        for (int n = 0; n <= L; n++)
            G[m][n] = G_inverse[m][n + L + 1];
    }
    for (i = 0; i <= L; i++) {
        free(G_inverse[i]);
        G_inverse[i] = NULL;
    }
    free(G_inverse);
    G_inverse = NULL;

    //return 0;
}

float Besseli (int n,float x) //第一类修正的贝塞尔函数
{
    int i, m;
    float t, y, p, b0, b1, q;
    static float a[7] = {1.0, 3.5156229, 3.0899424, 1.2067492, 0.2659732, 0.0360768, 0.0045813};
    static float b[7] = {0.5, 0.87890594, 0.51498869, 0.15084934, 0.02658773, 0.00301532, 0.00032411};
    static float c[9] = {0.39894228, 0.01328592, 0.00225319, -0.00157565, 0.00916281, -0.02057706,
                         0.02635537, -0.01647633, 0.00392377};
    static float d[9] = {0.39894228, -0.03988024, -0.00362018, 0.00163801, -0.01031555, 0.02282967,
                         -0.02895312, 0.01787654, -0.00420059};
    if (n < 0) n = -n;
    t = fabs(x);
    if (n != 1) {
        if (t < 3.75) {
            y = (x / 3.75) * (x / 3.75);
            p = a[6];
            for (i = 5; i >= 0; i--)
                p = p * y + a[i];
        } else {
            y = 3.75 / t;
            p = c[8];
            for (i = 7; i >= 0; i--)
                p = p * y + c[i];
            p = p * exp(t) / sqrt(t);
        }
    }
    if (n == 0) return (p);
    q = p;
    if (t < 3.75) {
        y = (x / 3.75) * (x / 3.75);
        p = b[6];
        for (i = 5; i >= 0; i--) p = p * y + b[i];
        p = p * t;
    } else {
        y = 3.75 / t;
        p = d[8];
        for (i = 7; i >= 0; i--) p = p * y + d[i];
        p = p * exp(t) / sqrt(t);
    }
    if (x < 0.0) p = -p;
    if (n == 1) return (p);
    if (x == 0.0) return (0.0);
    y = 2.0 / t;
    t = 0.0;
    b1 = 1.0;
    b0 = 0.0;
    m = n + (int) sqrt(40.0 * n);
    m = 2 * m;

    for (i = m; i > 0; i--) {
        p = b0 + i * y * b1;
        b0 = b1;
        b1 = p;
        if (fabs(b1) > 1.0e+10) {
            t = t * 1.0e-10;
            b0 = b0 * 1.0e-10;
            b1 = b1 * 1.0e-10;
        }
        if (i == n) t = b0;
    }
    p = t * q / b1;
    if ((x < 0.0) && (n % 2 == 1)) p = -p;
    return (p);
}


int Kaiser (float * Wn_Kaiser,int L,float beta)//凯撒窗函数，默认beat值是0.5；
{
    int i, odd, n, xind;
    //beta=5;
    float bes = abs(Besseli(0, beta));
    float *W, *x, *W1;
    W = (float *) malloc(sizeof(float) * L);
    x = (float *) malloc(sizeof(float) * L);
    W1 = (float *) malloc(sizeof(float) * L);
    //Wn = (float *)malloc(sizeof (float) * L);
    if (L % 2 == 0) { odd = 0; }
    else { odd = 1; }
    xind = (L - 1) * (L - 1);
    n = (L + 1) / 2;
    for (i = 0; i < n; i++) {
        x[i] = i + 0.5 * (1 - odd);
        x[i] = 4 * x[i] * x[i];
        W[i] = Besseli(0, beta * sqrt(1 - x[i] / xind)) / bes;
    }
    //对数组W进行逆序处理，存储在W1中，最后再跟W合并到Wn中；

    for (i = 0; i < n; i++) {
        W1[i] = W[n - 1 - i];
    }
    for (i = 0; i < n; i++) {
        Wn_Kaiser[i] = W1[i];
    }
    if (L % 2 == 0) {
        for (i = 0; i < n; i++) {
            Wn_Kaiser[i + n] = W[i];
        }
        //return Wn;
    } else {
        for (i = 0; i < n; i++) {
            Wn_Kaiser[i + n - 1] = W[i];
        }
        //return Wn;
    }
    free(W);
    free(x);
    free(W1);
    W = x = W1 = NULL;
    return 0;

}

