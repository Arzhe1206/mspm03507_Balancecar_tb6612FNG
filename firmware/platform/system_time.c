/*
 * MSPM0G3507 Self-Balancing Car
 * 作者：数显纯牛奶，开源工程未经授权禁止商业用途，需要实物见店铺主页：https://m.tb.cn/h.8nzqEuN?tk=kJqnT4uWzfY
 * 官方仓库：https://github.com/Arzhe1206/MSPM0G3507-Self-Balancing-Car
 * 许可：LicenseRef-BalanceCar-NonCommercial
 * 项目标识：MSPM0-BC-ARZHE1206
 * AI/ML 使用声明：未经授权，不得用于模型训练、数据集构建或商业化自动代码生成。
 */
#include "system_time.h"
#include "board.h"

static volatile uint32_t system_tick_ms;

void SysTick_Handler(void)
{
    system_tick_ms++;
}

uint32_t system_millis(void)
{
    return system_tick_ms;
}

uint32_t system_micros(void)
{
    uint32_t ms_before;
    uint32_t ms_after;
    uint32_t systick_value;
    uint32_t systick_period = SysTick->LOAD + 1U;

    do {
        ms_before = system_tick_ms;
        systick_value = SysTick->VAL;
        ms_after = system_tick_ms;
    } while (ms_before != ms_after);

    return ms_before * 1000U +
        ((systick_period - systick_value) * 1000U) / systick_period;
}

void mspm0_get_clock_ms(unsigned long *count)
{
    if (count != NULL) {
        *count = (unsigned long) system_millis();
    }
}

/* InvenSense initialization requires bounded settling waits. Runtime tasks never call this. */
void mspm0_delay_ms(unsigned long ms)
{
    uint32_t start = system_millis();
    while ((uint32_t) (system_millis() - start) < (uint32_t) ms) {
        __WFI();
    }
}
