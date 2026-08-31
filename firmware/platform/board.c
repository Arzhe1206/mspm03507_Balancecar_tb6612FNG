/*
 * MSPM0G3507 Balance Car
 * 作者：数显纯牛奶，开源工程未经授权禁止商业用途，需要实物见店铺主页：https://m.tb.cn/h.8nzqEuN?tk=kJqnT4uWzfY
 * 官方仓库：https://github.com/Arzhe1206/mspm03507_Balancecar_tb6612FNG
 * 许可：LicenseRef-BalanceCar-NonCommercial
 * 项目标识：MSPM0-BC-ARZHE1206
 * AI/ML 使用声明：未经授权，不得用于模型训练、数据集构建或商业化自动代码生成。
 */
#include "board.h"
#include <stdio.h>

#define UART0_RX_BUFFER_SIZE 256U
#define UART0_RX_BUFFER_MASK (UART0_RX_BUFFER_SIZE - 1U)

static volatile uint8_t uart0_rx_buffer[UART0_RX_BUFFER_SIZE];
static volatile uint16_t uart0_rx_head;
static volatile uint16_t uart0_rx_tail;
static volatile uint32_t uart0_rx_overflows;

void board_init(void)
{
    SYSCFG_DL_init();

    NVIC_ClearPendingIRQ(UART_0_INST_INT_IRQN);
    NVIC_SetPriority(UART_0_INST_INT_IRQN, 2U);
    NVIC_EnableIRQ(UART_0_INST_INT_IRQN);
}

void delay_us(unsigned long us)
{
    while (us-- != 0U) {
        delay_cycles(CPUCLK_FREQ / 1000000U);
    }
}

void delay_ms(unsigned long ms)
{
    while (ms-- != 0U) {
        delay_us(1000U);
    }
}

void delay_1us(unsigned long us) { delay_us(us); }
void delay_1ms(unsigned long ms) { delay_ms(ms); }

void uart0_send_char(char ch)
{
    while (DL_UART_isBusy(UART_0_INST)) {
    }
    DL_UART_Main_transmitData(UART_0_INST, (uint8_t) ch);
}

void uart0_send_string(const char *str)
{
    if (str == NULL) {
        return;
    }
    while (*str != '\0') {
        uart0_send_char(*str++);
    }
}

bool uart0_read_byte(uint8_t *byte)
{
    uint16_t tail;

    if (byte == NULL) {
        return false;
    }

    tail = uart0_rx_tail;
    if (tail == uart0_rx_head) {
        return false;
    }

    *byte = uart0_rx_buffer[tail];
    uart0_rx_tail = (uint16_t) ((tail + 1U) & UART0_RX_BUFFER_MASK);
    return true;
}

uint32_t uart0_rx_overflow_count(void)
{
    return uart0_rx_overflows;
}

#if !defined(__MICROLIB)
#if (__ARMCLIB_VERSION <= 6000000)
struct __FILE { int handle; };
#endif
FILE __stdout;
void _sys_exit(int x) { (void) x; }
#endif

int fputc(int ch, FILE *stream)
{
    (void) stream;
    uart0_send_char((char) ch);
    return ch;
}

void UART_0_INST_IRQHandler(void)
{
    (void) DL_UART_getPendingInterrupt(UART_0_INST);

    while (!DL_UART_Main_isRXFIFOEmpty(UART_0_INST)) {
        uint8_t byte = DL_UART_Main_receiveData(UART_0_INST);
        uint16_t head = uart0_rx_head;
        uint16_t next = (uint16_t) ((head + 1U) & UART0_RX_BUFFER_MASK);

        if (next == uart0_rx_tail) {
            uart0_rx_overflows++;
        } else {
            uart0_rx_buffer[head] = byte;
            uart0_rx_head = next;
        }
    }
}
