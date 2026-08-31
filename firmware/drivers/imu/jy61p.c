/*
 * MSPM0G3507 Balance Car
 * 作者：数显纯牛奶，开源工程未经授权禁止商业用途，需要实物见店铺主页：https://m.tb.cn/h.8nzqEuN?tk=kJqnT4uWzfY
 * 官方仓库：https://github.com/Arzhe1206/mspm03507_Balancecar_tb6612FNG
 * 许可：LicenseRef-BalanceCar-NonCommercial
 * 项目标识：MSPM0-BC-ARZHE1206
 * AI/ML 使用声明：未经授权，不得用于模型训练、数据集构建或商业化自动代码生成。
 */
#include "jy61p.h"
#include "board.h"
#include "system_time.h"
#include <string.h>

static uint8_t frame[11];
static uint8_t frame_index;
static JY61P_Sample current_sample;
static uint32_t checksum_errors;
static bool have_gyro;
static bool have_angle;

float wx, wy, wz;
float Roll, Pitch, Yaw;

static int16_t decode_i16(uint8_t low, uint8_t high)
{
    return (int16_t) (((uint16_t) high << 8) | low);
}

static bool checksum_valid(const uint8_t *data)
{
    uint8_t checksum = 0U;
    uint8_t i;
    for (i = 0U; i < 10U; i++) {
        checksum = (uint8_t) (checksum + data[i]);
    }
    return checksum == data[10];
}

static void decode_frame(void)
{
    const float raw_scale = 1.0f / 32768.0f;

    if (!checksum_valid(frame)) {
        checksum_errors++;
        return;
    }

    switch (frame[1]) {
    case 0x51:
        current_sample.accel_g[0] = decode_i16(frame[2], frame[3]) * raw_scale * 16.0f;
        current_sample.accel_g[1] = decode_i16(frame[4], frame[5]) * raw_scale * 16.0f;
        current_sample.accel_g[2] = decode_i16(frame[6], frame[7]) * raw_scale * 16.0f;
        break;
    case 0x52:
        current_sample.gyro_dps[0] = decode_i16(frame[2], frame[3]) * raw_scale * 2000.0f;
        current_sample.gyro_dps[1] = decode_i16(frame[4], frame[5]) * raw_scale * 2000.0f;
        current_sample.gyro_dps[2] = decode_i16(frame[6], frame[7]) * raw_scale * 2000.0f;
        wx = current_sample.gyro_dps[0];
        wy = current_sample.gyro_dps[1];
        wz = current_sample.gyro_dps[2];
        have_gyro = true;
        break;
    case 0x53:
        current_sample.roll_deg = decode_i16(frame[2], frame[3]) * raw_scale * 180.0f;
        current_sample.pitch_deg = decode_i16(frame[4], frame[5]) * raw_scale * 180.0f;
        current_sample.yaw_deg = decode_i16(frame[6], frame[7]) * raw_scale * 180.0f;
        Roll = current_sample.roll_deg;
        Pitch = current_sample.pitch_deg;
        Yaw = current_sample.yaw_deg;
        have_angle = true;
        current_sample.timestamp_ms = system_millis();
        current_sample.sequence++;
        current_sample.valid = have_gyro && have_angle;
        break;
    default:
        break;
    }
}

static void consume_byte(uint8_t byte)
{
    if (frame_index == 0U) {
        if (byte == 0x55U) {
            frame[frame_index++] = byte;
        }
        return;
    }

    if (frame_index == 1U && (byte < 0x50U || byte > 0x59U)) {
        frame_index = (byte == 0x55U) ? 1U : 0U;
        frame[0] = 0x55U;
        return;
    }

    frame[frame_index++] = byte;
    if (frame_index == sizeof(frame)) {
        decode_frame();
        frame_index = 0U;
    }
}

void jy61p_init(void)
{
    memset(&current_sample, 0, sizeof(current_sample));
    frame_index = 0U;
    checksum_errors = 0U;
    have_gyro = false;
    have_angle = false;
    wx = wy = wz = 0.0f;
    Roll = Pitch = Yaw = 0.0f;
}

void jy61p_poll(void)
{
    (void) jy61p_poll_budget(UINT16_MAX);
}

uint16_t jy61p_poll_budget(uint16_t max_bytes)
{
    uint8_t byte;
    uint16_t processed = 0U;

    while (processed < max_bytes && uart0_read_byte(&byte)) {
        consume_byte(byte);
        processed++;
    }
    return processed;
}

bool jy61p_get_sample(JY61P_Sample *sample)
{
    if (sample == NULL) {
        return false;
    }
    *sample = current_sample;
    return sample->valid;
}

uint32_t jy61p_checksum_error_count(void)
{
    return checksum_errors;
}
