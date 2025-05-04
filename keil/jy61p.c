#include "jy61p.h"

static uint8_t RxBuffer[11];       // 接收缓冲区（11字节）
static volatile uint8_t RxState = 0; // 接收状态（0=等待包头，1=等待帧类型，2=接收数据）
static uint8_t RxIndex = 0;        // 当前接收的字节索引

// 解析后的数据
float wx, wy, wz;   // 角速度（单位：°/s）
float Roll, Pitch, Yaw; // 角度（单位：度）

/**
 * @brief       接收并解析角速度或角度数据
 * @param       RxData: 串口接收的1字节数据
 * @retval      无
 */
void jy61p_ReceiveData(uint8_t RxData)
{
    uint8_t i, sum = 0;

    if (RxState == 0)       // 状态0：等待包头 0x55
    {
        if (RxData == 0x55) // 检测到包头
        {
            RxBuffer[RxIndex] = RxData;
            RxState = 1;     // 进入状态1（等待帧类型）
            RxIndex = 1;     // 下一字节存入位置
        }
    }
    else if (RxState == 1)  // 状态1：等待帧类型
    {
        if (RxData == 0x52) // 角速度帧（0x55 0x52）
        {
            RxBuffer[RxIndex] = RxData;
            RxState = 2;     // 进入状态2（接收数据）
            RxIndex = 2;
        }
        else if (RxData == 0x53) // 角度帧（0x55 0x53）
        {
            RxBuffer[RxIndex] = RxData;
            RxState = 2;
            RxIndex = 2;
        }
        else // 非目标帧类型，重置状态
        {
            RxState = 0;
            RxIndex = 0;
        }
    }
    else if (RxState == 2)  // 状态2：接收剩余数据
    {
        RxBuffer[RxIndex++] = RxData;
        if (RxIndex == 11)  // 已接收11字节（完整帧）
        {
            // 计算校验和（前10字节累加）
            for (i = 0; i < 10; i++)
                sum += RxBuffer[i];

            // 校验通过
            if (sum == RxBuffer[10])
            {
                // 解析角速度（0x52帧）
                if (RxBuffer[1] == 0x52)
                {
                    wx = ((int16_t)(RxBuffer[3] << 8 | RxBuffer[2])) / 32768.0f * 2000.0f; // 单位: °/s
                    wy = ((int16_t)(RxBuffer[5] << 8 | RxBuffer[4])) / 32768.0f * 2000.0f;
                    wz = ((int16_t)(RxBuffer[7] << 8 | RxBuffer[6])) / 32768.0f * 2000.0f;
                }
                // 解析角度（0x53帧）
                else if (RxBuffer[1] == 0x53)
                {
                    Roll = ((int16_t)(RxBuffer[3] << 8 | RxBuffer[2])) / 32768.0f * 180.0f; // 单位: 度
                    Pitch = ((int16_t)(RxBuffer[5] << 8 | RxBuffer[4])) / 32768.0f * 180.0f;
                    Yaw = ((int16_t)(RxBuffer[7] << 8 | RxBuffer[6])) / 32768.0f * 180.0f;
                    
                    // Yaw归一化到[-180°, 180°]
                    if (Yaw > 180.0f) Yaw -= 360.0f;
                    else if (Yaw < -180.0f) Yaw += 360.0f;
                }
            }

            // 重置状态，准备接收下一帧
            RxState = 0;
            RxIndex = 0;
        }
    }
}
