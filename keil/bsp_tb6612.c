/*
 * ������������Ӳ�������������չ����Ӳ�����Ϲ���ȫ����Դ
 * �����������www.lckfb.com
 * ����֧�ֳ�פ��̳���κμ������⻶ӭ��ʱ����ѧϰ
 * ������̳��https://oshwhub.com/forum
 * ��עbilibili�˺ţ������������塿���������ǵ����¶�̬��
 * ��������׬Ǯ���������й�����ʦΪ����
 * Change Logs:
 * Date           Author       Notes
 * 2024-07-08     LCKFB-LP    first version
 */
#include "bsp_tb6612.h"
uint8_t MOTOR1_ENABLE_FLAG = 0;//���ʹ�ܱ�־λ
uint8_t MOTOR2_ENABLE_FLAG = 0;

uint8_t Turn_PID_Flag = 1;
uint8_t Distance_PID_Flag = 0;
uint8_t Gyro_PID_Flag = 0;
uint8_t Angle_PID_Flag = 0;

/*---------------------------------------------------------------------------------------------------*/
/*---------------------------------------PID���㲿�ֲ���---------------------------------------------*/
/*---------------------------------------------------------------------------------------------------*/
//�ٶȻ������޸�
pid_t pid_Motor1_Speed ={	\
	.Kp = 100,				\
	.Ki = 20,				\
	.Kd = 10,				\
	.Target = 0,			\
	.Measure = 0,			\
	{0,0,0},				\
	.KpOut = 0,				\
	.KiOut = 0,				\
	.KdOut = 0,				\
	.PID_Out = 0,			\
	.PID_Limit_MAX = 9999,	\
	.Ki_Limit_MAX = 9999,	\
};
pid_t pid_Motor2_Speed ={	\
	.Kp = 100,				\
	.Ki = 20,				\
	.Kd = 10,				\
	.Target = 0,			\
	.Measure = 0,			\
	{0,0,0},				\
	.KpOut = 0,				\
	.KiOut = 0,				\
	.KdOut = 0,				\
	.PID_Out = 0,			\
	.PID_Limit_MAX = 9999,	\
	.Ki_Limit_MAX = 9999,	\
};
//ת�򻷲����޸�			60cm/s	6	140
pid_t pid_Turn ={			\
	.Kp = 5,				\
	.Ki = 0,				\
	.Kd = 20,				\
	.Target = 0,			\
	.Measure = 0,			\
	{0,0,0},				\
	.KpOut = 0,				\
	.KiOut = 0,				\
	.KdOut = 0,				\
	.PID_Out = 0,			\
	.PID_Limit_MAX = 9999,	\
	.Ki_Limit_MAX = 9999,	\
};
//���뻷�����޸�
pid_t pid_Distance ={		\
	.Kp = 10,				\
	.Ki = 0,				\
	.Kd = 5,				\
	.Target = 0,			\
	.Measure = 0,			\
	{0,0,0},				\
	.KpOut = 0,				\
	.KiOut = 0,				\
	.KdOut = 0,				\
	.PID_Out = 0,			\
	.PID_Limit_MAX = 9999,	\
	.Ki_Limit_MAX = 9999,	\
};
//���ٶȻ������޸�
pid_t pid_Gyro ={			\
	.Kp = 0.09,				\
	.Ki = 0.015,				\
	.Kd = 0,				\
	.Target = 0,			\
	.Measure = 0,			\
	{0,0,0},				\
	.KpOut = 0,				\
	.KiOut = 0,				\
	.KdOut = 0,				\
	.PID_Out = 0,			\
	.PID_Limit_MAX = 9999,	\
	.Ki_Limit_MAX = 9999,	\
};
//ת�򻷲����޸�
pid_t pid_Angle ={			\
	.Kp = 5,				\
	.Ki = 0,				\
	.Kd = 30,				\
	.Target = 0,			\
	.Measure = 0,			\
	{0,0,0},				\
	.KpOut = 0,				\
	.KiOut = 0,				\
	.KdOut = 0,				\
	.PID_Out = 0,			\
	.PID_Limit_MAX = 1000,	\
	.Ki_Limit_MAX = 1000,	\
};

