/*
 * MSPM0G3507 Balance Car
 * 作者：数显纯牛奶，开源工程未经授权禁止商业用途，需要实物见店铺主页：https://m.tb.cn/h.8nzqEuN?tk=kJqnT4uWzfY
 * 官方仓库：https://github.com/Arzhe1206/mspm03507_Balancecar_tb6612FNG
 * 许可：LicenseRef-BalanceCar-NonCommercial
 * 项目标识：MSPM0-BC-ARZHE1206
 * AI/ML 使用声明：未经授权，不得用于模型训练、数据集构建或商业化自动代码生成。
 */
#include "board.h"
#include "balance_line_follow.h"
#include "balance_trace_config.h"
#include "tb6612.h"
#include "commissioning.h"
#include "commissioning_config.h"
#include "control.h"
#include "hw_encoder.h"
#include "hw_timer.h"
#include "imu_startup.h"
#include "jy61p.h"
#include "mpu6050.h"
#include "oled.h"
#include "system_time.h"
#include "trace_sensor.h"
#include "safety_config.h"
#include <stdio.h>

#define BUTTON_TASK_PERIOD_MS 20U
#define DISPLAY_TASK_PERIOD_MS 100U
#define HEARTBEAT_TASK_PERIOD_MS 1000U
#define MPU_RECOVERY_PERIOD_MS 50U
#define CONTROL_WATCHDOG_US 30000U
#define CONTROL_SINGLE_FRAME_MAX_US 14500U
#define CONTROL_MAX_PREDICTION_US 25000U
#define CONTROL_MAX_PREDICTED_FRAMES 2U
#define CONTROL_RECOVERY_CONFIRM_FRAMES 2U
#define JY61P_FOREGROUND_BUDGET 32U

typedef struct {
    uint32_t processed_frames;
    uint32_t dropped_frames;
    uint32_t imu_errors;
    uint32_t recoveries;
    uint32_t predicted_frames;
    uint32_t last_execution_us;
    uint32_t max_execution_us;
    uint32_t max_sample_age_us;
} ControlFrameStats;

static bool mpu_initialized;
static bool previous_latch_valid;
static EncoderControlLatch previous_latch;
static bool last_good_sample_valid;
static MPU6050_Sample last_good_sample;
static float last_good_control_pitch_deg;
static float last_good_control_roll_deg;
static float last_good_control_yaw_deg;
static bool degraded_active;
static uint8_t predicted_frame_count;
static uint8_t recovery_valid_frames;
static ControlFrameStats control_stats;
static uint32_t watchdog_last_tick;
static uint32_t button_task_last_ms;
static uint32_t display_task_last_ms;
static uint32_t heartbeat_task_last_ms;
static uint32_t recovery_task_last_ms;
static uint32_t balance_running_since_ms;
static bool emergency_stop_latched;
static bool emergency_wait_release;

static void reset_control_alignment(void)
{
    Commissioning_StopOutputs();
    BalanceLineFollow_Stop();
    Control_Init();
    encoder_reset_control_latch();
    previous_latch_valid = false;
    last_good_sample_valid = false;
    last_good_control_pitch_deg = 0.0f;
    last_good_control_roll_deg = 0.0f;
    last_good_control_yaw_deg = 0.0f;
    degraded_active = false;
    predicted_frame_count = 0U;
    recovery_valid_frames = 0U;
    balance_running_since_ms = 0U;
}

static bool service_startup_events(void)
{
    uint8_t events = ImuStartup_ConsumeEvents();

    if ((events & IMU_STARTUP_EVENT_ZERO_APPLIED) != 0U) {
        reset_control_alignment();
        return true;
    }
    if ((events & IMU_STARTUP_EVENT_ARM_READY) != 0U) {
        if (Commissioning_PidAllowed() && !emergency_stop_latched) {
            Control_SetArmed(true);
            balance_running_since_ms = system_millis();
        } else {
            Control_SetArmed(false);
        }
    }
    return false;
}

static void latch_control_fault(ControlState state)
{
    BalanceLineFollow_Stop();
    Control_SetTargetSpeed(0.0f);
    Commissioning_StopOutputs();
    Control_LatchFault(state);
    ImuStartup_AbortToStabilizing();
}

static void latch_emergency_stop(void)
{
    emergency_stop_latched = true;
    emergency_wait_release =
        DL_GPIO_readPins(KEY_PIN_21_PORT, KEY_PIN_21_PIN) == 0U;
    BalanceLineFollow_Stop();
    Control_SetTargetSpeed(0.0f);
    Control_SetArmed(false);
    Commissioning_StopOutputs();
    ImuStartup_AbortToStabilizing();
}

