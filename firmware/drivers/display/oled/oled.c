/*
 * MSPM0G3507 Balance Car
 * 作者：数显纯牛奶，开源工程未经授权禁止商业用途，需要实物见店铺主页：https://m.tb.cn/h.8nzqEuN?tk=kJqnT4uWzfY
 * 官方仓库：https://github.com/Arzhe1206/mspm03507_Balancecar_tb6612FNG
 * 许可：LicenseRef-BalanceCar-NonCommercial
 * 项目标识：MSPM0-BC-ARZHE1206
 * AI/ML 使用声明：未经授权，不得用于模型训练、数据集构建或商业化自动代码生成。
 */
/**********************************************************
 * OLED.c
 *
 * 软件 I2C SSD1306 128x64 OLED 驱动
 *
 * 主要优化：
 * - page-major framebuffer
 * - 脏页/脏列刷新
 * - 非阻塞分片刷新
 * - 命令合并 / 数据块发送
 * - 降低软件 I2C 阻塞时间
 **********************************************************/

#include "oled.h"
#include "oledfont.h"
#include <string.h>

/* ============================================================
 * 帧缓冲
 *
 * 旧版：
 *   OLED_GRAM[x][page]
 *
 * 新版：
 *   OLED_FB[page][x]
 *
 * 这样每个 page 的 128 byte 在内存中连续。
 * ============================================================ */

static u8 OLED_FB[OLED_PAGES][OLED_WIDTH];

/* ============================================================
 * 脏区管理
 * ============================================================ */

static u8 g_page_dirty[OLED_PAGES];
static u8 g_page_min[OLED_PAGES];
static u8 g_page_max[OLED_PAGES];

typedef enum
{
    FLUSH_IDLE = 0,
    FLUSH_CMD,
    FLUSH_DATA
} FlushState_t;

typedef struct
{
    FlushState_t state;
    u8 page;
    u8 x0;
    u8 x1;
    u8 x;
} FlushCtx_t;

static FlushCtx_t g_flush;

/* ============================================================
 * 内部辅助函数
 * ============================================================ */

static void Display_ClearDirty(void)
{
    u8 i;

    for (i = 0; i < OLED_PAGES; i++)
    {
        g_page_dirty[i] = 0;
        g_page_min[i] = OLED_WIDTH - 1;
        g_page_max[i] = 0;
    }
}

static void Display_DirtyInit(void)
{
    Display_ClearDirty();
    g_flush.state = FLUSH_IDLE;
    g_flush.page = 0;
    g_flush.x0 = 0;
    g_flush.x1 = 0;
    g_flush.x = 0;
}

static u8 oled_min_u8(u8 a, u8 b)
{
    return (a < b) ? a : b;
}

static u8 oled_max_u8(u8 a, u8 b)
{
    return (a > b) ? a : b;
}

static int16_t oled_abs16(int16_t v)
{
    return (v < 0) ? (-v) : v;
}

/* ============================================================
 * 软件 I2C 延时
 * ============================================================ */

static void OLED_IIC_delay(void)
{
#if (OLED_I2C_DELAY_US > 0)
    delay_us(OLED_I2C_DELAY_US);
#else
    __NOP();
    __NOP();
#endif
}

/* ============================================================
 * 软件 I2C 基本时序
 * ============================================================ */

void OLED_I2C_Start(void)
{
    OLED_SDA_Set();
    OLED_SCL_Set();
    OLED_IIC_delay();

    OLED_SDA_Clr();
    OLED_IIC_delay();

    OLED_SCL_Clr();
    OLED_IIC_delay();
}

void OLED_I2C_Stop(void)
{
    OLED_SDA_Clr();
    OLED_IIC_delay();

    OLED_SCL_Set();
    OLED_IIC_delay();

    OLED_SDA_Set();
    OLED_IIC_delay();
}

/*
 * 原来的 OLED_I2C_WaitAck() 并没有真正读取 SDA，
 * 只是产生几个时钟脉冲，本质上是在浪费时间。
 *
 * 如果 OLED 接线稳定，建议 OLED_SKIP_ACK = 1。
 */
void OLED_I2C_WaitAck(void)
{
    OLED_SDA_Set();
    OLED_IIC_delay();

    OLED_SCL_Set();
    OLED_IIC_delay();

    OLED_SCL_Clr();
    OLED_IIC_delay();
}

void OLED_Send_Byte(u8 dat)
{
    u8 i;

    for (i = 0; i < 8; i++)
    {
        if (dat & 0x80)
        {
            OLED_SDA_Set();
        }
        else
        {
            OLED_SDA_Clr();
        }

        OLED_IIC_delay();

        OLED_SCL_Set();
        OLED_IIC_delay();

        OLED_SCL_Clr();
        OLED_IIC_delay();

        dat <<= 1;
    }
}

/* ============================================================
 * I2C 写命令 / 写数据
 * ============================================================ */

