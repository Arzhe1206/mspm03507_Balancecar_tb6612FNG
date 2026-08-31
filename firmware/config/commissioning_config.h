/*
 * MSPM0G3507 Self-Balancing Car
 * 作者：数显纯牛奶，开源工程未经授权禁止商业用途，需要实物见店铺主页：https://m.tb.cn/h.8nzqEuN?tk=kJqnT4uWzfY
 * 官方仓库：https://github.com/Arzhe1206/MSPM0G3507-Self-Balancing-Car
 * 许可：LicenseRef-BalanceCar-NonCommercial
 * 项目标识：MSPM0-BC-ARZHE1206
 * AI/ML 使用声明：未经授权，不得用于模型训练、数据集构建或商业化自动代码生成。
 */
#ifndef COMMISSIONING_CONFIG_H
#define COMMISSIONING_CONFIG_H

/* Select exactly one stage, rebuild, and pass it before moving forward. */
#define COMM_STAGE_IMU_DIAGNOSTIC            1U
#define COMM_STAGE_TRACE_DIAGNOSTIC          2U
#define COMM_STAGE_ENCODER_MANUAL            3U
#define COMM_STAGE_MOTOR_DEADZONE            4U
#define COMM_STAGE_MOTOR_ENCODER             5U
#define COMM_STAGE_BALANCE_DIRECTION         6U
#define COMM_STAGE_MECHANICAL_ZERO           7U
#define COMM_STAGE_BALANCE_PD                8U
#define COMM_STAGE_SPEED_PI                  9U
#define COMM_STAGE_YAW_TURN                 10U
#define COMM_STAGE_TRACE_TUNING             11U
#define COMM_STAGE_AIR_PROTECTION           12U
#define COMM_STAGE_RELEASE                  13U

/* Safe delivery default: sensor-only, motors cannot arm. */
#ifndef COMMISSION_STAGE
#define COMMISSION_STAGE                    COMM_STAGE_IMU_DIAGNOSTIC
#endif

#define COMM_MOTOR_LEFT                      1U
#define COMM_MOTOR_RIGHT                     2U
#define COMM_MOTOR_BOTH                      3U
#define COMM_TEST_MOTOR_CHANNEL             COMM_MOTOR_LEFT
#define COMM_TEST_MOTOR_DIRECTION            1
#define COMM_TEST_MOTOR_PWM                 80
#define COMM_TEST_PULSE_MS                  200U
#define COMM_TEST_REST_MS                   500U
#define COMM_TEST_PWM_HARD_LIMIT            300
#define COMM_TURN_TEST_PWM                   60
#define COMM_TURN_TEST_MS                   200U

/* Stage power ceilings rise only after the preceding loop is qualified. */
#ifndef COMM_CONTROL_PWM_LIMIT
#if (COMMISSION_STAGE == COMM_STAGE_BALANCE_PD)
#define COMM_CONTROL_PWM_LIMIT              400
#elif (COMMISSION_STAGE == COMM_STAGE_SPEED_PI) || \
      (COMMISSION_STAGE == COMM_STAGE_YAW_TURN)
#define COMM_CONTROL_PWM_LIMIT              600
#elif (COMMISSION_STAGE == COMM_STAGE_TRACE_TUNING)
#define COMM_CONTROL_PWM_LIMIT              700
#elif (COMMISSION_STAGE == COMM_STAGE_AIR_PROTECTION)
#define COMM_CONTROL_PWM_LIMIT              800
#else
#define COMM_CONTROL_PWM_LIMIT              999
#endif
#endif

#define COMM_OLED_PAGE_AUTO                  0U
#define COMM_OLED_PAGE_FIXED                 1U
#define COMM_OLED_PAGE_MODE                 COMM_OLED_PAGE_AUTO
#define COMM_OLED_FIXED_PAGE                 0U
#define COMM_OLED_PAGE_PERIOD_MS            1000U

#define COMM_STAGE_PID_ALLOWED \
    ((COMMISSION_STAGE >= COMM_STAGE_BALANCE_PD) && \
     (COMMISSION_STAGE <= COMM_STAGE_RELEASE))
#define COMM_STAGE_MOTOR_TEST_ALLOWED \
    ((COMMISSION_STAGE == COMM_STAGE_MOTOR_DEADZONE) || \
     (COMMISSION_STAGE == COMM_STAGE_MOTOR_ENCODER))
#define COMM_STAGE_TRACE_CONTROL_ALLOWED \
    ((COMMISSION_STAGE == COMM_STAGE_TRACE_TUNING) || \
     (COMMISSION_STAGE == COMM_STAGE_AIR_PROTECTION) || \
     (COMMISSION_STAGE == COMM_STAGE_RELEASE))

#if (COMMISSION_STAGE < COMM_STAGE_IMU_DIAGNOSTIC) || \
    (COMMISSION_STAGE > COMM_STAGE_RELEASE)
#error Invalid COMMISSION_STAGE
#endif

#if (COMM_TEST_MOTOR_DIRECTION != 1) && (COMM_TEST_MOTOR_DIRECTION != -1)
#error COMM_TEST_MOTOR_DIRECTION must be +1 or -1
#endif

#if (COMM_TEST_MOTOR_PWM <= 0) || \
    (COMM_TEST_MOTOR_PWM > COMM_TEST_PWM_HARD_LIMIT)
#error Motor test PWM must be inside the commissioning hard limit
#endif

#if (COMM_TEST_PULSE_MS == 0U) || (COMM_TEST_PULSE_MS > 300U)
#error Motor test pulse must be 1..300 ms
#endif

#if (COMM_TURN_TEST_PWM <= 0) || (COMM_TURN_TEST_PWM > 100)
#error Turn commissioning PWM must be 1..100
#endif

#if (COMM_CONTROL_PWM_LIMIT < 100) || (COMM_CONTROL_PWM_LIMIT > 999)
#error Commissioning control PWM limit must be 100..999
#endif

#endif
