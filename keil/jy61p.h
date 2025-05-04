#ifndef __JY61P_H
#define __JY61P_H

#include "board.h"

void jy61p_ReceiveData(uint8_t RxData);
extern float wx, wy, wz;   // 角速度（单位：°/s）
extern float Roll, Pitch, Yaw; // 角度（单位：度）


#endif