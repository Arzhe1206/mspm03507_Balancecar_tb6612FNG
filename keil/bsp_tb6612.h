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

#ifndef _BSP_TB6612_H
#define _BSP_TB6612_H

#include "board.h"
#include "ti_msp_dl_config.h"
/*-------------------------------------------------------------------------------------------*/
/*---------------------------------------PID����---------------------------------------------*/
/*-------------------------------------------------------------------------------------------*/
/*PID�ṹ�岿��*/
/*PID����Ȩ��*/
/*Ŀ��ֵ����ȡֵ���������*/
/*���������֡�΢������������*/
/*����޷��Լ������޷�*/	
typedef struct pid_t
{
  float Kp;					//P����
  float Ki;					//I����
  float Kd;					//D����
		
  float Target;				//Ŀ��ֵ
  float Measure;			//��ǰ��ȡֵ
  float Error[3];			//�������
		
  float KpOut;				//�������
  float KiOut;				//�������
  float KdOut;				//΢�����
  float PID_Out;			//PID�����
		
  uint32_t PID_Limit_MAX; 	//�������޷�
  uint32_t Ki_Limit_MAX;	//����������޷�
}pid_t;
float PID_Calculate(pid_t *pid,float Measure,  float Target);
void Set_PID_Param(pid_t *pid, float P, float I,float D);
/*-------------------------------------------------------------------------------------------*/
/*------------------------------------�����������-------------------------------------------*/
/*-------------------------------------------------------------------------------------------*/
#define MOTORS_ENABLE()			{MOTOR1_ENABLE_FLAG = 1;MOTOR2_ENABLE_FLAG = 1;}
#define MOTORS_DISABLE()		{MOTOR1_ENABLE_FLAG = 0;MOTOR2_ENABLE_FLAG = 0;}

#define AIN1(X)  ( (X) ? (DL_GPIO_setPins(GPIO_MOTOR_AIN1_PORT,GPIO_MOTOR_AIN1_PIN)) : (GPIO_MOTOR_AIN1_PORT,GPIO_MOTOR_AIN1_PIN))
#define AIN2(X)  ( (X) ? (DL_GPIO_setPins(GPIO_MOTOR_AIN2_PORT,GPIO_MOTOR_AIN2_PIN)) : (GPIO_MOTOR_AIN2_PORT,GPIO_MOTOR_AIN2_PIN))
#define BIN1(X)  ( (X) ? (DL_GPIO_setPins(GPIO_MOTOR_BIN1_PORT,GPIO_MOTOR_BIN1_PIN)) : (GPIO_MOTOR_BIN1_PORT,GPIO_MOTOR_BIN1_PIN))
#define BIN2(X)  ( (X) ? (DL_GPIO_setPins(GPIO_MOTOR_BIN2_PORT,GPIO_MOTOR_BIN2_PIN)) : (GPIO_MOTOR_BIN2_PORT,GPIO_MOTOR_BIN2_PIN))

#define Motor1_Forward()	{AIN1(1);AIN2(0);}
#define Motor1_Backward()	{AIN1(0);AIN2(1);}
#define Motor1_Stop()		{AIN1(0);AIN2(0);}

#define Motor2_Forward()	{BIN1(0);BIN2(1);}
#define Motor2_Backward()	{BIN1(1);BIN2(0);}
#define Motor2_Stop()		{BIN1(0);BIN2(0);}

void Set_Motor_PWM(int Target_PWM1,int Target_PWM2);

#endif  /* _BSP_TB6612_H */
