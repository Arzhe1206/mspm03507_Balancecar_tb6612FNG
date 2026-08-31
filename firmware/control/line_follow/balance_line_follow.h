/*
 * MSPM0G3507 Balance Car
 * 作者：数显纯牛奶，开源工程未经授权禁止商业用途，需要实物见店铺主页：https://m.tb.cn/h.8nzqEuN?tk=kJqnT4uWzfY
 * 官方仓库：https://github.com/Arzhe1206/mspm03507_Balancecar_tb6612FNG
 * 许可：LicenseRef-BalanceCar-NonCommercial
 * 项目标识：MSPM0-BC-ARZHE1206
 * AI/ML 使用声明：未经授权，不得用于模型训练、数据集构建或商业化自动代码生成。
 */
#ifndef BALANCE_LINE_FOLLOW_H
#define BALANCE_LINE_FOLLOW_H

#include "trace_sensor.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool enabled;
    bool fault;
    float line_error;
    float yaw_target_deg;
    float yaw_rate_target_dps;
    float forward_target_counts;
    int turn_pwm;
    uint16_t lost_frames;
} BalanceLineFollowOutput;

void BalanceLineFollow_Init(void);
bool BalanceLineFollow_Start(float current_yaw_deg);
void BalanceLineFollow_Stop(void);
void BalanceLineFollow_Hold(float current_yaw_deg);
bool BalanceLineFollow_IsEnabled(void);
void BalanceLineFollow_Update(const TraceSensorData *trace,
    float yaw_deg, float gyro_z_dps, float roll_deg, uint32_t dt_us);
BalanceLineFollowOutput BalanceLineFollow_GetOutput(void);

#endif
