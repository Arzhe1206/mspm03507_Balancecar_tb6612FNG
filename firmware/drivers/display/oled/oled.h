/*
 * MSPM0G3507 Balance Car
 * 作者：数显纯牛奶，开源工程未经授权禁止商业用途，需要实物见店铺主页：https://m.tb.cn/h.8nzqEuN?tk=kJqnT4uWzfY
 * 官方仓库：https://github.com/Arzhe1206/mspm03507_Balancecar_tb6612FNG
 * 许可：LicenseRef-BalanceCar-NonCommercial
 * 项目标识：MSPM0-BC-ARZHE1206
 * AI/ML 使用声明：未经授权，不得用于模型训练、数据集构建或商业化自动代码生成。
 */
/**********************************************************
 * OLED.h
 *
 * 软件 I2C SSD1306 128x64 OLED 驱动
 *
 * 特性：
 * - page-major framebuffer
 * - 脏页/脏列局部刷新
 * - 非阻塞分片刷新 Display_Poll()
 * - 适配菜单方框、波形、三角形等 UI 元素
 **********************************************************/

#ifndef __OLED_H
#define __OLED_H

#include "board.h"

/* ============================================================
 * 基本参数
 * ============================================================ */

#define OLED_WIDTH          128
#define OLED_HEIGHT         64
#define OLED_PAGES          8

/* ============================================================
 * 软件 I2C 配置
 *
 * OLED_I2C_DELAY_US:
 *   原来为 10，太慢，约 33kHz。
 *   建议先改为 1。
 *   如果显示不稳定，可以改为 2 或 3。
 *
 * OLED_SKIP_ACK:
 *   1：跳过无意义 WaitAck，推荐。
 *   0：保留原伪 WaitAck，会慢很多。
 *
 * OLED_FLUSH_CHUNK_BYTES:
 *   每次 Display_Poll() 最多发送的数据字节数。
 *   delay_us(1) 时可用 8 或 16。
 *   如果 delay_us(3~5)，建议 4。
 *   如果仍然使用 delay_us(10)，建议 1 或 2。
 * ============================================================ */

#ifndef OLED_I2C_DELAY_US
#define OLED_I2C_DELAY_US           1
#endif

#ifndef OLED_SKIP_ACK
#define OLED_SKIP_ACK               1
#endif

#ifndef OLED_FLUSH_CHUNK_BYTES
#define OLED_FLUSH_CHUNK_BYTES      8
#endif

/* ============================================================
 * GPIO 定义
 * ============================================================ */

#define OLED_SCL_Clr()      DL_GPIO_clearPins(OLED_PORT, OLED_SCL_PIN)
#define OLED_SCL_Set()      DL_GPIO_setPins(OLED_PORT, OLED_SCL_PIN)

#define OLED_SDA_Clr()      DL_GPIO_clearPins(OLED_PORT, OLED_SDA_PIN)
#define OLED_SDA_Set()      DL_GPIO_setPins(OLED_PORT, OLED_SDA_PIN)

#define OLED_CMD            0
#define OLED_DATA           1

/* ============================================================
 * 小三角方向
 * ============================================================ */

typedef enum
{
    OLED_TRI_UP    = 0,
    OLED_TRI_DOWN  = 1,
    OLED_TRI_LEFT  = 2,
    OLED_TRI_RIGHT = 3
} OLED_TriDir_t;

/* ============================================================
 * 初始化 / 刷新
 * ============================================================ */

void OLED_Init(void);

/*
 * OLED_Clear()
 *
 * 阻塞清屏，只建议在初始化阶段使用。
 * 运行时不要频繁调用。
 */
void OLED_Clear(void);

/*
 * OLED_ClearBuffer()
 *
 * 清空本地帧缓冲，并标记全屏脏区。
 * 不会阻塞发送，需要配合 Display_Poll()。
 */
void OLED_ClearBuffer(void);

/*
 * OLED_Refresh()
 *
 * 兼容旧接口。
 * 现在不会阻塞刷屏，而是标记全屏脏区并启动异步刷新。
 * 实际发送由 Display_Poll() 完成。
 */
void OLED_Refresh(void);

/*
 * OLED_RefreshBlocking()
 *
 * 阻塞刷新整屏。
 * 只建议初始化、切页面、开机画面使用。
 */
