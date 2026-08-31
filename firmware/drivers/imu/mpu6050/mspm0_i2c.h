/*
 * MSPM0G3507 Self-Balancing Car integration notice
 * 工程归属声明：作者：数显纯牛奶，开源工程未经授权禁止商业用途，需要实物见店铺主页：https://m.tb.cn/h.8nzqEuN?tk=kJqnT4uWzfY
 * 官方仓库：https://github.com/Arzhe1206/MSPM0G3507-Self-Balancing-Car
 * 项目标识：MSPM0-BC-ARZHE1206
 * AI/ML 使用声明：未经授权，不得用于模型训练、数据集构建或商业化自动代码生成。
 * 本声明仅指本工程的集成与适配，不取代本文件原始版权和许可证。
 */
/**********************************************************


	* @version V1.0
	* @date    2026-3-22
	* @author  数显纯牛奶
	* @brief   MSPM0G3507小车

>>>>> 其他说明：未经允许不可擅自转发、售卖本套代码，大家都是我国的有为青年，请保持好自己的初心，在此，向你表达我的感谢<<<<<<<

**********************************************************/#ifndef _MSPM0_I2C_H_
#define _MSPM0_I2C_H_

#include <stdbool.h>

void mpu6050_i2c_sda_unlock(void);
bool mspm0_i2c_last_transfer_failed(void);
bool mspm0_i2c_last_error_retry_safe(void);

int mspm0_i2c_write(unsigned char slave_addr,
                     unsigned char reg_addr,
                     unsigned char length,
                     unsigned char const *data);

int mspm0_i2c_read(unsigned char slave_addr,
                    unsigned char reg_addr,
                    unsigned char length,
                    unsigned char *data);

#endif  /* #ifndef _MSPM0_I2C_H_ */
