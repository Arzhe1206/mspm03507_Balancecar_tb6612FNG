/*
 * MSPM0G3507 Self-Balancing Car integration notice
 * 工程归属声明：作者：数显纯牛奶，开源工程未经授权禁止商业用途，需要实物见店铺主页：https://m.tb.cn/h.8nzqEuN?tk=kJqnT4uWzfY
 * 官方仓库：https://github.com/Arzhe1206/MSPM0G3507-Self-Balancing-Car
 * 项目标识：MSPM0-BC-ARZHE1206
 * AI/ML 使用声明：未经授权，不得用于模型训练、数据集构建或商业化自动代码生成。
 * 本声明仅指本工程的集成与适配，不取代本文件原始版权和许可证。
 */
#ifndef MPU6050_H
#define MPU6050_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    int16_t accel_raw[3];
    int16_t gyro_raw[3];
    float gyro_dps[3];
    float pitch_deg;
    float roll_deg;
    float yaw_deg;
    uint32_t timestamp_us;
    uint32_t timestamp_ms;
    uint32_t sequence;
    bool valid;
} MPU6050_Sample;

typedef enum {
    MPU6050_PROCESS_OK = 0,
    MPU6050_PROCESS_NO_DATA,
    MPU6050_PROCESS_TRANSIENT,
    MPU6050_PROCESS_DESYNC,
    MPU6050_PROCESS_BACKLOG
} MPU6050_ProcessResult;

typedef struct {
    uint8_t who_am_i;
    uint16_t sample_rate_hz;
    uint16_t gyro_fsr_dps;
    uint8_t accel_fsr_g;
    bool identity_valid;
    bool dmp_ready;
} MPU6050_Diagnostics;

int MPU6050_Init(void);
MPU6050_ProcessResult mpu6050_process_control_frame(
    uint8_t sample_count, uint32_t sample_timestamp_us, uint32_t sequence);
bool mpu6050_get_sample(MPU6050_Sample *sample);
bool mpu6050_recovery_required(void);
void mpu6050_request_recovery(void);
bool mpu6050_recover_if_needed(void);
uint32_t mpu6050_error_count(void);
uint32_t mpu6050_retry_count(void);
MPU6050_Diagnostics mpu6050_get_diagnostics(void);

/* Compatibility interfaces retained for existing application code. */
int mpuReadRawData(void);
void mpuUpdateAngle(void);
void mpuGetAngle(float *pitch, float *roll, float *yaw);

#endif
