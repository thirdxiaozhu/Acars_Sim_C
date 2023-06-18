#ifndef DEFINITION_H
#define  DEFINITION_H
#define PI 3.1415926
#define EPSILON 0.000001

void Firls_LP(double *h_firls,int N, double * F, int * M);   //����FIR�˲�����ͷϵ��
bool Constant_Diff(double *);             //�ж�Ƶ�ʷ�Χ�Ƿ����
double Sinc(double);                      //������
double * Divide_Matrix(double **, double *, int);  //�������
void Inverse_Matrix(double **, int);      //�������
double Besseli (int n,double x) ;
int Kaiser (double * Wn_Kaiser,int L,double beta);
int gcd(int a,int b	);
static void upfirdnmex(
	double y[],  unsigned int Ly,  unsigned int ky, 
	double x[],  unsigned int Lx,  unsigned int kx, 
	double h[],  unsigned int Lh,  unsigned int kh, 
	int p,
	int q
	);
double *upfirdn(
	double *Input,
	int len_Input,
	double *h,
	int len_h,
	int p,
	int q,
	double *Output_initial
	);

#endif
