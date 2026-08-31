/*
 * MSPM0G3507 Balance Car
 * 作者：数显纯牛奶，开源工程未经授权禁止商业用途，需要实物见店铺主页：https://m.tb.cn/h.8nzqEuN?tk=kJqnT4uWzfY
 * 官方仓库：https://github.com/Arzhe1206/mspm03507_Balancecar_tb6612FNG
 * 许可：LicenseRef-BalanceCar-NonCommercial
 * 项目标识：MSPM0-BC-ARZHE1206
 * AI/ML 使用声明：未经授权，不得用于模型训练、数据集构建或商业化自动代码生成。
 */
#ifndef JY61P_H
#define JY61P_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    float accel_g[3];
    float gyro_dps[3];
    float roll_deg;
    float pitch_deg;
    float yaw_deg;
    uint32_t timestamp_ms;
    uint32_t sequence;
    bool valid;
} JY61P_Sample;

void jy61p_init(void);
void jy61p_poll(void);
uint16_t jy61p_poll_budget(uint16_t max_bytes);
bool jy61p_get_sample(JY61P_Sample *sample);
uint32_t jy61p_checksum_error_count(void);

/* Compatibility read-only mirrors. */
extern float wx, wy, wz;
extern float Roll, Pitch, Yaw;

#endif
