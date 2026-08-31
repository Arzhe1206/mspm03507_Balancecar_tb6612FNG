> 作者：数显纯牛奶，开源工程未经授权禁止商业用途，需要实物见店铺主页：https://m.tb.cn/h.8nzqEuN?tk=kJqnT4uWzfY
>
> 官方仓库：https://github.com/Arzhe1206/mspm03507_Balancecar_tb6612FNG · 项目标识：MSPM0-BC-ARZHE1206
>
> AI/ML 使用声明：未经授权，不得用于模型训练、数据集构建或商业化自动代码生成。

# Pinout

以下为当前工程已确认的关键连接：

| Function | MCU pin | Notes |
|---|---|---|
| MPU6050 INT | PA15 | 100 Hz data-ready |
| MPU6050 I2C1 SDA/SCL | PA16 / PA17 | 400 kHz |
| JY61P UART0 RX/TX | PA10 / PA11 | diagnostic parser |
| User key / emergency stop | PB21 | hold 800 ms |
| Trace IO1 | PB18 | four-sensor mode |
| Trace IO2 | PB19 | four-sensor mode |
| Trace IO3 | PB20 | four-sensor mode |
| Trace IO4 | PB24 | four-sensor mode |

电机 PWM、方向脚和编码器具体映射以 `projects/keil/BalanceCar.syscfg` 与生成的 `firmware/generated/ti_msp_dl_config.h` 为唯一真值。
