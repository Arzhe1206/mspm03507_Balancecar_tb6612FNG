#ifndef __MPU6050_H
#define __MPU6050_H
#include "board.h"   												  	  

void mpu6050_init(void);
void mpu6050_read(int16_t *gyro,int16_t *accel,float *temperature);
typedef struct 
{	//欧拉角
	float Pitch;
	float Roll;
	float Yaw;
	//温度
	float temperature;
	//原始值
	int16_t  Accel_Original[3];
	int16_t Gyro_Original[3];
	//校准值
	float Accel_Offset[3];
	float Gyro_Offset[3];
	//校准后的值
	float Accel_Calulate[3];
	float Gyro_Calulate[3]; 
	//滤波后的值
	float Accel_Average[3];
	float Gyro_Average[3];
	
}MPU6050_DEF;

extern MPU6050_DEF mpu6050;
extern float Gyro_Z_Measeure;
void MPU6050_ReadDatas_Proc(void);

void AHRS_Geteuler(void);


//卡尔曼滤波参数结构体
struct KalmanFilter{
	float LastP;			//上一次协方差
	float NewP;				//最新的协方差
	float Out;				//卡尔曼输出
	float Kg;				//卡尔曼增益
	float Q;				//过程噪声的协方差
	float R;				//观测噪声的协方差
};

//MyMath
#define M_PI  				(float)3.1415926535f
#define squa( Sq )       	(((float)Sq)*((float)Sq))
#define toRad( Math_D )	  	((float)(Math_D)*0.0174532925f)
#define toDeg( Math_R )	  	((float)(Math_R)*57.2957795f)
#define absu16( Math_X )  	(Math_X<0? -Math_X:Math_X)
#define absFloat( Math_X )	(Math_X<0? -Math_X:Math_X)
double IIR_I_Filter(double InData, double *x, double *y, double *b, short nb, double *a, short na);
float LPF_1st(float oldData, float newData, float lpf_factor);
void kalmanfiter(struct KalmanFilter *EKF,float input);

#endif