static void acknowledge_emergency_stop(void)
{
    if (!emergency_stop_latched || emergency_wait_release) return;
    emergency_stop_latched = false;
    reset_control_alignment();
    Commissioning_Init();
    ImuStartup_AbortToStabilizing();
}

static int32_t encoder_total_delta(int32_t current, int32_t previous)
{
    return (int32_t) ((uint32_t) current - (uint32_t) previous);
}

static void finish_control_timing(uint32_t start_us,
                                  uint32_t sample_timestamp_us)
{
    uint32_t end_us = system_micros();
    uint32_t execution_us = end_us - start_us;
    uint32_t sample_age_us = end_us - sample_timestamp_us;

    control_stats.last_execution_us = execution_us;
    if (execution_us > control_stats.max_execution_us) {
        control_stats.max_execution_us = execution_us;
    }
    if (sample_age_us > control_stats.max_sample_age_us) {
        control_stats.max_sample_age_us = sample_age_us;
    }
}

static bool control_task_10ms(void)
{
    EncoderControlLatch frames[ENCODER_CONTROL_QUEUE_CAPACITY];
    EncoderControlLatch latch;
    EncoderControlLatch previous;
    MPU6050_ProcessResult process_result;
    MPU6050_Sample sample;
    ControlInput input = {0};
    uint32_t start_us;
    uint32_t sequence_gap;
    uint32_t dt_us = 0U;
    uint32_t prediction_us;
    uint32_t retry_count_before;
    uint8_t frame_count;
    uint8_t frame_index;
    uint8_t frame_span = 1U;
    bool queue_overflowed = false;

    frame_count = encoder_take_control_frames(frames,
        ENCODER_CONTROL_QUEUE_CAPACITY, &queue_overflowed);
    if (frame_count == 0U) {
        return false;
    }

    for (frame_index = 0U; frame_index < frame_count; frame_index++) {
        TraceSensor_Update(frames[frame_index].trace_raw,
            frames[frame_index].timestamp_us, frames[frame_index].sequence);
    }

    latch = frames[frame_count - 1U];
    start_us = system_micros();
    previous = previous_latch;

    if (!mpu_initialized || mpu6050_recovery_required()) {
        control_stats.imu_errors++;
        latch_control_fault(CONTROL_STOP_SENSOR_INVALID);
        finish_control_timing(start_us, latch.timestamp_us);
        return true;
    }

    if (queue_overflowed) {
        control_stats.dropped_frames++;
        mpu6050_request_recovery();
        latch_control_fault(CONTROL_STOP_TIMING_FAULT);
        finish_control_timing(start_us, latch.timestamp_us);
        return true;
    }

    if (previous_latch_valid) {
        sequence_gap = latch.sequence - previous.sequence;
        dt_us = latch.timestamp_us - previous.timestamp_us;
        frame_span = dt_us > CONTROL_SINGLE_FRAME_MAX_US ? 2U : 1U;
        if (sequence_gap == 0U || sequence_gap > 2U ||
            frame_count != sequence_gap || frame_span > 2U ||
            dt_us > CONTROL_WATCHDOG_US) {
            if (sequence_gap > 1U && sequence_gap <= 4U) {
                control_stats.dropped_frames += sequence_gap - 1U;
            } else {
                control_stats.dropped_frames++;
            }
            mpu6050_request_recovery();
            latch_control_fault(CONTROL_STOP_TIMING_FAULT);
            finish_control_timing(start_us, latch.timestamp_us);
            return true;
        }
    }

    retry_count_before = mpu6050_retry_count();
    process_result = mpu6050_process_control_frame(
        frame_count, latch.timestamp_us, latch.sequence);
    if (process_result == MPU6050_PROCESS_TRANSIENT) {
        control_stats.imu_errors++;
        previous_latch = latch;
        previous_latch_valid = true;

        if (!last_good_sample_valid || !Control_IsArmed()) {
            ImuStartup_InvalidateFrame();
            finish_control_timing(start_us, latch.timestamp_us);
            return true;
        }

        predicted_frame_count = (uint8_t) (predicted_frame_count + frame_span);
        prediction_us = latch.timestamp_us - last_good_sample.timestamp_us;
        if (predicted_frame_count > CONTROL_MAX_PREDICTED_FRAMES ||
            prediction_us > CONTROL_MAX_PREDICTION_US) {
            mpu6050_request_recovery();
            latch_control_fault(CONTROL_STOP_SENSOR_STALE);
            finish_control_timing(start_us, latch.timestamp_us);
            return true;
        }

        degraded_active = true;
        recovery_valid_frames = 0U;
        input.roll_deg = last_good_control_roll_deg +
            last_good_sample.gyro_dps[0] * (prediction_us * 0.000001f);
        input.gyro_x_dps = last_good_sample.gyro_dps[0];
        input.encoder_left_delta = encoder_total_delta(
            latch.total.left, previous.total.left);
        input.encoder_right_delta = encoder_total_delta(
            latch.total.right, previous.total.right);
        input.sample_timestamp_us = latch.timestamp_us;
        input.sample_sequence = latch.sequence;
        input.dt_us = dt_us;
        input.frame_span = frame_span;
        input.quality = CONTROL_FRAME_PREDICTED;
        input.sensor_valid = true;
        input.turn_pwm = 0;
        BalanceLineFollow_Hold(last_good_control_yaw_deg +
            last_good_sample.gyro_dps[2] * (prediction_us * 0.000001f));
        Control_SetTargetSpeed(0.0f);
        Control_Update(&input, system_micros());
        if (!Control_IsArmed()) {
            ImuStartup_AbortToStabilizing();
        }
        control_stats.predicted_frames++;
        control_stats.processed_frames++;
        finish_control_timing(start_us, latch.timestamp_us);
        return true;
    }

    if (process_result != MPU6050_PROCESS_OK ||
        !mpu6050_get_sample(&sample) || sample.sequence != latch.sequence) {
        control_stats.imu_errors++;
        mpu6050_request_recovery();
        latch_control_fault(CONTROL_STOP_SENSOR_INVALID);
        finish_control_timing(start_us, latch.timestamp_us);
        return true;
    }

    ImuStartup_ProcessSample(&sample,
        frame_count == 1U && frame_span == 1U &&
        retry_count_before == mpu6050_retry_count());
    if (service_startup_events()) {
        finish_control_timing(start_us, latch.timestamp_us);
        return true;
    }

    if (!previous_latch_valid) {
        previous_latch = latch;
        previous_latch_valid = true;
        last_good_sample = sample;
        last_good_sample_valid = true;
        ImuStartup_GetAngles(&last_good_control_pitch_deg,
            &last_good_control_roll_deg,
            &last_good_control_yaw_deg);
        Commissioning_RecordFrame(&sample, last_good_control_pitch_deg,
            last_good_control_roll_deg, last_good_control_yaw_deg, &latch,
            0, 0, 0U);
        predicted_frame_count = 0U;
        finish_control_timing(start_us, latch.timestamp_us);
        return true;
    }

    previous_latch = latch;
    last_good_sample = sample;
    last_good_sample_valid = true;
    {
        TraceSensorData trace;
        BalanceLineFollowOutput line_output;

        ImuStartup_GetAngles(&last_good_control_pitch_deg,
            &last_good_control_roll_deg,
            &last_good_control_yaw_deg);
        predicted_frame_count = 0U;
        if (!ImuStartup_IsRunning()) {
            Commissioning_RecordFrame(&sample, last_good_control_pitch_deg,
                last_good_control_roll_deg, last_good_control_yaw_deg, &latch,
                encoder_total_delta(latch.total.left, previous.total.left),
                encoder_total_delta(latch.total.right, previous.total.right),
                dt_us);
            finish_control_timing(start_us, latch.timestamp_us);
            return true;
        }
        if (frame_span > 1U) {
            control_stats.dropped_frames += frame_span - 1U;
            degraded_active = true;
            recovery_valid_frames = 0U;
            input.quality = CONTROL_FRAME_RECOVERY;
        } else if (degraded_active) {
            recovery_valid_frames++;
            input.quality = CONTROL_FRAME_RECOVERY;
            if (recovery_valid_frames >= CONTROL_RECOVERY_CONFIRM_FRAMES) {
                degraded_active = false;
                recovery_valid_frames = 0U;
            }
        } else {
            input.quality = CONTROL_FRAME_NORMAL;
        }

        trace = TraceSensor_GetData();
        if (input.quality == CONTROL_FRAME_NORMAL) {
            BalanceLineFollow_Update(&trace, last_good_control_yaw_deg,
                sample.gyro_dps[2], last_good_control_roll_deg, dt_us);
        } else {
            BalanceLineFollow_Hold(last_good_control_yaw_deg);
        }
        line_output = BalanceLineFollow_GetOutput();
        if (line_output.enabled && input.quality == CONTROL_FRAME_NORMAL) {
            Control_SetTargetSpeed(line_output.forward_target_counts);
            input.turn_pwm = line_output.turn_pwm;
        } else {
            Control_SetTargetSpeed(0.0f);
            input.turn_pwm = 0;
        }
    }

    input.roll_deg = last_good_control_roll_deg;
    input.gyro_x_dps = sample.gyro_dps[0];
    input.encoder_left_delta = encoder_total_delta(
        latch.total.left, previous.total.left);
    input.encoder_right_delta = encoder_total_delta(
        latch.total.right, previous.total.right);
    input.sample_timestamp_us = latch.timestamp_us;
    input.sample_sequence = latch.sequence;
    input.dt_us = dt_us;
    input.frame_span = frame_span;
    input.sensor_valid = sample.valid;

    if (Commissioning_PidAllowed()) {
        if (COMMISSION_STAGE == COMM_STAGE_YAW_TURN &&
            input.quality == CONTROL_FRAME_NORMAL) {
            input.turn_pwm = Commissioning_GetTurnTestPwm(system_millis());
        }
        Control_Update(&input, system_micros());
        if (!Control_IsArmed() && !emergency_stop_latched) {
            ImuStartup_AbortToStabilizing();
        }
    } else {
        Control_SetTargetSpeed(0.0f);
        input.turn_pwm = 0;
    }
    Commissioning_RecordFrame(&sample, last_good_control_pitch_deg,
        last_good_control_roll_deg, last_good_control_yaw_deg, &latch,
        input.encoder_left_delta, input.encoder_right_delta, dt_us);
    Commissioning_RecordControl();
    if (Commissioning_AirProtectionTripped()) latch_emergency_stop();
    control_stats.processed_frames++;
    finish_control_timing(start_us, latch.timestamp_us);
    return true;
}

