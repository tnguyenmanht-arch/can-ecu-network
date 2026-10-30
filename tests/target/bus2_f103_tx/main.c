/*
 * main.c — Test bus thật 2 board (Mốc 1 bước 2), phía GỬI: STM32F103 (ECU2 tạm thời).
 * Khung từ tests/target/bxcan_loopback (code của Thắng), bỏ phần nhận/so sánh.
 * Mỗi 100 ms gửi khung 0x080, 8 byte, byte 0 = bộ đếm 0–255 quay vòng, các byte khác 0.
 * Chế độ thường (LBKM = 0): khung chỉ gửi xong khi có node khác ACK (ở đây là F407).
 * Đọc các biến g_* qua ST-Link HOTPLUG (xem README.md).
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
volatile uint8_t  g_clk_err  = 0;   /* kết quả clock_init(): 0 OK, 1 HSE, 2 PLL, 3 chuyển SW */
volatile uint8_t  g_init_err = 0;   /* kết quả Can_Init: 0 OK, 1 kẹt vào init, 2 kẹt thoát init */
volatile uint32_t g_tx_req   = 0;   /* số khung đã nạp vào mailbox 0 */
volatile uint32_t g_tx_busy  = 0;   /* số lần mailbox 0 còn bận (khung trước chưa gửi xong) */
volatile uint32_t g_tx_done  = 0;   /* số khung TSR báo TXOK0 = đã có node ACK */
volatile uint32_t g_esr      = 0;   /* bản sao CAN1->ESR, cập nhật mỗi 100 ms */

/* Khung trước đã xong (RQCP0 = 1) thì đếm nếu TXOK0 = 1, rồi xoá cờ.
 * RQCP0 là bit rc_w1: ghi 1 vào RQCP0 xoá luôn TXOK0, ALST0, TERR0 → dùng "=" không dùng "|=". */
static void check_tx_done(void)
{
    uint32_t tsr = CAN1->TSR;

    if ((tsr & CAN_TSR_RQCP0) != 0U)
    {
        if ((tsr & CAN_TSR_TXOK0) != 0U)
        {
            g_tx_done++;
        }
        CAN1->TSR = CAN_TSR_RQCP0;
    }
}

int main(void)
{
    const uint16_t tx_id  = 0x080;     /* khung Pedal */
    const uint8_t  tx_dlc = 8;
    uint8_t  tx[8] = {0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U};
    uint8_t  seq = 0;                  /* bộ đếm byte 0, uint8_t tự quay vòng 255 → 0 */
    uint32_t t_last;

    /* 72 MHz từ HSE. Lỗi thì vẫn chạy tiếp ở HSI 8 MHz, không dừng, để còn đọc bộ đếm */
    g_clk_err = clock_init();

    SystemCoreClockUpdate();                   /* SystemCoreClock = tần số CPU (Hz) */
    SysTick_Config(SystemCoreClock / 1000U);   /* chia 1000 = số nhịp trong 1 ms → ngắt mỗi 1 ms */

    /* BRP=8, TS1=5, TS2=0, SJW=0, LBKM=0 (APB1 = 36 MHz): 500 kbit/s, 8 tq, mẫu 87,5 % */
    g_init_err = Can_Init(0x00050008U);
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

        /* 2. Kết quả khung trước + trạng thái lỗi bus */
        check_tx_done();
        g_esr = CAN1->ESR;

        /* 3. Gửi khung mới; mailbox bận thì đếm bận, giữ nguyên số thứ tự */
        tx[0] = seq;
        if (can_send(tx_id, tx, tx_dlc) != 0)
        {
            g_tx_busy++;
            continue;
        }
        g_tx_req++;
        seq++;
    }
}
