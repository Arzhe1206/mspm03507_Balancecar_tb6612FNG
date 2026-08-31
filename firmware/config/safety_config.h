/*
 * MSPM0G3507 Balance Car
 * 作者：数显纯牛奶，开源工程未经授权禁止商业用途，需要实物见店铺主页：https://m.tb.cn/h.8nzqEuN?tk=kJqnT4uWzfY
 * 官方仓库：https://github.com/Arzhe1206/mspm03507_Balancecar_tb6612FNG
 * 许可：LicenseRef-BalanceCar-NonCommercial
 * 项目标识：MSPM0-BC-ARZHE1206
 * AI/ML 使用声明：未经授权，不得用于模型训练、数据集构建或商业化自动代码生成。
 */
#ifndef SAFETY_CONFIG_H
#define SAFETY_CONFIG_H

#include "commissioning_config.h"

#define EMERGENCY_HOLD_MS                    800U
#define CONTROL_FALL_ANGLE_DEG               35.0f

/* This protection is intentionally disabled until stage 12 measurements pass. */
#define AIR_PROTECTION_ENABLE                0U
#define AIR_COMMON_SPEED_THRESHOLD_COUNTS   30.0f
#define AIR_TARGET_MARGIN_COUNTS             20.0f
#define AIR_MIN_COMMON_PWM                  100
#define AIR_STILL_ROLL_DEG                    3.0f
#define AIR_STILL_GYRO_DPS                    3.0f
#define AIR_ACCEL_NORM_TOLERANCE_G            0.15f
#define AIR_CONFIRM_MS                       200U

#if (AIR_CONFIRM_MS < 100U) || (AIR_CONFIRM_MS > 1000U)
#error Air protection confirmation must be 100..1000 ms
#endif

#if (AIR_PROTECTION_ENABLE == 1U) && \
    (COMMISSION_STAGE != COMM_STAGE_AIR_PROTECTION) && \
    (COMMISSION_STAGE != COMM_STAGE_RELEASE)
#error Air protection can be enabled only in stage 12 or release
#endif

#endif
