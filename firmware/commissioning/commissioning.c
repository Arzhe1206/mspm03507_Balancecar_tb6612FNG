/*
 * MSPM0G3507 Balance Car
 * 作者：数显纯牛奶，开源工程未经授权禁止商业用途，需要实物见店铺主页：https://m.tb.cn/h.8nzqEuN?tk=kJqnT4uWzfY
 * 官方仓库：https://github.com/Arzhe1206/mspm03507_Balancecar_tb6612FNG
 * 许可：LicenseRef-BalanceCar-NonCommercial
 * 项目标识：MSPM0-BC-ARZHE1206
 * AI/ML 使用声明：未经授权，不得用于模型训练、数据集构建或商业化自动代码生成。
 */
#include "commissioning.h"
#include "balance_trace_config.h"
#include "tb6612.h"
#include "board_profile.h"
#include "commissioning_config.h"
#include "control_tuning.h"
#include "oled.h"
#include "jy61p.h"
#include "safety_config.h"
#include "system_time.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    MPU6050_Sample sample;
    float pitch_deg;
    float roll_deg;
    float yaw_deg;
    EncoderControlLatch latch;
    int32_t left_delta;
    int32_t right_delta;
    uint32_t dt_us;
    TraceSensorData trace;
    BalanceLineFollowOutput line;
    ControlDiagnostics control;
    float peak_abs_roll;
    float peak_abs_gyro_x;
    int peak_abs_pwm;
    bool frame_valid;
} CommissioningSnapshot;

static CommissioningSnapshot snapshot;
static AirProtectionDiagnostics air_diagnostics;
static bool motor_pulse_active;
static uint32_t motor_pulse_start_ms;
static uint32_t motor_rest_until_ms;
static EncoderCounts motor_pulse_origin;
static EncoderCounts motor_pulse_result;
static bool motor_result_valid;
static bool turn_test_active;
static uint32_t turn_test_start_ms;
static uint8_t manual_page;
static uint32_t peak_window_start_ms;
static float window_peak_abs_roll;
static float window_peak_abs_gyro_x;
static int window_peak_abs_pwm;

static int abs_int(int value)
{
    return value < 0 ? -value : value;
}

static const char *startup_name(ImuStartupState state)
{
    switch (state) {
    case IMU_STARTUP_DMP_LOADING: return "DMP LOAD";
    case IMU_STARTUP_STABILIZING: return "STABILIZE";
    case IMU_STARTUP_ZERO_SAMPLING: return "ZERO SAMPLE";
    case IMU_STARTUP_ZERO_READY: return "ZERO READY";
    case IMU_STARTUP_ARM_CONFIRM: return "ARM CHECK";
    case IMU_STARTUP_RUNNING: return "READY";
    default: return "IMU ERROR";
    }
}

static uint8_t display_page(uint32_t now_ms, uint8_t page_count)
{
    if (page_count == 0U) return 0U;
#if (COMM_OLED_PAGE_MODE == COMM_OLED_PAGE_FIXED)
    return (uint8_t) (COMM_OLED_FIXED_PAGE % page_count);
#else
    return (uint8_t) (((now_ms / COMM_OLED_PAGE_PERIOD_MS) + manual_page) %
        page_count);
#endif
}

static void show_line(uint8_t y, const char *text)
{
    OLED_ShowString(0U, y, text, 12U, 1U);
}

