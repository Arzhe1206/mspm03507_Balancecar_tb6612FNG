/*
 * MSPM0G3507 Balance Car
 * 作者：数显纯牛奶，开源工程未经授权禁止商业用途，需要实物见店铺主页：https://m.tb.cn/h.8nzqEuN?tk=kJqnT4uWzfY
 * 官方仓库：https://github.com/Arzhe1206/mspm03507_Balancecar_tb6612FNG
 * 许可：LicenseRef-BalanceCar-NonCommercial
 * 项目标识：MSPM0-BC-ARZHE1206
 * AI/ML 使用声明：未经授权，不得用于模型训练、数据集构建或商业化自动代码生成。
 */
#include "trace_sensor.h"
#include "balance_trace_config.h"
#include "ti_msp_dl_config.h"
#include <string.h>

#define TRACE_MASK 0x0FU

static TraceSensorData trace_data;
static uint8_t filter_history[TRACE_FILTER_SAMPLES];
static uint8_t filter_index;
static uint8_t filter_count;

#if (TRACE_SENSOR_ORDER_REVERSED == 1U)
static uint8_t reverse_four_bits(uint8_t value)
{
    return (uint8_t) (((value & 0x01U) << 3) |
        ((value & 0x02U) << 1) |
        ((value & 0x04U) >> 1) |
        ((value & 0x08U) >> 3));
}
#endif

static uint8_t normalize_raw(uint8_t raw)
{
    raw &= TRACE_MASK;
#if (TRACE_ACTIVE_HIGH == 0U)
    raw = (uint8_t) (~raw) & TRACE_MASK;
#endif
#if (TRACE_SENSOR_ORDER_REVERSED == 1U)
    raw = reverse_four_bits(raw);
#endif
    return raw;
}

static uint8_t majority_filter(uint8_t raw)
{
    uint8_t bit;
    uint8_t sample;
    uint8_t output = 0U;

    filter_history[filter_index] = raw;
    filter_index = (uint8_t) ((filter_index + 1U) % TRACE_FILTER_SAMPLES);
    if (filter_count < TRACE_FILTER_SAMPLES) filter_count++;
    if (filter_count < TRACE_FILTER_SAMPLES) return raw;

    for (bit = 0U; bit < 4U; bit++) {
        uint8_t votes = 0U;
        for (sample = 0U; sample < TRACE_FILTER_SAMPLES; sample++) {
            if ((filter_history[sample] & (uint8_t) (1U << bit)) != 0U) {
                votes++;
            }
        }
        if (votes > TRACE_FILTER_SAMPLES / 2U) {
            output |= (uint8_t) (1U << bit);
        }
    }
    return output;
}

static uint8_t count_active(uint8_t raw)
{
    uint8_t bit;
    uint8_t count = 0U;

    for (bit = 0U; bit < 4U; bit++) {
        if ((raw & (uint8_t) (1U << bit)) != 0U) count++;
    }
    return count;
}

static uint8_t count_clusters(uint8_t raw)
{
    uint8_t bit;
    uint8_t clusters = 0U;
    bool previous_active = false;

    for (bit = 0U; bit < 4U; bit++) {
        bool active = (raw & (uint8_t) (1U << bit)) != 0U;
        if (active && !previous_active) clusters++;
        previous_active = active;
    }
    return clusters;
}

static float calculate_position(uint8_t raw, uint8_t active_count)
{
    static const int8_t weights[4] = {-3, -1, 1, 3};
    int16_t sum = 0;
    uint8_t bit;

    if (active_count == 0U) return 0.0f;
    for (bit = 0U; bit < 4U; bit++) {
        if ((raw & (uint8_t) (1U << bit)) != 0U) sum += weights[bit];
    }
    return (float) sum / (float) active_count;
}

void TraceSensor_Init(void)
{
    memset(&trace_data, 0, sizeof(trace_data));
    memset(filter_history, 0, sizeof(filter_history));
    trace_data.state = TRACE_STATE_UNKNOWN;
    filter_index = 0U;
    filter_count = 0U;
}

uint8_t TraceSensor_ReadRawFromPins(void)
{
    uint32_t pins = DL_GPIO_readPins(TRACE_IR_PORT, TRACE_IR_ALL_PINS);
    uint8_t raw = 0U;

    if ((pins & TRACE_IR_IO1_PIN) != 0U) raw |= 0x01U;
    if ((pins & TRACE_IR_IO2_PIN) != 0U) raw |= 0x02U;
    if ((pins & TRACE_IR_IO3_PIN) != 0U) raw |= 0x04U;
    if ((pins & TRACE_IR_IO4_PIN) != 0U) raw |= 0x08U;
    return raw;
}

void TraceSensor_Update(uint8_t raw, uint32_t timestamp_us, uint32_t sequence)
{
    uint8_t filtered = majority_filter(normalize_raw(raw));
    uint8_t active_count = count_active(filtered);
    float position = calculate_position(filtered, active_count);
    TraceState state;

    if (active_count == 0U) {
        state = TRACE_STATE_ALL_WHITE;
    } else if (filtered == TRACE_MASK) {
        state = TRACE_STATE_ALL_BLACK;
    } else if (count_clusters(filtered) > 1U) {
        state = TRACE_STATE_AMBIGUOUS;
    } else if ((filtered & 0x03U) != 0U && (filtered & 0x0CU) == 0U) {
        state = TRACE_STATE_LEFT_EDGE;
    } else if ((filtered & 0x0CU) != 0U && (filtered & 0x03U) == 0U) {
        state = TRACE_STATE_RIGHT_EDGE;
    } else {
        state = TRACE_STATE_NORMAL;
    }

    trace_data.raw = normalize_raw(raw);
    trace_data.filtered = filtered;
    trace_data.active_count = active_count;
    trace_data.position = position;
    trace_data.state = state;
    trace_data.timestamp_us = timestamp_us;
    trace_data.sequence = sequence;

    if (state == TRACE_STATE_ALL_WHITE) {
        if (trace_data.lost_frames < UINT16_MAX) trace_data.lost_frames++;
        trace_data.ambiguous_frames = 0U;
        trace_data.error = trace_data.last_valid_error;
    } else if (state == TRACE_STATE_AMBIGUOUS) {
        if (trace_data.ambiguous_frames < UINT16_MAX)
            trace_data.ambiguous_frames++;
        trace_data.lost_frames = 0U;
        trace_data.error = trace_data.last_valid_error;
    } else if (state == TRACE_STATE_ALL_BLACK) {
        trace_data.lost_frames = 0U;
        trace_data.ambiguous_frames = 0U;
        trace_data.error = 0.0f;
    } else {
        trace_data.lost_frames = 0U;
        trace_data.ambiguous_frames = 0U;
        trace_data.error = -position / 3.0f;
        trace_data.last_valid_error = trace_data.error;
    }
}

TraceSensorData TraceSensor_GetData(void)
{
    return trace_data;
}

bool TraceSensor_HasUsableLine(void)
{
    return trace_data.state == TRACE_STATE_NORMAL ||
        trace_data.state == TRACE_STATE_LEFT_EDGE ||
        trace_data.state == TRACE_STATE_RIGHT_EDGE;
}
