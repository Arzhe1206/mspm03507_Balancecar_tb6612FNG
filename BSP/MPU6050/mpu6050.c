/***
	*******************************************************************************************************************************************************************
	* @file    mpu6050.c
	* @version V2.1
	* @date    2024-7-22
	* @author  御龙	
	* @brief   MSPM0G3507小车PID调试通用模板
   *************************************************************************************************
   *  @description
	*	
	*  初始化采用了无名创新的硬件IIC的初始化和读取原始值的函数
	*  在上面基础上，本人加上了卡尔曼滤波和一阶低通滤波，对原始值进行滤波处理，并且通过一些计算，算出了欧拉角
	*  如果Yaw不准，有偏差，可跳转337行，更改补偿系数，有一些人是不需要那个补偿系数的，可以删掉，有些人需要，看情况更改
   *
>>>>> 其他说明：未经允许不可擅自转发、售卖本套代码，大家都是我国的有为青年，请保持好自己的初心，在此，向你表达我的感谢
	*************************************************************************************************************************************************************
***/
#include "mpu6050.h"
#include "board.h"
/*-------------------------------------------------------------------------------------------*/
/*----------------------------------------使用-----------------------------------------------*/
/*-------------------------------------------------------------------------------------------*/
#define    MPU6050_I2C_INST		I2C_0_INST		//硬件IIC的宏定义去ti_msp_dl_config.h去看
//在主函数初始化的地方--调用		mpu6050_init()
//在定时器10ms中断中----调用		AHRS_Geteuler()
//读取	mpu6050.Pitch	mpu6050.Roll	mpu6050.Yaw



MPU6050_DEF mpu6050;

