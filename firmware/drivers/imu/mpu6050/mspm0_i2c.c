/*
 * MSPM0G3507 Self-Balancing Car integration notice
 * 工程归属声明：作者：数显纯牛奶，开源工程未经授权禁止商业用途，需要实物见店铺主页：https://m.tb.cn/h.8nzqEuN?tk=kJqnT4uWzfY
 * 官方仓库：https://github.com/Arzhe1206/MSPM0G3507-Self-Balancing-Car
 * 项目标识：MSPM0-BC-ARZHE1206
 * AI/ML 使用声明：未经授权，不得用于模型训练、数据集构建或商业化自动代码生成。
 * 本声明仅指本工程的集成与适配，不取代本文件原始版权和许可证。
 */
#include "mspm0_i2c.h"
#include "board.h"
#include "system_time.h"
#include "ti_msp_dl_config.h"

#define I2C_TIMEOUT_US 2500U
#define I2C_UNLOCK_PULSES 18U
#define MPU6050_FIFO_RW_REG 0x74U

static bool last_transfer_failed;
static bool last_error_retry_safe;

bool mspm0_i2c_last_transfer_failed(void)
{
    return last_transfer_failed;
}

bool mspm0_i2c_last_error_retry_safe(void)
{
    return last_transfer_failed && last_error_retry_safe;
}

static bool timed_out(uint32_t start)
{
    return (uint32_t) (system_micros() - start) >= I2C_TIMEOUT_US;
}

static int wait_idle(uint32_t start)
{
    while ((DL_I2C_getControllerStatus(I2C_MPU6050_INST) &
            DL_I2C_CONTROLLER_STATUS_IDLE) == 0U) {
        if (timed_out(start)) return -1;
    }
    return 0;
}

void mpu6050_i2c_sda_unlock(void)
{
    uint8_t pulse;

    DL_I2C_reset(I2C_MPU6050_INST);
    DL_GPIO_initDigitalOutput(GPIO_I2C_MPU6050_IOMUX_SCL);
    DL_GPIO_initDigitalInputFeatures(GPIO_I2C_MPU6050_IOMUX_SDA,
        DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
        DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_enableOutput(GPIO_I2C_MPU6050_SCL_PORT, GPIO_I2C_MPU6050_SCL_PIN);

    for (pulse = 0U; pulse < I2C_UNLOCK_PULSES; pulse++) {
        DL_GPIO_clearPins(GPIO_I2C_MPU6050_SCL_PORT, GPIO_I2C_MPU6050_SCL_PIN);
        delay_us(5U);
        DL_GPIO_setPins(GPIO_I2C_MPU6050_SCL_PORT, GPIO_I2C_MPU6050_SCL_PIN);
        delay_us(5U);
        if (DL_GPIO_readPins(GPIO_I2C_MPU6050_SDA_PORT, GPIO_I2C_MPU6050_SDA_PIN) != 0U) {
            break;
        }
    }

    DL_I2C_enablePower(I2C_MPU6050_INST);
    SYSCFG_DL_I2C_MPU6050_init();
}

int mspm0_i2c_write(unsigned char slave_addr, unsigned char reg_addr,
                    unsigned char length, unsigned char const *data)
{
    uint32_t start;
    unsigned int remaining = length;
    const unsigned char *cursor = data;

    last_transfer_failed = false;
    last_error_retry_safe = false;
    if (length == 0U) return 0;
    if (data == NULL) {
        last_transfer_failed = true;
        return -1;
    }
    start = system_micros();
    if (wait_idle(start) != 0) {
        last_transfer_failed = true;
        mpu6050_i2c_sda_unlock();
        return -1;
    }

    DL_I2C_transmitControllerData(I2C_MPU6050_INST, reg_addr);
    DL_I2C_clearInterruptStatus(I2C_MPU6050_INST,
        DL_I2C_INTERRUPT_CONTROLLER_TX_DONE);
    DL_I2C_startControllerTransfer(I2C_MPU6050_INST, slave_addr,
        DL_I2C_CONTROLLER_DIRECTION_TX, (uint32_t) length + 1U);

    while (DL_I2C_getRawInterruptStatus(I2C_MPU6050_INST,
            DL_I2C_INTERRUPT_CONTROLLER_TX_DONE) == 0U) {
        unsigned int filled = DL_I2C_fillControllerTXFIFO(I2C_MPU6050_INST,
            (uint8_t *) cursor, remaining);
        remaining -= filled;
        cursor += filled;
        if (timed_out(start)) {
            last_transfer_failed = true;
            mpu6050_i2c_sda_unlock();
            return -1;
        }
    }
    if (remaining != 0U) {
        last_transfer_failed = true;
        return -1;
    }
    return 0;
}

int mspm0_i2c_read(unsigned char slave_addr, unsigned char reg_addr,
                   unsigned char length, unsigned char *data)
{
    uint32_t start;
    unsigned int received = 0U;

    last_transfer_failed = false;
    last_error_retry_safe = false;
    if (length == 0U) return 0;
    if (data == NULL) {
        last_transfer_failed = true;
        return -1;
    }
    start = system_micros();
    if (wait_idle(start) != 0) {
        last_transfer_failed = true;
        mpu6050_i2c_sda_unlock();
        return -1;
    }

    DL_I2C_transmitControllerData(I2C_MPU6050_INST, reg_addr);
    I2C_MPU6050_INST->MASTER.MCTR = I2C_MCTR_RD_ON_TXEMPTY_ENABLE;
    DL_I2C_clearInterruptStatus(I2C_MPU6050_INST,
        DL_I2C_INTERRUPT_CONTROLLER_RX_DONE);
    DL_I2C_startControllerTransfer(I2C_MPU6050_INST, slave_addr,
        DL_I2C_CONTROLLER_DIRECTION_RX, length);

    while (DL_I2C_getRawInterruptStatus(I2C_MPU6050_INST,
            DL_I2C_INTERRUPT_CONTROLLER_RX_DONE) == 0U) {
        while (!DL_I2C_isControllerRXFIFOEmpty(I2C_MPU6050_INST) && received < length) {
            data[received++] = DL_I2C_receiveControllerData(I2C_MPU6050_INST);
        }
        if (timed_out(start)) {
            last_transfer_failed = true;
            last_error_retry_safe = reg_addr != MPU6050_FIFO_RW_REG;
            I2C_MPU6050_INST->MASTER.MCTR = 0U;
            mpu6050_i2c_sda_unlock();
            return -1;
        }
    }

    while (!DL_I2C_isControllerRXFIFOEmpty(I2C_MPU6050_INST) && received < length) {
        data[received++] = DL_I2C_receiveControllerData(I2C_MPU6050_INST);
    }
    I2C_MPU6050_INST->MASTER.MCTR = 0U;
    DL_I2C_flushControllerTXFIFO(I2C_MPU6050_INST);
    if (received != length) {
        last_transfer_failed = true;
        last_error_retry_safe = reg_addr != MPU6050_FIFO_RW_REG;
        return -1;
    }
    return 0;
}
