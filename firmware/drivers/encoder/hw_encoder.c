/*
 * MSPM0G3507 Balance Car
 * 作者：数显纯牛奶，开源工程未经授权禁止商业用途，需要实物见店铺主页：https://m.tb.cn/h.8nzqEuN?tk=kJqnT4uWzfY
 * 官方仓库：https://github.com/Arzhe1206/mspm03507_Balancecar_tb6612FNG
 * 许可：LicenseRef-BalanceCar-NonCommercial
 * 项目标识：MSPM0-BC-ARZHE1206
 * AI/ML 使用声明：未经授权，不得用于模型训练、数据集构建或商业化自动代码生成。
 */
#include "hw_encoder.h"
#include "system_time.h"
#include "ti_msp_dl_config.h"
#include "trace_sensor.h"
#include "board_profile.h"

static volatile int32_t encoder_left_total;
static volatile int32_t encoder_right_total;
static int32_t encoder_left_origin;
static int32_t encoder_right_origin;
static EncoderCounts encoder_latest_delta;
static volatile EncoderControlLatch control_frame_queue[
    ENCODER_CONTROL_QUEUE_CAPACITY];
static volatile uint8_t control_queue_head;
static volatile uint8_t control_queue_count;
static volatile bool control_queue_overflowed;
static volatile uint32_t control_last_timestamp_us;
static volatile uint32_t control_latch_sequence;

static EncoderCounts atomic_total_snapshot(void)
{
    EncoderCounts value;
    uint32_t primask = __get_PRIMASK();

    __disable_irq();
    value.left = encoder_left_total;
    value.right = encoder_right_total;
    if (primask == 0U) {
        __enable_irq();
    }
    return value;
}

void encoder_init(void)
{
    encoder_left_total = 0;
    encoder_right_total = 0;
    encoder_left_origin = 0;
    encoder_right_origin = 0;
    encoder_latest_delta.left = 0;
    encoder_latest_delta.right = 0;
    control_queue_head = 0U;
    control_queue_count = 0U;
    control_queue_overflowed = false;
    control_last_timestamp_us = 0U;
    control_latch_sequence = 0U;

    NVIC_ClearPendingIRQ(GPIO_ENCODER_INT_IRQN);
    NVIC_SetPriority(GPIO_ENCODER_INT_IRQN, 1U);
    NVIC_EnableIRQ(GPIO_ENCODER_INT_IRQN);
    NVIC_ClearPendingIRQ(MPU6050_INT_IRQN);
    NVIC_SetPriority(MPU6050_INT_IRQN, 1U);
    NVIC_EnableIRQ(MPU6050_INT_IRQN);
}

EncoderCounts encoder_get_total(void)
{
    return atomic_total_snapshot();
}

EncoderCounts encoder_get_delta(void)
{
    EncoderCounts now = atomic_total_snapshot();

    encoder_latest_delta.left = now.left - encoder_left_origin;
    encoder_latest_delta.right = now.right - encoder_right_origin;
    encoder_left_origin = now.left;
    encoder_right_origin = now.right;
    return encoder_latest_delta;
}

void encoder_reset_delta_origin(void)
{
    EncoderCounts now = atomic_total_snapshot();
    encoder_left_origin = now.left;
    encoder_right_origin = now.right;
    encoder_latest_delta.left = 0;
    encoder_latest_delta.right = 0;
}

bool encoder_take_control_latch(EncoderControlLatch *latch)
{
    EncoderControlLatch frames[ENCODER_CONTROL_QUEUE_CAPACITY];
    uint8_t count;

    if (latch == NULL) {
        return false;
    }
    count = encoder_take_control_frames(frames,
        ENCODER_CONTROL_QUEUE_CAPACITY, NULL);
    if (count == 0U) return false;
    *latch = frames[count - 1U];
    latch->pending_samples = count;
    return true;
}

uint8_t encoder_take_control_frames(EncoderControlLatch *frames,
    uint8_t capacity, bool *overflowed)
{
    uint32_t primask;
    uint8_t index;
    uint8_t count;

    if (frames == NULL || capacity == 0U) return 0U;
    primask = __get_PRIMASK();
    __disable_irq();
    count = control_queue_count;
    if (count > capacity) count = capacity;
    for (index = 0U; index < count; index++) {
        frames[index] = control_frame_queue[
            (uint8_t) ((control_queue_head + index) %
            ENCODER_CONTROL_QUEUE_CAPACITY)];
    }
    if (overflowed != NULL) *overflowed = control_queue_overflowed;
    control_queue_head = 0U;
    control_queue_count = 0U;
    control_queue_overflowed = false;
    if (primask == 0U) __enable_irq();
    return count;
}

