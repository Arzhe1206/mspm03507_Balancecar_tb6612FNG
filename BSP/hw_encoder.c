#include "hw_encoder.h"

static ENCODER_RES motor_encoder ; 
//��������ʼ��
void encoder_init(void)
{
	//�����������ⲿ�ж�
	NVIC_ClearPendingIRQ(GPIO_ENCODER_INT_IRQN);
	NVIC_EnableIRQ(GPIO_ENCODER_INT_IRQN);
}

//��ȡ��������ֵ
int get_encoder_count(void)
{
	return motor_encoder.count;

}
int get_encoder_count2(void)
{	
	return motor_encoder.count2;
}
//��ȡ�������ķ���
ENCODER_DIR get_encoder_dir(void)
{
	return motor_encoder.dir;
}

//����һ��ʱ�����
void encoder_update(void)
{
	motor_encoder.count = motor_encoder.temp_count;
	motor_encoder.count2 = motor_encoder.temp_count2;

}

//���������ݸ���
void encoder_reset(void){
    motor_encoder.temp_count = 0;
    motor_encoder.temp_count2 = 0;
}

//�ⲿ�жϴ�����
void GROUP1_IRQHandler(void)
{
	uint32_t gpio_status;

	//��ȡ�ж��ź����
	gpio_status = DL_GPIO_getEnabledInterruptStatus(GPIO_ENCODER_PORT, GPIO_ENCODER_PIN_A_PIN | GPIO_ENCODER_PIN_B_PIN | GPIO_ENCODER_PIN_C_PIN | GPIO_ENCODER_PIN_D_PIN);
	//������A�������ش���
	if((gpio_status & GPIO_ENCODER_PIN_A_PIN) == GPIO_ENCODER_PIN_A_PIN)
	{
		//�����A���������£�B��Ϊ�͵�ƽ
		if(!DL_GPIO_readPins(GPIO_ENCODER_PORT,GPIO_ENCODER_PIN_B_PIN))
		{
			motor_encoder.temp_count--;
		}
		else
		{
			motor_encoder.temp_count++;
		}
	}//������B�������ش���
	else if((gpio_status & GPIO_ENCODER_PIN_B_PIN)==GPIO_ENCODER_PIN_B_PIN)
	{
		//�����B���������£�A��Ϊ�͵�ƽ
		if(!DL_GPIO_readPins(GPIO_ENCODER_PORT,GPIO_ENCODER_PIN_A_PIN))
		{
			motor_encoder.temp_count++;
		}
		else
		{
			motor_encoder.temp_count--;
		}
	}
	else if((gpio_status & GPIO_ENCODER_PIN_C_PIN)==GPIO_ENCODER_PIN_C_PIN)
	{
		//�����C���������£�D��Ϊ�͵�ƽ
		if(!DL_GPIO_readPins(GPIO_ENCODER_PORT,GPIO_ENCODER_PIN_D_PIN))
		{
			motor_encoder.temp_count2--;
		}
		else
		{
			motor_encoder.temp_count2++;
		}
	}
  else if((gpio_status & GPIO_ENCODER_PIN_D_PIN)==GPIO_ENCODER_PIN_D_PIN)
	{
		//�����D���������£�C��Ϊ�͵�ƽ
		if(!DL_GPIO_readPins(GPIO_ENCODER_PORT,GPIO_ENCODER_PIN_C_PIN))
		{
			motor_encoder.temp_count2++;
		}
		else
		{
			motor_encoder.temp_count2--;
		}
	}
	//���״̬
	DL_GPIO_clearInterruptStatus(GPIO_ENCODER_PORT,GPIO_ENCODER_PIN_A_PIN|GPIO_ENCODER_PIN_B_PIN | GPIO_ENCODER_PIN_C_PIN | GPIO_ENCODER_PIN_D_PIN);
}
