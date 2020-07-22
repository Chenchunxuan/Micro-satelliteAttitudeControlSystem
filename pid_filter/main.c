#include <stdio.h>
#include <stdlib.h>
#include "math.h"
//#include "read_noise.h"

#define N 2000

typedef unsigned char  		UCHAR8;                    /* defined for unsigned 8-bits integer variable 	    �޷���8λ���ͱ���  */
typedef signed   char  		SCHAR8;                    /* defined for signed 8-bits integer variable		�з���8λ���ͱ���  */
typedef unsigned short 		USHORT16;                  /* defined for unsigned 16-bits integer variable 	�޷���16λ���ͱ��� */
typedef signed   short 		SSHORT16;                  /* defined for signed 16-bits integer variable 		�з���16λ���ͱ��� */
typedef unsigned int   		UINT32;                    /* defined for unsigned 32-bits integer variable 	�޷���32λ���ͱ��� */
typedef int   				SINT32;                    /* defined for signed 32-bits integer variable 		�з���32λ���ͱ��� */
typedef float          		FP32;                      /* single precision floating point variable (32bits) �����ȸ�������32λ���ȣ� */
typedef double         		DB64;                      /* double precision floating point variable (64bits) ˫���ȸ�������64λ���ȣ� */

/*PID�ṹ��*/
typedef struct
{
  FP32 fpDes;   //���Ʊ���Ŀ��ֵ        1
  FP32 fpFB;    //���Ʊ�������ֵ        2

	FP32 fpKp;  //����ϵ��Kp            3
	FP32 fpKi;  //����ϵ��Ki            4
	FP32 fpKd;  //΢��ϵ��Kd            5

	FP32 fpE;   //����ƫ��              6
	FP32 fpPreE;//�ϴ�ƫ��              7
    FP32 fpSumE;//��ƫ��                8

	FP32 fpU;   //����PID������       9
	FP32 fpUMax;//PID�����������ֵ������������ʱ������ֵ     10

	FP32 fpTs;  //PID��������(s)
	FP32 fpUKp; //���������
	FP32 fpUKi; //���������
	FP32 fpUKd; //΢�������

	FP32 fpEmin;
}ST_PID;

#define K_DEG2RAD 0.017453292519943F
#define K_RAD2DEG 57.295779513082323F

//�����˲�pid����
#define PID_DT      0.005F  //pid���Ƽ��(s)

//roll
#define KP_ROLL     200.0f
#define KI_ROLL     0.0F
#define KD_ROLL     10.0F
#define UMAX_ROLL   100.0F  //����޷� deg/s

//yaw
#define KP_YAW     200.0F
#define KI_YAW     0.0F
#define KD_YAW     10.0F
#define UMAX_YAW   100.0F  //����޷� deg/s

//pitch
#define KP_PITCH     200.0F
#define KI_PITCH     0.0F
#define KD_PITCH     50.0F
#define UMAX_PITCH   100.0F //����޷� deg/s

FP32 roll,yaw,pitch; 		                //������ŷ����
FP32 roll_star,yaw_star,pitch_star; 		//��������ŷ����
float t;
FP32 w[3];
FP32 zstar[4];
FP32 q[4]={1, 0, 0, 0};
FP32 dQ_pre[4]={1, 0, 0, 0};
FP32 yaw_pre;

//�洢matlab���ɵķ�������ֵ
FP32 nstar0[N];   //���������������
FP32 nstar1[N];
FP32 nstar2[N];
FP32 nstar3[N];
FP32 ngyroPhi[N]; //�������������
FP32 ngyroTheta[N];
FP32 ngyroPsi[N];
FILE *fpread;

//�����˲���PID
ST_PID st_pid_roll={0, 0, KP_ROLL, KI_ROLL, KD_ROLL, 0, 0, 0, 0, UMAX_ROLL, PID_DT, 0, 0, 0, 0.005};
ST_PID st_pid_yaw={0, 0, KP_YAW, KI_YAW, KD_YAW, 0, 0, 0, 0, UMAX_YAW,PID_DT, 0, 0, 0, 0.005};
ST_PID st_pid_pitch={0, 0, KP_PITCH, KI_PITCH, KD_PITCH, 0, 0, 0, 0, UMAX_PITCH, PID_DT, 0, 0, 0, 0.005};

void calculate_attitude_form_gyro(FP32 wx, FP32 wy, FP32 wz, FP32 dt,FP32 *p_roll, FP32 *p_yaw, FP32 *p_pitch);
void calculate_attitude_from_star(FP32 star0, FP32 star1, FP32 star2, FP32 star3, FP32 *p_roll, FP32 *p_yaw, FP32 *p_pitch);
void CalPID(ST_PID *pstPid);
void get_noise(FP32 nstar0[], FP32 nstar1[], FP32 nstar2[], FP32 nstar3[], FP32 ngyroPhi[], FP32 ngyroTheta[], FP32 ngyroPsi[]);

