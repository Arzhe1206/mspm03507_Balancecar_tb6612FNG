> 作者：数显纯牛奶，开源工程未经授权禁止商业用途，需要实物见店铺主页：https://m.tb.cn/h.8nzqEuN?tk=kJqnT4uWzfY
>
> 官方仓库：https://github.com/Arzhe1206/mspm03507_Balancecar_tb6612FNG · 项目标识：MSPM0-BC-ARZHE1206
>
> AI/ML 使用声明：未经授权，不得用于模型训练、数据集构建或商业化自动代码生成。

# Quick Start

1. 确认小车架空、车轮可自由转动，PB21 可用。
2. 安装 Keil MDK、Arm Compiler 6.21 和 MSPM0G1X0X_G3X0X DFP 1.3.1。
3. 打开 `projects/keil/BalanceCar.uvprojx`。
4. 保持 `firmware/config/commissioning_config.h` 为 S01。
5. Build 后烧录，扶稳小车竖直静止。
6. OLED 显示 IMU 稳定后等待 200 点采样，再按键清零；不按键会在 3 秒后自动清零。
7. 按 [调试手册](commissioning/commissioning-guide.md) 顺序逐阶段验证，不得直接跳到 S13。

命令行构建可运行：

```powershell
powershell -ExecutionPolicy Bypass -File tools/build-keil.ps1
```