uint8_t i2c0_write_n_byte(uint8_t DevAddr, uint8_t RegAddr, uint8_t *buf, uint8_t nBytes){
		static uint8_t temp_reg_dddr=0;
    uint8_t n;
    uint32_t Byte4Fill;
    temp_reg_dddr = RegAddr;
    DL_I2C_fillControllerTXFIFO(MPU6050_I2C_INST, &buf[0], nBytes);

    /* Wait for I2C to be Idle */
    while (!(DL_I2C_getControllerStatus(MPU6050_I2C_INST) & DL_I2C_CONTROLLER_STATUS_IDLE));

    DL_I2C_flushControllerTXFIFO(MPU6050_I2C_INST); 
    DL_I2C_fillControllerTXFIFO(MPU6050_I2C_INST, &temp_reg_dddr, 1);

    /* Send the packet to the controller.
     * This function will send Start + Stop automatically.
     */
    DL_I2C_startControllerTransfer(MPU6050_I2C_INST, DevAddr,DL_I2C_CONTROLLER_DIRECTION_TX, (nBytes+1));
    n = 0;
    do {
        Byte4Fill = DL_I2C_getControllerTXFIFOCounter(MPU6050_I2C_INST);
        if(Byte4Fill > 1)
        {
            DL_I2C_fillControllerTXFIFO(MPU6050_I2C_INST, &buf[n], 1);
            n++;
        }
    }while(n<nBytes);

    /* Poll until the Controller writes all bytes */
    while (DL_I2C_getControllerStatus(MPU6050_I2C_INST) &DL_I2C_CONTROLLER_STATUS_BUSY_BUS);

    /* Trap if there was an error */
    if (DL_I2C_getControllerStatus(MPU6050_I2C_INST) &DL_I2C_CONTROLLER_STATUS_ERROR) 
		{
        /* LED will remain high if there is an error */
        __NOP();//__BKPT(0);
    }

    /* Add delay between transfers */
    delay_cycles(1000);

    return nBytes;
}
void  i2c0_read_n_byte(uint8_t DevAddr, uint8_t RegAddr, uint8_t *buf, uint8_t nBytes){
		static uint8_t temp_reg_dddr=0;
    uint8_t n;
    uint32_t Byte4Fill;
    temp_reg_dddr = RegAddr;
    DL_I2C_fillControllerTXFIFO(MPU6050_I2C_INST, &buf[0], nBytes);

    /* Wait for I2C to be Idle */
    while (!(DL_I2C_getControllerStatus(MPU6050_I2C_INST) & DL_I2C_CONTROLLER_STATUS_IDLE));

    DL_I2C_flushControllerRXFIFO(MPU6050_I2C_INST); 
    DL_I2C_fillControllerTXFIFO(MPU6050_I2C_INST, &temp_reg_dddr, 1);

    /* Send the packet to the controller.
     * This function will send Start + Stop automatically.
     */
    DL_I2C_startControllerTransfer(MPU6050_I2C_INST, DevAddr,DL_I2C_CONTROLLER_DIRECTION_TX, (nBytes+1));
    n = 0;
    do {
        Byte4Fill = DL_I2C_getControllerTXFIFOCounter(MPU6050_I2C_INST);
        if(Byte4Fill > 1)
        {
            DL_I2C_fillControllerTXFIFO(MPU6050_I2C_INST, &buf[n], 1);
            n++;
        }
    }while(n<nBytes);

    /* Poll until the Controller writes all bytes */
    while (DL_I2C_getControllerStatus(MPU6050_I2C_INST) &DL_I2C_CONTROLLER_STATUS_BUSY_BUS);

    /* Trap if there was an error */
    if (DL_I2C_getControllerStatus(MPU6050_I2C_INST) &DL_I2C_CONTROLLER_STATUS_ERROR) 
		{
        /* LED will remain high if there is an error */
        __NOP();//__BKPT(0);
    }
}
//************I2C write register **********************
void I2C_WriteReg(uint8_t DevAddr,uint8_t reg_addr, uint8_t *reg_data, uint8_t count){
    unsigned char I2Ctxbuff[8] = {0x00};

    I2Ctxbuff[0] = reg_addr;
    unsigned char i, j = 1;

    for (i = 0; i < count; i++) {
        I2Ctxbuff[j] = reg_data[i];
        j++;
    }

    //    DL_I2C_flushControllerTXFIFO(MPU6050_I2C_INST);
    DL_I2C_fillControllerTXFIFO(MPU6050_I2C_INST, &I2Ctxbuff[0], count + 1);

    /* Wait for I2C to be Idle */
    while (!(DL_I2C_getControllerStatus(MPU6050_I2C_INST) &
             DL_I2C_CONTROLLER_STATUS_IDLE))
        ;

    DL_I2C_startControllerTransfer(MPU6050_I2C_INST, DevAddr,
        DL_I2C_CONTROLLER_DIRECTION_TX, count + 1);

    while (DL_I2C_getControllerStatus(MPU6050_I2C_INST) &
           DL_I2C_CONTROLLER_STATUS_BUSY_BUS)
        ;
    /* Wait for I2C to be Idle */
    while (!(DL_I2C_getControllerStatus(MPU6050_I2C_INST) &
             DL_I2C_CONTROLLER_STATUS_IDLE))
        ;
    //Avoid BQ769x2 to stretch the SCLK too long and generate a timeout interrupt at 400kHz because of low power mode
    // if(DL_I2C_getRawInterruptStatus(MPU6050_I2C_INST,DL_I2C_INTERRUPT_CONTROLLER_CLOCK_TIMEOUT))
    // {
    //     DL_I2C_flushControllerTXFIFO(MPU6050_I2C_INST);
    //     DL_I2C_clearInterruptStatus(MPU6050_I2C_INST,DL_I2C_INTERRUPT_CONTROLLER_CLOCK_TIMEOUT);
    //     I2C_WriteReg(reg_addr, reg_data, count);
    // }
    DL_I2C_flushControllerTXFIFO(MPU6050_I2C_INST);
}
//************I2C read register **********************
void I2C_ReadReg(uint8_t DevAddr,uint8_t reg_addr, uint8_t *reg_data, uint8_t count){
    DL_I2C_fillControllerTXFIFO(MPU6050_I2C_INST, &reg_addr, count);

    /* Wait for I2C to be Idle */
    while (!(DL_I2C_getControllerStatus(MPU6050_I2C_INST) &
             DL_I2C_CONTROLLER_STATUS_IDLE))
        ;

    DL_I2C_startControllerTransfer(
        MPU6050_I2C_INST, DevAddr, DL_I2C_CONTROLLER_DIRECTION_TX, 1);

    while (DL_I2C_getControllerStatus(MPU6050_I2C_INST) &
           DL_I2C_CONTROLLER_STATUS_BUSY_BUS)
        ;
    /* Wait for I2C to be Idle */
    while (!(DL_I2C_getControllerStatus(MPU6050_I2C_INST) &
             DL_I2C_CONTROLLER_STATUS_IDLE))
        ;

    DL_I2C_flushControllerTXFIFO(MPU6050_I2C_INST);

    /* Send a read request to Target */
    DL_I2C_startControllerTransfer(
        MPU6050_I2C_INST, DevAddr, DL_I2C_CONTROLLER_DIRECTION_RX, count);

    for (uint8_t i = 0; i < count; i++) {
        while (DL_I2C_isControllerRXFIFOEmpty(MPU6050_I2C_INST))
            ;
        reg_data[i] = DL_I2C_receiveControllerData(MPU6050_I2C_INST);
    }
}

