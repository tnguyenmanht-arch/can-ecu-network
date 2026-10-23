/* f407_can_loopback.c — Mốc 1 phần 2: CAN1 loopback bằng ISR2 trên Hiwonder V1.2 (ECU1)
 * Code do Thắng điền khung (23/10/2026), đã chấm và sửa trong Claude app.
 * Vị trí: can-ecu-network/tests/target/f407_can_loopback/f407_can_loopback.c
 *
 * Chuỗi: Alarm 100 ms → Task_CanTx (can_send 0x080) → [loopback] FIFO0 → ngắt CAN1_RX0
 *        → ISR can_rx_isr (can_receive = đọc + nhả FIFO, ActivateTask) → Task_CanRx (kiểm khung, đảo LED)
 * LED PE10: sáng khi mức THẤP. Buzzer PA8: kêu khi mức CAO.
 * Target Trampoline dùng CMSIS cũ: BSRRL = set (lên 1), BSRRH = reset (về 0). */
#include "tpl_os.h"
#include "stm32f4xx.h"
#include "bxcan.h"

#define LED_PIN      (10U)      /* PE10 */
#define BUZZER_PIN   (8U)       /* PA8  */
#define CAN_BTR_F407_500K_LBK  (0x401A0005U)  /* BRP=5 (chia 6), TS1=10 (11 tq), TS2=1 (2 tq), LBKM=1 */

DeclareTask(Task_CanRx);        /* khai báo tên task để ISR gọi ActivateTask */

/* Bộ đệm 1 khung: ISR ghi, Task_CanRx đọc (chép trong Suspend/ResumeOSInterrupts) */
static uint16_t s_rx_id;
static uint8_t  s_rx_data[8];
static uint8_t  s_rx_dlc;

/* Bộ đếm kiểm chứng — đọc qua ST-Link */
volatile uint8_t  g_init_err    = 0;   /* kết quả Can_Init: 0 OK, 1 kẹt vào init, 2 kẹt thoát init */
volatile uint32_t g_tx_ok       = 0;   /* số khung đã nạp mailbox */
volatile uint32_t g_tx_busy     = 0;   /* số lần mailbox 0 đang bận */
volatile uint32_t g_rx_isr      = 0;   /* số lần ISR đọc được khung */
volatile uint32_t g_rx_ok       = 0;   /* số khung Task_CanRx kiểm đúng */
volatile uint32_t g_rx_mismatch = 0;   /* số khung sai id/dlc/dữ liệu */

#define APP_Task_Task_CanTx_START_SEC_CODE
#include "tpl_memmap.h"

/* Bíp ngắn trước StartOS: "main đã chạy tới đây" */
static void beep_once(void)
{
    volatile uint32_t i;
    GPIOA->BSRRL = (1U << BUZZER_PIN);             /* buzzer kêu: PA8 = 1 */
    for (i = 0; i < 2000000U; i++) { }             /* chờ thô ~0,1 s (SysTick chưa chạy) */
    GPIOA->BSRRH = (1U << BUZZER_PIN);             /* buzzer tắt: PA8 = 0 */
}

FUNC(int, OS_APPL_CODE) main(void)
{
    /* 1. Bật clock GPIOA (bit 0) và GPIOE (bit 4) */
    RCC->AHB1ENR |= (1U << 0) | (1U << 4);

    /* 2. Đặt mức an toàn TRƯỚC khi chuyển sang output: LED tắt = mức cao, buzzer im = mức thấp */
    GPIOE->BSRRL = (1U << LED_PIN);
    GPIOA->BSRRH = (1U << BUZZER_PIN);

    /* 3. MODER: 2 bit mỗi chân, 01 = output; xoá trước rồi ghi */
    GPIOE->MODER &= ~(3U << (LED_PIN * 2U));
    GPIOE->MODER |=  (1U << (LED_PIN * 2U));
    GPIOA->MODER &= ~(3U << (BUZZER_PIN * 2U));
    GPIOA->MODER |=  (1U << (BUZZER_PIN * 2U));

    /* 4. Cho phép debug khi CPU ngủ (idle chạy wfe) */
    DBGMCU->CR |= DBGMCU_CR_DBG_SLEEP;

    /* 5. CAN1: 500 kbit/s, loopback; rồi bật công tắc 1 của ngắt: FIFO0 có khung → báo ngắt */
    g_init_err = Can_Init(CAN_BTR_F407_500K_LBK);
    CAN1->IER |= CAN_IER_FMPIE0;

    beep_once();                         /* nghe bíp = main chạy tới đây */
    StartOS(OSDEFAULTAPPMODE);           /* không bao giờ trả về */
    return 0;
}

/* Task gửi: alarm gọi mỗi 100 ms, gửi khung Pedal 0x080 */
TASK(Task_CanTx)
{
    uint8_t tx[8] = {0U, 0xAAU, 0x55U, 0x01U, 0x02U, 0x03U, 0x04U, 0x05U};
    tx[0] = (uint8_t)g_tx_ok;                /* byte 0 = số thứ tự khung */
    if (can_send(0x080U, tx, 8U) == 0U) { g_tx_ok++; } else { g_tx_busy++; }
    TerminateTask();
}

#define APP_Task_Task_CanTx_STOP_SEC_CODE
#include "tpl_memmap.h"

#define APP_ISR_can_rx_isr_START_SEC_CODE
#include "tpl_memmap.h"

/* ISR loại 2: ngắt CAN1_RX0 (FIFO0 có khung). Phải nhả FIFO, nếu không ngắt vào lại liên tục */
ISR(can_rx_isr)
{
    if (can_receive(&s_rx_id, s_rx_data, &s_rx_dlc) == 0U)   /* đọc + nhả FIFO0 */
    {
        g_rx_isr++;
        ActivateTask(Task_CanRx);                            /* giao việc xử lý cho task */
    }
}

#define APP_ISR_can_rx_isr_STOP_SEC_CODE
#include "tpl_memmap.h"

#define APP_Task_Task_CanRx_START_SEC_CODE
#include "tpl_memmap.h"

/* Task nhận: chạy khi ISR kích hoạt; kiểm khung, đúng thì đảo LED */
TASK(Task_CanRx)
{
    uint16_t id;
    uint8_t  data[8];
    uint8_t  dlc, i;

    SuspendOSInterrupts();                   /* chặn ISR2 trước khi chép bộ đệm */
    id  = s_rx_id;
    dlc = s_rx_dlc;
    for (i = 0U; i < 8U; i++) { data[i] = s_rx_data[i]; }
    ResumeOSInterrupts();                    /* mở lại */

    if ((id == 0x080U) && (dlc == 8U) && (data[1] == 0xAAU))
    {
        g_rx_ok++;
        if ((GPIOE->ODR & (1U << LED_PIN)) != 0U)    /* đang 1 (LED tắt) */
        {
            GPIOE->BSRRH = (1U << LED_PIN);          /* → 0: LED sáng */
        }
        else
        {
            GPIOE->BSRRL = (1U << LED_PIN);          /* → 1: LED tắt */
        }
    }
    else
    {
        g_rx_mismatch++;
    }
    TerminateTask();
}

#define APP_Task_Task_CanRx_STOP_SEC_CODE
#include "tpl_memmap.h"