void OLED_WR_Byte(u8 dat, u8 mode)
{
    OLED_I2C_Start();

    OLED_Send_Byte(0x78);
    OLED_I2C_WaitAck();

    if (mode)
    {
        OLED_Send_Byte(0x40);
    }
    else
    {
        OLED_Send_Byte(0x00);
    }
    OLED_I2C_WaitAck();

    OLED_Send_Byte(dat);
    OLED_I2C_WaitAck();

    OLED_I2C_Stop();
}

/*
 * 一次发送 3 个命令。
 * 用于设置 page / column 地址，减少 start/stop 次数。
 */
static void OLED_WriteCmd3(u8 cmd0, u8 cmd1, u8 cmd2)
{
    OLED_I2C_Start();

    OLED_Send_Byte(0x78);
    OLED_I2C_WaitAck();

    OLED_Send_Byte(0x00);
    OLED_I2C_WaitAck();

    OLED_Send_Byte(cmd0);
    OLED_I2C_WaitAck();

    OLED_Send_Byte(cmd1);
    OLED_I2C_WaitAck();

    OLED_Send_Byte(cmd2);
    OLED_I2C_WaitAck();

    OLED_I2C_Stop();
}

/*
 * 一次发送一段显示数据。
 */
static void OLED_WriteDataBlock(const u8 *data, u8 len)
{
    u8 i;

    if (len == 0)
    {
        return;
    }

    OLED_I2C_Start();

    OLED_Send_Byte(0x78);
    OLED_I2C_WaitAck();

    OLED_Send_Byte(0x40);
    OLED_I2C_WaitAck();

    for (i = 0; i < len; i++)
    {
        OLED_Send_Byte(data[i]);
        OLED_I2C_WaitAck();
    }

    OLED_I2C_Stop();
}

/* ============================================================
 * 脏区标记
 * ============================================================ */

void Display_InvalidateRect(u8 x0, u8 y0, u8 x1, u8 y1)
{
    u8 page;
    u8 page_start;
    u8 page_end;

    if (x0 > x1)
    {
        u8 t = x0;
        x0 = x1;
        x1 = t;
    }

    if (y0 > y1)
    {
        u8 t = y0;
        y0 = y1;
        y1 = t;
    }

    if (x1 >= OLED_WIDTH)
    {
        x1 = OLED_WIDTH - 1;
    }

    if (y1 >= OLED_HEIGHT)
    {
        y1 = OLED_HEIGHT - 1;
    }

    if (x0 > x1 || y0 > y1)
    {
        return;
    }

    page_start = y0 >> 3;
    page_end   = y1 >> 3;

    for (page = page_start; page <= page_end; page++)
    {
        g_page_dirty[page] = 1;

        if (x0 < g_page_min[page])
        {
            g_page_min[page] = x0;
        }

        if (x1 > g_page_max[page])
        {
            g_page_max[page] = x1;
        }
    }

    /*
     * 自动启动异步刷新。
     * 如果当前已经在刷新，则不打断。
     */
    if (g_flush.state == FLUSH_IDLE)
    {
        g_flush.state = FLUSH_CMD;
        g_flush.page = 0;
    }
}

void Display_InvalidateAll(void)
{
    u8 i;

    for (i = 0; i < OLED_PAGES; i++)
    {
        g_page_dirty[i] = 1;
        g_page_min[i] = 0;
        g_page_max[i] = OLED_WIDTH - 1;
    }

    if (g_flush.state == FLUSH_IDLE)
    {
        g_flush.state = FLUSH_CMD;
        g_flush.page = 0;
    }
}

/* ============================================================
 * 非阻塞刷新状态机
 * ============================================================ */

u8 Display_IsBusy(void)
{
    return (g_flush.state != FLUSH_IDLE) ? 1 : 0;
}

void OLED_StartFlush(void)
{
    if (g_flush.state == FLUSH_IDLE)
    {
        g_flush.state = FLUSH_CMD;
        g_flush.page = 0;
    }
}