//下面搬的是无名创新的硬件IIC读取MPU6050
#define	SMPLRT_DIV		0x19
#define	MPU_CONFIG		0x1A
#define	GYRO_CONFIG		0x1B
#define	ACCEL_CONFIG	        0x1C
#define	ACCEL_XOUT_H	        0x3B
#define	ACCEL_XOUT_L	        0x3C
#define	ACCEL_YOUT_H	        0x3D
#define	ACCEL_YOUT_L	        0x3E
#define	ACCEL_ZOUT_H	        0x3F
#define	ACCEL_ZOUT_L	        0x40
#define	TEMP_OUT_H		0x41
#define	TEMP_OUT_L		0x42
#define	GYRO_XOUT_H		0x43
#define	GYRO_XOUT_L		0x44
#define	GYRO_YOUT_H		0x45
#define	GYRO_YOUT_L		0x46
#define	GYRO_ZOUT_H		0x47
#define	GYRO_ZOUT_L		0x48
#define	PWR_MGMT_1		0x6B
#define	WHO_AM_I		0x75
#define USER_CTRL		0x6A
#define INT_PIN_CFG		0x37

void Single_WriteI2C(unsigned char SlaveAddress,unsigned char REG_Address,unsigned char REG_data){
	I2C_WriteReg(SlaveAddress,REG_Address,&REG_data,1);
}
unsigned char Single_ReadI2C(unsigned char SlaveAddress,unsigned char REG_Address){
	uint8_t data;
	I2C_ReadReg(SlaveAddress,REG_Address,&data,1);
	return data;
}
#define imu_adress 0x68

