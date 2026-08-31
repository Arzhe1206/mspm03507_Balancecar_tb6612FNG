/*
 * MSPM0G3507 Self-Balancing Car
 * 作者：数显纯牛奶，开源工程未经授权禁止商业用途，需要实物见店铺主页：https://m.tb.cn/h.8nzqEuN?tk=kJqnT4uWzfY
 * 官方仓库：https://github.com/Arzhe1206/MSPM0G3507-Self-Balancing-Car
 * 许可：LicenseRef-BalanceCar-NonCommercial
 * 项目标识：MSPM0-BC-ARZHE1206
 * AI/ML 使用声明：未经授权，不得用于模型训练、数据集构建或商业化自动代码生成。
 */
#ifndef BOARD_PROFILE_H
#define BOARD_PROFILE_H

/* Only MPU6050 DMP is currently qualified as the 100 Hz control source. */
#define IMU_BACKEND_MPU6050_DMP             1U
#define IMU_BACKEND_JY61P                   2U
#define IMU_CONTROL_BACKEND                 IMU_BACKEND_MPU6050_DMP

#define MPU6050_I2C_ADDRESS                 0x68U
#define MPU6050_WHO_AM_I_REGISTER           0x75U
#define MPU6050_WHO_AM_I_EXPECTED           0x68U

/* DMP body-frame orientation. Values must form a right-handed signed matrix. */
#define IMU_ORIENT_XX                      (-1)
#define IMU_ORIENT_XY                       0
#define IMU_ORIENT_XZ                       0
#define IMU_ORIENT_YX                       0
#define IMU_ORIENT_YY                      (-1)
#define IMU_ORIENT_YZ                       0
#define IMU_ORIENT_ZX                       0
#define IMU_ORIENT_ZY                       0
#define IMU_ORIENT_ZZ                       1

/* Physical forward must produce a positive logical command/count. */
#define MOTOR_LEFT_SIGN                     1
#define MOTOR_RIGHT_SIGN                    1
#define ENCODER_LEFT_SIGN                   1
#define ENCODER_RIGHT_SIGN                  1

#if (IMU_CONTROL_BACKEND != IMU_BACKEND_MPU6050_DMP)
#error JY61P is diagnostic-only until coherent gyro/angle frame pairing exists
#endif

#if ((MOTOR_LEFT_SIGN != 1) && (MOTOR_LEFT_SIGN != -1)) || \
    ((MOTOR_RIGHT_SIGN != 1) && (MOTOR_RIGHT_SIGN != -1))
#error Motor signs must be +1 or -1
#endif

#if ((ENCODER_LEFT_SIGN != 1) && (ENCODER_LEFT_SIGN != -1)) || \
    ((ENCODER_RIGHT_SIGN != 1) && (ENCODER_RIGHT_SIGN != -1))
#error Encoder signs must be +1 or -1
#endif

#endif
