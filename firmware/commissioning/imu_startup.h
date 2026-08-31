/*
 * MSPM0G3507 Balance Car
 * 作者：数显纯牛奶，开源工程未经授权禁止商业用途，需要实物见店铺主页：https://m.tb.cn/h.8nzqEuN?tk=kJqnT4uWzfY
 * 官方仓库：https://github.com/Arzhe1206/mspm03507_Balancecar_tb6612FNG
 * 许可：LicenseRef-BalanceCar-NonCommercial
 * 项目标识：MSPM0-BC-ARZHE1206
 * AI/ML 使用声明：未经授权，不得用于模型训练、数据集构建或商业化自动代码生成。
 */
#ifndef IMU_STARTUP_H
#define IMU_STARTUP_H

#include "mpu6050.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    IMU_STARTUP_DMP_LOADING = 0,
    IMU_STARTUP_STABILIZING,
    IMU_STARTUP_ZERO_SAMPLING,
    IMU_STARTUP_ZERO_READY,
    IMU_STARTUP_ARM_CONFIRM,
    IMU_STARTUP_RUNNING,
    IMU_STARTUP_ERROR
} ImuStartupState;

typedef enum {
    IMU_STARTUP_EVENT_NONE = 0,
    IMU_STARTUP_EVENT_ZERO_APPLIED = 1,
    IMU_STARTUP_EVENT_ARM_READY = 2
} ImuStartupEvent;

void ImuStartup_Init(void);
void ImuStartup_DmpReady(bool success);
void ImuStartup_ProcessSample(const MPU6050_Sample *sample, bool clean_frame);
void ImuStartup_InvalidateFrame(void);
void ImuStartup_Update(uint32_t now_ms);
void ImuStartup_HandleButton(void);
void ImuStartup_AbortToStabilizing(void);
uint8_t ImuStartup_ConsumeEvents(void);

ImuStartupState ImuStartup_GetState(void);
bool ImuStartup_IsRunning(void);
void ImuStartup_GetAngles(float *pitch_deg, float *roll_deg, float *yaw_deg);
uint32_t ImuStartup_GetStableElapsedMs(void);
uint16_t ImuStartup_GetZeroSampleCount(void);
uint32_t ImuStartup_GetAutoZeroRemainingMs(uint32_t now_ms);

#endif
