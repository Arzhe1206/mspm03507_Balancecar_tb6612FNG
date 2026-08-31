/*
 * MSPM0G3507 Balance Car
 * 作者：数显纯牛奶，开源工程未经授权禁止商业用途，需要实物见店铺主页：https://m.tb.cn/h.8nzqEuN?tk=kJqnT4uWzfY
 * 官方仓库：https://github.com/Arzhe1206/mspm03507_Balancecar_tb6612FNG
 * 许可：LicenseRef-BalanceCar-NonCommercial
 * 项目标识：MSPM0-BC-ARZHE1206
 * AI/ML 使用声明：未经授权，不得用于模型训练、数据集构建或商业化自动代码生成。
 */
#include "imu_startup.h"
#include "system_time.h"
#include <math.h>
#include <string.h>

#define STABLE_REQUIRED_US 2000000U
#define ZERO_SAMPLE_COUNT 200U
#define AUTO_ZERO_DELAY_MS 3000U
#define MAX_YAW_DRIFT_DEG 0.1f
#define MAX_LEVEL_RANGE_DEG 0.3f
#define MAX_STILL_GYRO_DPS 1.0f

typedef struct {
    ImuStartupState state;
    MPU6050_Sample latest;
    bool latest_valid;
    bool yaw_initialized;
    float previous_yaw_deg;
    float yaw_unwrapped_deg;
    uint32_t stable_start_us;
    float stable_yaw_start_deg;
    float pitch_min_deg;
    float pitch_max_deg;
    float roll_min_deg;
    float roll_max_deg;
    float zero_pitch_sum;
    float zero_roll_sum;
    float zero_yaw_sum;
    float zero_pitch_deg;
    float zero_roll_deg;
    float zero_yaw_deg;
    uint16_t zero_samples;
    uint8_t arm_confirm_frames;
    uint32_t ready_start_ms;
    uint8_t pending_events;
    bool zero_valid;
    bool zero_requested;
} ImuStartupContext;

static ImuStartupContext context;

static float angle_delta(float current, float previous)
{
    float delta = current - previous;

    if (delta > 180.0f) delta -= 360.0f;
    if (delta < -180.0f) delta += 360.0f;
    return delta;
}

static bool sample_is_stationary(const MPU6050_Sample *sample)
{
    return fabsf(sample->gyro_dps[0]) <= MAX_STILL_GYRO_DPS &&
        fabsf(sample->gyro_dps[1]) <= MAX_STILL_GYRO_DPS &&
        fabsf(sample->gyro_dps[2]) <= MAX_STILL_GYRO_DPS;
}

static void begin_stability_window(const MPU6050_Sample *sample)
{
    context.stable_start_us = sample->timestamp_us;
    context.stable_yaw_start_deg = context.yaw_unwrapped_deg;
    context.pitch_min_deg = sample->pitch_deg;
    context.pitch_max_deg = sample->pitch_deg;
    context.roll_min_deg = sample->roll_deg;
    context.roll_max_deg = sample->roll_deg;
}

static bool update_motion_window(const MPU6050_Sample *sample)
{
    if (!sample_is_stationary(sample)) return false;
    if (sample->pitch_deg < context.pitch_min_deg)
        context.pitch_min_deg = sample->pitch_deg;
    if (sample->pitch_deg > context.pitch_max_deg)
        context.pitch_max_deg = sample->pitch_deg;
    if (sample->roll_deg < context.roll_min_deg)
        context.roll_min_deg = sample->roll_deg;
    if (sample->roll_deg > context.roll_max_deg)
        context.roll_max_deg = sample->roll_deg;

    return fabsf(context.yaw_unwrapped_deg -
            context.stable_yaw_start_deg) <= MAX_YAW_DRIFT_DEG &&
        context.pitch_max_deg - context.pitch_min_deg <= MAX_LEVEL_RANGE_DEG &&
        context.roll_max_deg - context.roll_min_deg <= MAX_LEVEL_RANGE_DEG;
}