void Display_Poll(void)
{
    u8 budget = OLED_FLUSH_CHUNK_BYTES;

    if (g_flush.state == FLUSH_IDLE)
    {
        return;
    }

    while (budget)
    {
        if (g_flush.state == FLUSH_CMD)
        {
            u8 page = g_flush.page;

            for (;;)
            {
                while (page < OLED_PAGES)
                {
                    if (g_page_dirty[page] &&
                        g_page_min[page] <= g_page_max[page])
                    {
                        break;
                    }
                    page++;
                }

                if (page >= OLED_PAGES)
                {
                    /*
                     * 如果本轮已经从中间扫到末尾，
                     * 但刷新期间又有低 page 被标记脏区，
                     * 则回到 0 再扫一次。
                     */
                    if (g_flush.page != 0)
                    {
                        page = 0;
                        g_flush.page = 0;
                        continue;
                    }

                    g_flush.state = FLUSH_IDLE;
                    return;
                }

                break;
            }

            g_flush.page = page;
            g_flush.x0 = g_page_min[page];
            g_flush.x1 = g_page_max[page];
            g_flush.x  = g_flush.x0;

            /*
             * 在发送当前 page 前清除该 page 脏标志。
             *
             * 这样如果在发送过程中又有新的绘图修改了这个 page，
             * 它会重新变脏，并在后续再刷一次。
             */
            g_page_dirty[page] = 0;
            g_page_min[page] = OLED_WIDTH - 1;
            g_page_max[page] = 0;

            /*
             * Page Addressing Mode:
             *   0xB0 | page
             *   0x00 | low column
             *   0x10 | high column
             */
            OLED_WriteCmd3((u8)(0xB0 | page),
                           (u8)(0x00 | (g_flush.x0 & 0x0F)),
                           (u8)(0x10 | (g_flush.x0 >> 4)));

            g_flush.state = FLUSH_DATA;

            /*
             * 命令也占用一定时间，
             * 这里简单扣掉 1 byte 预算。
             */
            if (budget)
            {
                budget--;
            }
        }
        else if (g_flush.state == FLUSH_DATA)
        {
            u8 remaining;
            u8 len;

            remaining = (u8)(g_flush.x1 - g_flush.x + 1);
            len = (remaining < budget) ? remaining : budget;

            OLED_WriteDataBlock(&OLED_FB[g_flush.page][g_flush.x], len);

            g_flush.x += len;
            budget -= len;

            if (g_flush.x > g_flush.x1)
            {
                g_flush.page++;
                g_flush.state = FLUSH_CMD;
            }
        }
        else
        {
            g_flush.state = FLUSH_IDLE;
            return;
        }
    }
}

/* ============================================================
 * 阻塞全屏刷新
 *
 * 仅建议初始化、切页、开机画面使用。
 * 正常运行不建议使用。
 * ============================================================ */

void OLED_RefreshBlocking(void)
{
    u8 page;

    g_flush.state = FLUSH_IDLE;

    for (page = 0; page < OLED_PAGES; page++)
    {
        OLED_WriteCmd3((u8)(0xB0 | page), 0x00, 0x10);
        OLED_WriteDataBlock(&OLED_FB[page][0], OLED_WIDTH);
    }

    Display_ClearDirty();
}

/*
 * 兼容旧接口。
 * 现在 OLED_Refresh() 是异步全屏刷新请求。
 */
void OLED_Refresh(void)
{
    Display_InvalidateAll();
}

/* ============================================================
 * 内部像素操作
 * ============================================================ */

static void OLED_PutPixel(u8 x, u8 y, u8 color)
{
    u8 page;
    u8 mask;

    if (x >= OLED_WIDTH || y >= OLED_HEIGHT)
    {
        return;
    }

    page = y >> 3;
    mask = (u8)(1 << (y & 0x07));

    if (color)
    {
        OLED_FB[page][x] |= mask;
    }
    else
    {
        OLED_FB[page][x] &= (u8)(~mask);
    }
}

static void OLED_PutPixelInt(int16_t x, int16_t y, u8 color)
{
    if (x < 0 || x >= OLED_WIDTH || y < 0 || y >= OLED_HEIGHT)
    {
        return;
    }

    OLED_PutPixel((u8)x, (u8)y, color);
}

/* ============================================================
 * 基础绘图 API
 * ============================================================ */

void OLED_DrawPoint(u8 x, u8 y, u8 t)
{
    OLED_PutPixel(x, y, t);
    Display_InvalidateRect(x, y, x, y);
}

void OLED_ClearPoint(u8 x, u8 y)
{
    OLED_DrawPoint(x, y, 0);
}

void OLED_DrawHLine(u8 x0, u8 x1, u8 y, u8 color)
{
    u8 page;
    u8 mask;
    u8 x;

    if (y >= OLED_HEIGHT)
    {
        return;
    }

    if (x0 > x1)
    {
        u8 t = x0;
        x0 = x1;
        x1 = t;
    }

    if (x0 >= OLED_WIDTH)
    {
        return;
    }

    if (x1 >= OLED_WIDTH)
    {
        x1 = OLED_WIDTH - 1;
    }

    if (x0 > x1)
    {
        return;
    }

    page = y >> 3;
    mask = (u8)(1 << (y & 0x07));

    for (x = x0; x <= x1; x++)
    {
        if (color)
        {
            OLED_FB[page][x] |= mask;
        }
        else
        {
            OLED_FB[page][x] &= (u8)(~mask);
        }
    }

    Display_InvalidateRect(x0, y, x1, y);
}

