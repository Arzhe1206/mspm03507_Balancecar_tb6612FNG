#include "control.h"
#include "hw_encoder.h"
#include "bsp_tb6612.h"
#include "jy61p.h"

long long Encoder_Left,Encoder_Right;
float pitch3,roll3,yaw3;
short gyrox,gyroy,gyroz;
short	aacx,aacy,aacz;

//�ջ������м����
int Vertical_out,Velocity_out,Turn_out,Target_Speed,Target_turn,MOTO1,MOTO2;
//ƽ��ʱ�Ƕ�ֵƫ��������е��ֵ��
float Med_Angle=3.5;
//����
float Vertical_Kp=-38.5,Vertical_Kd=-1.01;			//ֱ���� ��������Kp��0~1000��Kd��0~10��-38.5  -1.01
float Velocity_Kp=-0.5,Velocity_Ki;		//�ٶȻ� ��������Kp��0~10��
float Turn_Kp=0,Turn_Kd=0;											//ת��

uint8_t stop=0;


#define SPEED_Y 30 //����(ǰ��)����趨�ٶ�
#define SPEED_Z 150//ƫ��(����)����趨�ٶ� 

//ֱ����PD������
//���룺�����Ƕȡ���ʵ�Ƕȡ����ٶ�
int Vertical(float Med,float Angle,float gyro_Y)
{
	int temp;
	temp=Vertical_Kp*(Angle-Med)+Vertical_Kd*gyro_Y;
	return temp;
}

// �ٶȻ�PI������
int Velocity(int Target,int encoder_L,int encoder_R)
{
	static int Err_LowOut_last,Encoder_S;
	static float a=0.7;
	int Err,Err_LowOut,temp;
	Velocity_Ki=Velocity_Kp/200;
	//1������ƫ��ֵ
	Err=(encoder_L+encoder_R)-Target;
	//2����ͨ�˲�
	Err_LowOut=(1-a)*Err+a*Err_LowOut_last;
	Err_LowOut_last=Err_LowOut;
	//3������
	Encoder_S+=Err_LowOut;
	//4�������޷�(-20000~20000)
	Encoder_S=Encoder_S>20000?20000:(Encoder_S<(-20000)?(-20000):Encoder_S);
	if(stop==1)Encoder_S=0,stop=0;
	//5���ٶȻ�����
	temp=Velocity_Kp*Err_LowOut+Velocity_Ki*Encoder_S;
	return temp;
}

//ת��PD������
//���룺���ٶȡ��Ƕ�ֵ
int Turn(float gyro_Z,int Target_turn)
{
	int temp;
	temp=Turn_Kp*Target_turn+Turn_Kd*gyro_Z;
	return temp;
}

void Control(void)	//ÿ��10ms����һ��
{
	int PWM_out;
	//1����ȡ�������������ǵ�����
	Encoder_Left=get_encoder_count();
	Encoder_Right=get_encoder_count2();
	encoder_reset();
	gyroy=wx;
	roll3=Roll;
	
	//2�������ݴ���PID�������������������������ҵ��ת��ֵ
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

//	//ң��
//	if((Fore==0)&&(Back==0))Target_Speed=0;//δ���ܵ�ǰ������ָ��-->�ٶ����㣬����ԭ��
//	if(Fore==1)
//	{
//		if(distance<50)
//			Target_Speed--;
//		else
//			Target_Speed++;
//	}
//	if(Back==1){Target_Speed--;}//
//	Target_Speed=Target_Speed>SPEED_Y?SPEED_Y:(Target_Speed<-SPEED_Y?(-SPEED_Y):Target_Speed);//�޷�
//	
//	/*����*/
//	if((Left==0)&&(Right==0))Target_turn=0;
//	if(Left==1)Target_turn-=30;	//��ת
//	if(Right==1)Target_turn+=30;	//��ת
//	Target_turn=Target_turn>SPEED_Z?SPEED_Z:(Target_turn<-SPEED_Z?(-SPEED_Z):Target_turn);//�޷�( (20*100) * 100   )
//	
//	/*ת��Լ��*/
//	if((Left==0)&&(Right==0))Turn_Kd=0.6;//��������ת��ָ�����ת��Լ��
//	else if((Left==1)||(Right==1))Turn_Kd=0;//������ת��ָ����յ�����ȥ��ת��Լ��