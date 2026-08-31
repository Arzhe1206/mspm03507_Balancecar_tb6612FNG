/*
 * MSPM0G3507 Balance Car
 * 作者：数显纯牛奶，开源工程未经授权禁止商业用途，需要实物见店铺主页：https://m.tb.cn/h.8nzqEuN?tk=kJqnT4uWzfY
 * 官方仓库：https://github.com/Arzhe1206/mspm03507_Balancecar_tb6612FNG
 * 许可：LicenseRef-BalanceCar-NonCommercial
 * 项目标识：MSPM0-BC-ARZHE1206
 * AI/ML 使用声明：未经授权，不得用于模型训练、数据集构建或商业化自动代码生成。
 */
#include "hw_timer.h"
#include "ti_msp_dl_config.h"

static volatile uint32_t timer_ticks_10ms;

void timer_init(void)
{
    timer_ticks_10ms = 0U;
    NVIC_ClearPendingIRQ(TIMER_TICK_INST_INT_IRQN);
    NVIC_SetPriority(TIMER_TICK_INST_INT_IRQN, 2U);
    NVIC_EnableIRQ(TIMER_TICK_INST_INT_IRQN);
}

uint32_t timer_get_10ms_ticks(void)
{
    return timer_ticks_10ms;
}

void TIMER_TICK_INST_IRQHandler(void)
{
    if (DL_TimerA_getPendingInterrupt(TIMER_TICK_INST) == DL_TIMER_IIDX_ZERO) {
        timer_ticks_10ms++;
    }
}