static void control_watchdog_task(void)
{
    uint32_t now_tick = timer_get_10ms_ticks();

    if (now_tick == watchdog_last_tick) {
        return;
    }
    watchdog_last_tick = now_tick;

    if (Control_IsArmed()) {
        uint32_t last_sample_us = encoder_last_control_timestamp_us();
        if (last_sample_us == 0U ||
            (uint32_t) (system_micros() - last_sample_us) > CONTROL_WATCHDOG_US) {
            mpu6050_request_recovery();
            latch_control_fault(CONTROL_STOP_TIMING_FAULT);
        }
    }
}

static void button_task(uint32_t now_ms)
{
    static bool previous_raw_pressed;
    static bool stable_pressed;
    static uint8_t stable_samples;
    static uint32_t press_start_ms;
    static bool long_press_fired;
    bool raw_pressed = DL_GPIO_readPins(KEY_PIN_21_PORT, KEY_PIN_21_PIN) == 0U;

    if (raw_pressed == previous_raw_pressed) {
        if (stable_samples < 3U) stable_samples++;
    } else {
        stable_samples = 0U;
        previous_raw_pressed = raw_pressed;
    }

    if (stable_samples >= 2U && stable_pressed != raw_pressed) {
        stable_pressed = raw_pressed;
        if (stable_pressed) {
            press_start_ms = now_ms;
            long_press_fired = false;
        } else {
            if (emergency_stop_latched && emergency_wait_release) {
                emergency_wait_release = false;
            } else if (!long_press_fired && !mpu6050_recovery_required()) {
                if (emergency_stop_latched) {
                    acknowledge_emergency_stop();
                } else if (ImuStartup_IsRunning()) {
                    bool balance_ready = Control_IsArmed() &&
                        (uint32_t) (now_ms - balance_running_since_ms) >=
                        TRACE_ARM_DELAY_MS;
                    bool diagnostic_ready = !Control_IsArmed();

                    if (Commissioning_HandleShortPress(now_ms,
                        COMM_STAGE_MOTOR_TEST_ALLOWED ? diagnostic_ready :
                        balance_ready)) {
                        /* Stage-specific action consumed the short press. */
                    } else if (Control_IsArmed() &&
                        Commissioning_TraceControlAllowed()) {
#if (BALANCE_TRACE_ENABLE == 1U)
                        if (balance_ready) {
                            if (BalanceLineFollow_IsEnabled()) {
                                BalanceLineFollow_Stop();
                                Control_SetTargetSpeed(0.0f);
                            } else {
                                float yaw_deg;
                                ImuStartup_GetAngles(NULL, NULL, &yaw_deg);
                                (void) BalanceLineFollow_Start(yaw_deg);
                            }
                        }
#endif
                    } else {
                        Commissioning_NextPage();
                    }
                } else {
                    ImuStartup_HandleButton();
                    (void) service_startup_events();
                }
            }
        }
    }
    if (stable_pressed && !long_press_fired &&
        (uint32_t) (now_ms - press_start_ms) >= EMERGENCY_HOLD_MS) {
        long_press_fired = true;
        latch_emergency_stop();
    }
    button_task_last_ms = now_ms;
}