static void render_imu(uint32_t now_ms, ImuStartupState startup_state)
{
    char line[24];
    MPU6050_Diagnostics imu = mpu6050_get_diagnostics();
    JY61P_Sample jy;
    uint8_t page = display_page(now_ms, 3U);

    show_line(0U, "S01 IMU DIAG");
    if (page == 0U) {
        snprintf(line, sizeof(line), "ID:%02X %s",
            (unsigned int) imu.who_am_i,
            imu.identity_valid ? "PASS" : "FAIL");
        show_line(12U, line);
        snprintf(line, sizeof(line), "DMP:%s HZ:%u",
            imu.dmp_ready ? "OK" : "WAIT", imu.sample_rate_hz);
        show_line(24U, line);
        snprintf(line, sizeof(line), "SEQ:%lu ERR:%lu",
            (unsigned long) snapshot.sample.sequence,
            (unsigned long) mpu6050_error_count());
        show_line(38U, line);
        snprintf(line, sizeof(line), "STATE:%s", startup_name(startup_state));
        show_line(52U, line);
    } else if (page == 1U) {
        snprintf(line, sizeof(line), "P:%+6.2f R:%+6.2f",
            snapshot.pitch_deg, snapshot.roll_deg);
        show_line(12U, line);
        snprintf(line, sizeof(line), "Y:%+7.2f", snapshot.yaw_deg);
        show_line(24U, line);
        snprintf(line, sizeof(line), "GX:%+5.1f GZ:%+5.1f",
            snapshot.sample.gyro_dps[0], snapshot.sample.gyro_dps[2]);
        show_line(38U, line);
        snprintf(line, sizeof(line), "PKR:%4.1f PKG:%4.1f",
            snapshot.peak_abs_roll, snapshot.peak_abs_gyro_x);
        show_line(52U, line);
    } else {
        bool valid = jy61p_get_sample(&jy);
        snprintf(line, sizeof(line), "JY61P:%s SEQ:%lu",
            valid ? "OK" : "WAIT", (unsigned long) jy.sequence);
        show_line(12U, line);
        snprintf(line, sizeof(line), "R:%+5.1f P:%+5.1f",
            jy.roll_deg, jy.pitch_deg);
        show_line(24U, line);
        snprintf(line, sizeof(line), "Y:%+6.1f GZ:%+5.1f",
            jy.yaw_deg, jy.gyro_dps[2]);
        show_line(38U, line);
        snprintf(line, sizeof(line), "CRCERR:%lu DIAG ONLY",
            (unsigned long) jy61p_checksum_error_count());
        show_line(52U, line);
    }
}

static void render_trace(void)
{
    char line[24];

    show_line(0U, "S02 TRACE DIAG");
    snprintf(line, sizeof(line), "RAW:%X FIL:%X N:%u",
        snapshot.trace.raw, snapshot.trace.filtered,
        snapshot.trace.active_count);
    show_line(12U, line);
    snprintf(line, sizeof(line), "POS:%+4.1f E:%+.2f",
        snapshot.trace.position, snapshot.trace.error);
    show_line(24U, line);
    snprintf(line, sizeof(line), "STATE:%u LOST:%u",
        (unsigned int) snapshot.trace.state,
        (unsigned int) snapshot.trace.lost_frames);
    show_line(38U, line);
    show_line(52U, "IO1 L -> E POS");
}

static void render_encoder(void)
{
    char line[24];
    EncoderCounts total = encoder_get_total();

    show_line(0U, "S03 ENCODER");
    snprintf(line, sizeof(line), "L:%+8ld",
        (long) total.left);
    show_line(12U, line);
    snprintf(line, sizeof(line), "R:%+8ld",
        (long) total.right);
    show_line(24U, line);
    snprintf(line, sizeof(line), "DL:%+4ld DR:%+4ld",
        (long) snapshot.left_delta, (long) snapshot.right_delta);
    show_line(38U, line);
    show_line(52U, "FWD MUST BE POS");
}