void OLED_DrawVLine(u8 x, u8 y0, u8 y1, u8 color)
{
    u8 page_start;
    u8 page_end;
    u8 page;

    if (x >= OLED_HEIGHT)
    {
        /* 这里应该是 x >= OLED_WIDTH，下面已修正 */
    }

    if (x >= OLED_WIDTH)
    {
        return;
    }

    if (y0 > y1)
    {
        u8 t = y0;
        y0 = y1;
        y1 = t;
    }

    if (y0 >= OLED_HEIGHT)
    {
        return;
    }

    if (y1 >= OLED_HEIGHT)
    {
        y1 = OLED_HEIGHT - 1;
    }

    page_start = y0 >> 3;
    page_end   = y1 >> 3;

    for (page = page_start; page <= page_end; page++)
    {
        u8 start_bit = 0;
        u8 end_bit = 7;
        u8 mask = 0;
        u8 b;

        if (page == page_start)
        {
            start_bit = y0 & 0x07;
        }

        if (page == page_end)
        {
            end_bit = y1 & 0x07;
        }

        for (b = start_bit; b <= end_bit; b++)
        {
            mask |= (u8)(1 << b);
        }

        if (color)
        {
            OLED_FB[page][x] |= mask;
        }
        else
        {
            OLED_FB[page][x] &= (u8)(~mask);
        }
    }

    Display_InvalidateRect(x, y0, x, y1);
}

/*
 * 内部整数坐标画线。
 * 支持负坐标，内部会裁剪。
 */
static void OLED_DrawLineInt(int16_t x0, int16_t y0,
                             int16_t x1, int16_t y1,
                             u8 color)
{
    int16_t dx;
    int16_t dy;
    int16_t sx;
    int16_t sy;
    int16_t err;
    int16_t e2;

    int16_t minx;
    int16_t maxx;
    int16_t miny;
    int16_t maxy;

    dx = oled_abs16((int16_t)(x1 - x0));
    dy = oled_abs16((int16_t)(y1 - y0));

    sx = (x0 < x1) ? 1 : -1;
    sy = (y0 < y1) ? 1 : -1;

    err = (dx > dy ? dx : -dy) / 2;

    minx = (x0 < x1) ? x0 : x1;
    maxx = (x0 > x1) ? x0 : x1;
    miny = (y0 < y1) ? y0 : y1;
    maxy = (y0 > y1) ? y0 : y1;

    for (;;)
    {
        OLED_PutPixelInt(x0, y0, color);

        if (x0 == x1 && y0 == y1)
        {
            break;
        }

        e2 = err;

        if (e2 > -dx)
        {
            err -= dy;
            x0 += sx;
        }

        if (e2 < dx)
        {
            err += dy;
            y0 += sy;
        }
    }

    if (minx < 0) minx = 0;
    if (miny < 0) miny = 0;
    if (maxx >= OLED_WIDTH) maxx = OLED_WIDTH - 1;
    if (maxy >= OLED_HEIGHT) maxy = OLED_HEIGHT - 1;

    if (minx <= maxx && miny <= maxy)
    {
        Display_InvalidateRect((u8)minx,
                               (u8)miny,
                               (u8)maxx,
                               (u8)maxy);
    }
}

void OLED_DrawLine(u8 x1, u8 y1, u8 x2, u8 y2, u8 mode)
{
    OLED_DrawLineInt((int16_t)x1, (int16_t)y1,
                     (int16_t)x2, (int16_t)y2,
                     mode);
}

void OLED_DrawRect(u8 x0, u8 y0, u8 x1, u8 y1, u8 color)
{
    if (x0 > x1 || y0 > y1)
    {
        return;
    }

    OLED_DrawHLine(x0, x1, y0, color);
    OLED_DrawHLine(x0, x1, y1, color);

    OLED_DrawVLine(x0, y0, y1, color);
    OLED_DrawVLine(x1, y0, y1, color);

    Display_InvalidateRect(x0, y0, x1, y1);
}

void OLED_FillRect(u8 x0, u8 y0, u8 x1, u8 y1, u8 color)
{
    u8 page_start;
    u8 page_end;
    u8 page;
    u8 x;

    if (x0 > x1)
    {
        u8 t = x0;
        x0 = x1;
        x1 = t;
    }

    if (y0 > y1)
    {
        u8 t = y0;
        y0 = y1;
        y1 = t;
    }

    if (x1 >= OLED_WIDTH)
    {
        x1 = OLED_WIDTH - 1;
    }

    if (y1 >= OLED_HEIGHT)
    {
        y1 = OLED_HEIGHT - 1;
    }

    if (x0 > x1 || y0 > y1)
    {
        return;
    }

    page_start = y0 >> 3;
    page_end   = y1 >> 3;

    for (page = page_start; page <= page_end; page++)
    {
        u8 start_bit = 0;
        u8 end_bit = 7;
        u8 mask = 0;
        u8 b;

        if (page == page_start)
        {
            start_bit = y0 & 0x07;
        }

        if (page == page_end)
        {
            end_bit = y1 & 0x07;
        }

        for (b = start_bit; b <= end_bit; b++)
        {
            mask |= (u8)(1 << b);
        }

        for (x = x0; x <= x1; x++)
        {
            if (color)
            {
                OLED_FB[page][x] |= mask;
            }
            else
            {
                OLED_FB[page][x] &= (u8)(~mask);
            }
        }
    }

    Display_InvalidateRect(x0, y0, x1, y1);
}

