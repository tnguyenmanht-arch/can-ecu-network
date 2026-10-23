/* f407_blink.c — App Trampoline đầu tiên trên Hiwonder V1.2 (ECU1)
 * LED PE10: sáng khi mức THẤP. Buzzer PA8: kêu khi mức CAO.
 * Vị trí: can-ecu-network/tests/target/f407_blink/f407_blink.c
 * Target Trampoline dùng CMSIS cũ (StdPeriph V1.6.0): BSRR tách 2 nửa 16 bit, BSRRL = set (lên 1), BSRRH = reset (về 0). */
#include "tpl_os.h"
#include "stm32f4xx.h"

#define LED_PIN      (10U)      /* PE10 */
#define BUZZER_PIN   (8U)       /* PA8  */

#define APP_Task_blink_START_SEC_CODE
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

    beep_once();                         /* nghe bíp = main chạy tới đây */

    StartOS(OSDEFAULTAPPMODE);           /* không bao giờ trả về */
    return 0;
}

/* Task do alarm blink_al gọi mỗi 500 tick (500 ms trên F407) */
TASK(blink)
{
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

#define APP_Task_blink_STOP_SEC_CODE
#include "tpl_memmap.h"