static void render_motor(uint32_t now_ms, uint8_t stage)
{
    char line[24];
    uint8_t page = display_page(now_ms, 2U);
    const char *channel = COMM_TEST_MOTOR_CHANNEL == COMM_MOTOR_LEFT ?
        "LEFT" : (COMM_TEST_MOTOR_CHANNEL == COMM_MOTOR_RIGHT ?
        "RIGHT" : "BOTH");

    show_line(0U, stage == COMM_STAGE_MOTOR_DEADZONE ?
        "S04 MOTOR DZ" : "S05 MOTOR+ENC");
    if (page != 0U) {
        snprintf(line, sizeof(line), "DZ LF:%d LR:%d",
            MOTOR_LEFT_FORWARD_DEADZONE, MOTOR_LEFT_REVERSE_DEADZONE);
        show_line(12U, line);
        snprintf(line, sizeof(line), "DZ RF:%d RR:%d",
            MOTOR_RIGHT_FORWARD_DEADZONE, MOTOR_RIGHT_REVERSE_DEADZONE);
        show_line(24U, line);
        snprintf(line, sizeof(line), "SIGN M:%+d,%+d",
            MOTOR_LEFT_SIGN, MOTOR_RIGHT_SIGN);
        show_line(38U, line);
        show_line(52U, MOTOR_DEADZONE_ENABLE ? "DEADZONE:ON" :
            "DEADZONE:OFF");
        return;
    }
    snprintf(line, sizeof(line), "%s %s PWM:%d", channel,
        COMM_TEST_MOTOR_DIRECTION > 0 ? "FWD" : "REV",
        COMM_TEST_MOTOR_PWM);
    show_line(12U, line);
    snprintf(line, sizeof(line), "PULSE:%s",
        motor_pulse_active ? "ACTIVE" : "READY");
    show_line(24U, line);
    if (motor_result_valid) {
        snprintf(line, sizeof(line), "DL:%+ld DR:%+ld",
            (long) motor_pulse_result.left,
            (long) motor_pulse_result.right);
        show_line(38U, line);
        if (stage == COMM_STAGE_MOTOR_ENCODER) {
            bool left_ok = COMM_TEST_MOTOR_CHANNEL == COMM_MOTOR_RIGHT ||
                (COMM_TEST_MOTOR_DIRECTION > 0 ?
                 motor_pulse_result.left > 0 : motor_pulse_result.left < 0);
            bool right_ok = COMM_TEST_MOTOR_CHANNEL == COMM_MOTOR_LEFT ||
                (COMM_TEST_MOTOR_DIRECTION > 0 ?
                 motor_pulse_result.right > 0 : motor_pulse_result.right < 0);
            show_line(52U, left_ok && right_ok ? "DIR:PASS" : "DIR:REVERSE");
        } else {
            show_line(52U, (motor_pulse_result.left != 0 ||
                motor_pulse_result.right != 0) ? "MOVE:YES" : "MOVE:NO");
        }
    } else {
        show_line(38U, "SHORT KEY=PULSE");
        show_line(52U, "HOLD KEY=STOP");
    }
}

static void render_balance(uint32_t now_ms, uint8_t stage)
{
    char line[24];
    uint8_t page = display_page(now_ms, 2U);
    int preview_pwm = (int) (CONTROL_VERTICAL_KP *
        (snapshot.roll_deg - CONTROL_BALANCE_TRIM_DEG) +
        CONTROL_VERTICAL_KD * snapshot.sample.gyro_dps[0]);

    if (stage == COMM_STAGE_BALANCE_DIRECTION)
        show_line(0U, "S06 BAL DIR");
    else if (stage == COMM_STAGE_MECHANICAL_ZERO)
        show_line(0U, "S07 MECH ZERO");
    else if (stage == COMM_STAGE_BALANCE_PD)
        show_line(0U, "S08 BALANCE PD");
    else
        show_line(0U, "S09 SPEED PI");

    if (page == 0U) {
        snprintf(line, sizeof(line), "R:%+6.2f GX:%+5.1f",
            snapshot.roll_deg, snapshot.sample.gyro_dps[0]);
        show_line(12U, line);
        if (stage == COMM_STAGE_BALANCE_DIRECTION) {
            snprintf(line, sizeof(line), "PRE:%+d MOTOR:OFF", preview_pwm);
        } else {
            snprintf(line, sizeof(line), "VP:%+d M:%+d",
                Vertical_out, snapshot.control.common_pwm);
        }
        show_line(24U, line);
        snprintf(line, sizeof(line), "DL:%+ld DR:%+ld",
            (long) snapshot.left_delta, (long) snapshot.right_delta);
        show_line(38U, line);
        snprintf(line, sizeof(line), "PKR:%3.1f PKP:%d",
            snapshot.peak_abs_roll, snapshot.peak_abs_pwm);
        show_line(52U, line);
    } else {
        snprintf(line, sizeof(line), "TRIM:%+.2f", CONTROL_BALANCE_TRIM_DEG);
        show_line(12U, line);
        snprintf(line, sizeof(line), "VKP:%+.1f KD:%+.2f",
            CONTROL_VERTICAL_KP, CONTROL_VERTICAL_KD);
        show_line(24U, line);
        snprintf(line, sizeof(line), "VE:%+.1f VI:%+.1f",
            snapshot.control.velocity_filtered_error,
            snapshot.control.velocity_integral_seconds);
        show_line(38U, line);
        snprintf(line, sizeof(line), "SAT:%lu STATE:%u",
            (unsigned long) snapshot.control.saturation_frames,
            (unsigned int) Control_GetState());
        show_line(52U, line);
    }
}

