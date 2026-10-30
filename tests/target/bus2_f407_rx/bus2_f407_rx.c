/* bus2_f407_rx.c — Test bus thật 2 board (Mốc 1 bước 2), phía NHẬN: Hiwonder V1.2 (ECU1)
 * Khung từ tests/target/f407_can_loopback (code của Thắng), bỏ phần gửi.
 * Vị trí: can-ecu-network/tests/target/bus2_f407_rx/bus2_f407_rx.c
 *
 * Chuỗi: F103 gửi 0x080 mỗi 100 ms → VP230 → PD0 → FIFO0 → ngắt CAN1_RX0
 *        → ISR can_rx_isr (can_receive = đọc + nhả FIFO, ActivateTask) → Task_CanRx (kiểm số thứ tự, đảo LED)
 *        Alarm_Esr 100 ms → Task_EsrCopy chép CAN1->ESR vào RAM (ngoại vi đọc HOTPLUG lúc CPU ngủ ra rác)
 * LED PE10: sáng khi mức THẤP. Buzzer PA8: kêu khi mức CAO.
 * Target Trampoline dùng CMSIS cũ: BSRRL = set (lên 1), BSRRH = reset (về 0). */
#include "tpl_os.h"
#include "stm32f4xx.h"
#include "bxcan.h"

#define LED_PIN      (10U)      /* PE10 */
#define BUZZER_PIN   (8U)       /* PA8  */
#define CAN_BTR_F407_500K  (0x001A0005U)  /* BRP=5 (chia 6), TS1=10 (11 tq), TS2=1 (2 tq), LBKM=0 */

DeclareTask(Task_CanRx);        /* khai báo tên task để ISR gọi ActivateTask */

/* Bộ đệm 1 khung: ISR ghi, Task_CanRx đọc (chép trong Suspend/ResumeOSInterrupts) */
static uint16_t s_rx_id;
static uint8_t  s_rx_data[8];
static uint8_t  s_rx_dlc;

/* Số thứ tự (byte 0) của khung 0x080 trước đó; s_have_last = 0 khi chưa có khung nào */
static uint8_t  s_last_seq;
static uint8_t  s_have_last = 0U;

/* Bộ đếm kiểm chứng — đọc qua ST-Link */
volatile uint8_t  g_init_err = 0;   /* kết quả Can_Init: 0 OK, 1 kẹt vào init, 2 kẹt thoát init */
volatile uint32_t g_rx_isr   = 0;   /* số lần ISR đọc được khung */
volatile uint32_t g_rx_ok    = 0;   /* số khung ID 0x080 Task_CanRx đã xử lý */
volatile uint32_t g_rx_gap   = 0;   /* số lần byte 0 không bằng lần trước + 1 (mất khung hoặc F103 reset) */
volatile uint32_t g_esr      = 0;   /* bản sao CAN1->ESR, cập nhật mỗi 100 ms */

#define APP_Task_Task_CanRx_START_SEC_CODE
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

    /* 5. CAN1: 500 kbit/s, chế độ thường; bật ngắt FIFO0 có khung (FMPIE0) và IRQ 20 ở NVIC */
    g_init_err = Can_Init(CAN_BTR_F407_500K);
    CAN1->IER |= CAN_IER_FMPIE0;
    NVIC_EnableIRQ(CAN1_RX0_IRQn);       /* Trampoline không tự bật NVIC cho ISR2 không phải EXTI */

    beep_once();                         /* nghe bíp = main chạy tới đây */
    StartOS(OSDEFAULTAPPMODE);           /* không bao giờ trả về */
    return 0;
}

/* Task nhận: chạy khi ISR kích hoạt; kiểm số thứ tự, đảo LED mỗi khung */
TASK(Task_CanRx)
{
    uint16_t id;
    uint8_t  data[8];
    uint8_t  i;

    SuspendOSInterrupts();                   /* chặn ISR2 trước khi chép bộ đệm */
    id = s_rx_id;
    for (i = 0U; i < 8U; i++) { data[i] = s_rx_data[i]; }
    ResumeOSInterrupts();                    /* mở lại */

    if (id == 0x080U)
    {
        g_rx_ok++;
        /* uint8_t quay vòng: 255 + 1 = 0 */
        if ((s_have_last != 0U) && (data[0] != (uint8_t)(s_last_seq + 1U)))
        {
            g_rx_gap++;
        }
        s_last_seq  = data[0];
        s_have_last = 1U;
    }

    if ((GPIOE->ODR & (1U << LED_PIN)) != 0U)    /* đang 1 (LED tắt) */
    {
        GPIOE->BSRRH = (1U << LED_PIN);          /* → 0: LED sáng */
    }
    else
    {
        GPIOE->BSRRL = (1U << LED_PIN);          /* → 1: LED tắt */
    }
    TerminateTask();
}

#define APP_Task_Task_CanRx_STOP_SEC_CODE
#include "tpl_memmap.h"

#define APP_Task_Task_EsrCopy_START_SEC_CODE
#include "tpl_memmap.h"

/* Task do Alarm_Esr gọi mỗi 100 ms: chép thanh ghi lỗi CAN ra RAM để đọc HOTPLUG không cần halt */
TASK(Task_EsrCopy)
{
    g_esr = CAN1->ESR;
    TerminateTask();
}

#define APP_Task_Task_EsrCopy_STOP_SEC_CODE
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
