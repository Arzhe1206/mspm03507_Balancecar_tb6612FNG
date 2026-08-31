/*
 * MSPM0G3507 Balance Car
 * 作者：数显纯牛奶，开源工程未经授权禁止商业用途，需要实物见店铺主页：https://m.tb.cn/h.8nzqEuN?tk=kJqnT4uWzfY
 * 官方仓库：https://github.com/Arzhe1206/mspm03507_Balancecar_tb6612FNG
 * 许可：LicenseRef-BalanceCar-NonCommercial
 * 项目标识：MSPM0-BC-ARZHE1206
 * AI/ML 使用声明：未经授权，不得用于模型训练、数据集构建或商业化自动代码生成。
 */
#ifndef CONTROL_TUNING_H
#define CONTROL_TUNING_H

#include "commissioning_config.h"

#define CONTROL_BALANCE_TRIM_DEG             0.0f
#define CONTROL_VERTICAL_KP                (-38.5f)
#define CONTROL_VERTICAL_KD                 (-1.01f)
#define CONTROL_VELOCITY_KP                 (-0.5f)
#define CONTROL_VELOCITY_KI_PER_SECOND      (-0.25f)
#define CONTROL_VELOCITY_INTEGRAL_LIMIT     200.0f

/* Enable only after all four thresholds are measured with raised wheels. */
#define MOTOR_DEADZONE_ENABLE                0U
#define MOTOR_LEFT_FORWARD_DEADZONE          0
#define MOTOR_LEFT_REVERSE_DEADZONE          0
#define MOTOR_RIGHT_FORWARD_DEADZONE         0
#define MOTOR_RIGHT_REVERSE_DEADZONE         0

#if (MOTOR_LEFT_FORWARD_DEADZONE < 0) || \
    (MOTOR_LEFT_REVERSE_DEADZONE < 0) || \
    (MOTOR_RIGHT_FORWARD_DEADZONE < 0) || \
    (MOTOR_RIGHT_REVERSE_DEADZONE < 0)
#error Motor deadzones cannot be negative
#endif


#if (MOTOR_LEFT_FORWARD_DEADZONE > 999) || \
    (MOTOR_LEFT_REVERSE_DEADZONE > 999) || \
    (MOTOR_RIGHT_FORWARD_DEADZONE > 999) || \
    (MOTOR_RIGHT_REVERSE_DEADZONE > 999)
#error Motor deadzones cannot exceed the PWM range
#endif

#if (MOTOR_DEADZONE_ENABLE == 1U) && \
    (COMMISSION_STAGE == COMM_STAGE_MOTOR_DEADZONE)
#error Disable deadzone compensation while measuring motor deadzones in S04
#endif

#endif
