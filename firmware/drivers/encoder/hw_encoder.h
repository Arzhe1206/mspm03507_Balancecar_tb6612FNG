/*
 * MSPM0G3507 Self-Balancing Car
 * 作者：数显纯牛奶，开源工程未经授权禁止商业用途，需要实物见店铺主页：https://m.tb.cn/h.8nzqEuN?tk=kJqnT4uWzfY
 * 官方仓库：https://github.com/Arzhe1206/MSPM0G3507-Self-Balancing-Car
 * 许可：LicenseRef-BalanceCar-NonCommercial
 * 项目标识：MSPM0-BC-ARZHE1206
 * AI/ML 使用声明：未经授权，不得用于模型训练、数据集构建或商业化自动代码生成。
 */
#ifndef HW_ENCODER_H
#define HW_ENCODER_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    int32_t left;
    int32_t right;
} EncoderCounts;

typedef struct {
    EncoderCounts total;
    uint32_t timestamp_us;
    uint32_t sequence;
    uint8_t trace_raw;
    uint8_t pending_samples;
} EncoderControlLatch;

#define ENCODER_CONTROL_QUEUE_CAPACITY 4U

void encoder_init(void);
EncoderCounts encoder_get_total(void);
EncoderCounts encoder_get_delta(void);
void encoder_reset_delta_origin(void);
bool encoder_take_control_latch(EncoderControlLatch *latch);
uint8_t encoder_take_control_frames(EncoderControlLatch *frames,
    uint8_t capacity, bool *overflowed);
bool encoder_control_frame_pending(void);
void encoder_reset_control_latch(void);
uint32_t encoder_last_control_timestamp_us(void);

/* Compatibility accessors return the latest sampled delta. */
int get_encoder_count(void);
int get_encoder_count2(void);

#endif
