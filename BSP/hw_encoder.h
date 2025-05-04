#ifndef _HW_ENCODER_H_
#define _HW_ENCODER_H_

#include "ti_msp_dl_config.h"

typedef enum {
    FORWARD,  // 正向
    REVERSAL  // 反向
} ENCODER_DIR;

typedef struct {
    volatile long long temp_count,temp_count2; //保存实时计数值
    int count,count2;         						 //根据定时器时间更新的计数值
    ENCODER_DIR dir;            	 //旋转方向
} ENCODER_RES;

void encoder_init(void);
int get_encoder_count(void);
int get_encoder_count2(void);
ENCODER_DIR get_encoder_dir(void);
void encoder_update(void);
void encoder_reset();


#endif


