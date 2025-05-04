#include "control.h"
#include "hw_encoder.h"
#include "bsp_tb6612.h"
#include "jy61p.h"

long long Encoder_Left,Encoder_Right;
float pitch3,roll3,yaw3;
short gyrox,gyroy,gyroz;
short	aacx,aacy,aacz;

//闭环控制中间变量
int Vertical_out,Velocity_out,Turn_out,Target_Speed,Target_turn,MOTO1,MOTO2;
//平衡时角度值偏移量（机械中值）
float Med_Angle=3.5;
//参数
float Vertical_Kp=-38.5,Vertical_Kd=-1.01;			//直立环 数量级（Kp：0~1000、Kd：0~10）-38.5  -1.01
float Velocity_Kp=-0.5,Velocity_Ki;		//速度环 数量级（Kp：0~10）
float Turn_Kp=0,Turn_Kd=0;											//转向环

uint8_t stop=0;


#define SPEED_Y 30 //俯仰(前后)最大设定速度
#define SPEED_Z 150//偏航(左右)最大设定速度 

//直立环PD控制器
//输入：期望角度、真实角度、角速度
int Vertical(float Med,float Angle,float gyro_Y)
{
	int temp;
	temp=Vertical_Kp*(Angle-Med)+Vertical_Kd*gyro_Y;
	return temp;
}

// 速度环PI控制器
int Velocity(int Target,int encoder_L,int encoder_R)
{
	static int Err_LowOut_last,Encoder_S;
	static float a=0.7;
	int Err,Err_LowOut,temp;
	Velocity_Ki=Velocity_Kp/200;
	//1、计算偏差值
	Err=(encoder_L+encoder_R)-Target;
	//2、低通滤波
	Err_LowOut=(1-a)*Err+a*Err_LowOut_last;
	Err_LowOut_last=Err_LowOut;
	//3、积分
	Encoder_S+=Err_LowOut;
	//4、积分限幅(-20000~20000)
	Encoder_S=Encoder_S>20000?20000:(Encoder_S<(-20000)?(-20000):Encoder_S);
	if(stop==1)Encoder_S=0,stop=0;
	//5、速度环计算
	temp=Velocity_Kp*Err_LowOut+Velocity_Ki*Encoder_S;
	return temp;
}

//转向环PD控制器
//输入：角速度、角度值
int Turn(float gyro_Z,int Target_turn)
{
	int temp;
	temp=Turn_Kp*Target_turn+Turn_Kd*gyro_Z;
	return temp;
}

void Control(void)	//每隔10ms调用一次
{
	int PWM_out;
	//1、读取编码器和陀螺仪的数据
	Encoder_Left=get_encoder_count();
	Encoder_Right=get_encoder_count2();
	encoder_reset();
	gyroy=wx;
	roll3=Roll;
	
	//2、将数据传入PID控制器，计算输出结果，即左右电机转速值
	Velocity_out=Velocity(Target_Speed,Encoder_Left,Encoder_Right);
	Vertical_out=Vertical(-Velocity_out+Med_Angle,roll3,gyroy);//
	Turn_out=Turn(gyroz,Target_turn);
	PWM_out=Vertical_out;
//	MOTO1=PWM_out-Turn_out;
//	MOTO2=PWM_out+Turn_out;
	MOTO1=PWM_out;
	MOTO2=PWM_out;
	Set_Motor_PWM(MOTO1,MOTO2);
//	Set_Motor_PWM(500,500);
}

//	//遥控
//	if((Fore==0)&&(Back==0))Target_Speed=0;//未接受到前进后退指令-->速度清零，稳在原地
//	if(Fore==1)
//	{
//		if(distance<50)
//			Target_Speed--;
//		else
//			Target_Speed++;
//	}
//	if(Back==1){Target_Speed--;}//
//	Target_Speed=Target_Speed>SPEED_Y?SPEED_Y:(Target_Speed<-SPEED_Y?(-SPEED_Y):Target_Speed);//限幅
//	
//	/*左右*/
//	if((Left==0)&&(Right==0))Target_turn=0;
//	if(Left==1)Target_turn-=30;	//左转
//	if(Right==1)Target_turn+=30;	//右转
//	Target_turn=Target_turn>SPEED_Z?SPEED_Z:(Target_turn<-SPEED_Z?(-SPEED_Z):Target_turn);//限幅( (20*100) * 100   )
//	
//	/*转向约束*/
//	if((Left==0)&&(Right==0))Turn_Kd=0.6;//若无左右转向指令，则开启转向约束
//	else if((Left==1)||(Right==1))Turn_Kd=0;//若左右转向指令接收到，则去掉转向约束