static void display_task(uint32_t now_ms)
{
    char line[24];
    float pitch;
    float roll;
    float yaw;
    uint32_t progress;
    TraceSensorData trace;
#if (BALANCE_TRACE_ENABLE == 1U)
    BalanceLineFollowOutput line_output;
#endif
    ImuStartupState state = ImuStartup_GetState();

    ImuStartup_GetAngles(&pitch, &roll, &yaw);
    OLED_ClearBuffer();

    if (emergency_stop_latched) {
        OLED_ShowString(0, 0, "EMERGENCY STOP", 12, 1);
        OLED_ShowString(0, 16, "MOTOR OFF", 12, 1);
        OLED_ShowString(0, 32, emergency_wait_release ?
            "RELEASE KEY" : "PRESS TO RESET", 12, 1);
        OLED_ShowString(0, 48, "REZERO REQUIRED", 12, 1);
        OLED_Refresh();
        display_task_last_ms = now_ms;
        return;
    }

    if (COMMISSION_STAGE != COMM_STAGE_RELEASE) {
        Commissioning_Render(now_ms, state);
        display_task_last_ms = now_ms;
        return;
    }

    if (state == IMU_STARTUP_DMP_LOADING) {
        OLED_ShowString(0, 0, "DMP LOADING", 12, 1);
        OLED_ShowString(0, 16, "HOLD UPRIGHT", 12, 1);
        OLED_ShowString(0, 32, "KEEP STILL", 12, 1);
        OLED_ShowString(0, 48, "MOTOR OFF", 12, 1);
    } else if (state == IMU_STARTUP_ERROR) {
        OLED_ShowString(0, 0, "IMU ERROR", 12, 1);
        OLED_ShowString(0, 20, "CHECK WIRING", 12, 1);
        OLED_ShowString(0, 40, "MOTOR OFF", 12, 1);
    } else {
        snprintf(line, sizeof(line), "P:%+7.2f", pitch);
        OLED_ShowString(0, 0, line, 12, 1);
        snprintf(line, sizeof(line), "R:%+7.2f", roll);
        OLED_ShowString(0, 12, line, 12, 1);
        snprintf(line, sizeof(line), "Y:%+7.2f", yaw);
        OLED_ShowString(0, 24, line, 12, 1);
        (void) control_task_10ms();

        if (state == IMU_STARTUP_STABILIZING) {
            progress = ImuStartup_GetStableElapsedMs();
            if (progress > 2000U) progress = 2000U;
            snprintf(line, sizeof(line), "SETTLE %lu.%lu/2.0",
                (unsigned long) (progress / 1000U),
                (unsigned long) ((progress % 1000U) / 100U));
            OLED_ShowString(0, 38, line, 12, 1);
            OLED_ShowString(0, 52, "KEEP STILL", 12, 1);
        } else if (state == IMU_STARTUP_ZERO_SAMPLING) {
            OLED_ShowString(0, 38, "ZERO SAMPLING", 12, 1);
            snprintf(line, sizeof(line), "SAMPLES %03u/200",
                (unsigned int) ImuStartup_GetZeroSampleCount());
            OLED_ShowString(0, 52, line, 12, 1);
        } else if (state == IMU_STARTUP_ZERO_READY) {
            progress = ImuStartup_GetAutoZeroRemainingMs(now_ms);
            OLED_ShowString(0, 38, "PRESS KEY TO ZERO", 12, 1);
            snprintf(line, sizeof(line), "AUTO IN %lu.%luS",
                (unsigned long) (progress / 1000U),
                (unsigned long) ((progress % 1000U) / 100U));
            OLED_ShowString(0, 52, line, 12, 1);
        } else if (state == IMU_STARTUP_ARM_CONFIRM) {
            OLED_ShowString(0, 38, "ZERO COMPLETE", 12, 1);
            OLED_ShowString(0, 52, "CHECKING DATA", 12, 1);
        } else {
            trace = TraceSensor_GetData();
#if (BALANCE_TRACE_ENABLE == 1U)
            line_output = BalanceLineFollow_GetOutput();
#endif
            snprintf(line, sizeof(line), "IR:%X E:%+.2f",
                (unsigned int) trace.filtered, trace.error);
            OLED_ShowString(0, 38, line, 12, 1);
#if (BALANCE_TRACE_ENABLE == 0U)
            snprintf(line, sizeof(line), "LF:LOCKED");
#else
            if (line_output.enabled) {
                snprintf(line, sizeof(line), "LF:ON T:%d", line_output.turn_pwm);
            } else if (line_output.fault) {
                snprintf(line, sizeof(line), "LF:FAULT KEY RETRY");
            } else if ((uint32_t) (now_ms - balance_running_since_ms) <
                TRACE_ARM_DELAY_MS) {
                progress = TRACE_ARM_DELAY_MS -
                    (uint32_t) (now_ms - balance_running_since_ms);
                snprintf(line, sizeof(line), "LF WAIT %lu.%luS",
                    (unsigned long) (progress / 1000U),
                    (unsigned long) ((progress % 1000U) / 100U));
            } else {
                snprintf(line, sizeof(line), "LF:KEY READY");
            }
#endif
            OLED_ShowString(0, 52, line, 12, 1);
        }
    }
    (void) control_task_10ms();
    OLED_Refresh();
    display_task_last_ms = now_ms;
}

