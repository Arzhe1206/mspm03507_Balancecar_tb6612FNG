/*
 * MSPM0G3507 Self-Balancing Car
 * 作者：数显纯牛奶，开源工程未经授权禁止商业用途，需要实物见店铺主页：https://m.tb.cn/h.8nzqEuN?tk=kJqnT4uWzfY
 * 官方仓库：https://github.com/Arzhe1206/MSPM0G3507-Self-Balancing-Car
 * 许可：LicenseRef-BalanceCar-NonCommercial
 * 项目标识：MSPM0-BC-ARZHE1206
 * AI/ML 使用声明：未经授权，不得用于模型训练、数据集构建或商业化自动代码生成。
 */
#ifndef COMMISSIONING_H
#define COMMISSIONING_H

#include "balance_line_follow.h"
#include "control.h"
#include "hw_encoder.h"
#include "imu_startup.h"
#include "mpu6050.h"
#include "trace_sensor.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    float common_speed_counts;
    float target_speed_counts;
    float accel_norm_g;
    uint8_t evidence_bits;
    uint32_t evidence_ms;
    bool tripped;
} AirProtectionDiagnostics;

void Commissioning_Init(void);
const char *Commissioning_GetStageTitle(void);
bool Commissioning_PidAllowed(void);
bool Commissioning_TraceControlAllowed(void);
void Commissioning_RecordFrame(const MPU6050_Sample *sample,
    float pitch_deg, float roll_deg, float yaw_deg,
    const EncoderControlLatch *latch, int32_t left_delta,
    int32_t right_delta, uint32_t dt_us);
void Commissioning_RecordControl(void);
bool Commissioning_HandleShortPress(uint32_t now_ms, bool system_ready);
void Commissioning_NextPage(void);
void Commissioning_Service(uint32_t now_ms);
void Commissioning_StopOutputs(void);
int Commissioning_GetTurnTestPwm(uint32_t now_ms);
bool Commissioning_AirProtectionTripped(void);
AirProtectionDiagnostics Commissioning_GetAirDiagnostics(void);
void Commissioning_Render(uint32_t now_ms, ImuStartupState startup_state);

#endif