bool encoder_control_frame_pending(void)
{
    return control_queue_count != 0U;
}

void encoder_reset_control_latch(void)
{
    uint32_t primask = __get_PRIMASK();

    __disable_irq();
    control_queue_head = 0U;
    control_queue_count = 0U;
    control_queue_overflowed = false;
    control_last_timestamp_us = 0U;
    if (primask == 0U) __enable_irq();
}

uint32_t encoder_last_control_timestamp_us(void)
{
    return control_last_timestamp_us;
}

int get_encoder_count(void) { return (int) encoder_latest_delta.left; }
int get_encoder_count2(void) { return (int) encoder_latest_delta.right; }

void GROUP1_IRQHandler(void)
{
    uint32_t encoder_status = DL_GPIO_getEnabledInterruptStatus(
        GPIO_ENCODER_PORT,
        GPIO_ENCODER_PIN_A_PIN | GPIO_ENCODER_PIN_B_PIN |
        GPIO_ENCODER_PIN_C_PIN | GPIO_ENCODER_PIN_D_PIN);
    uint32_t mpu_status = DL_GPIO_getEnabledInterruptStatus(
        MPU6050_INT_PORT, MPU6050_INT_PIN);

    if ((encoder_status & GPIO_ENCODER_PIN_A_PIN) != 0U) {
        encoder_left_total += ENCODER_LEFT_SIGN *
            (DL_GPIO_readPins(GPIO_ENCODER_PORT,
            GPIO_ENCODER_PIN_B_PIN) ? 1 : -1);
    }
    if ((encoder_status & GPIO_ENCODER_PIN_B_PIN) != 0U) {
        encoder_left_total += ENCODER_LEFT_SIGN *
            (DL_GPIO_readPins(GPIO_ENCODER_PORT,
            GPIO_ENCODER_PIN_A_PIN) ? -1 : 1);
    }
    if ((encoder_status & GPIO_ENCODER_PIN_C_PIN) != 0U) {
        encoder_right_total += ENCODER_RIGHT_SIGN *
            (DL_GPIO_readPins(GPIO_ENCODER_PORT,
            GPIO_ENCODER_PIN_D_PIN) ? 1 : -1);
    }
    if ((encoder_status & GPIO_ENCODER_PIN_D_PIN) != 0U) {
        encoder_right_total += ENCODER_RIGHT_SIGN *
            (DL_GPIO_readPins(GPIO_ENCODER_PORT,
            GPIO_ENCODER_PIN_C_PIN) ? -1 : 1);
    }

    if (encoder_status != 0U) {
        DL_GPIO_clearInterruptStatus(GPIO_ENCODER_PORT, encoder_status);
    }
    if ((mpu_status & MPU6050_INT_PIN) != 0U) {
        uint8_t write_index;

        control_latch_sequence++;
        control_last_timestamp_us = system_micros();
        if (control_queue_count < ENCODER_CONTROL_QUEUE_CAPACITY) {
            write_index = (uint8_t) ((control_queue_head +
                control_queue_count) % ENCODER_CONTROL_QUEUE_CAPACITY);
            control_queue_count++;
        } else {
            control_queue_head = (uint8_t) ((control_queue_head + 1U) %
                ENCODER_CONTROL_QUEUE_CAPACITY);
            write_index = (uint8_t) ((control_queue_head +
                ENCODER_CONTROL_QUEUE_CAPACITY - 1U) %
                ENCODER_CONTROL_QUEUE_CAPACITY);
            control_queue_overflowed = true;
        }
        control_frame_queue[write_index].total.left = encoder_left_total;
        control_frame_queue[write_index].total.right = encoder_right_total;
        control_frame_queue[write_index].timestamp_us = control_last_timestamp_us;
        control_frame_queue[write_index].sequence = control_latch_sequence;
        control_frame_queue[write_index].trace_raw = TraceSensor_ReadRawFromPins();
        control_frame_queue[write_index].pending_samples = 1U;
        DL_GPIO_clearInterruptStatus(MPU6050_INT_PORT, MPU6050_INT_PIN);
    }
}