void OLED_DrawCircle(u8 x, u8 y, u8 r)
{
    int16_t cx = (int16_t)x;
    int16_t cy = (int16_t)y;
    int16_t a = 0;
    int16_t b = (int16_t)r;
    int16_t d = (int16_t)(1 - r);

    int16_t bx0 = (int16_t)x - (int16_t)r;
    int16_t by0 = (int16_t)y - (int16_t)r;
    int16_t bx1 = (int16_t)x + (int16_t)r;
    int16_t by1 = (int16_t)y + (int16_t)r;

    while (a <= b)
    {
        OLED_PutPixelInt(cx + a, cy - b, 1);
        OLED_PutPixelInt(cx - a, cy - b, 1);
        OLED_PutPixelInt(cx - a, cy + b, 1);
        OLED_PutPixelInt(cx + a, cy + b, 1);

        OLED_PutPixelInt(cx + b, cy + a, 1);
        OLED_PutPixelInt(cx + b, cy - a, 1);
        OLED_PutPixelInt(cx - b, cy - a, 1);
        OLED_PutPixelInt(cx - b, cy + a, 1);

        a++;

        if (d < 0)
        {
            d += (int16_t)(2 * a + 1);
        }
        else
        {
            b--;
            d += (int16_t)(2 * (a - b) + 1);
        }
    }

    if (bx0 < 0) bx0 = 0;
    if (by0 < 0) by0 = 0;
    if (bx1 >= OLED_WIDTH) bx1 = OLED_WIDTH - 1;
    if (by1 >= OLED_HEIGHT) by1 = OLED_HEIGHT - 1;

    if (bx0 <= bx1 && by0 <= by1)
    {
        Display_InvalidateRect((u8)bx0,
                               (u8)by0,
                               (u8)bx1,
                               (u8)by1);
    }
}

void OLED_DrawTriangle(u8 x0, u8 y0,
                       u8 x1, u8 y1,
                       u8 x2, u8 y2,
                       u8 color)
{
    OLED_DrawLineInt((int16_t)x0, (int16_t)y0,
                     (int16_t)x1, (int16_t)y1,
                     color);

    OLED_DrawLineInt((int16_t)x1, (int16_t)y1,
                     (int16_t)x2, (int16_t)y2,
                     color);

    OLED_DrawLineInt((int16_t)x2, (int16_t)y2,
                     (int16_t)x0, (int16_t)y0,
                     color);
}

void OLED_DrawSmallTriangle(u8 cx, u8 cy, u8 size, u8 dir, u8 color)
{
    int16_t x = (int16_t)cx;
    int16_t y = (int16_t)cy;
    int16_t s = (int16_t)size;

    switch (dir)
    {
        case OLED_TRI_UP:
            OLED_DrawLineInt(x,     y - s, x - s, y + s, color);
            OLED_DrawLineInt(x - s, y + s, x + s, y + s, color);
            OLED_DrawLineInt(x + s, y + s, x,     y - s, color);
            break;

        case OLED_TRI_DOWN:
            OLED_DrawLineInt(x,     y + s, x - s, y - s, color);
            OLED_DrawLineInt(x - s, y - s, x + s, y - s, color);
            OLED_DrawLineInt(x + s, y - s, x,     y + s, color);
            break;

        case OLED_TRI_LEFT:
            OLED_DrawLineInt(x - s, y,     x + s, y - s, color);
            OLED_DrawLineInt(x + s, y - s, x + s, y + s, color);
            OLED_DrawLineInt(x + s, y + s, x - s, y,     color);
            break;

        case OLED_TRI_RIGHT:
        default:
            OLED_DrawLineInt(x + s, y,     x - s, y - s, color);
            OLED_DrawLineInt(x - s, y - s, x - s, y + s, color);
            OLED_DrawLineInt(x - s, y + s, x + s, y,     color);
            break;
    }
}

void OLED_FillSmallTriangle(u8 cx, u8 cy, u8 size, u8 dir, u8 color)
{
    int16_t x = (int16_t)cx;
    int16_t y = (int16_t)cy;
    int16_t s = (int16_t)size;
    int16_t i;
    int16_t total = 2 * s;

    switch (dir)
    {
        case OLED_TRI_UP:
            for (i = 0; i <= total; i++)
            {
                int16_t yy = y - s + i;
                int16_t half = i / 2;
                OLED_DrawLineInt(x - half, yy, x + half, yy, color);
            }
            break;

        case OLED_TRI_DOWN:
            for (i = 0; i <= total; i++)
            {
                int16_t yy = y - s + i;
                int16_t half = (total - i) / 2;
                OLED_DrawLineInt(x - half, yy, x + half, yy, color);
            }
            break;

        case OLED_TRI_LEFT:
            for (i = 0; i <= total; i++)
            {
                int16_t xx = x - s + i;
                int16_t half = i / 2;
                OLED_DrawLineInt(xx, y - half, xx, y + half, color);
            }
            break;

        case OLED_TRI_RIGHT:
        default:
            for (i = 0; i <= total; i++)
            {
                int16_t xx = x - s + i;
                int16_t half = (total - i) / 2;
                OLED_DrawLineInt(xx, y - half, xx, y + half, color);
            }
            break;
    }
}

