#include "hw_timer.h"
#include "hw_encoder.h"
#include "bsp_tb6612.h"
#include "control.h"

//#include "mpu6050.h"
void timer_init(void)
{
    //定时器中断
	NVIC_ClearPendingIRQ(TIMER_TICK_INST_INT_IRQN);
	NVIC_EnableIRQ(TIMER_TICK_INST_INT_IRQN);
}

void Read_Key(void)
{
	if( DL_GPIO_readPins(KEY_PIN_21_PORT, KEY_PIN_21_PIN) == 0 ){			
			
		
			DL_GPIO_togglePins(LED_PORT, LED_LED1_PIN);
			
					
		}			
}
//电机编码器脉冲计数
void TIMER_TICK_INST_IRQHandler(void)
{
	static uint16_t count_10ms=0;
	//10ms归零中断触发
	if( DL_TimerA_getPendingInterrupt(TIMER_TICK_INST) == DL_TIMER_IIDX_ZERO )
	{
		count_10ms++;
		//编码器更新
		encoder_update();
		//平衡控制
		Control();
		if(count_10ms>=100)
		{
			//验证系统是否正常
			DL_GPIO_togglePins(LED_PORT, LED_LED1_PIN);
			//读取按键
//			Read_Key();
			count_10ms=0;
		}

	}
	
	
}