FP32 systime;
SINT32 my_count;
FILE *file;

int main(void)
{
	file = fopen("data.txt","w");
	get_noise(nstar0, nstar1, nstar2, nstar3, ngyroPhi, ngyroTheta, ngyroPsi);
	while(systime < 10.0f)
	{
		systime += PID_DT;
		SysTick_Handler();
		printf("time=%f\t roll=%f\t yaw=%f\t pitch=%f\n", systime, roll, yaw, pitch);
		fprintf(file, "%.30f\t %.30f\t %.30f\t %.30f\t %.30f\t %.30f\t %.30f\n", systime, st_pid_roll.fpDes, st_pid_roll.fpFB, st_pid_yaw.fpDes, st_pid_yaw.fpFB, st_pid_pitch.fpDes, st_pid_pitch.fpFB);
        my_count++;
	}
	fclose(file);
	return 0;
}

void get_noise(FP32 nstar0[], FP32 nstar1[], FP32 nstar2[], FP32 nstar3[], FP32 ngyroPhi[], FP32 ngyroTheta[], FP32 ngyroPsi[])
{
    fpread = fopen("nstar0.txt", "r");
    read_noise(fpread, N, nstar0);
    fpread = fopen("nstar1.txt", "r");
    read_noise(fpread, N, nstar1);
    fpread = fopen("nstar2.txt", "r");
    read_noise(fpread, N, nstar2);
    fpread = fopen("nstar3.txt", "r");
    read_noise(fpread, N, nstar3);
    fpread = fopen("ngyroPhi.txt", "r");
    read_noise(fpread, N, ngyroPhi);
    fpread = fopen("ngyroTheta.txt", "r");
    read_noise(fpread, N, ngyroTheta);
    fpread = fopen("ngyroPsi.txt", "r");
    read_noise(fpread, N, ngyroPsi);
}

//��શ�ʱ���жϷ�����
void SysTick_Handler(void)
{
    //��ȡ����������ֵ
    zstar[0] = nstar0[my_count];
    zstar[1] = nstar1[my_count];
    zstar[2] = nstar2[my_count];
    zstar[3] = nstar3[my_count];

    //������������̬����(Ŀ��ֵ)
    calculate_attitude_from_star(zstar[0], zstar[1], zstar[2], zstar[3], &roll_star, &yaw_star, &pitch_star);

    //��ȡ��������ֵ
    w[0] = ngyroPhi[my_count];
    w[1] = ngyroTheta[my_count];
    w[2] = ngyroPsi[my_count];

    //PID�����˲�
    //roll
    st_pid_roll.fpDes = 1.0F;
    //st_pid_roll.fpDes = roll_star;
    st_pid_roll.fpFB = roll;
    CalPID(&st_pid_roll);
    FP32 fpu_roll = st_pid_roll.fpU * K_DEG2RAD;  //������ٶ�����ֵ�����ֻ���
    //yaw
    st_pid_yaw.fpDes = 1.0F;
    //st_pid_yaw.fpDes = yaw_star;
    st_pid_yaw.fpFB = pitch;
    CalPID(&st_pid_yaw);
    FP32 fpu_yaw = st_pid_yaw.fpU * K_DEG2RAD;  //������ٶ�����ֵ�����ֻ���
    //pitch
    st_pid_pitch.fpDes = 1.0F;
    //st_pid_pitch.fpDes = pitch_star;
    st_pid_pitch.fpFB = pitch;
    CalPID(&st_pid_pitch);
    FP32 fpu_pitch = st_pid_pitch.fpU * K_DEG2RAD;  //������ٶ�����ֵ�����ֻ���

    //��̬�ǽ���(����ֵ)
    calculate_attitude_form_gyro(w[0]+fpu_roll, w[1]+fpu_yaw, w[2]+fpu_pitch, 0.005f, &roll, &yaw, &pitch);
    //calculate_attitude_form_gyro(w[0],w[1],w[2],0.005f,&roll,&yaw,&pitch);
}