/* ============================================================
 * 字体尺寸
 * ============================================================ */

u8 OLED_GetFontWidth(u8 size1)
{
    if (size1 == 8)
    {
        return 6;
    }

    return (u8)(size1 / 2);
}

/*
 * 注意：
 * size1 = 12 时，虽然名字叫 12，
 * 但现有字体绘制方式会占用 2 个 page，即 16 pixel 高。
 *
 * 所以菜单框选时，size12 推荐行高 16。
 */
u8 OLED_GetFontCellHeight(u8 size1)
{
    u8 pages = (u8)((size1 + 7) / 8);
    return (u8)(pages * 8);
}

/* ============================================================
 * 字符显示
 * ============================================================ */

void OLED_ShowChar(u8 x, u8 y, u8 chr, u8 size1, u8 mode)
{
    u8 width;
    u8 pages;
    u8 height;
    u8 chr_index;
    const u8 *font = 0;

    u16 i = 0;
    u8 p;
    u8 col;
    u8 bit;

    if (chr < ' ' || chr > '~')
    {
        return;
    }

    if (x >= OLED_WIDTH || y >= OLED_HEIGHT)
    {
        return;
    }

    width = OLED_GetFontWidth(size1);
    pages = (u8)((size1 + 7) / 8);
    height = (u8)(pages * 8);

    chr_index = (u8)(chr - ' ');

    switch (size1)
    {
        case 8:
            font = asc2_0806[chr_index];
            break;

        case 12:
            font = asc2_1206[chr_index];
            break;

        case 16:
            font = asc2_1608[chr_index];
            break;

        case 24:
            font = asc2_2412[chr_index];
            break;

        default:
            return;
    }

    for (p = 0; p < pages; p++)
    {
        for (col = 0; col < width; col++)
        {
            u8 temp = font[i++];

            for (bit = 0; bit < 8; bit++)
            {
                u8 yy = (u8)(y + p * 8 + bit);

                if (yy >= OLED_HEIGHT)
                {
                    continue;
                }

                if (temp & 0x01)
                {
                    OLED_PutPixel((u8)(x + col), yy, mode);
                }
                else
                {
                    OLED_PutPixel((u8)(x + col), yy, (u8)(!mode));
                }

                temp >>= 1;
            }
        }
    }

    Display_InvalidateRect(x,
                           y,
                           (u8)(x + width - 1),
                           (u8)(y + height - 1));
}

void OLED_ShowChar6x8(u8 x, u8 y, u8 chr, u8 mode)
{
    OLED_ShowChar(x, y, chr, 8, mode);
}

void OLED_ShowString(u8 x, u8 y, const char *chr, u8 size1, u8 mode)
{
    while ((*chr >= ' ') && (*chr <= '~'))
    {
        OLED_ShowChar(x, y, (u8)(*chr), size1, mode);

        if (size1 == 8)
        {
            x += 6;
        }
        else
        {
            x += size1 / 2;
        }

        chr++;
    }
}

/* ============================================================
 * 数字显示
 * ============================================================ */

u32 OLED_Pow(u8 m, u8 n)
{
    u32 result = 1;

    while (n--)
    {
        result *= m;
    }

    return result;
}

void OLED_ShowNum(u8 x, u8 y, u32 num, u8 len, u8 size1, u8 mode)
{
    u8 t;
    u8 temp;
    u8 m = 0;
    u8 width;
    u8 cell;

    if (size1 == 8)
    {
        m = 2;
    }

    width = OLED_GetFontWidth(size1);
    cell = (u8)(width + m);

    for (t = 0; t < len; t++)
    {
        temp = (u8)((num / OLED_Pow(10, (u8)(len - t - 1))) % 10);

        OLED_ShowChar((u8)(x + cell * t),
                      y,
                      (u8)(temp + '0'),
                      size1,
                      mode);
    }
}

void OLED_ShowFloatNum(uint8_t x,
                       uint8_t y,
                       float num,
                       uint8_t intlen,
                       uint8_t fralen,
                       uint8_t size1,
                       uint8_t mode)
{
    u32 PowNum;
    u32 IntNum;
    u32 FraNum;

    u8 m = 0;
    u8 width;
    u8 cell;

    float frac;

    if (size1 == 8)
    {
        m = 2;
    }

    width = OLED_GetFontWidth(size1);
    cell = (u8)(width + m);

    if (num > 0.0f)
    {
        OLED_ShowChar(x, y, '+', size1, mode);
    }
    else if (num < 0.0f)
    {
        OLED_ShowChar(x, y, '-', size1, mode);
        num = -num;
    }
    else
    {
        OLED_ShowChar(x, y, ' ', size1, mode);
    }

    IntNum = (u32)num;
    frac = num - (float)IntNum;

    PowNum = OLED_Pow(10, fralen);
    FraNum = (u32)(frac * (float)PowNum + 0.5f);

    if (FraNum >= PowNum)
    {
        FraNum -= PowNum;
        IntNum++;
    }

    OLED_ShowNum((u8)(x + cell), y, IntNum, intlen, size1, mode);

    OLED_ShowChar((u8)(x + cell * (intlen + 1)),
                  y,
                  '.',
                  size1,
                  mode);

    OLED_ShowNum((u8)(x + cell * (intlen + 2)),
                 y,
                 FraNum,
                 fralen,
                 size1,
                 mode);
}

