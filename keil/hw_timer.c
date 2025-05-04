#include "hw_timer.h"
#include "hw_encoder.h"
#include "bsp_tb6612.h"
#include "control.h"

//#include "mpu6050.h"
void timer_init(void)
{
    //��ʱ���ж�
	NVIC_ClearPendingIRQ(TIMER_TICK_INST_INT_IRQN);
	NVIC_EnableIRQ(TIMER_TICK_INST_INT_IRQN);
}

void Read_Key(void)
{
	if( DL_GPIO_readPins(KEY_PIN_21_PORT, KEY_PIN_21_PIN) == 0 ){			
			
		
			DL_GPIO_togglePins(LED_PORT, LED_LED1_PIN);
			
					
		}			
}
//����������������
void TIMER_TICK_INST_IRQHandler(void)
{
	static uint16_t count_10ms=0;
	//10ms�����жϴ���
	if( DL_TimerA_getPendingInterrupt(TIMER_TICK_INST) == DL_TIMER_IIDX_ZERO )
	{
		count_10ms++;
		//����������
		encoder_update();
		//ƽ�����
		Control();
		if(count_10ms>=100)
		{
			//��֤ϵͳ�Ƿ�����
			DL_GPIO_togglePins(LED_PORT, LED_LED1_PIN);
			//��ȡ����
//			Read_Key();
			count_10ms=0;
		}

	}
	
	
}