void OLED_RefreshBlocking(void);

/*
 * OLED_StartFlush()
 *
 * 启动异步局部刷新。
 * 如果已经正在刷新，则不做任何操作。
 */
void OLED_StartFlush(void);

/*
 * Display_Poll()
 *
 * 非阻塞刷新服务函数。
 * 必须放到主循环中周期调用。
 */
void Display_Poll(void);

/*
 * Display_IsBusy()
 *
 * 返回是否还有显示数据正在等待发送。
 */
u8 Display_IsBusy(void);

/* ============================================================
 * 脏区管理
 * ============================================================ */

void Display_InvalidateRect(u8 x0, u8 y0, u8 x1, u8 y1);
void Display_InvalidateAll(void);

/* ============================================================
 * I2C 底层接口
 * ============================================================ */

void OLED_I2C_Start(void);
void OLED_I2C_Stop(void);
void OLED_I2C_WaitAck(void);
void OLED_Send_Byte(u8 dat);
void OLED_WR_Byte(u8 dat, u8 mode);

/* ============================================================
 * 显示控制
 * ============================================================ */

void OLED_ColorTurn(u8 i);
void OLED_DisplayTurn(u8 i);
void OLED_DisPlay_On(void);
void OLED_DisPlay_Off(void);

/* ============================================================
 * 基础绘图
 * ============================================================ */

void OLED_ClearPoint(u8 x, u8 y);

void OLED_DrawPoint(u8 x, u8 y, u8 t);

void OLED_DrawHLine(u8 x0, u8 x1, u8 y, u8 color);
void OLED_DrawVLine(u8 x, u8 y0, u8 y1, u8 color);
void OLED_DrawLine(u8 x1, u8 y1, u8 x2, u8 y2, u8 mode);

void OLED_DrawRect(u8 x0, u8 y0, u8 x1, u8 y1, u8 color);
void OLED_FillRect(u8 x0, u8 y0, u8 x1, u8 y1, u8 color);

void OLED_DrawCircle(u8 x, u8 y, u8 r);

void OLED_DrawTriangle(u8 x0, u8 y0,
                       u8 x1, u8 y1,
                       u8 x2, u8 y2,
                       u8 color);

void OLED_DrawSmallTriangle(u8 cx, u8 cy, u8 size, u8 dir, u8 color);
void OLED_FillSmallTriangle(u8 cx, u8 cy, u8 size, u8 dir, u8 color);

/* ============================================================
 * 字体尺寸
 * ============================================================ */

u8 OLED_GetFontWidth(u8 size1);
u8 OLED_GetFontCellHeight(u8 size1);

/* ============================================================
 * 文字显示
 * ============================================================ */

void OLED_ShowChar(u8 x, u8 y, u8 chr, u8 size1, u8 mode);
void OLED_ShowChar6x8(u8 x, u8 y, u8 chr, u8 mode);

void OLED_ShowString(u8 x, u8 y, const char *chr, u8 size1, u8 mode);

void OLED_ShowNum(u8 x, u8 y, u32 num, u8 len, u8 size1, u8 mode);

void OLED_ShowChinese(u8 x, u8 y, u8 num, u8 size1, u8 mode);

void OLED_ShowPicture(u8 x, u8 y, u8 sizex, u8 sizey, u8 BMP[], u8 mode);

void OLED_ShowFloatNum(uint8_t x,
                       uint8_t y,
                       float num,
                       uint8_t intlen,
                       uint8_t fralen,
                       uint8_t size1,
                       uint8_t mode);

void OLED_ShowBin8(u8 x, u8 y, u8 num, u8 size1, u8 mode);

void OLED_ShowSignedNum(uint8_t x,
                        uint8_t y,
                        int32_t num,
                        uint8_t len,
                        uint8_t size1,
                        uint8_t mode);

void OLED_ShowTargetValueEx(u8 x,
                            u8 y,
                            const char *label,
                            int value,
                            u8 digits,
                            u8 size,
                            u8 mode);

/*
 * 旧版滚动显示函数。
 * 新驱动不推荐继续使用阻塞式全屏滚动。
 * 这里保留接口仅为兼容，实际为空实现。
 */
void OLED_ScrollDisplay(u8 num, u8 space, u8 mode);

#endif
														
