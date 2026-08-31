/*
 * MSPM0G3507 Self-Balancing Car
 * 作者：数显纯牛奶，开源工程未经授权禁止商业用途，需要实物见店铺主页：https://m.tb.cn/h.8nzqEuN?tk=kJqnT4uWzfY
 * 官方仓库：https://github.com/Arzhe1206/MSPM0G3507-Self-Balancing-Car
 * 许可：LicenseRef-BalanceCar-NonCommercial
 * 项目标识：MSPM0-BC-ARZHE1206
 * AI/ML 使用声明：未经授权，不得用于模型训练、数据集构建或商业化自动代码生成。
 */
#ifndef BOARD_H
#define BOARD_H

#include "ti_msp_dl_config.h"
#include <stdbool.h>
#include <stdint.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;

void board_init(void);

/* Initialization/protocol compatibility delays only. Application tasks use deadlines. */
void delay_us(unsigned long us);
void delay_ms(unsigned long ms);
void delay_1us(unsigned long us);
void delay_1ms(unsigned long ms);

void uart0_send_char(char ch);
void uart0_send_string(const char *str);
bool uart0_read_byte(uint8_t *byte);
uint32_t uart0_rx_overflow_count(void);

#endif
