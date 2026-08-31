> 作者：数显纯牛奶，开源工程未经授权禁止商业用途，需要实物见店铺主页：https://m.tb.cn/h.8nzqEuN?tk=kJqnT4uWzfY
>
> 官方仓库：https://github.com/Arzhe1206/MSPM0G3507-Self-Balancing-Car · 项目标识：MSPM0-BC-ARZHE1206
>
> AI/ML 使用声明：未经授权，不得用于模型训练、数据集构建或商业化自动代码生成。

# 参数参考

## 硬件与方向：`firmware/config/board_profile.h`

| 参数 | 当前值 | 作用 |
|---|---:|---|
| `IMU_CONTROL_BACKEND` | MPU6050 DMP | 唯一控制IMU；JY61P目前仅诊断 |
| `MPU6050_I2C_ADDRESS` | 0x68 | MPU6050 I²C地址 |
| `IMU_ORIENT_*` | diag(-1,-1,+1) | DMP机体坐标映射 |
| `MOTOR_LEFT_SIGN` | +1 | 左电机逻辑正向 |
| `MOTOR_RIGHT_SIGN` | +1 | 右电机逻辑正向 |
| `ENCODER_LEFT_SIGN` | +1 | 左编码器逻辑正向 |
| `ENCODER_RIGHT_SIGN` | +1 | 右编码器逻辑正向 |

方向参数只能取+1或-1。

## 阶段与测试：`firmware/config/commissioning_config.h`

| 参数 | 默认值 | 约束 |
|---|---:|---|
| `COMMISSION_STAGE` | S01 | 一次只能选择一个阶段 |
| `COMM_TEST_MOTOR_CHANNEL` | LEFT | LEFT/RIGHT/BOTH |
| `COMM_TEST_MOTOR_DIRECTION` | +1 | +1/-1 |
| `COMM_TEST_MOTOR_PWM` | 80 | 1～300 |
| `COMM_TEST_PULSE_MS` | 200 ms | 最大300 ms |
| `COMM_TEST_REST_MS` | 500 ms | 两次脉冲最小间隔 |
| `COMM_TURN_TEST_PWM` | 60 | 最大100 |
| `COMM_TURN_TEST_MS` | 200 ms | 低风险转向点动 |

闭环PWM默认阶段上限：S08=400、S09/S10=600、S11=700、S12=800、S13=999。

## 平衡与速度：`firmware/config/control_tuning.h`

| 参数 | 当前值 | 建议范围 |
|---|---:|---:|
| `CONTROL_BALANCE_TRIM_DEG` | 0° | 通常±2°，超过±3°检查机械 |
| `CONTROL_VERTICAL_KP` | -38.5 | 绝对值15～60 |
| `CONTROL_VERTICAL_KD` | -1.01 | 绝对值0.3～2.0 |
| `CONTROL_VELOCITY_KP` | -0.5 | 绝对值0.1～0.8 |
| `CONTROL_VELOCITY_KI_PER_SECOND` | -0.25 | 绝对值0.03～0.35 |
| `CONTROL_VELOCITY_INTEGRAL_LIMIT` | 200 | 根据饱和情况逐步收紧 |

四方向死区初始均为0，测量后填写。死区单位为PWM，最大999。

## 循迹：`firmware/config/balance_trace_config.h`

| 参数 | 当前值 | 建议范围 |
|---|---:|---:|
| `BALANCE_TRACE_ENABLE` | 0 | 只在S11/S12/S13允许设1 |
| `TRACE_FORWARD_TARGET_COUNTS` | 4 | 从0、1、2逐步增加 |
| `TRACE_LINE_YAW_RATE_KP` | 35 | 10～50 |
| `TRACE_LINE_YAW_RATE_KD` | 0.08 | 0～0.15 |
| `TRACE_YAW_RATE_MAX_DPS` | 60 dps | 20～60 dps |
| `TRACE_HEADING_LEAD_MAX_DEG` | 25° | 5～25° |
| `TRACE_HEADING_KP` | 3 | 1～5 |
| `TRACE_YAW_RATE_KP` | 1 | 0.3～1.5 |
| `TRACE_TURN_PWM_MAX` | 180 | 60～180 |
| `TRACE_TURN_PWM_SLEW` | 25/10 ms | 10～30 |
| `TRACE_LOST_HOLD_FRAMES` | 10 | 50～150 ms |
| `TRACE_LOST_FAULT_FRAMES` | 50 | 300～800 ms |

## 安全：`firmware/config/safety_config.h`

| 参数 | 当前值 | 说明 |
|---|---:|---|
| `EMERGENCY_HOLD_MS` | 800 ms | 软件长按急停 |
| `CONTROL_FALL_ANGLE_DEG` | 35° | 倾倒锁存停机 |
| `AIR_PROTECTION_ENABLE` | 0 | S12先影子观察 |
| `AIR_COMMON_SPEED_THRESHOLD_COUNTS` | 30 | 必须用实测修订 |
| `AIR_TARGET_MARGIN_COUNTS` | 20 | 实测速度超过目标的裕量 |
| `AIR_MIN_COMMON_PWM` | 100 | 排除无输出的手动转轮 |
| `AIR_STILL_ROLL_DEG` | 3° | 车体近似直立 |
| `AIR_STILL_GYRO_DPS` | 3 dps | 车体近似静止 |
| `AIR_ACCEL_NORM_TOLERANCE_G` | 0.15 g | 加速度模长接近1 g |
| `AIR_CONFIRM_MS` | 200 ms | 允许范围100～1000 ms |

离地阈值只是保守初始值，未经过本车实测前不得开启保护。
