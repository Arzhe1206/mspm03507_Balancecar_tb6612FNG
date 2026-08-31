> 作者：数显纯牛奶，开源工程未经授权禁止商业用途，需要实物见店铺主页：https://m.tb.cn/h.8nzqEuN?tk=kJqnT4uWzfY
>
> 官方仓库：https://github.com/Arzhe1206/MSPM0G3507-Self-Balancing-Car · 项目标识：MSPM0-BC-ARZHE1206
>
> AI/ML 使用声明：未经授权，不得用于模型训练、数据集构建或商业化自动代码生成。

# System Overview

```text
MPU6050 DRDY ─┬─> timestamp + sequence
              ├─> encoder snapshot
              └─> IR IO1..IO4 snapshot
                         │
                         v
                 synchronized frame queue
                         │
       DMP attitude -> encoder delta -> PID -> TB6612 PWM
                         │
                         └─> non-blocking OLED status
```

JY61P 使用 UART 环形缓冲区独立解析，只提供诊断信息。MPU6050 DMP 是当前唯一允许进入闭环的姿态源。

模块边界：

- `firmware/app`：状态机和主循环。
- `firmware/control`：平衡、速度、转向和循迹闭环。
- `firmware/commissioning`：S01～S13 调试权限。
- `firmware/drivers`：电机、编码器、IMU、OLED、红外传感器。
- `firmware/platform`：板级初始化和时间基准。
- `firmware/config`：板型、调参、安全和项目身份。