static void reset_zero_collection(void)
{
    context.zero_pitch_sum = 0.0f;
    context.zero_roll_sum = 0.0f;
    context.zero_yaw_sum = 0.0f;
    context.zero_samples = 0U;
}

static void return_to_stabilizing(void)
{
    context.state = IMU_STARTUP_STABILIZING;
    context.stable_start_us = 0U;
    context.arm_confirm_frames = 0U;
    context.ready_start_ms = 0U;
    context.zero_valid = false;
    context.zero_requested = false;
    reset_zero_collection();
}

static void apply_zero(void)
{
    context.zero_valid = true;
    context.zero_requested = false;
    context.arm_confirm_frames = 0U;
    context.state = IMU_STARTUP_ARM_CONFIRM;
    context.pending_events |= IMU_STARTUP_EVENT_ZERO_APPLIED;
}

void ImuStartup_Init(void)
{
    memset(&context, 0, sizeof(context));
    context.state = IMU_STARTUP_DMP_LOADING;
}

void ImuStartup_DmpReady(bool success)
{
    if (success) {
        return_to_stabilizing();
    } else {
        context.state = IMU_STARTUP_ERROR;
    }
}

void ImuStartup_ProcessSample(const MPU6050_Sample *sample, bool clean_frame)
{
    if (sample == NULL || !sample->valid ||
        !isfinite(sample->pitch_deg) || !isfinite(sample->roll_deg) ||
        !isfinite(sample->yaw_deg)) {
        ImuStartup_InvalidateFrame();
        return;
    }

    context.latest = *sample;
    context.latest_valid = true;
    if (!context.yaw_initialized) {
        context.previous_yaw_deg = sample->yaw_deg;
        context.yaw_unwrapped_deg = sample->yaw_deg;
        context.yaw_initialized = true;
    } else {
        context.yaw_unwrapped_deg += angle_delta(
            sample->yaw_deg, context.previous_yaw_deg);
        context.previous_yaw_deg = sample->yaw_deg;
    }

    if (context.state == IMU_STARTUP_RUNNING ||
        context.state == IMU_STARTUP_DMP_LOADING ||
        context.state == IMU_STARTUP_ERROR) {
        return;
    }
    if (!clean_frame) {
        return_to_stabilizing();
        return;
    }

    if (context.state == IMU_STARTUP_STABILIZING) {
        if (context.stable_start_us == 0U) {
            begin_stability_window(sample);
            return;
        }
        if (!update_motion_window(sample)) {
            begin_stability_window(sample);
            return;
        }
        if ((uint32_t) (sample->timestamp_us - context.stable_start_us) >=
            STABLE_REQUIRED_US) {
            context.state = IMU_STARTUP_ZERO_SAMPLING;
            reset_zero_collection();
            begin_stability_window(sample);
        }
        return;
    }

    if (context.state == IMU_STARTUP_ZERO_SAMPLING) {
        if (!update_motion_window(sample)) {
            return_to_stabilizing();
            return;
        }
        context.zero_pitch_sum += sample->pitch_deg;
        context.zero_roll_sum += sample->roll_deg;
        context.zero_yaw_sum += context.yaw_unwrapped_deg;
        context.zero_samples++;
        if (context.zero_samples >= ZERO_SAMPLE_COUNT) {
            context.zero_pitch_deg = context.zero_pitch_sum /
                (float) ZERO_SAMPLE_COUNT;
            context.zero_roll_deg = context.zero_roll_sum /
                (float) ZERO_SAMPLE_COUNT;
            context.zero_yaw_deg = context.zero_yaw_sum /
                (float) ZERO_SAMPLE_COUNT;
            context.ready_start_ms = system_millis();
            context.state = IMU_STARTUP_ZERO_READY;
        }
        return;
    }

    if (context.state == IMU_STARTUP_ZERO_READY) {
        if (!sample_is_stationary(sample) ||
            fabsf(sample->pitch_deg - context.zero_pitch_deg) >
                MAX_LEVEL_RANGE_DEG ||
            fabsf(sample->roll_deg - context.zero_roll_deg) >
                MAX_LEVEL_RANGE_DEG ||
            fabsf(context.yaw_unwrapped_deg - context.zero_yaw_deg) >
                MAX_YAW_DRIFT_DEG) {
            return_to_stabilizing();
            return;
        }
        if (context.zero_requested) {
            apply_zero();
        }
        return;
    }

    if (context.state == IMU_STARTUP_ARM_CONFIRM) {
        if (!sample_is_stationary(sample)) {
            return_to_stabilizing();
            return;
        }
        context.arm_confirm_frames++;
        if (context.arm_confirm_frames >= 2U) {
            context.state = IMU_STARTUP_RUNNING;
            context.pending_events |= IMU_STARTUP_EVENT_ARM_READY;
        }
    }
}

