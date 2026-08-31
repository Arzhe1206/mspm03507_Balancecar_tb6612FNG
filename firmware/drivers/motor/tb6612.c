/*
 * MSPM0G3507 Balance Car
 * 作者：数显纯牛奶，开源工程未经授权禁止商业用途，需要实物见店铺主页：https://m.tb.cn/h.8nzqEuN?tk=kJqnT4uWzfY
 * 官方仓库：https://github.com/Arzhe1206/mspm03507_Balancecar_tb6612FNG
 * 许可：LicenseRef-BalanceCar-NonCommercial
 * 项目标识：MSPM0-BC-ARZHE1206
 * AI/ML 使用声明：未经授权，不得用于模型训练、数据集构建或商业化自动代码生成。
 */
#include "tb6612.h"
#include "board_profile.h"
#include "control_tuning.h"

#define TB6612_PWM_PERIOD 1000U

static int16_t left_command;
static int16_t right_command;

static int clamp_command(int command)
{
    if (command > TB6612_PWM_MAX) {
        return TB6612_PWM_MAX;
    }
    if (command < -TB6612_PWM_MAX) {
        return -TB6612_PWM_MAX;
    }
    return command;
}

static int apply_deadzone(int command, int forward_deadzone,
    int reverse_deadzone)
{
#if (MOTOR_DEADZONE_ENABLE == 1U)
    int magnitude;
    int deadzone;

    if (command == 0) return 0;
    magnitude = command > 0 ? command : -command;
    deadzone = command > 0 ? forward_deadzone : reverse_deadzone;
    magnitude = deadzone +
        magnitude * (TB6612_PWM_MAX - deadzone) / TB6612_PWM_MAX;
    if (magnitude > TB6612_PWM_MAX) magnitude = TB6612_PWM_MAX;
    return command > 0 ? magnitude : -magnitude;
#else
    (void) forward_deadzone;
    (void) reverse_deadzone;
    return command;
#endif
}

static void set_channel_a(int command)
{
    uint32_t magnitude;

    command = clamp_command(command);
    if (command > 0) {
        DL_GPIO_setPins(GPIO_MOTOR_AIN1_PORT, GPIO_MOTOR_AIN1_PIN);
        DL_GPIO_clearPins(GPIO_MOTOR_AIN2_PORT, GPIO_MOTOR_AIN2_PIN);
        magnitude = (uint32_t) command;
    } else if (command < 0) {
        DL_GPIO_clearPins(GPIO_MOTOR_AIN1_PORT, GPIO_MOTOR_AIN1_PIN);
        DL_GPIO_setPins(GPIO_MOTOR_AIN2_PORT, GPIO_MOTOR_AIN2_PIN);
        magnitude = (uint32_t) (-command);
    } else {
        DL_GPIO_clearPins(GPIO_MOTOR_AIN1_PORT,
                          GPIO_MOTOR_AIN1_PIN | GPIO_MOTOR_AIN2_PIN);
        magnitude = 0U;
    }

    DL_TimerA_setCaptureCompareValue(PWM_0_INST,
        TB6612_PWM_PERIOD - magnitude, GPIO_PWM_0_C0_IDX);
}

static void set_channel_b(int command)
{
    uint32_t magnitude;

    command = clamp_command(command);
    if (command > 0) {
        DL_GPIO_setPins(GPIO_MOTOR_BIN1_PORT, GPIO_MOTOR_BIN1_PIN);
        DL_GPIO_clearPins(GPIO_MOTOR_BIN2_PORT, GPIO_MOTOR_BIN2_PIN);
        magnitude = (uint32_t) command;
    } else if (command < 0) {
        DL_GPIO_clearPins(GPIO_MOTOR_BIN1_PORT, GPIO_MOTOR_BIN1_PIN);
        DL_GPIO_setPins(GPIO_MOTOR_BIN2_PORT, GPIO_MOTOR_BIN2_PIN);
        magnitude = (uint32_t) (-command);
    } else {
        DL_GPIO_clearPins(GPIO_MOTOR_BIN1_PORT, GPIO_MOTOR_BIN1_PIN);
        DL_GPIO_clearPins(GPIO_MOTOR_BIN2_PORT, GPIO_MOTOR_BIN2_PIN);
        magnitude = 0U;
    }

    DL_TimerA_setCaptureCompareValue(PWM_0_INST,
        TB6612_PWM_PERIOD - magnitude, GPIO_PWM_0_C1_IDX);
}

void TB6612_Init(void)
{
    TB6612_Stop();
}

void Set_Motor_PWM(int left_pwm, int right_pwm)
{
    left_command = (int16_t) clamp_command(left_pwm);
    right_command = (int16_t) clamp_command(right_pwm);
    set_channel_a(MOTOR_LEFT_SIGN * apply_deadzone(left_command,
        MOTOR_LEFT_FORWARD_DEADZONE, MOTOR_LEFT_REVERSE_DEADZONE));
    set_channel_b(MOTOR_RIGHT_SIGN * apply_deadzone(right_command,
        MOTOR_RIGHT_FORWARD_DEADZONE, MOTOR_RIGHT_REVERSE_DEADZONE));
}

void TB6612_Stop(void)
{
    left_command = 0;
    right_command = 0;
    set_channel_a(0);
    set_channel_b(0);
}

void TB6612_Brake(void)
{
    left_command = 0;
    right_command = 0;
    DL_GPIO_setPins(GPIO_MOTOR_AIN1_PORT, GPIO_MOTOR_AIN1_PIN);
    DL_GPIO_setPins(GPIO_MOTOR_AIN2_PORT, GPIO_MOTOR_AIN2_PIN);
    DL_GPIO_setPins(GPIO_MOTOR_BIN1_PORT, GPIO_MOTOR_BIN1_PIN);
    DL_GPIO_setPins(GPIO_MOTOR_BIN2_PORT, GPIO_MOTOR_BIN2_PIN);
    DL_TimerA_setCaptureCompareValue(PWM_0_INST, 0U, GPIO_PWM_0_C0_IDX);
    DL_TimerA_setCaptureCompareValue(PWM_0_INST, 0U, GPIO_PWM_0_C1_IDX);
}

int16_t TB6612_GetLeftCommand(void) { return left_command; }
int16_t TB6612_GetRightCommand(void) { return right_command; }