uint8_t read_imu[5];
void mpu6050_init(void){
  Single_WriteI2C(imu_adress,PWR_MGMT_1  , 0x00);//关闭所有中断,解除休眠
  Single_WriteI2C(imu_adress,SMPLRT_DIV  , 0x09);// sample rate.  Fsample= 1Khz/(<this value>+1) = 1000Hz
  Single_WriteI2C(imu_adress,MPU_CONFIG  , 0x06);//
  Single_WriteI2C(imu_adress,GYRO_CONFIG , 0x18);//
  Single_WriteI2C(imu_adress,ACCEL_CONFIG, 0x18);// 
	
	read_imu[0]=Single_ReadI2C(imu_adress,PWR_MGMT_1);
	read_imu[1]=Single_ReadI2C(imu_adress,SMPLRT_DIV);
	read_imu[2]=Single_ReadI2C(imu_adress,MPU_CONFIG);
	read_imu[3]=Single_ReadI2C(imu_adress,GYRO_CONFIG);
	read_imu[4]=Single_ReadI2C(imu_adress,ACCEL_CONFIG);
}
void mpu6050_read(int16_t *gyro,int16_t *accel,float *temperature){
	uint8_t buf[14];
	int16_t temp;
	I2C_ReadReg(imu_adress,ACCEL_XOUT_H,buf,14);
	accel[0]=(int16_t)((buf[0]<<8)|buf[1]);
	accel[1]=(int16_t)((buf[2]<<8)|buf[3]);
	accel[2]=(int16_t)((buf[4]<<8)|buf[5]);	
	temp		=(int16_t)((buf[6]<<8)|buf[7]);
	gyro[0]	=(int16_t)((buf[8]<<8)|buf[9]);
	gyro[1]	=(int16_t)((buf[10]<<8)|buf[11]);
	gyro[2]	=(int16_t)((buf[12]<<8)|buf[13]);	
	*temperature=36.53f+(float)(temp/340.0f);	
}

/*-------------------------------------------------------------------------------------------*/
/*------------------------------------角度计算部分-------------------------------------------*/
/*-------------------------------------------------------------------------------------------*/
/*---------------陀螺仪采集---------------------*/
#define GYRO_GATHER   	700 //原来是100
#define RtA 			57.324841f				
#define AtR    			0.0174533f				
#define Acc_G 			0.0011963f				
#define Gyro_G 			0.03051756f				
#define Gyro_Gr			0.0005426f

#define Offset_Times 	200.0		//上电校准次数
#define Sampling_Time	0.01		//采样读取时间5ms