void OLED_ShowBin8(u8 x, u8 y, u8 num, u8 size1, u8 mode)
{
    u8 i;
    u8 m = (size1 == 8) ? 2 : 0;
    u8 width = OLED_GetFontWidth(size1);
    u8 cell = (u8)(width + m);

    char bin_str[9] = {0};

    for (i = 0; i < 8; i++)
    {
        bin_str[i] = (num & (0x80 >> i)) ? '1' : '0';
    }

    for (i = 0; i < 8; i++)
    {
        OLED_ShowChar((u8)(x + cell * i),
                      y,
                      (u8)bin_str[i],
                      size1,
                      mode);
    }
}

void OLED_ShowSignedNum(uint8_t x,
                        uint8_t y,
                        int32_t num,
                        uint8_t len,
                        uint8_t size1,
                        uint8_t mode)
{
    u8 m = 0;
    u8 width;
    u8 cell;

    if (size1 == 8)
    {
        m = 2;
    }

    width = OLED_GetFontWidth(size1);
    cell = (u8)(width + m);

    if (num > 0)
    {
        OLED_ShowChar(x, y, '+', size1, mode);
    }
    else if (num < 0)
    {
        OLED_ShowChar(x, y, '-', size1, mode);
        num = -num;
    }
    else
    {
        OLED_ShowChar(x, y, ' ', size1, mode);
    }

    OLED_ShowNum((u8)(x + cell), y, (u32)num, len, size1, mode);
}

void OLED_ShowTargetValueEx(u8 x,
                            u8 y,
                            const char *label,
                            int value,
                            u8 digits,
                            u8 size,
                            u8 mode)
{
    u8 width = OLED_GetFontWidth(size);
    u8 m = (size == 8) ? 2 : 0;
    u8 cell = (u8)(width + m);

    u8 label_len = (u8)strlen(label);
    u8 num_x = (u8)(x + label_len * cell);

    OLED_ShowString(x, y, label, size, mode);

    if (value < 0)
    {
        OLED_ShowChar(num_x, y, '-', size, mode);
        num_x += cell;
        value = -value;
    }

    OLED_ShowNum(num_x, y, (u32)value, digits, size, mode);
}

/* ============================================================
 * 中文 / 图片
 * ============================================================ */

void OLED_ShowChinese(u8 x, u8 y, u8 num, u8 size1, u8 mode)
{
    u8 width;
    u8 pages;
    u8 height;
    u16 bytes;
    const u8 *font = 0;

    u16 i = 0;
    u8 p;
    u8 col;
    u8 bit;

    if (x >= OLED_WIDTH || y >= OLED_HEIGHT)
    {
        return;
    }

    width = size1;
    pages = (u8)((size1 + 7) / 8);
    height = (u8)(pages * 8);
    bytes = (u16)(width * pages);

    switch (size1)
    {
        case 16:
            font = Hzk1[num];
            break;

        case 24:
            font = Hzk2[num];
            break;

        case 32:
            font = Hzk3[num];
            break;

        case 64:
            font = Hzk4[num];
            break;

        default:
            return;
    }

    for (p = 0; p < pages; p++)
    {
        for (col = 0; col < width; col++)
        {
            u8 temp = font[i++];

            for (bit = 0; bit < 8; bit++)
            {
                u8 yy = (u8)(y + p * 8 + bit);

                if (yy >= OLED_HEIGHT)
                {
                    continue;
                }

                if (temp & 0x01)
                {
                    OLED_PutPixel((u8)(x + col), yy, mode);
                }
                else
                {
                    OLED_PutPixel((u8)(x + col), yy, (u8)(!mode));
                }

                temp >>= 1;
            }
        }
    }

    (void)bytes;

    Display_InvalidateRect(x,
                           y,
                           (u8)(x + width - 1),
                           (u8)(y + height - 1));
}

void OLED_ShowPicture(u8 x, u8 y, u8 sizex, u8 sizey, u8 BMP[], u8 mode)
{
    u8 pages;
    u8 height;
    u16 j = 0;

    u8 p;
    u8 col;
    u8 bit;

    if (x >= OLED_WIDTH || y >= OLED_HEIGHT)
    {
        return;
    }

    pages = (u8)((sizey + 7) / 8);
    height = (u8)(pages * 8);

    for (p = 0; p < pages; p++)
    {
        for (col = 0; col < sizex; col++)
        {
            u8 temp = BMP[j++];

            for (bit = 0; bit < 8; bit++)
            {
                u8 yy = (u8)(y + p * 8 + bit);

                if (yy >= OLED_HEIGHT)
                {
                    continue;
                }

                if (temp & 0x01)
                {
                    OLED_PutPixel((u8)(x + col), yy, mode);
                }
                else
                {
                    OLED_PutPixel((u8)(x + col), yy, (u8)(!mode));
                }

                temp >>= 1;
            }
        }
    }

    Display_InvalidateRect(x,
                           y,
                           (u8)(x + sizex - 1),
                           (u8)(y + height - 1));
}

