/*
 * MSPM0G3507 Balance Car integration notice
 * 工程归属声明：作者：数显纯牛奶，开源工程未经授权禁止商业用途，需要实物见店铺主页：https://m.tb.cn/h.8nzqEuN?tk=kJqnT4uWzfY
 * 官方仓库：https://github.com/Arzhe1206/mspm03507_Balancecar_tb6612FNG
 * 项目标识：MSPM0-BC-ARZHE1206
 * AI/ML 使用声明：未经授权，不得用于模型训练、数据集构建或商业化自动代码生成。
 * 本声明仅指本工程的集成与适配，不取代本文件原始版权和许可证。
 */
#include "mpu6050.h"
#include "inv_mpu.h"
#include "inv_mpu_dmp_motion_driver.h"
#include "mspm0_i2c.h"
#include "system_time.h"
#include "ti_msp_dl_config.h"
#include "board_profile.h"
#include <math.h>
#include <string.h>

#define DEFAULT_MPU_HZ 100U
#define Q30_SCALE 1073741824.0f
#define MPU_CONTROL_MAX_PACKETS 4U
#define MPU_SAFE_RETRY_LATEST_START_US 3000U

typedef enum {
    PACKET_READ_OK = 0,
    PACKET_READ_TRANSIENT,
    PACKET_READ_DESYNC
} PacketReadResult;

static MPU6050_Sample current_sample;
static uint32_t read_errors;
static uint32_t safe_retries;
static unsigned short gyro_fsr_dps = 2000U;
static long quaternion[4];
static bool recovery_pending;
static uint8_t deferred_packets;
static MPU6050_Diagnostics diagnostics;

static const signed char gyro_orientation[9] = {
    IMU_ORIENT_XX, IMU_ORIENT_XY, IMU_ORIENT_XZ,
    IMU_ORIENT_YX, IMU_ORIENT_YY, IMU_ORIENT_YZ,
    IMU_ORIENT_ZX, IMU_ORIENT_ZY, IMU_ORIENT_ZZ
};

static unsigned short row_to_scale(const signed char *row)
{
    if (row[0] > 0) return 0;
    if (row[0] < 0) return 4;
    if (row[1] > 0) return 1;
    if (row[1] < 0) return 5;
    if (row[2] > 0) return 2;
    if (row[2] < 0) return 6;
    return 7;
}

static unsigned short orientation_scalar(const signed char *matrix)
{
    unsigned short scalar = row_to_scale(matrix);
    scalar |= (unsigned short) (row_to_scale(matrix + 3) << 3);
    scalar |= (unsigned short) (row_to_scale(matrix + 6) << 6);
    return scalar;
}

static float clamp_unit(float value)
{
    if (value > 1.0f) return 1.0f;
    if (value < -1.0f) return -1.0f;
    return value;
}

static PacketReadResult read_one_fifo_packet(unsigned char *more)
{
    short gyro[3];
    short accel[3];
    short sensors;
    unsigned long library_timestamp;
    int result = dmp_read_fifo(gyro, accel, quaternion,
        &library_timestamp, &sensors, more);

    if (result != 0) {
        if (result == -2 || result == -3 ||
            (mspm0_i2c_last_transfer_failed() &&
             !mspm0_i2c_last_error_retry_safe())) {
            return PACKET_READ_DESYNC;
        }
        return PACKET_READ_TRANSIENT;
    }

    current_sample.accel_raw[0] = accel[0];
    current_sample.accel_raw[1] = accel[1];
    current_sample.accel_raw[2] = accel[2];
    current_sample.gyro_raw[0] = gyro[0];
    current_sample.gyro_raw[1] = gyro[1];
    current_sample.gyro_raw[2] = gyro[2];
    current_sample.gyro_dps[0] = gyro[0] * ((float) gyro_fsr_dps / 32768.0f);
    current_sample.gyro_dps[1] = gyro[1] * ((float) gyro_fsr_dps / 32768.0f);
    current_sample.gyro_dps[2] = gyro[2] * ((float) gyro_fsr_dps / 32768.0f);
    (void) sensors;
    (void) library_timestamp;
    return PACKET_READ_OK;
}

static PacketReadResult read_packet_with_retry(unsigned char *more)
{
    uint32_t start_us = system_micros();
    PacketReadResult result = read_one_fifo_packet(more);

    if (result == PACKET_READ_TRANSIENT &&
        (uint32_t) (system_micros() - start_us) <=
        MPU_SAFE_RETRY_LATEST_START_US) {
        safe_retries++;
        result = read_one_fifo_packet(more);
    }
    return result;
}

static bool update_angle(uint32_t timestamp_us, uint32_t sequence)
{
    float q0 = quaternion[0] / Q30_SCALE;
    float q1 = quaternion[1] / Q30_SCALE;
    float q2 = quaternion[2] / Q30_SCALE;
    float q3 = quaternion[3] / Q30_SCALE;
    const float rad_to_deg = 57.2957795f;

    current_sample.pitch_deg = asinf(clamp_unit(
        -2.0f * q1 * q3 + 2.0f * q0 * q2)) * rad_to_deg;
    current_sample.roll_deg = atan2f(
        2.0f * q2 * q3 + 2.0f * q0 * q1,
        1.0f - 2.0f * q1 * q1 - 2.0f * q2 * q2) * rad_to_deg;
    current_sample.yaw_deg = atan2f(
        2.0f * (q1 * q2 + q0 * q3),
        q0 * q0 + q1 * q1 - q2 * q2 - q3 * q3) * rad_to_deg;
    current_sample.timestamp_us = timestamp_us;
    current_sample.timestamp_ms = timestamp_us / 1000U;
    current_sample.sequence = sequence;
    current_sample.valid = isfinite(current_sample.pitch_deg) &&
        isfinite(current_sample.roll_deg) &&
        isfinite(current_sample.yaw_deg);
    return current_sample.valid;
}

