#include "board.h"
#include <stdio.h>
#include "oled.h"
#include "hw_encoder.h"
#include "hw_timer.h"
#include "bsp_tb6612.h"
//#include "mpu6050.h"
#include "jy61p.h"
#include "control.h"

int i = 0;
void ui_encoder_value_update(void);//编码器的数据显示
float a,b,c;
uint8_t OledString[128];

int main(void)
{
    //开发板初始化
    board_init();
	  Set_Motor_PWM(0,0);	
    OLED_Init();
	  OLED_Clear();
    encoder_init();
		timer_init();
		delay_ms(500);
//		mpu6050_init(); 
		while(1)
		{
			
//			delay_ms(10);	
		  ui_encoder_value_update();
			a=Velocity_out;
			c=Vertical_out;
		sprintf((char *)OledString,"x:%.2f  \r\n",a);//显示6050数据  航向角
		OLED_ShowString(0,0,OledString,12,1);		
		sprintf((char *)OledString,"x:%.2f  \r\n",c);//显示6050数据  航向角
		OLED_ShowString(0,24,OledString,12,1);				
//		a=mpu6050.Accel_Average[0];
//		b=mpu6050.Accel_Average[1];
//		c=mpu6050.Accel_Average[2];		
//			
//		a=mpu6050.Gyro_Average[0];
//		b=mpu6050.Gyro_Average[1];
//		c=mpu6050.Gyro_Average[2];	
		
//			sprintf((char*)OledString, "Encoder1:%d", get_encoder_count() );
//			OLED_ShowString(0,36,OledString,12,1);
//			sprintf((char*)OledString, "Encoder2:%d", get_encoder_count2() );
//			OLED_ShowString(0,48,OledString,12,1);
			
//		a=Pitch;
		b=Roll;
//		c=Yaw;
//		a=wx;
//		b=wy;
//		c=wz;
//		sprintf((char *)OledString,"x:%.2f  \r\n",a);//显示6050数据  航向角
//		OLED_ShowString(0,0,OledString,12,1);				
		sprintf((char *)OledString,"y:%.2f  \r\n",b);//显示6050数据  航向角
		OLED_ShowString(0,12,OledString,12,1);				
//		sprintf((char *)OledString,"z:%.2f  \r\n",c);//显示6050数据  航向角
//		OLED_ShowString(0,24,OledString,12,1);			           
		OLED_Refresh();

    }
}

void ui_encoder_value_update(void)
{
		static int last_encoder_count = 255;
		static int last_encoder_count2 = 255;	
		static int last_encoder_dir = 255;
		uint8_t disp_buff[50]={0};
		
		if( last_encoder_count != get_encoder_count() )
		{
			last_encoder_count = get_encoder_count();
			sprintf((char*)disp_buff, "Encoder1:%d", get_encoder_count() );
			OLED_ShowString(0,36,disp_buff,12,1);

		}
		if( last_encoder_count2 != get_encoder_count2() )
		{
			last_encoder_count2 = get_encoder_count2();
			sprintf((char*)disp_buff, "Encoder2:%d", get_encoder_count2() );
			OLED_ShowString(0,48,disp_buff,12,1);

		}


}
//	a=mpu6050.Accel_Offset[0];
//	b=mpu6050.Accel_Offset[1];
//	c=mpu6050.Accel_Offset[2];

//	a=mpu6050.Gyro_Offset[0];
//	b=mpu6050.Gyro_Offset[1];
//	c=mpu6050.Gyro_Offset[2];
//			
//	a=mpu6050.Gyro_Calulate[0];
//	b=mpu6050.Gyro_Calulate[1];
//	c=mpu6050.Gyro_Calulate[2];

//	a=mpu6050.Accel_Calulate[0];
//	b=mpu6050.Accel_Calulate[1];
//	c=mpu6050.Accel_Calulate[2];

//            for(int i = 100; i <= 900; i+=100)
//            {
//									
//                  Set_Motor_PWM(i,i);
//                  delay_ms(200);
//            }
//            for(int j = 900; j >= 100; j-=100)
//            {
//									
//                  Set_Motor_PWM(i,i);
//                  delay_ms(200);
//            }

//		if( last_encoder_dir != get_encoder_dir()  )
//		{
//			last_encoder_dir = get_encoder_dir();
//			sprintf((char*)disp_buff, "%s", (get_encoder_dir() == FORWARD) ? "FORWARD" : "REVERSAL" );
//			OLED_ShowString(0,0,disp_buff,16,1);
//		}

//	q=Motor1_Encoder_Value;
//	p=Motor2_Encoder_Value;

//	sprintf((char *)OledString,"a:%d  \r\n",p);//显示6050数据  航向角
//	OLED_ShowString(0,0,OledString,12,1);				
//	sprintf((char *)OledString,"b:%d  \r\n",q);//显示6050数据  航向角
//	OLED_ShowString(0,12,OledString,12,1);