//PWM�޷����� *a����Ҫ�޷��Ĳ���  ABS_MAX�޷���С
void PID_Limit(float *a, float ABS_MAX){
  if (*a > ABS_MAX)
    *a = ABS_MAX;
  if (*a < -ABS_MAX)
    *a = -ABS_MAX;
}
/*************************************************************************************************
*	�� �� ��:	PID_Calculate
*
*	��������:	PID���㹫ʽ
*
*   ��    ����  PID�ṹ�壬��ǰֵ��Ŀ��ֵ
*************************************************************************************************/
float PID_Calculate(pid_t *pid,float Measure,  float Target)
{
	pid->Target = Target;//��ֵĿ��ֵ
	pid->Measure = Measure;//��ֵ����ֵ
	pid->Error[0] = Target - Measure;//���㵱ǰ��� 

    pid->KpOut = pid->Kp * (pid->Error[0]);//�������㣬Kp*��ǰ���
    pid->KiOut += pid->Ki * pid->Error[0];//���ּ��㣬Ki*����ۼ�ֵ
	PID_Limit(&(pid->KiOut), pid->Ki_Limit_MAX);//�����޷�
    pid->KdOut = pid->Kd * (pid->Error[0] - pid->Error[1]);//΢�ּ��㣬Kd*(��ǰ���-��һ�����)

    pid->PID_Out = pid->KpOut + pid->KiOut + pid->KdOut;//PID����
    PID_Limit(&(pid->PID_Out), pid->PID_Limit_MAX);//PID����޷�

	pid->Error[2] = pid->Error[1];
	pid->Error[1] = pid->Error[0];

    return pid->PID_Out;
}
void Set_PID_Param(pid_t *pid, float P, float I,float D)
{
	pid->Kp = P;pid->Ki = I;pid->Kd = D;
	pid->KpOut = 0;pid->KiOut = 0;pid->KdOut = 0;pid->PID_Out = 0;//�������
}
//PWM�޷����� *a����Ҫ�޷��Ĳ���  ABS_MAX�޷���С
void PWM_Limit(int *a, int ABS_MAX){
  if (*a > ABS_MAX)
    *a = ABS_MAX;
  if (*a < -ABS_MAX)
    *a = -ABS_MAX;
}
// ���õ��1��PWM
void Set_Motor1_PWM(int Target_PWM){
	PWM_Limit(&Target_PWM,999);
	DL_TimerA_setCaptureCompareValue(PWM_0_INST,Target_PWM,GPIO_PWM_0_C0_IDX);
}
// ���õ��2��PWM
void Set_Motor2_PWM(int Target_PWM){
	PWM_Limit(&Target_PWM,999);
	DL_TimerA_setCaptureCompareValue(PWM_0_INST,Target_PWM,GPIO_PWM_0_C1_IDX);
}
void Set_Motor_PWM(int Target_PWM2,int Target_PWM1){
	if(Target_PWM1>0){
		DL_GPIO_setPins(GPIO_MOTOR_AIN1_PORT,GPIO_MOTOR_AIN1_PIN);
		DL_GPIO_clearPins(GPIO_MOTOR_AIN2_PORT,GPIO_MOTOR_AIN2_PIN);
	}
	else if(Target_PWM1<0){
		Target_PWM1=-Target_PWM1;
		DL_GPIO_setPins(GPIO_MOTOR_AIN2_PORT,GPIO_MOTOR_AIN2_PIN);
		DL_GPIO_clearPins(GPIO_MOTOR_AIN1_PORT,GPIO_MOTOR_AIN1_PIN);		
	}
	else{
		DL_GPIO_clearPins(GPIO_MOTOR_AIN1_PORT,GPIO_MOTOR_AIN1_PIN);
		DL_GPIO_clearPins(GPIO_MOTOR_AIN2_PORT,GPIO_MOTOR_AIN2_PIN);
	}
	Target_PWM1=-Target_PWM1+999;
	Set_Motor1_PWM(Target_PWM1);	
	
	if(Target_PWM2>0){
		DL_GPIO_setPins(GPIO_MOTOR_BIN1_PORT,GPIO_MOTOR_BIN1_PIN);
		DL_GPIO_clearPins(GPIO_MOTOR_BIN2_PORT,GPIO_MOTOR_BIN2_PIN);
	}
	else if(Target_PWM2<0){
		Target_PWM2=-Target_PWM2;
		DL_GPIO_setPins(GPIO_MOTOR_BIN2_PORT,GPIO_MOTOR_BIN2_PIN);
		DL_GPIO_clearPins(GPIO_MOTOR_BIN1_PORT,GPIO_MOTOR_BIN1_PIN);			
	}
	else{
		DL_GPIO_clearPins(GPIO_MOTOR_BIN1_PORT,GPIO_MOTOR_BIN1_PIN);
		DL_GPIO_clearPins(GPIO_MOTOR_BIN2_PORT,GPIO_MOTOR_BIN2_PIN);		
	}
	Target_PWM2=-Target_PWM2+999;
	Set_Motor2_PWM(Target_PWM2);
}
