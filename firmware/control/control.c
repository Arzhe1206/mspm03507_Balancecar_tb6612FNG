/*
 * MSPM0G3507 Balance Car
 * 作者：数显纯牛奶，开源工程未经授权禁止商业用途，需要实物见店铺主页：https://m.tb.cn/h.8nzqEuN?tk=kJqnT4uWzfY
 * 官方仓库：https://github.com/Arzhe1206/mspm03507_Balancecar_tb6612FNG
 * 许可：LicenseRef-BalanceCar-NonCommercial
 * 项目标识：MSPM0-BC-ARZHE1206
 * AI/ML 使用声明：未经授权，不得用于模型训练、数据集构建或商业化自动代码生成。
 */
#include "control.h"
#include "tb6612.h"
#include "commissioning_config.h"
#include "control_tuning.h"
#include "safety_config.h"
#include <math.h>

#define CONTROL_SAMPLE_MAX_AGE_US 8000U
#define CONTROL_FRAME_MIN_US 6000U
#define CONTROL_FRAME_MAX_US 14500U
#define CONTROL_NOMINAL_PERIOD_US 10000.0f
#define CONTROL_PWM_LIMIT COMM_CONTROL_PWM_LIMIT
#define CONTROL_MAX_FRAME_SPAN 2U
#define CONTROL_DEGRADED_PWM_STEP 120
#define CONTROL_STEERING_RESERVE 50
#define CONTROL_TARGET_SPEED_LIMIT 50.0f

int Vertical_out;
int Velocity_out;
int MOTO1;
int MOTO2;

static float target_speed;
static float velocity_filtered_error;
static float velocity_integral_seconds;
static int common_pwm;
static bool control_armed;
static ControlState control_state;
static uint32_t last_input_sequence;
static uint32_t saturation_frames;

static int clamp_int(int value, int limit)
{
    if (value > limit) return limit;
    if (value < -limit) return -limit;
    return value;
}

static int abs_int(int value)
{
    return value < 0 ? -value : value;
}

static int slew_limit(int desired, int previous, int max_step)
{
    if (desired > previous + max_step) return previous + max_step;
    if (desired < previous - max_step) return previous - max_step;
    return desired;
}

static void reset_dynamic_state(void)
{
    velocity_filtered_error = 0.0f;
    velocity_integral_seconds = 0.0f;
    Velocity_out = 0;
    Vertical_out = 0;
    common_pwm = 0;
    MOTO1 = 0;
    MOTO2 = 0;
}

static void stop_with_state(ControlState state)
{
    control_state = state;
    target_speed = 0.0f;
    reset_dynamic_state();
    TB6612_Stop();
}

void Control_Init(void)
{
    target_speed = 0.0f;
    control_armed = false;
    control_state = CONTROL_STOP_DISARMED;
    last_input_sequence = 0U;
    saturation_frames = 0U;
    reset_dynamic_state();
    TB6612_Stop();
}

void Control_SetArmed(bool armed)
{
    control_armed = armed;
    if (!armed) {
        stop_with_state(CONTROL_STOP_DISARMED);
    }
}

bool Control_IsArmed(void)
{
    return control_armed;
}

void Control_SetTargetSpeed(float target_counts_per_10ms)
{
    if (!isfinite(target_counts_per_10ms)) {
        target_speed = 0.0f;
    } else if (target_counts_per_10ms > CONTROL_TARGET_SPEED_LIMIT) {
        target_speed = CONTROL_TARGET_SPEED_LIMIT;
    } else if (target_counts_per_10ms < -CONTROL_TARGET_SPEED_LIMIT) {
        target_speed = -CONTROL_TARGET_SPEED_LIMIT;
    } else {
        target_speed = target_counts_per_10ms;
    }
}

void Control_LatchFault(ControlState state)
{
    control_armed = false;
    stop_with_state(state);
}

