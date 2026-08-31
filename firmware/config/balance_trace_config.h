/*
 * MSPM0G3507 Balance Car
 * 作者：数显纯牛奶，开源工程未经授权禁止商业用途，需要实物见店铺主页：https://m.tb.cn/h.8nzqEuN?tk=kJqnT4uWzfY
 * 官方仓库：https://github.com/Arzhe1206/mspm03507_Balancecar_tb6612FNG
 * 许可：LicenseRef-BalanceCar-NonCommercial
 * 项目标识：MSPM0-BC-ARZHE1206
 * AI/ML 使用声明：未经授权，不得用于模型训练、数据集构建或商业化自动代码生成。
 */
#ifndef BALANCE_TRACE_CONFIG_H
#define BALANCE_TRACE_CONFIG_H

#include "commissioning_config.h"

/* Set to 1 only after shadow data and motor direction are verified. */
#ifndef BALANCE_TRACE_ENABLE
#define BALANCE_TRACE_ENABLE                 0U
#endif

#define TRACE_ACTIVE_HIGH                    1U
#define TRACE_SENSOR_ORDER_REVERSED          0U
#define TRACE_FILTER_SAMPLES                 3U

#define TRACE_ARM_DELAY_MS                   5000U
#define TRACE_FORWARD_TARGET_COUNTS          4.0f
#define TRACE_LINE_YAW_RATE_KP               35.0f
#define TRACE_LINE_YAW_RATE_KD               0.08f
#define TRACE_LINE_D_FILTER_ALPHA            0.20f
#define TRACE_YAW_RATE_MAX_DPS               60.0f
#define TRACE_HEADING_LEAD_MAX_DEG           25.0f
#define TRACE_HEADING_KP                     3.0f
#define TRACE_YAW_RATE_KP                    1.0f
#define TRACE_STEER_SIGN                     1.0f
#define TRACE_TURN_PWM_MAX                   180
#define TRACE_TURN_PWM_SLEW                  25
#define TRACE_LOST_HOLD_FRAMES               10U
#define TRACE_LOST_FAULT_FRAMES              50U
#define TRACE_FULL_AUTHORITY_ROLL_DEG        8.0f
#define TRACE_ZERO_AUTHORITY_ROLL_DEG        20.0f

#if (TRACE_FILTER_SAMPLES == 0U) || ((TRACE_FILTER_SAMPLES % 2U) == 0U)
#error TRACE_FILTER_SAMPLES must be a non-zero odd number
#endif

#if (TRACE_LOST_HOLD_FRAMES >= TRACE_LOST_FAULT_FRAMES)
#error TRACE_LOST_HOLD_FRAMES must be less than TRACE_LOST_FAULT_FRAMES
#endif

#if (BALANCE_TRACE_ENABLE == 1U) && !COMM_STAGE_TRACE_CONTROL_ALLOWED
#error Trace control is allowed only in stage 11, stage 12, or release
#endif

#endif
