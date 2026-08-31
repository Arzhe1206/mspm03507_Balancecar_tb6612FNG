> 作者：数显纯牛奶，开源工程未经授权禁止商业用途，需要实物见店铺主页：https://m.tb.cn/h.8nzqEuN?tk=kJqnT4uWzfY
>
> 官方仓库：https://github.com/Arzhe1206/mspm03507_Balancecar_tb6612FNG · 项目标识：MSPM0-BC-ARZHE1206
>
> AI/ML 使用声明：未经授权，不得用于模型训练、数据集构建或商业化自动代码生成。

# MSPM0G3507 平衡车分阶段调试手册

## 1. 总原则

编辑 `firmware/config/commissioning_config.h` 中唯一的 `COMMISSION_STAGE`，一次只运行一个阶段。每次修改后重新编译、下载并填写 `TUNING_RECORD.md`。上一阶段没有明确通过时，不得进入下一阶段。

当前默认值是：

```c
#define COMMISSION_STAGE COMM_STAGE_IMU_DIAGNOSTIC
```

本工程不实现硬件 STBY 急停、不实现电池欠压检测、不增加调参遥测。全部调参信息通过 OLED 查看。传感器阶段电机禁止输出；电机阶段必须架空车轮；闭环阶段必须先使用保护架或保护绳。

## 2. PB21 按键规则

- 短按动作在松键时确认，避免与长按冲突。
- 任意电机阶段长按 800 ms：软件急停，立即清零速度、转向、PID和电机输出。
- 急停锁存后先松键，再短按确认；随后必须重新稳定2秒、采样200点并清零。
- `ZERO READY`：短按立即清零。
- PID启动不足5秒：短按忽略。
- PID启动超过5秒：S10触发一次低幅转向测试；S11/S12/S13在循迹宏打开时启动或退出循迹。
- S04/S05：短按触发一次不超过300 ms的电机测试脉冲。

软件急停依赖MCU正常运行，不等同于硬件断电。

## 3. OLED页面

OLED以10 Hz非阻塞刷新。默认每1秒自动换页；如需固定页面，在 `commissioning_config.h` 中设置：

```c
#define COMM_OLED_PAGE_MODE  COMM_OLED_PAGE_FIXED
#define COMM_OLED_FIXED_PAGE 0U
```

故障和急停页面始终覆盖普通诊断页。OLED只显示英文ASCII。

## 4. 阶段流程

### S01：IMU型号、通信与姿态

宏：`COMM_STAGE_IMU_DIAGNOSTIC`

电机始终关闭。OLED轮换显示：

- MPU6050 `WHO_AM_I`、DMP状态、采样率、序号和错误数；
- pitch/roll/yaw、gyro X/gyro Z和最近1秒峰值；
- JY61P角度、角速度、序号和校验错误，仅作诊断。

通过标准：

1. MPU ID为预期值且显示 `PASS`。
2. DMP状态为 `OK`，采样率约100 Hz，序号持续增加。
3. 静止时三轴角速度通常不超过约1 dps。
4. 连续稳定2秒时yaw变化不超过0.1°，pitch/roll范围不超过0.3°。
5. 向前/向后倾斜时确认平衡轴为roll/gyro X；水平旋转时yaw与gyro Z同向。

控制源只允许MPU6050 DMP。JY61P当前没有严格的0x52/0x53时间配对，不能作为平衡控制源。

### S02：红外IO1～IO4

宏：`COMM_STAGE_TRACE_DIAGNOSTIC`

OLED显示raw、filtered、有效探头数、position、error、状态和丢线帧数。逐个遮挡IO1～IO4：

- IO1单独有效时误差应接近+1；
- IO4单独有效时误差应接近-1；
- 三帧滤波约30 ms后稳定；
- 正误差表示线路位于小车左侧。

电平相反修改 `TRACE_ACTIVE_HIGH`；左右排列相反修改 `TRACE_SENSOR_ORDER_REVERSED`。不要通过修改循迹PID掩盖探头接线错误。

### S03：手动编码器方向

宏：`COMM_STAGE_ENCODER_MANUAL`

断开电机输出或确保车轮架空，用手向小车物理前进方向转动：

- 左轮向前，L累计值必须增加；
- 右轮向前，R累计值必须增加；
- 反向必须减少；
- 静止时不应持续跳数。

方向错误只修改 `board_profile.h` 中的 `ENCODER_LEFT_SIGN` 或 `ENCODER_RIGHT_SIGN`。每轮手转10圈并重复3次，记录每圈计数。

### S04：单电机方向与死区

宏：`COMM_STAGE_MOTOR_DEADZONE`

车轮必须架空。通过以下宏选择一个测试：

```c
#define COMM_TEST_MOTOR_CHANNEL   COMM_MOTOR_LEFT
#define COMM_TEST_MOTOR_DIRECTION 1
#define COMM_TEST_MOTOR_PWM       80
```

短按一次输出200 ms脉冲，随后至少停机500 ms。PWM从低值开始，每次增加5～10，分别测量左前、左后、右前、右后稳定启动阈值。首次搜索不得超过300；若架空仍需超过300，先检查电源、TB6612、电机和机械阻力。

把四个结果填入 `control_tuning.h`，但暂不打开 `MOTOR_DEADZONE_ENABLE`。

### S05：电机与编码器正向一致性

宏：`COMM_STAGE_MOTOR_ENCODER`