static void render_yaw(void)
{
    char line[24];

    show_line(0U, "S10 YAW TURN");
    snprintf(line, sizeof(line), "Y:%+7.2f GZ:%+5.1f",
        snapshot.yaw_deg, snapshot.sample.gyro_dps[2]);
    show_line(12U, line);
    snprintf(line, sizeof(line), "TURN:%+d %s", COMM_TURN_TEST_PWM,
        turn_test_active ? "ACTIVE" : "READY");
    show_line(24U, line);
    snprintf(line, sizeof(line), "M:%+d,%+d", MOTO1, MOTO2);
    show_line(38U, line);
    show_line(52U, "SHORT KEY=TURN");
}

static void render_line_follow(void)
{
    char line[24];

    show_line(0U, "S11 TRACE TUNE");
    snprintf(line, sizeof(line), "IR:%X E:%+.2f",
        snapshot.trace.filtered, snapshot.trace.error);
    show_line(12U, line);
    snprintf(line, sizeof(line), "YR:%+5.1f YT:%+6.1f",
        snapshot.line.yaw_rate_target_dps, snapshot.line.yaw_target_deg);
    show_line(24U, line);
    snprintf(line, sizeof(line), "TURN:%+d LOST:%u",
        snapshot.line.turn_pwm, snapshot.line.lost_frames);
    show_line(38U, line);
    snprintf(line, sizeof(line), "LF:%s SPD:%+.1f",
        snapshot.line.enabled ? "ON" : "OFF",
        snapshot.line.forward_target_counts);
    show_line(52U, line);
}

static void render_air(void)
{
    char line[24];

    show_line(0U, "S12 AIR CHECK");
    snprintf(line, sizeof(line), "SPD:%+.1f TGT:%+.1f",
        air_diagnostics.common_speed_counts,
        air_diagnostics.target_speed_counts);
    show_line(12U, line);
    snprintf(line, sizeof(line), "ACC:%4.2f E:%u",
        air_diagnostics.accel_norm_g,
        (unsigned int) air_diagnostics.evidence_bits);
    show_line(24U, line);
    snprintf(line, sizeof(line), "TIME:%lums %s",
        (unsigned long) air_diagnostics.evidence_ms,
        air_diagnostics.tripped ? "TRIP" : "OK");
    show_line(38U, line);
#if (AIR_PROTECTION_ENABLE == 1U)
    show_line(52U, "PROTECT:ON");
#else
    show_line(52U, "PROTECT:SHADOW");
#endif
}

void Commissioning_Init(void)
{
    memset(&snapshot, 0, sizeof(snapshot));
    memset(&air_diagnostics, 0, sizeof(air_diagnostics));
    motor_pulse_active = false;
    motor_pulse_start_ms = 0U;
    motor_rest_until_ms = 0U;
    motor_result_valid = false;
    turn_test_active = false;
    turn_test_start_ms = 0U;
    manual_page = 0U;
    peak_window_start_ms = 0U;
    window_peak_abs_roll = 0.0f;
    window_peak_abs_gyro_x = 0.0f;
    window_peak_abs_pwm = 0;
}

const char *Commissioning_GetStageTitle(void)
{
    switch (COMMISSION_STAGE) {
    case COMM_STAGE_IMU_DIAGNOSTIC: return "S01 IMU DIAG";
    case COMM_STAGE_TRACE_DIAGNOSTIC: return "S02 TRACE DIAG";
    case COMM_STAGE_ENCODER_MANUAL: return "S03 ENCODER";
    case COMM_STAGE_MOTOR_DEADZONE: return "S04 MOTOR DZ";
    case COMM_STAGE_MOTOR_ENCODER: return "S05 MOTOR+ENC";
    case COMM_STAGE_BALANCE_DIRECTION: return "S06 BAL DIR";
    case COMM_STAGE_MECHANICAL_ZERO: return "S07 MECH ZERO";
    case COMM_STAGE_BALANCE_PD: return "S08 BALANCE PD";
    case COMM_STAGE_SPEED_PI: return "S09 SPEED PI";
    case COMM_STAGE_YAW_TURN: return "S10 YAW TURN";
    case COMM_STAGE_TRACE_TUNING: return "S11 TRACE TUNE";
    case COMM_STAGE_AIR_PROTECTION: return "S12 AIR CHECK";
    default: return "S13 RELEASE";
    }
}

bool Commissioning_PidAllowed(void)
{
    return COMM_STAGE_PID_ALLOWED;
}