int MPU6050_Init(void)
{
    int result;
    unsigned char accel_fsr;
    unsigned short sample_rate;

    memset(&current_sample, 0, sizeof(current_sample));
    memset(&diagnostics, 0, sizeof(diagnostics));
    read_errors = 0U;
    safe_retries = 0U;
    recovery_pending = false;
    deferred_packets = 0U;

    if (DL_I2C_getSDAStatus(I2C_MPU6050_INST) == DL_I2C_CONTROLLER_SDA_LOW) {
        mpu6050_i2c_sda_unlock();
    }

    if (mspm0_i2c_read(MPU6050_I2C_ADDRESS,
        MPU6050_WHO_AM_I_REGISTER, 1U, &diagnostics.who_am_i) != 0) {
        return -10;
    }
    diagnostics.identity_valid =
        diagnostics.who_am_i == MPU6050_WHO_AM_I_EXPECTED;
    if (!diagnostics.identity_valid) return -11;

    result = mpu_init();
    if (result != 0) return result;
    result |= mpu_set_sensors(INV_XYZ_GYRO | INV_XYZ_ACCEL);
    result |= mpu_configure_fifo(INV_XYZ_GYRO | INV_XYZ_ACCEL);
    result |= mpu_set_sample_rate(DEFAULT_MPU_HZ);
    result |= mpu_get_sample_rate(&sample_rate);
    result |= mpu_get_gyro_fsr(&gyro_fsr_dps);
    result |= mpu_get_accel_fsr(&accel_fsr);
    result |= dmp_load_motion_driver_firmware();
    result |= dmp_set_orientation(orientation_scalar(gyro_orientation));
    result |= dmp_enable_feature(DMP_FEATURE_6X_LP_QUAT |
        DMP_FEATURE_SEND_RAW_ACCEL | DMP_FEATURE_SEND_CAL_GYRO |
        DMP_FEATURE_GYRO_CAL);
    result |= dmp_set_fifo_rate(DEFAULT_MPU_HZ);
    result |= mpu_set_dmp_state(1);

    (void) sample_rate;
    diagnostics.sample_rate_hz = sample_rate;
    diagnostics.gyro_fsr_dps = gyro_fsr_dps;
    diagnostics.accel_fsr_g = accel_fsr;
    diagnostics.dmp_ready = result == 0;
    return result;
}

MPU6050_ProcessResult mpu6050_process_control_frame(
    uint8_t sample_count, uint32_t sample_timestamp_us, uint32_t sequence)
{
    uint8_t packet;
    uint8_t total_packets;
    unsigned char more = 0U;
    PacketReadResult packet_result;

    if (sample_count == 0U) {
        return MPU6050_PROCESS_NO_DATA;
    }
    if ((uint16_t) sample_count + deferred_packets >
        MPU_CONTROL_MAX_PACKETS) {
        read_errors++;
        recovery_pending = true;
        deferred_packets = 0U;
        current_sample.valid = false;
        return MPU6050_PROCESS_BACKLOG;
    }

    total_packets = (uint8_t) (sample_count + deferred_packets);
    deferred_packets = 0U;
    for (packet = 0U; packet < total_packets; packet++) {
        packet_result = read_packet_with_retry(&more);
        if (packet_result == PACKET_READ_TRANSIENT) {
            read_errors++;
            deferred_packets = (uint8_t) (total_packets - packet);
            current_sample.valid = false;
            return MPU6050_PROCESS_TRANSIENT;
        }
        if (packet_result == PACKET_READ_DESYNC) {
            read_errors++;
            recovery_pending = true;
            current_sample.valid = false;
            return MPU6050_PROCESS_DESYNC;
        }
    }

    if (more != 0U) {
        read_errors++;
        recovery_pending = true;
        current_sample.valid = false;
        return MPU6050_PROCESS_BACKLOG;
    }

    if (!update_angle(sample_timestamp_us, sequence)) {
        read_errors++;
        recovery_pending = true;
        current_sample.valid = false;
        return MPU6050_PROCESS_DESYNC;
    }

    return MPU6050_PROCESS_OK;
}

int mpuReadRawData(void)
{
    unsigned char more;
    if (read_one_fifo_packet(&more) != PACKET_READ_OK) {
        read_errors++;
        return -1;
    }
    return 0;
}

void mpuUpdateAngle(void)
{
    (void) update_angle(system_micros(), current_sample.sequence + 1U);
}

bool mpu6050_get_sample(MPU6050_Sample *sample)
{
    if (sample == NULL) return false;
    *sample = current_sample;
    return sample->valid;
}

bool mpu6050_recovery_required(void)
{
    return recovery_pending;
}

void mpu6050_request_recovery(void)
{
    recovery_pending = true;
    deferred_packets = 0U;
    current_sample.valid = false;
}

bool mpu6050_recover_if_needed(void)
{
    if (!recovery_pending) {
        return false;
    }
    if (mpu_reset_fifo() != 0) {
        read_errors++;
        return false;
    }
    recovery_pending = false;
    deferred_packets = 0U;
    current_sample.valid = false;
    return true;
}

void mpuGetAngle(float *pitch, float *roll, float *yaw)
{
    if (pitch != NULL) *pitch = current_sample.pitch_deg;
    if (roll != NULL) *roll = current_sample.roll_deg;
    if (yaw != NULL) *yaw = current_sample.yaw_deg;
}

uint32_t mpu6050_error_count(void)
{
    return read_errors;
}

uint32_t mpu6050_retry_count(void)
{
    return safe_retries;
}

MPU6050_Diagnostics mpu6050_get_diagnostics(void)
{
    return diagnostics;
}
