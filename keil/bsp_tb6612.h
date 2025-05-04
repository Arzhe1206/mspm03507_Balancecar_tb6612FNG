/*
 * 立创开发板软硬件资料与相关扩展板软硬件资料官网全部开源
 * 开发板官网：www.lckfb.com
 * 技术支持常驻论坛，任何技术问题欢迎随时交流学习
 * 立创论坛：https://oshwhub.com/forum
 * 关注bilibili账号：【立创开发板】，掌握我们的最新动态！
 * 不靠卖板赚钱，以培养中国工程师为己任
 * Change Logs:
 * Date           Author       Notes
 * 2024-07-08     LCKFB-LP    first version
 */

#ifndef _BSP_TB6612_H
#define _BSP_TB6612_H

#include "board.h"
#include "ti_msp_dl_config.h"
/*-------------------------------------------------------------------------------------------*/
/*---------------------------------------PID函数---------------------------------------------*/
/*-------------------------------------------------------------------------------------------*/
/*PID结构体部分*/
/*PID三个权重*/
/*目标值、获取值、三个误差*/
/*比例、积分、微分输出、总输出*/
/*输出限幅以及积分限幅*/	
typedef struct pid_t
{
  float Kp;					//P参数
  float Ki;					//I参数
  float Kd;					//D参数
		
  float Target;				//目标值
  float Measure;			//当前获取值
  float Error[3];			//三次误差
		
  float KpOut;				//比例输出
  float KiOut;				//积分输出
  float KdOut;				//微分输出
  float PID_Out;			//PID总输出
		
  uint32_t PID_Limit_MAX; 	//最大输出限幅
  uint32_t Ki_Limit_MAX;	//最大积分输出限幅
}pid_t;
float PID_Calculate(pid_t *pid,float Measure,  float Target);
void Set_PID_Param(pid_t *pid, float P, float I,float D);
/*-------------------------------------------------------------------------------------------*/
/*------------------------------------电机操作函数-------------------------------------------*/
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