FP32 yaw_temp_pre;
FP32 yaw_temp_e;
void calculate_attitude_form_gyro(FP32 wx, FP32 wy, FP32 wz, FP32 dt,FP32 *p_roll, FP32 *p_yaw, FP32 *p_pitch)
{
  double dv0[16];
  double roll_tmp;
  double b_roll_tmp;
  double c_roll_tmp;
  int i0;
  double dQ[4];
  double d_roll_tmp;

  /* ��ȡ�����ǽ��ٶ�rad/s */
  /* �˶�ѧ����dQ=f(w) */
  dv0[0] = 0.5*q[0];
  roll_tmp = 0.5*(-q[1]);
  dv0[4] = roll_tmp;
  b_roll_tmp = 0.5*(-q[2]);
  dv0[8] = b_roll_tmp;
  c_roll_tmp = 0.5*(-q[3]);
  dv0[12] = c_roll_tmp;
  dv0[1] = 0.5*q[1];
  dv0[5] = 0.5*q[0];
  dv0[9] = c_roll_tmp;
  dv0[13] = 0.5*q[2];
  dv0[2] = 0.5*q[2];
  dv0[6] = 0.5*q[3];
  dv0[10] = 0.5*q[0];
  dv0[14] = roll_tmp;
  dv0[3] = 0.5*q[3];
  dv0[7] = b_roll_tmp;
  dv0[11] = 0.5*q[1];
  dv0[15] = 0.5*q[0];

  /* ���� */
  /* ���λ��ֹ�ʽ */
  for (i0 = 0; i0 < 4; i0++) {
    dQ[i0] = 0.0;
    roll_tmp = ((dv0[i0]*0.0+dv0[i0 + 4]*wx)+dv0[i0+8]*wy)+dv0[i0+12]*wz;
    dQ[i0] = roll_tmp;
    q[i0] += 0.5*(roll_tmp+dQ_pre[i0])*dt;
  }

  /* ��һ�� */
  roll_tmp = ((q[0]*q[0]+q[1]*q[1])+q[2]*q[2])+q[3]*q[3];
  q[0] /= roll_tmp;
  q[1] /= roll_tmp;
  q[2] /= roll_tmp;
  q[3] /= roll_tmp;

  /* ת��Ϊ��̬����� */
  b_roll_tmp = q[0]*q[0];
  c_roll_tmp = q[1]*q[1];
  roll_tmp = q[2]*q[2];
  d_roll_tmp = q[3]*q[3];

  *p_roll = atan2(2.0*q[0]*q[1]-2.0*q[2]*q[3], ((b_roll_tmp-c_roll_tmp)+roll_tmp)-d_roll_tmp)*K_RAD2DEG;
  FP32 yaw_temp = atan2(2.0*q[0]*q[2]-2.0*q[1]*q[3], ((b_roll_tmp+c_roll_tmp)-roll_tmp)-d_roll_tmp)*K_RAD2DEG;
  *p_pitch = asin(2.0*q[1]*q[2]+2.0*q[0]*q[3])*K_RAD2DEG;

  /* yaw��������� */
  yaw_temp_e = yaw_temp-yaw_temp_pre; //���������ۼ�ʽ����++
	*p_yaw += yaw_temp_e;
  if (yaw_temp_e < -180.0)
	{
    *p_yaw += 360.0;
  }
	else  if (yaw_temp_e > 180.0)
	{
    *p_yaw -= 360.0;
  }

	//��¼�ϴβ���
  yaw_temp_pre = yaw_temp;
  dQ_pre[0] = dQ[0];
  dQ_pre[1] = dQ[1];
  dQ_pre[2] = dQ[2];
  dQ_pre[3] = dQ[3];
}


void calculate_attitude_from_star(FP32 star0, FP32 star1, FP32 star2, FP32 star3, FP32 *p_roll, FP32 *p_yaw, FP32 *p_pitch)
{
	//��Ԫ����һ��
	FP32 temp = sqrt(star0*star0+star1*star1+star2*star2+star3*star3);
	star0 = star0/temp;
	star1 = star1/temp;
	star2 = star2/temp;
	star3 = star3/temp;

	*p_roll = atan2(2*(star0*star1+star2*star3),1-2*(star1*star1+star2*star2));
	*p_yaw = asin(2*(star0*star2-star3*star1));
	*p_pitch = atan2(2*(star0*star3+star1*star2),1-2*(star2*star2+star3*star3));
}

/*******************************************************************
�������ƣ�CalPID(ST_PID *pstPid)
�������ܣ���ͨ��PID�㷨����PID��
��    ע��
********************************************************************/
void CalPID(ST_PID *pstPid)
{
  //���㵱ǰƫ��
	pstPid->fpE = pstPid->fpDes - pstPid->fpFB;

	//λ��ʽPID���㹫ʽ
	//if(fabs(pstPid->fpE) <= pstPid->fpEmin){pstPid->fpE=0.0f;}
    pstPid->fpSumE += pstPid->fpE * pstPid->fpKi;
    pstPid->fpU = pstPid->fpKp * pstPid->fpE
	            + pstPid->fpSumE
                + pstPid->fpKd * (pstPid->fpE - pstPid->fpPreE);
	pstPid->fpPreE = pstPid->fpE;//���汾��

  //PID��������޷�
  if(pstPid->fpU > pstPid->fpUMax)
	{
	  pstPid->fpU = pstPid->fpUMax;
	}
	else if(pstPid->fpU < -pstPid->fpUMax)
	{
	  pstPid->fpU = -pstPid->fpUMax;
	}
}