void Control_Update(const ControlInput *input, uint32_t now_us)
{
    float encoder_error;
    float normalized_encoder_sum;
    float target_angle;
    float dt_seconds;
    float pwm;
    uint32_t minimum_period_us;
    uint32_t maximum_period_us;
    int desired_pwm;
    int requested_turn;
    int available_turn;
    int applied_turn;

    if (!control_armed) {
        reset_dynamic_state();
        TB6612_Stop();
        return;
    }
    if (input == NULL || !input->sensor_valid ||
        !isfinite(input->roll_deg) || !isfinite(input->gyro_x_dps)) {
        Control_LatchFault(CONTROL_STOP_SENSOR_INVALID);
        return;
    }
    if (input->sample_sequence == last_input_sequence ||
        input->frame_span == 0U ||
        input->frame_span > CONTROL_MAX_FRAME_SPAN) {
        Control_LatchFault(CONTROL_STOP_TIMING_FAULT);
        return;
    }
    minimum_period_us = CONTROL_FRAME_MIN_US * input->frame_span;
    maximum_period_us = CONTROL_FRAME_MAX_US * input->frame_span;
    if (input->dt_us < minimum_period_us ||
        input->dt_us > maximum_period_us) {
        Control_LatchFault(CONTROL_STOP_TIMING_FAULT);
        return;
    }
    if ((uint32_t) (now_us - input->sample_timestamp_us) >
        CONTROL_SAMPLE_MAX_AGE_US) {
        Control_LatchFault(CONTROL_STOP_SENSOR_STALE);
        return;
    }
    if (fabsf(input->roll_deg) > CONTROL_FALL_ANGLE_DEG) {
        Control_LatchFault(CONTROL_STOP_FALLEN);
        return;
    }

    last_input_sequence = input->sample_sequence;
    dt_seconds = input->dt_us * 0.000001f;
    normalized_encoder_sum =
        ((float) input->encoder_left_delta +
         (float) input->encoder_right_delta) *
        (CONTROL_NOMINAL_PERIOD_US / (float) input->dt_us);
    encoder_error = normalized_encoder_sum - target_speed;
    velocity_filtered_error =
        0.3f * encoder_error + 0.7f * velocity_filtered_error;
#if (COMMISSION_STAGE == COMM_STAGE_BALANCE_PD)
    velocity_integral_seconds = 0.0f;
    Velocity_out = 0;
#else
    if (input->quality != CONTROL_FRAME_PREDICTED &&
        abs_int(common_pwm) < CONTROL_PWM_LIMIT - CONTROL_STEERING_RESERVE) {
        velocity_integral_seconds += velocity_filtered_error * dt_seconds;
    }
    if (velocity_integral_seconds > CONTROL_VELOCITY_INTEGRAL_LIMIT)
        velocity_integral_seconds = CONTROL_VELOCITY_INTEGRAL_LIMIT;
    if (velocity_integral_seconds < -CONTROL_VELOCITY_INTEGRAL_LIMIT)
        velocity_integral_seconds = -CONTROL_VELOCITY_INTEGRAL_LIMIT;
    Velocity_out = (int) (CONTROL_VELOCITY_KP * velocity_filtered_error +
        CONTROL_VELOCITY_KI_PER_SECOND * velocity_integral_seconds);
#endif

    target_angle = CONTROL_BALANCE_TRIM_DEG - (float) Velocity_out;
    Vertical_out = (int) (CONTROL_VERTICAL_KP *
        (input->roll_deg - target_angle) +
        CONTROL_VERTICAL_KD * input->gyro_x_dps);
    pwm = (float) Vertical_out;
    desired_pwm = clamp_int((int) pwm, CONTROL_PWM_LIMIT);
    if ((int) pwm != desired_pwm) saturation_frames++;
    if (input->quality == CONTROL_FRAME_NORMAL) {
        common_pwm = desired_pwm;
        control_state = CONTROL_RUNNING;
    } else {
        common_pwm = slew_limit(desired_pwm, common_pwm,
            CONTROL_DEGRADED_PWM_STEP);
        control_state = input->quality == CONTROL_FRAME_PREDICTED ?
            CONTROL_DEGRADED : CONTROL_RECOVERING;
    }

    requested_turn = input->quality == CONTROL_FRAME_NORMAL ?
        input->turn_pwm : 0;
    available_turn = CONTROL_PWM_LIMIT - abs_int(common_pwm) -
        CONTROL_STEERING_RESERVE;
    if (available_turn < 0) available_turn = 0;
    applied_turn = clamp_int(requested_turn, available_turn);
    MOTO1 = clamp_int(common_pwm - applied_turn, CONTROL_PWM_LIMIT);
    MOTO2 = clamp_int(common_pwm + applied_turn, CONTROL_PWM_LIMIT);
    Set_Motor_PWM(MOTO1, MOTO2);
}

ControlState Control_GetState(void)
{
    return control_state;
}

ControlDiagnostics Control_GetDiagnostics(void)
{
    ControlDiagnostics diagnostics;

    diagnostics.target_speed_counts = target_speed;
    diagnostics.velocity_filtered_error = velocity_filtered_error;
    diagnostics.velocity_integral_seconds = velocity_integral_seconds;
    diagnostics.common_pwm = common_pwm;
    diagnostics.saturation_frames = saturation_frames;
    return diagnostics;
}
