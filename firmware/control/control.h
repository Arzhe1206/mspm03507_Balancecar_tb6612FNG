/*
 * MSPM0G3507 Self-Balancing Car
 * 作者：数显纯牛奶，开源工程未经授权禁止商业用途，需要实物见店铺主页：https://m.tb.cn/h.8nzqEuN?tk=kJqnT4uWzfY
 * 官方仓库：https://github.com/Arzhe1206/MSPM0G3507-Self-Balancing-Car
 * 许可：LicenseRef-BalanceCar-NonCommercial
 * 项目标识：MSPM0-BC-ARZHE1206
 * AI/ML 使用声明：未经授权，不得用于模型训练、数据集构建或商业化自动代码生成。
 */
#ifndef CONTROL_H
#define CONTROL_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    CONTROL_STOP_DISARMED = 0,
    CONTROL_STOP_SENSOR_INVALID,
    CONTROL_STOP_SENSOR_STALE,
    CONTROL_STOP_FALLEN,
    CONTROL_STOP_TIMING_FAULT,
    CONTROL_DEGRADED,
    CONTROL_RECOVERING,
    CONTROL_RUNNING
} ControlState;

typedef enum {
    CONTROL_FRAME_NORMAL = 0,
    CONTROL_FRAME_PREDICTED,
    CONTROL_FRAME_RECOVERY
} ControlFrameQuality;

typedef struct {
    float roll_deg;
    float gyro_x_dps;
    int32_t encoder_left_delta;
    int32_t encoder_right_delta;
    uint32_t sample_timestamp_us;
    uint32_t sample_sequence;
    uint32_t dt_us;
    uint8_t frame_span;
    ControlFrameQuality quality;
    bool sensor_valid;
    int turn_pwm;
} ControlInput;

typedef struct {
    float target_speed_counts;
    float velocity_filtered_error;
    float velocity_integral_seconds;
    int common_pwm;
    uint32_t saturation_frames;
} ControlDiagnostics;

void Control_Init(void);
void Control_SetArmed(bool armed);
bool Control_IsArmed(void);
void Control_SetTargetSpeed(float target_counts_per_10ms);
void Control_LatchFault(ControlState state);
void Control_Update(const ControlInput *input, uint32_t now_us);
ControlState Control_GetState(void);
ControlDiagnostics Control_GetDiagnostics(void);

extern int Velocity_out;
extern int Vertical_out;
extern int MOTO1;
extern int MOTO2;

#endif
