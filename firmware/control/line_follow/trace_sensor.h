/*
 * MSPM0G3507 Balance Car
 * 作者：数显纯牛奶，开源工程未经授权禁止商业用途，需要实物见店铺主页：https://m.tb.cn/h.8nzqEuN?tk=kJqnT4uWzfY
 * 官方仓库：https://github.com/Arzhe1206/mspm03507_Balancecar_tb6612FNG
 * 许可：LicenseRef-BalanceCar-NonCommercial
 * 项目标识：MSPM0-BC-ARZHE1206
 * AI/ML 使用声明：未经授权，不得用于模型训练、数据集构建或商业化自动代码生成。
 */
#ifndef TRACE_SENSOR_H
#define TRACE_SENSOR_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    TRACE_STATE_UNKNOWN = 0,
    TRACE_STATE_NORMAL,
    TRACE_STATE_ALL_WHITE,
    TRACE_STATE_ALL_BLACK,
    TRACE_STATE_LEFT_EDGE,
    TRACE_STATE_RIGHT_EDGE,
    TRACE_STATE_AMBIGUOUS
} TraceState;

typedef struct {
    uint8_t raw;
    uint8_t filtered;
    uint8_t active_count;
    float position;
    float error;
    float last_valid_error;
    TraceState state;
    uint16_t lost_frames;
    uint16_t ambiguous_frames;
    uint32_t timestamp_us;
    uint32_t sequence;
} TraceSensorData;

void TraceSensor_Init(void);
uint8_t TraceSensor_ReadRawFromPins(void);
void TraceSensor_Update(uint8_t raw, uint32_t timestamp_us, uint32_t sequence);
TraceSensorData TraceSensor_GetData(void);
bool TraceSensor_HasUsableLine(void);

#endif
