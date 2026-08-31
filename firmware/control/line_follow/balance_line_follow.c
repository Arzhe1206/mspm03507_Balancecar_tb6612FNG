/*
 * MSPM0G3507 Balance Car
 * 作者：数显纯牛奶，开源工程未经授权禁止商业用途，需要实物见店铺主页：https://m.tb.cn/h.8nzqEuN?tk=kJqnT4uWzfY
 * 官方仓库：https://github.com/Arzhe1206/mspm03507_Balancecar_tb6612FNG
 * 许可：LicenseRef-BalanceCar-NonCommercial
 * 项目标识：MSPM0-BC-ARZHE1206
 * AI/ML 使用声明：未经授权，不得用于模型训练、数据集构建或商业化自动代码生成。
 */
#include "balance_line_follow.h"
#include "balance_trace_config.h"
#include <math.h>
#include <string.h>

static BalanceLineFollowOutput output;
static float last_error;
static float derivative_filter;
static int last_turn_pwm;
static bool previous_line_valid;

static float clamp_float(float value, float minimum, float maximum)
{
    if (value > maximum) return maximum;
    if (value < minimum) return minimum;
    return value;
}

static int clamp_int(int value, int minimum, int maximum)
{
    if (value > maximum) return maximum;
    if (value < minimum) return minimum;
    return value;
}

static int slew_turn(int desired)
{
    desired = clamp_int(desired, -TRACE_TURN_PWM_MAX, TRACE_TURN_PWM_MAX);
    if (desired > last_turn_pwm + TRACE_TURN_PWM_SLEW)
        desired = last_turn_pwm + TRACE_TURN_PWM_SLEW;
    if (desired < last_turn_pwm - TRACE_TURN_PWM_SLEW)
        desired = last_turn_pwm - TRACE_TURN_PWM_SLEW;
    last_turn_pwm = desired;
    return desired;
}

static float roll_authority(float roll_deg)
{
    float magnitude = fabsf(roll_deg);

    if (magnitude <= TRACE_FULL_AUTHORITY_ROLL_DEG) return 1.0f;
    if (magnitude >= TRACE_ZERO_AUTHORITY_ROLL_DEG) return 0.0f;
    return (TRACE_ZERO_AUTHORITY_ROLL_DEG - magnitude) /
        (TRACE_ZERO_AUTHORITY_ROLL_DEG - TRACE_FULL_AUTHORITY_ROLL_DEG);
}

void BalanceLineFollow_Init(void)
{
    memset(&output, 0, sizeof(output));
    last_error = 0.0f;
    derivative_filter = 0.0f;
    last_turn_pwm = 0;
    previous_line_valid = false;
}

bool BalanceLineFollow_Start(float current_yaw_deg)
{
    if (!TraceSensor_HasUsableLine()) return false;

    output.enabled = true;
    output.fault = false;
    output.yaw_target_deg = current_yaw_deg;
    output.yaw_rate_target_dps = 0.0f;
    output.forward_target_counts = 0.0f;
    output.turn_pwm = 0;
    output.lost_frames = 0U;
    last_error = TraceSensor_GetData().error;
    derivative_filter = 0.0f;
    last_turn_pwm = 0;
    previous_line_valid = true;
    return true;
}

void BalanceLineFollow_Stop(void)
{
    output.enabled = false;
    output.forward_target_counts = 0.0f;
    output.yaw_rate_target_dps = 0.0f;
    output.turn_pwm = 0;
    output.lost_frames = 0U;
    last_turn_pwm = 0;
    previous_line_valid = false;
}

void BalanceLineFollow_Hold(float current_yaw_deg)
{
    if (!output.enabled) return;

    output.yaw_target_deg = current_yaw_deg;
    output.yaw_rate_target_dps = 0.0f;
    output.forward_target_counts = 0.0f;
    output.turn_pwm = 0;
    derivative_filter *= 0.5f;
    last_turn_pwm = 0;
    previous_line_valid = false;
}

bool BalanceLineFollow_IsEnabled(void)
{
    return output.enabled;
}

void BalanceLineFollow_Update(const TraceSensorData *trace,
    float yaw_deg, float gyro_z_dps, float roll_deg, uint32_t dt_us)
{
    float dt_seconds;
    float error;
    float derivative = 0.0f;
    float decay;
    float yaw_error;
    float turn;
    float authority;
    bool valid_line;

    if (!output.enabled || trace == NULL) {
        output.turn_pwm = 0;
        output.forward_target_counts = 0.0f;
        return;
    }

    dt_seconds = dt_us * 0.000001f;
    if (dt_seconds < 0.006f || dt_seconds > 0.029f) {
        output.fault = true;
        BalanceLineFollow_Stop();
        return;
    }

    valid_line = trace->state == TRACE_STATE_NORMAL ||
        trace->state == TRACE_STATE_LEFT_EDGE ||
        trace->state == TRACE_STATE_RIGHT_EDGE;
    error = trace->error;
    output.line_error = error;

    if (valid_line) {
        if (previous_line_valid) derivative = (error - last_error) / dt_seconds;
        derivative_filter = TRACE_LINE_D_FILTER_ALPHA * derivative +
            (1.0f - TRACE_LINE_D_FILTER_ALPHA) * derivative_filter;
        last_error = error;
        previous_line_valid = true;
        output.lost_frames = 0U;
        output.forward_target_counts = TRACE_FORWARD_TARGET_COUNTS;
        output.yaw_rate_target_dps = TRACE_LINE_YAW_RATE_KP * error +
            TRACE_LINE_YAW_RATE_KD * derivative_filter;
    } else if (trace->state == TRACE_STATE_ALL_BLACK) {
        previous_line_valid = false;
        output.lost_frames = 0U;
        output.forward_target_counts = 0.0f;
        output.yaw_rate_target_dps = 0.0f;
        derivative_filter *= 0.8f;
    } else {
        previous_line_valid = false;
        if (output.lost_frames < UINT16_MAX) output.lost_frames++;
        output.forward_target_counts = 0.0f;
        if (output.lost_frames <= TRACE_LOST_HOLD_FRAMES) {
            decay = (float) (TRACE_LOST_HOLD_FRAMES - output.lost_frames) /
                (float) TRACE_LOST_HOLD_FRAMES;
            output.yaw_rate_target_dps =
                TRACE_LINE_YAW_RATE_KP * last_error * decay;
        } else {
            output.yaw_rate_target_dps = 0.0f;
        }
        if (output.lost_frames >= TRACE_LOST_FAULT_FRAMES) {
            output.fault = true;
            BalanceLineFollow_Stop();
            return;
        }
    }

    output.yaw_rate_target_dps = clamp_float(output.yaw_rate_target_dps,
        -TRACE_YAW_RATE_MAX_DPS, TRACE_YAW_RATE_MAX_DPS);
    output.yaw_target_deg += output.yaw_rate_target_dps * dt_seconds;
    output.yaw_target_deg = clamp_float(output.yaw_target_deg,
        yaw_deg - TRACE_HEADING_LEAD_MAX_DEG,
        yaw_deg + TRACE_HEADING_LEAD_MAX_DEG);

    yaw_error = output.yaw_target_deg - yaw_deg;
    authority = roll_authority(roll_deg);
    turn = TRACE_STEER_SIGN * authority *
        (TRACE_HEADING_KP * yaw_error +
         TRACE_YAW_RATE_KP * (output.yaw_rate_target_dps - gyro_z_dps));
    output.turn_pwm = slew_turn((int) turn);
}

BalanceLineFollowOutput BalanceLineFollow_GetOutput(void)
{
    return output;
}
