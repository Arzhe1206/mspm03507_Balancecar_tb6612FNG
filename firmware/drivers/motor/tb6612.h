/*
 * MSPM0G3507 Balance Car
 * 作者：数显纯牛奶，开源工程未经授权禁止商业用途，需要实物见店铺主页：https://m.tb.cn/h.8nzqEuN?tk=kJqnT4uWzfY
 * 官方仓库：https://github.com/Arzhe1206/mspm03507_Balancecar_tb6612FNG
 * 许可：LicenseRef-BalanceCar-NonCommercial
 * 项目标识：MSPM0-BC-ARZHE1206
 * AI/ML 使用声明：未经授权，不得用于模型训练、数据集构建或商业化自动代码生成。
 */
#ifndef BSP_TB6612_H
#define BSP_TB6612_H

#include "board.h"
#include <stdint.h>

#define TB6612_PWM_MAX 999

void TB6612_Init(void);
void TB6612_Stop(void);
void TB6612_Brake(void);
void Set_Motor_PWM(int left_pwm, int right_pwm);
int16_t TB6612_GetLeftCommand(void);
int16_t TB6612_GetRightCommand(void);

#endif