bool Commissioning_TraceControlAllowed(void)
{
    return COMM_STAGE_TRACE_CONTROL_ALLOWED;
}

void Commissioning_RecordFrame(const MPU6050_Sample *sample,
    float pitch_deg, float roll_deg, float yaw_deg,
    const EncoderControlLatch *latch, int32_t left_delta,
    int32_t right_delta, uint32_t dt_us)
{
    float common_speed;
    float accel_x;
    float accel_y;
    float accel_z;
    bool speed_high;
    bool target_mismatch;
    bool body_still;

    if (sample == NULL || latch == NULL) return;
    snapshot.sample = *sample;
    snapshot.pitch_deg = pitch_deg;
    snapshot.roll_deg = roll_deg;
    snapshot.yaw_deg = yaw_deg;
    snapshot.latch = *latch;
    snapshot.left_delta = left_delta;
    snapshot.right_delta = right_delta;
    snapshot.dt_us = dt_us;
    snapshot.trace = TraceSensor_GetData();
    snapshot.line = BalanceLineFollow_GetOutput();
    snapshot.control = Control_GetDiagnostics();
    snapshot.frame_valid = sample->valid;
    if (peak_window_start_ms == 0U)
        peak_window_start_ms = sample->timestamp_ms;
    if ((uint32_t) (sample->timestamp_ms - peak_window_start_ms) >= 1000U) {
        snapshot.peak_abs_roll = window_peak_abs_roll;
        snapshot.peak_abs_gyro_x = window_peak_abs_gyro_x;
        snapshot.peak_abs_pwm = window_peak_abs_pwm;
        window_peak_abs_roll = 0.0f;
        window_peak_abs_gyro_x = 0.0f;
        window_peak_abs_pwm = 0;
        peak_window_start_ms = sample->timestamp_ms;
    }
    if (fabsf(roll_deg) > window_peak_abs_roll)
        window_peak_abs_roll = fabsf(roll_deg);
    if (fabsf(sample->gyro_dps[0]) > window_peak_abs_gyro_x)
        window_peak_abs_gyro_x = fabsf(sample->gyro_dps[0]);
    if (abs_int(MOTO1) > window_peak_abs_pwm)
        window_peak_abs_pwm = abs_int(MOTO1);
    if (abs_int(MOTO2) > window_peak_abs_pwm)
        window_peak_abs_pwm = abs_int(MOTO2);

    if (dt_us == 0U) return;
    common_speed = ((float) left_delta + (float) right_delta) * 0.5f *
        (10000.0f / (float) dt_us);
    accel_x = sample->accel_raw[0] / 16384.0f;
    accel_y = sample->accel_raw[1] / 16384.0f;
    accel_z = sample->accel_raw[2] / 16384.0f;
    air_diagnostics.common_speed_counts = common_speed;
    air_diagnostics.target_speed_counts = snapshot.control.target_speed_counts;
    air_diagnostics.accel_norm_g = sqrtf(accel_x * accel_x +
        accel_y * accel_y + accel_z * accel_z);
    speed_high = fabsf(common_speed) >= AIR_COMMON_SPEED_THRESHOLD_COUNTS;
    target_mismatch = fabsf(common_speed) >=
        fabsf(snapshot.control.target_speed_counts) + AIR_TARGET_MARGIN_COUNTS &&
        abs_int(snapshot.control.common_pwm) >= AIR_MIN_COMMON_PWM;
    body_still = fabsf(roll_deg) <= AIR_STILL_ROLL_DEG &&
        fabsf(sample->gyro_dps[0]) <= AIR_STILL_GYRO_DPS &&
        fabsf(air_diagnostics.accel_norm_g - 1.0f) <=
            AIR_ACCEL_NORM_TOLERANCE_G;
    air_diagnostics.evidence_bits = (uint8_t)
        ((speed_high ? 1U : 0U) |
         (target_mismatch ? 2U : 0U) |
         (body_still ? 4U : 0U));
    if (speed_high && target_mismatch && body_still) {
        air_diagnostics.evidence_ms += dt_us / 1000U;
    } else {
        air_diagnostics.evidence_ms = 0U;
    }
#if (AIR_PROTECTION_ENABLE == 1U)
    if ((COMMISSION_STAGE == COMM_STAGE_AIR_PROTECTION ||
         COMMISSION_STAGE == COMM_STAGE_RELEASE) &&
        air_diagnostics.evidence_ms >= AIR_CONFIRM_MS) {
        air_diagnostics.tripped = true;
    }
#endif
}