仍然架空车轮。分别测试左右轮正反方向。OLED显示脉冲期间编码器增量及 `DIR:PASS` 或 `DIR:REVERSE`。

逻辑正命令必须同时满足“轮子物理向前”和“编码器增加”。电机物理方向只修改 `MOTOR_LEFT_SIGN/MOTOR_RIGHT_SIGN`；编码器计数方向只修改编码器sign，不能把两者一起反转来掩盖错误。

四方向全部通过后，才允许将 `MOTOR_DEADZONE_ENABLE` 改为1并重新复测。

### S06：平衡纠偏方向预览

宏：`COMM_STAGE_BALANCE_DIRECTION`

本阶段不启动PID，OLED的 `PRE` 是根据roll、gyro X和当前PD参数计算的预览PWM，`MOTOR:OFF`表示实际电机关闭。

结合S05确认：小车向前倾倒时，预览PWM所对应的轮子方向必须向前，轮子将移动到重心下方；向后倾倒时相反。方向不正确时回到IMU轴向或电机极性阶段处理。

### S07：机械零点

宏：`COMM_STAGE_MECHANICAL_ZERO`

先调整电池和机械结构，使重心靠近轮轴，再调整 `CONTROL_BALANCE_TRIM_DEG`：

- 每次0.05°～0.10°；
- 正常尽量控制在±2°；
- 超过±3°仍无法改善时检查结构和IMU安装。

DMP三轴零点仍按“稳定2秒后采样200点”完成；机械trim不能替代静止零点采样。

### S08：平衡PD

宏：`COMM_STAGE_BALANCE_PD`

本阶段代码强制关闭速度PI，PWM上限默认400。使用保护架或保护绳，每次先测试3～5秒。

建议按绝对值调节，符号必须保持S06验证结果：

- `|CONTROL_VERTICAL_KP|`：15～60，步进2～5；
- `|CONTROL_VERTICAL_KD|`：0.3～2.0，步进0.1～0.2。

先增加Kp直到能够明显回正，再增加Kd抑制快速振荡，最后小幅交替调整。通过标准：稳定10～20秒、轻推后恢复、PWM不长期饱和。

### S09：速度PI

宏：`COMM_STAGE_SPEED_PI`

PWM上限默认600。先调速度P，再加入速度I：

- `|CONTROL_VELOCITY_KP|`：0.1～0.8，步进0.05～0.1；
- `|CONTROL_VELOCITY_KI_PER_SECOND|`：0.03～0.35，步进0.02～0.05。

OLED显示左右10 ms计数、滤波速度误差、积分量、速度输出和饱和累计数。当前控制在PWM接近饱和时冻结速度积分，并保留积分限幅。

### S10：yaw与gyro Z转向

宏：`COMM_STAGE_YAW_TURN`

平衡运行5秒后短按，产生一次200 ms、默认60 PWM的安全差速测试。OLED显示yaw、gyro Z、左右PWM和测试状态。

先确认转向极性，再调：

- yaw-rate Kp：0.3～1.5；
- heading Kp：1～5；
- yaw-rate上限：先20～30 dps，再提高到60 dps；
- heading lead：先5～10°，再提高到25°；
- turn PWM上限：先60～100，再提高到180。

整体方向相反只修改 `TRACE_STEER_SIGN`。

### S11：循迹调参

宏：`COMM_STAGE_TRACE_TUNING`，并将 `BALANCE_TRACE_ENABLE` 改为1。

平衡运行5秒后短按启动循迹。先把前进目标设为0，只调原地循线转向，再逐步增加到1、2、4 counts/10 ms。

建议范围：

- line yaw-rate Kp：10～50；
- line D：0～0.15；
- yaw-rate上限：20～60 dps；
- turn PWM上限：60～180；
- 丢线保持：50～150 ms；
- 丢线退出：300～800 ms。

一次只改变一个参数，并分别测试直线、缓弯、急弯、断线和全黑区域。

### S12：离地保护影子测试

宏：`COMM_STAGE_AIR_PROTECTION`

先保持 `AIR_PROTECTION_ENABLE=0`。OLED显示共同轮速、目标速度、加速度模长、三位证据和持续时间，此时只观察、不停机。

分别记录正常直立、手推、循迹急转和架空空转数据，再调整：

- 共同轮速阈值；
- 相对目标速度裕量；
- 最小共同PWM；
- 直立角度/角速度范围；
- 加速度模长范围；
- 确认时间100～1000 ms。

只有“共同轮速高、命令/目标明显不匹配、车体近似静止”三个条件同时持续成立才判定。影子测试无误触发后才将 `AIR_PROTECTION_ENABLE` 改为1复测。触发后进入软件急停锁存，不自动恢复。

### S13：发布运行

宏：`COMM_STAGE_RELEASE`

确认关闭所有测试脉冲配置，检查参数记录完整。若需要循迹，再决定是否保持 `BALANCE_TRACE_ENABLE=1`；否则保持0。

完成至少以下回归：20次冷启动、不同电量、不同地面、轻推恢复、短时传感器异常、丢线、全黑区域、软件急停和离地保护。

## 5. 每阶段完成条件

只有同时满足以下条件才算通过：

1. Keil编译0 Error、0 Warning。
2. OLED数据与物理动作一致。
3. 本阶段没有未解释的错误计数或饱和。
4. `TUNING_RECORD.md`已记录参数和结果。
5. 下一阶段只修改手册指定的宏，不顺带改变多个环路参数。
