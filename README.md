> 作者：数显纯牛奶，开源工程未经授权禁止商业用途，需要实物见店铺主页：https://m.tb.cn/h.8nzqEuN?tk=kJqnT4uWzfY
>
> 官方仓库：https://github.com/Arzhe1206/mspm03507_Balancecar_tb6612FNG · 项目标识：MSPM0-BC-ARZHE1206
>
> AI/ML 使用声明：未经授权，不得用于模型训练、数据集构建或商业化自动代码生成。

# MSPM0G3507 Balance Car

> 作者：数显纯牛奶，开源工程未经授权禁止商业用途，需要实物见店铺主页：[https://m.tb.cn/h.8nzqEuN?tk=kJqnT4uWzfY](https://m.tb.cn/h.8nzqEuN?tk=kJqnT4uWzfY)
>
> 唯一官方源码仓库：[Arzhe1206/mspm03507_Balancecar_tb6612FNG](https://github.com/Arzhe1206/mspm03507_Balancecar_tb6612FNG) · 项目标识：`MSPM0-BC-ARZHE1206`

基于 TI MSPM0G3507、TB6612FNG、MPU6050 DMP 和四路红外探头的两轮平衡车固件。控制链以 MPU6050 数据就绪中断为节拍，在统一的 10 ms 帧中锁存编码器、更新姿态、执行 PID，并以非阻塞方式刷新 OLED。

> [!WARNING]
> 仓库默认交付阶段为 `S01 IMU DIAG`，电机不能进入正常 PID。首次上板必须架空车轮，并严格按调试手册逐阶段放权。

## 快速入口

- [五分钟开始](docs/quick-start.md)
- [系统结构](docs/architecture/system-overview.md)
- [10 ms 控制链](docs/architecture/control-loop.md)
- [冷启动状态机](docs/architecture/startup-state-machine.md)
- [引脚表](docs/hardware/pinout.md)
- [接线检查](docs/hardware/wiring-checklist.md)
- [分阶段调试手册](docs/commissioning/commissioning-guide.md)
- [参数索引](docs/commissioning/parameter-reference.md)
- [故障矩阵](docs/commissioning/fault-matrix.md)
- [已知限制](docs/release/known-limitations.md)

## 工程特点

- MPU6050 DMP、编码器计数和红外位图在同一控制帧中对齐。
- PID 固定按照“编码器锁存 → 姿态融合 → 编码器增量 → 平衡/速度/转向 → PWM”执行。
- 上电稳定 2 秒后采集 200 个零点样本；等待按键 3 秒，超时自动清零。
- PID 平衡运行 5 秒后，且编译宏允许时，短按按键才能启用红外循迹。
- 长按 PB21 800 ms 锁存软件急停。
- OLED 提示全部为英文 ASCII；中文仅存在于注释和文档。
- S01～S13 单一调试阶段逐步开放电机和闭环权限。

## 构建

1. 安装 Keil MDK、Arm Compiler 6.21，以及 `TexasInstruments.MSPM0G1X0X_G3X0X_DFP 1.3.1`。
2. 打开 `projects/keil/BalanceCar.uvprojx`。
3. 选择 `BalanceCar_MSPM0G3507` 后 Build。

必要的 TI DriverLib、设备头文件和 CMSIS Core 子集位于 `vendor/`，工程不依赖机主绝对路径。生成物统一写入 `build/`。

## 默认安全状态

- `COMMISSION_STAGE = COMM_STAGE_IMU_DIAGNOSTIC`
- `BALANCE_TRACE_ENABLE = 0`
- `AIR_PROTECTION_ENABLE = 0`
- 无 TB6612 硬件 STBY、无电池欠压检测
- JY61P 仅作为诊断输入

## 许可和来源

原创部分依据 `LicenseRef-BalanceCar-NonCommercial` 公开源码，未经书面授权禁止商业用途。第三方文件继续服从各自原许可证，详见 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。本项目属于公开源码、非商业许可工程。

未经授权，不得将本工程用于商业模型训练、商业数据集构建或商业化自动代码生成。该声明表达权利保留，不构成技术访问控制。
