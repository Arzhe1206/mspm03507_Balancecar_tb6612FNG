#include "jy61p.h"

static uint8_t RxBuffer[11];       // ���ջ�������11�ֽڣ�
static volatile uint8_t RxState = 0; // ����״̬��0=�ȴ���ͷ��1=�ȴ�֡���ͣ�2=�������ݣ�
static uint8_t RxIndex = 0;        // ��ǰ���յ��ֽ�����

// �����������
float wx, wy, wz;   // ���ٶȣ���λ����/s��
float Roll, Pitch, Yaw; // �Ƕȣ���λ���ȣ�

/**
 * @brief       ���ղ��������ٶȻ�Ƕ�����
 * @param       RxData: ���ڽ��յ�1�ֽ�����
 * @retval      ��
 */
void jy61p_ReceiveData(uint8_t RxData)
{
    uint8_t i, sum = 0;

    if (RxState == 0)       // ״̬0���ȴ���ͷ 0x55
    {
        if (RxData == 0x55) // ��⵽��ͷ
        {
            RxBuffer[RxIndex] = RxData;
            RxState = 1;     // ����״̬1���ȴ�֡���ͣ�
            RxIndex = 1;     // ��һ�ֽڴ���λ��
        }
    }
    else if (RxState == 1)  // ״̬1���ȴ�֡����
    {
        if (RxData == 0x52) // ���ٶ�֡��0x55 0x52��
        {
            RxBuffer[RxIndex] = RxData;
            RxState = 2;     // ����״̬2���������ݣ�
            RxIndex = 2;
        }
        else if (RxData == 0x53) // �Ƕ�֡��0x55 0x53��
        {
            RxBuffer[RxIndex] = RxData;
            RxState = 2;
            RxIndex = 2;
        }
        else // ��Ŀ��֡���ͣ�����״̬
        {
            RxState = 0;
            RxIndex = 0;
        }
    }
    else if (RxState == 2)  // ״̬2������ʣ������
    {
        RxBuffer[RxIndex++] = RxData;
        if (RxIndex == 11)  // �ѽ���11�ֽڣ�����֡��
        {
            // ����У��ͣ�ǰ10�ֽ��ۼӣ�
            for (i = 0; i < 10; i++)
                sum += RxBuffer[i];

            // У��ͨ��
            if (sum == RxBuffer[10])
            {
                // �������ٶȣ�0x52֡��
                if (RxBuffer[1] == 0x52)
                {
                    wx = ((int16_t)(RxBuffer[3] << 8 | RxBuffer[2])) / 32768.0f * 2000.0f; // ��λ: ��/s
                    wy = ((int16_t)(RxBuffer[5] << 8 | RxBuffer[4])) / 32768.0f * 2000.0f;
                    wz = ((int16_t)(RxBuffer[7] << 8 | RxBuffer[6])) / 32768.0f * 2000.0f;
                }
                // �����Ƕȣ�0x53֡��
                else if (RxBuffer[1] == 0x53)
                {
                    Roll = ((int16_t)(RxBuffer[3] << 8 | RxBuffer[2])) / 32768.0f * 180.0f; // ��λ: ��
                    Pitch = ((int16_t)(RxBuffer[5] << 8 | RxBuffer[4])) / 32768.0f * 180.0f;
                    Yaw = ((int16_t)(RxBuffer[7] << 8 | RxBuffer[6])) / 32768.0f * 180.0f;
                    
                    // Yaw��һ����[-180��, 180��]
                    if (Yaw > 180.0f) Yaw -= 360.0f;
                    else if (Yaw < -180.0f) Yaw += 360.0f;
                }
            }

            // ����״̬��׼��������һ֡
            RxState = 0;
            RxIndex = 0;
        }
    }
}