void ImuStartup_InvalidateFrame(void)
{
    if (context.state != IMU_STARTUP_RUNNING &&
        context.state != IMU_STARTUP_DMP_LOADING &&
        context.state != IMU_STARTUP_ERROR) {
        return_to_stabilizing();
    }
}

void ImuStartup_Update(uint32_t now_ms)
{
    if (context.state == IMU_STARTUP_ZERO_READY) {
        if (!context.latest_valid ||
            (uint32_t) (system_micros() - context.latest.timestamp_us) >
                30000U) {
            return_to_stabilizing();
            return;
        }
        if ((uint32_t) (now_ms - context.ready_start_ms) >=
            AUTO_ZERO_DELAY_MS) {
            context.zero_requested = true;
        }
    }
}

void ImuStartup_HandleButton(void)
{
    if (context.state == IMU_STARTUP_ZERO_READY) {
        context.zero_requested = true;
    }
}

void ImuStartup_AbortToStabilizing(void)
{
    if (context.state != IMU_STARTUP_ERROR &&
        context.state != IMU_STARTUP_DMP_LOADING) {
        return_to_stabilizing();
    }
}

uint8_t ImuStartup_ConsumeEvents(void)
{
    uint8_t events = context.pending_events;
    context.pending_events = IMU_STARTUP_EVENT_NONE;
    return events;
}

ImuStartupState ImuStartup_GetState(void)
{
    return context.state;
}

bool ImuStartup_IsRunning(void)
{
    return context.state == IMU_STARTUP_RUNNING;
}

void ImuStartup_GetAngles(float *pitch_deg, float *roll_deg, float *yaw_deg)
{
    float pitch = context.latest_valid ? context.latest.pitch_deg : 0.0f;
    float roll = context.latest_valid ? context.latest.roll_deg : 0.0f;
    float yaw = context.yaw_initialized ? context.yaw_unwrapped_deg : 0.0f;

    if (context.zero_valid) {
        pitch -= context.zero_pitch_deg;
        roll -= context.zero_roll_deg;
        yaw -= context.zero_yaw_deg;
    }
    if (pitch_deg != NULL) *pitch_deg = pitch;
    if (roll_deg != NULL) *roll_deg = roll;
    if (yaw_deg != NULL) *yaw_deg = yaw;
}

uint32_t ImuStartup_GetStableElapsedMs(void)
{
    if (context.state != IMU_STARTUP_STABILIZING ||
        context.stable_start_us == 0U || !context.latest_valid) return 0U;
    return (context.latest.timestamp_us - context.stable_start_us) / 1000U;
}

uint16_t ImuStartup_GetZeroSampleCount(void)
{
    return context.zero_samples;
}

uint32_t ImuStartup_GetAutoZeroRemainingMs(uint32_t now_ms)
{
    uint32_t elapsed;

    if (context.state != IMU_STARTUP_ZERO_READY) return 0U;
    elapsed = now_ms - context.ready_start_ms;
    if (elapsed >= AUTO_ZERO_DELAY_MS) return 0U;
    return AUTO_ZERO_DELAY_MS - elapsed;
}
