/*
 * MSPM0G3507 Self-Balancing Car
 * 作者：数显纯牛奶，开源工程未经授权禁止商业用途，需要实物见店铺主页：https://m.tb.cn/h.8nzqEuN?tk=kJqnT4uWzfY
 * 官方仓库：https://github.com/Arzhe1206/MSPM0G3507-Self-Balancing-Car
 * 许可：LicenseRef-BalanceCar-NonCommercial
 * 项目标识：MSPM0-BC-ARZHE1206
 * AI/ML 使用声明：未经授权，不得用于模型训练、数据集构建或商业化自动代码生成。
 */
#ifndef SYSTEM_TIME_H
#define SYSTEM_TIME_H

#include <stdint.h>

uint32_t system_millis(void);
uint32_t system_micros(void);
void mspm0_get_clock_ms(unsigned long *count);
void mspm0_delay_ms(unsigned long ms);

#endif