void Commissioning_RecordControl(void)
{
    snapshot.control = Control_GetDiagnostics();
    snapshot.line = BalanceLineFollow_GetOutput();
}

bool Commissioning_HandleShortPress(uint32_t now_ms, bool system_ready)
{
    if (!system_ready) return false;
    if (COMM_STAGE_MOTOR_TEST_ALLOWED) {
        if (motor_pulse_active ||
            (int32_t) (now_ms - motor_rest_until_ms) < 0) return true;
        motor_pulse_origin = encoder_get_total();
        motor_pulse_start_ms = now_ms;
        motor_pulse_active = true;
        motor_result_valid = false;
        return true;
    }
    if (COMMISSION_STAGE == COMM_STAGE_YAW_TURN) {
        turn_test_active = true;
        turn_test_start_ms = now_ms;
        return true;
    }
    return false;
}

void Commissioning_NextPage(void)
{
    manual_page++;
}

void Commissioning_Service(uint32_t now_ms)
{
    int command;
    int left = 0;
    int right = 0;

    if (!motor_pulse_active) return;
    if ((uint32_t) (now_ms - motor_pulse_start_ms) >= COMM_TEST_PULSE_MS) {
        EncoderCounts total = encoder_get_total();
        TB6612_Stop();
        motor_pulse_result.left = total.left - motor_pulse_origin.left;
        motor_pulse_result.right = total.right - motor_pulse_origin.right;
        motor_result_valid = true;
        motor_pulse_active = false;
        motor_rest_until_ms = now_ms + COMM_TEST_REST_MS;
        return;
    }
    command = COMM_TEST_MOTOR_DIRECTION * COMM_TEST_MOTOR_PWM;
    if (COMM_TEST_MOTOR_CHANNEL == COMM_MOTOR_LEFT ||
        COMM_TEST_MOTOR_CHANNEL == COMM_MOTOR_BOTH) left = command;
    if (COMM_TEST_MOTOR_CHANNEL == COMM_MOTOR_RIGHT ||
        COMM_TEST_MOTOR_CHANNEL == COMM_MOTOR_BOTH) right = command;
    Set_Motor_PWM(left, right);
}

void Commissioning_StopOutputs(void)
{
    motor_pulse_active = false;
    turn_test_active = false;
    TB6612_Stop();
}

int Commissioning_GetTurnTestPwm(uint32_t now_ms)
{
    if (!turn_test_active) return 0;
    if ((uint32_t) (now_ms - turn_test_start_ms) >= COMM_TURN_TEST_MS) {
        turn_test_active = false;
        return 0;
    }
    return COMM_TURN_TEST_PWM;
}

bool Commissioning_AirProtectionTripped(void)
{
    return air_diagnostics.tripped;
}

AirProtectionDiagnostics Commissioning_GetAirDiagnostics(void)
{
    return air_diagnostics;
}

void Commissioning_Render(uint32_t now_ms, ImuStartupState startup_state)
{
    OLED_ClearBuffer();
    switch (COMMISSION_STAGE) {
    case COMM_STAGE_IMU_DIAGNOSTIC:
        render_imu(now_ms, startup_state);
        break;
    case COMM_STAGE_TRACE_DIAGNOSTIC:
        render_trace();
        break;
    case COMM_STAGE_ENCODER_MANUAL:
        render_encoder();
        break;
    case COMM_STAGE_MOTOR_DEADZONE:
    case COMM_STAGE_MOTOR_ENCODER:
        render_motor(now_ms, COMMISSION_STAGE);
        break;
    case COMM_STAGE_BALANCE_DIRECTION:
    case COMM_STAGE_MECHANICAL_ZERO:
    case COMM_STAGE_BALANCE_PD:
    case COMM_STAGE_SPEED_PI:
        render_balance(now_ms, COMMISSION_STAGE);
        break;
    case COMM_STAGE_YAW_TURN:
        render_yaw();
        break;
    case COMM_STAGE_TRACE_TUNING:
        render_line_follow();
        break;
    case COMM_STAGE_AIR_PROTECTION:
        render_air();
        break;
    default:
        show_line(0U, "S13 RELEASE");
        show_line(16U, startup_name(startup_state));
        show_line(32U, "USE NORMAL SCREEN");
        break;
    }
    OLED_Refresh();
}