#define IIR_ORDER     4      //使用IIR滤波器的阶数
double b_IIR[IIR_ORDER+1] ={0.0008f, 0.0032f, 0.0048f, 0.0032f, 0.0008f};  //系数b
double a_IIR[IIR_ORDER+1] ={1.0000f, -3.0176f, 3.5072f, -1.8476f, 0.3708f};//系数a
double InPut_IIR[3][IIR_ORDER+1]  = {0};
double OutPut_IIR[3][IIR_ORDER+1] = {0};
#define gxf 0
#define gyf 0
#define gzf 0
void MPU6050_ReadDatas_Proc(void){
	static uint16_t time=0;//初始化校准次数
	mpu6050_read(mpu6050.Gyro_Original,mpu6050.Accel_Original,&mpu6050.temperature);
	if(time<Offset_Times)//计算初始校准值
	{time++;
		mpu6050.Accel_Offset[0]+=(float)mpu6050.Accel_Original[0]/Offset_Times;//读取数据计算偏差
		mpu6050.Accel_Offset[1]+=(float)mpu6050.Accel_Original[1]/Offset_Times;//读取数据计算偏差
		mpu6050.Accel_Offset[2]+=(float)mpu6050.Accel_Original[2]/Offset_Times;//读取数据计算偏差
		mpu6050.Gyro_Offset[0] +=(float)mpu6050.Gyro_Original[0]/Offset_Times;//读取数据计算偏差
		mpu6050.Gyro_Offset[1] +=(float)mpu6050.Gyro_Original[1]/Offset_Times;//读取数据计算偏差
		mpu6050.Gyro_Offset[2] +=(float)mpu6050.Gyro_Original[2]/Offset_Times;//读取数据计算偏差		
	}
	else
	{	// 加速度值赋值（减去零漂）
		mpu6050.Accel_Calulate[0] = mpu6050.Accel_Original[0]- mpu6050.Accel_Offset[0];//角加速度不用
		mpu6050.Accel_Calulate[1] = mpu6050.Accel_Original[1]- mpu6050.Accel_Offset[1];
		mpu6050.Accel_Calulate[2] = mpu6050.Accel_Original[2]- mpu6050.Accel_Offset[2];
		// 陀螺仪值赋值（减去零漂）
		mpu6050.Gyro_Calulate[0] = mpu6050.Gyro_Original[0] - mpu6050.Gyro_Offset[0]-gxf;
		mpu6050.Gyro_Calulate[1] = mpu6050.Gyro_Original[1] - mpu6050.Gyro_Offset[1]-gyf;
		mpu6050.Gyro_Calulate[2] = mpu6050.Gyro_Original[2] - mpu6050.Gyro_Offset[2]-gzf;
		
		/***********角加速度滤波（方法二选一）***********/
		mpu6050.Accel_Average[0]=mpu6050.Accel_Calulate[0];
		mpu6050.Accel_Average[1]=mpu6050.Accel_Calulate[1];
		mpu6050.Accel_Average[2]=mpu6050.Accel_Calulate[2];	
//		mpu6050.Gyro_Average[0]=mpu6050.Gyro_Calulate[0]; 
//		mpu6050.Gyro_Average[1]=mpu6050.Gyro_Calulate[1];
//		mpu6050.Gyro_Average[2]=mpu6050.Gyro_Calulate[2]; 
		//一、角加速度IIR滤波
//		mpu6050.Accel_Average[0] = IIR_I_Filter(mpu6050.Accel_Calulate[0], InPut_IIR[0], OutPut_IIR[0], b_IIR, IIR_ORDER+1, a_IIR, IIR_ORDER+1);
//		mpu6050.Accel_Average[1] = IIR_I_Filter(mpu6050.Accel_Calulate[1], InPut_IIR[1], OutPut_IIR[1], b_IIR, IIR_ORDER+1, a_IIR, IIR_ORDER+1);
//		mpu6050.Accel_Average[2] = IIR_I_Filter(mpu6050.Accel_Calulate[2], InPut_IIR[2], OutPut_IIR[2], b_IIR, IIR_ORDER+1, a_IIR, IIR_ORDER+1);
		//二、角加速度卡尔曼滤波方法：
//		static struct KalmanFilter EKF[3]={{0.02,0,0,0,0.001,0.543},{0.02,0,0,0,0.001,0.543},{0.02,0,0,0,0.001,0.543}};
//		kalmanfiter(&EKF[0],(float)mpu6050.Accel_Calulate[0]);  
//		mpu6050.Accel_Average[0] =  (int16_t)EKF[0].Out;
//		kalmanfiter(&EKF[1],(float)mpu6050.Accel_Calulate[1]);  
//		mpu6050.Accel_Average[1] =  (int16_t)EKF[1].Out;
//		kalmanfiter(&EKF[2],(float)mpu6050.Accel_Calulate[2]);  
//		mpu6050.Accel_Average[2] =  (int16_t)EKF[2].Out;
		
		/*******************角速度滤波********************/

		static struct KalmanFilter EKF[3]={{0.02,0,0,0,0.001,0.543},{0.02,0,0,0,0.001,0.543},{0.02,0,0,0,0.001,0.543}};
		kalmanfiter(&EKF[0],(float)mpu6050.Gyro_Calulate[0]);  
		mpu6050.Gyro_Average[0] =  (int16_t)EKF[0].Out;
		kalmanfiter(&EKF[1],(float)mpu6050.Gyro_Calulate[1]);  
		mpu6050.Gyro_Average[1] =  (int16_t)EKF[1].Out;
		kalmanfiter(&EKF[2],(float)mpu6050.Gyro_Calulate[2]);  
		mpu6050.Gyro_Average[2] =  (int16_t)EKF[2].Out;		
	}
}
#define MPU_Aceel_Gyro_Kp	0.95
float pitch,roll,Yaw;
float Gyro_Z_Measeure = 0;
void AHRS_Geteuler(void){
	MPU6050_ReadDatas_Proc();//读取滤波数据
	
	float ax,ay,az;
	ax=mpu6050.Accel_Average[0];
	ay=mpu6050.Accel_Average[1];
	az=mpu6050.Accel_Average[2];

	float pitch1	= 	RtA*atan(ay/sqrtf(ax*ax+az*az));  // 俯仰角
	float roll1		=	-RtA*atan(ax/sqrtf(ay*ay+az*az)); // 横滚角
	
	pitch+= (mpu6050.Gyro_Average[0])*2000/32768*Sampling_Time;//俯仰角
	roll+= (mpu6050.Gyro_Average[1])*2000/32768*Sampling_Time;//横滚角
//	Yaw += (mpu6050.Gyro_Calulate[2])*2000/32768.0*Sampling_Time;	
	mpu6050.Pitch =	 pitch1*MPU_Aceel_Gyro_Kp+pitch*(1-MPU_Aceel_Gyro_Kp);		// 俯仰角
	mpu6050.Roll  =  roll1*MPU_Aceel_Gyro_Kp+roll*(1-MPU_Aceel_Gyro_Kp);	 		// 横滚角	
	
//	Gyro_Z_Measeure = (mpu6050.Gyro_Calulate[2])*2000/32768.0;
//	Yaw += Gyro_Z_Measeure*Sampling_Time;
//	mpu6050.Yaw  = 	Yaw - Yaw*0.16667;//后面这个0.16667是为了补偿角度	
//	static struct KalmanFilter EKF[3]={{0.02,0,0,0,0.001,0.543},{0.02,0,0,0,0.001,0.543},{0.02,0,0,0,0.001,0.543}};
//	kalmanfiter(&EKF[0],(float)Yaw); 	
//	mpu6050.Yaw =  (int16_t)EKF[0].Out-Yaw*0.16667;	
}
//-------------------------------------------MyFilter-------------------------------------------------//
/*====================================================================================================*/
/*====================================================================================================*
** 函数名称: IIR_I_Filter
** 功能描述: IIR直接I型滤波器
** 输    入: InData 为当前数据
**           *x     储存未滤波的数据
**           *y     储存滤波后的数据
**           *b     储存系数b
**           *a     储存系数a
**           nb     数组*b的长度
**           na     数组*a的长度
**           LpfFactor
** 输    出: OutData         
** 说    明: 无
** 函数原型: y(n) = b0*x(n) + b1*x(n-1) + b2*x(n-2) -
                    a1*y(n-1) - a2*y(n-2)
**====================================================================================================*/
/*====================================================================================================*/
double IIR_I_Filter(double InData, double *x, double *y, double *b, short nb, double *a, short na){
  double z1,z2;
  short i;
  double OutData;
  
  for(i=nb-1; i>0; i--)
  {
    x[i]=x[i-1];
  }
  
  x[0] = InData;
  
  for(z1=0,i=0; i<nb; i++)
  {
    z1 += x[i]*b[i];
  }
  
  for(i=na-1; i>0; i--)
  {
    y[i]=y[i-1];
  }
  
  for(z2=0,i=1; i<na; i++)
  {
    z2 += y[i]*a[i];
  }
  
  y[0] = z1 - z2; 
  OutData = y[0];
    
  return OutData;
}
/*====================================================================================================*/
/*====================================================================================================*
**函数 : LPF_1st
**功能 : 一阶低通滤波
**输入 :  
**輸出 : None
**备注 : None
**====================================================================================================*/
/*====================================================================================================*/
float LPF_1st(float oldData, float newData, float lpf_factor){
	return oldData * (1 - lpf_factor) + newData * lpf_factor;    //上个数据*一定的比例+现在的数据*一定的比例
}
//一维卡尔曼滤波
void kalmanfiter(struct KalmanFilter *EKF,float input){
	EKF->NewP = EKF->LastP + EKF->Q;
	EKF->Kg = EKF->NewP / (EKF->NewP + EKF->R);
	EKF->Out = EKF->Out + EKF->Kg * (input - EKF->Out);
	EKF->LastP = (1 - EKF->Kg) * EKF->NewP;
}