static void recovery_task(uint32_t now_ms)
{
    if (!Control_IsArmed() && mpu6050_recovery_required() &&
        (uint32_t) (now_ms - recovery_task_last_ms) >= MPU_RECOVERY_PERIOD_MS) {
        recovery_task_last_ms = now_ms;
        if (mpu6050_recover_if_needed()) {
            reset_control_alignment();
            ImuStartup_AbortToStabilizing();
            control_stats.recoveries++;
        }
    }
}

static void run_soft_tasks(void)
{
    uint32_t now_ms = system_millis();

    recovery_task(now_ms);
    ImuStartup_Update(now_ms);
    (void) service_startup_events();
    if ((uint32_t) (now_ms - button_task_last_ms) >= BUTTON_TASK_PERIOD_MS) {
        button_task(now_ms);
    }
    if (!emergency_stop_latched) Commissioning_Service(now_ms);
    if ((uint32_t) (now_ms - display_task_last_ms) >= DISPLAY_TASK_PERIOD_MS) {
        display_task(now_ms);
    }
    if ((uint32_t) (now_ms - heartbeat_task_last_ms) >= HEARTBEAT_TASK_PERIOD_MS) {
        DL_GPIO_togglePins(LED_PORT, LED_LED1_PIN);
        heartbeat_task_last_ms = now_ms;
    }
}