/* ============================================================
 * 清屏
 * ============================================================ */

static void OLED_ClearBufferNoDirty(void)
{
    memset(OLED_FB, 0, sizeof(OLED_FB));
    Display_ClearDirty();
    g_flush.state = FLUSH_IDLE;
}

void OLED_ClearBuffer(void)
{
    OLED_ClearBufferNoDirty();
    Display_InvalidateAll();
}

void OLED_Clear(void)
{
    /*
     * 阻塞清屏。
     * 只建议初始化使用。
     */
    while (Display_IsBusy())
    {
        Display_Poll();
    }

    OLED_ClearBufferNoDirty();
    OLED_Refresh();
}

/* ============================================================
 * 显示方向 / 反色 / 开关
 * ============================================================ */

void OLED_ColorTurn(u8 i)
{
    while (Display_IsBusy())
    {
        Display_Poll();
    }

    if (i == 0)
    {
        OLED_WR_Byte(0xA6, OLED_CMD);
    }
    else
    {
        OLED_WR_Byte(0xA7, OLED_CMD);
    }
}

void OLED_DisplayTurn(u8 i)
{
    while (Display_IsBusy())
    {
        Display_Poll();
    }

    if (i == 0)
    {
        OLED_WR_Byte(0xC8, OLED_CMD);
        OLED_WR_Byte(0xA1, OLED_CMD);
    }
    else
    {
        OLED_WR_Byte(0xC0, OLED_CMD);
        OLED_WR_Byte(0xA0, OLED_CMD);
    }
}

void OLED_DisPlay_On(void)
{
    while (Display_IsBusy())
    {
        Display_Poll();
    }

    OLED_WR_Byte(0x8D, OLED_CMD);
    OLED_WR_Byte(0x14, OLED_CMD);
    OLED_WR_Byte(0xAF, OLED_CMD);
}

void OLED_DisPlay_Off(void)
{
    while (Display_IsBusy())
    {
        Display_Poll();
    }

    OLED_WR_Byte(0x8D, OLED_CMD);
    OLED_WR_Byte(0x10, OLED_CMD);
    OLED_WR_Byte(0xAE, OLED_CMD);
}

/* ============================================================
 * 兼容旧接口：滚动显示
 *
 * 新驱动不建议使用阻塞式全屏滚动。
 * 这里保留空函数，防止旧代码链接失败。
 * ============================================================ */

void OLED_ScrollDisplay(u8 num, u8 space, u8 mode)
{
    (void)num;
    (void)space;
    (void)mode;
}

/* ============================================================
 * 初始化
 * ============================================================ */

void OLED_Init(void)
{
    Display_DirtyInit();
    OLED_ClearBufferNoDirty();

    OLED_WR_Byte(0xAE, OLED_CMD);

    OLED_WR_Byte(0x00, OLED_CMD);
    OLED_WR_Byte(0x10, OLED_CMD);

    OLED_WR_Byte(0x40, OLED_CMD);

    OLED_WR_Byte(0x81, OLED_CMD);
    OLED_WR_Byte(0xCF, OLED_CMD);

    OLED_WR_Byte(0xA1, OLED_CMD);
    OLED_WR_Byte(0xC8, OLED_CMD);

    OLED_WR_Byte(0xA6, OLED_CMD);

    OLED_WR_Byte(0xA8, OLED_CMD);
    OLED_WR_Byte(0x3F, OLED_CMD);

    OLED_WR_Byte(0xD3, OLED_CMD);
    OLED_WR_Byte(0x00, OLED_CMD);

    OLED_WR_Byte(0xD5, OLED_CMD);
    OLED_WR_Byte(0x80, OLED_CMD);

    OLED_WR_Byte(0xD9, OLED_CMD);
    OLED_WR_Byte(0xF1, OLED_CMD);

    OLED_WR_Byte(0xDA, OLED_CMD);
    OLED_WR_Byte(0x12, OLED_CMD);

    OLED_WR_Byte(0xDB, OLED_CMD);
    OLED_WR_Byte(0x40, OLED_CMD);

    OLED_WR_Byte(0x20, OLED_CMD);
    OLED_WR_Byte(0x02, OLED_CMD);

    OLED_WR_Byte(0x8D, OLED_CMD);
    OLED_WR_Byte(0x14, OLED_CMD);

    OLED_WR_Byte(0xA4, OLED_CMD);
    OLED_WR_Byte(0xA6, OLED_CMD);

    /*
     * 先打开显示，再刷新帧缓冲。
     */
    OLED_WR_Byte(0xAF, OLED_CMD);
    OLED_RefreshBlocking();
}
