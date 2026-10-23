/*
 * main.c — Target test loopback cho driver bxCAN trên STM32F103 (GĐ 4 ĐAKS)
 * Code do Thắng viết. Gửi khung Pedal mỗi 100 ms, nhận lại qua loopback, so sánh.
 * Đọc các biến g_* bằng Live Expressions trong CubeIDE.
 */
#include "stm32f1xx.h"
#include "bxcan.h"
#include "clock_f103.h"

/* ===== Đồng hồ mili-giây ===== */
volatile uint32_t g_tick_ms = 0;

void SysTick_Handler(void)
{
    g_tick_ms++;                       /* ngắt mỗi 1 ms */
}

/* ===== Bộ đếm, đọc bằng debugger ===== */
volatile uint8_t  g_clk_err     = 0;   /* kết quả clock_init(): 0 OK, 1 HSE, 2 PLL, 3 chuyển SW */
volatile uint8_t  g_init_err    = 0;
volatile uint32_t g_tx_ok       = 0;
volatile uint32_t g_tx_busy     = 0;
volatile uint32_t g_rx_ok       = 0;
volatile uint32_t g_rx_mismatch = 0;
volatile uint32_t g_rx_timeout  = 0;

/* Chờ 1 khung trong tối đa timeout_ms.
   Trả về 0 nếu nhận được, 1 nếu quá giờ. */
uint8_t can_receive_timeout(uint16_t *id, uint8_t *data, uint8_t *dlc, uint32_t timeout_ms)
{
    uint32_t t_start = g_tick_ms;

    while (can_receive(id, data, dlc) != 0)     /* lặp khi CHƯA nhận được khung */
    {
        if ((g_tick_ms - t_start) >= timeout_ms)
        {
            return 1;                           /* quá giờ */
        }
    }
    return 0;
}

/* So 2 khung. Trả về 1 nếu giống hệt, 0 nếu khác. */
uint8_t frame_equal(uint16_t id_a, const uint8_t *a, uint8_t dlc_a,
                    uint16_t id_b, const uint8_t *b, uint8_t dlc_b)
{
    uint8_t i;

    if (id_a != id_b || dlc_a != dlc_b) { return 0; }   /* khác ID hoặc khác DLC */

    for (i = 0; i < dlc_a; i++)                          /* chỉ so số byte theo DLC */
    {
        if (a[i] != b[i]) { return 0; }
    }
    return 1;
}

int main(void)
{
    const uint16_t tx_id  = 0x080;     /* khung Pedal */
    const uint8_t  tx_dlc = 8;
    uint8_t  tx[8] = {0x8C, 0x01, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t  rx[8];
    uint16_t rx_id;
    uint8_t  rx_dlc;
    uint32_t t_last;

    /* 72 MHz từ HSE. Lỗi thì vẫn chạy tiếp ở HSI 8 MHz, không dừng, để còn đọc bộ đếm */
    g_clk_err = clock_init();

    SystemCoreClockUpdate();                   /* SystemCoreClock = tần số CPU (Hz) */
    SysTick_Config(SystemCoreClock / 1000U);   /* chia 1000 = số nhịp trong 1 ms → ngắt mỗi 1 ms */

    g_init_err = Can_Init(0x40050008U);   /* BRP=8, TS1=5, TS2=0, SJW=0, LBKM=1 (APB1 = 36 MHz) */
    if (g_init_err != 0)
    {
        while (1) { }                          /* dừng lại để đọc mã lỗi */
    }

    t_last = g_tick_ms;
    while (1)
    {
        /* 1. Chờ đủ 100 ms kể từ lần gửi trước (lịch cố định, không trôi) */
        while ((g_tick_ms - t_last) < 100U) { }
        t_last += 100U;

        /* 2. Đổi dữ liệu để phát hiện đọc lại khung cũ */
        tx[3]++;

        /* 3. Gửi */
        if (can_send(tx_id, tx, tx_dlc) != 0)
        {
            g_tx_busy++;
            continue;
        }
        g_tx_ok++;

        /* 4. Chờ nhận lại, tối đa 10 ms */
        if (can_receive_timeout(&rx_id, rx, &rx_dlc, 10U) != 0)
        {
            g_rx_timeout++;
            continue;
        }

        /* 5. So sánh */
        if (frame_equal(tx_id, tx, tx_dlc, rx_id, rx, rx_dlc))
        {
            g_rx_ok++;
        }
        else
        {
            g_rx_mismatch++;
        }
    }
}