int main(void)
{
    board_init();
    TB6612_Init();
    Control_Init();
    TraceSensor_Init();
    BalanceLineFollow_Init();
    Commissioning_Init();
    encoder_init();
    timer_init();
    jy61p_init();
    OLED_Init();

    ImuStartup_Init();
    OLED_ClearBuffer();
    OLED_ShowString(0, 0, Commissioning_GetStageTitle(), 12, 1);
    OLED_ShowString(0, 16, "DMP LOADING", 12, 1);
    OLED_ShowString(0, 32, "HOLD UPRIGHT", 12, 1);
    OLED_ShowString(0, 48, "MOTOR OFF", 12, 1);
    OLED_RefreshBlocking();

    mpu_initialized = (MPU6050_Init() == 0);
    ImuStartup_DmpReady(mpu_initialized);
    reset_control_alignment();
    watchdog_last_tick = timer_get_10ms_ticks();
    button_task_last_ms = system_millis();
    display_task_last_ms = system_millis() - DISPLAY_TASK_PERIOD_MS;
    heartbeat_task_last_ms = system_millis();
    recovery_task_last_ms = system_millis();
    emergency_stop_latched = false;
    emergency_wait_release = false;

    while (1) {
        (void) control_task_10ms();
        Display_Poll();
        (void) control_task_10ms();
        (void) jy61p_poll_budget(JY61P_FOREGROUND_BUDGET);
        (void) control_task_10ms();
        control_watchdog_task();
        run_soft_tasks();
        (void) control_task_10ms();

        if (!Display_IsBusy() && !encoder_control_frame_pending()) {
            __WFI();
        }
    }
}
