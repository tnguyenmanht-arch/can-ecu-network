/*
 * clock_f103.c — Cấu hình clock STM32F103 bằng thanh ghi (CMSIS, không HAL)
 * Xem bảng giá trị trả về trong clock_f103.h.
 */
#include "stm32f1xx.h"
#include "clock_f103.h"

/* Quay về HSI 8 MHz như sau reset. Gọi khi một bước chờ phần cứng bị quá giờ. */
static void clock_fallback_hsi(void)
{
    uint32_t count = 0;

    /* SYSCLK = HSI (SW = 00), chờ SWS báo HSI */
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_HSI;
    while (((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI) && (count < CLOCK_SW_TIMEOUT))
    {
        count++;
    }

    /* Tắt PLL và HSE. An toàn cả khi chưa về được HSI: phần cứng không cho
       tắt PLLON/HSEON khi chúng đang làm nguồn SYSCLK (lệnh ghi bị bỏ qua). */
    RCC->CR &= ~(RCC_CR_PLLON | RCC_CR_HSEON);

    /* Chỉ hạ bộ chia và Flash latency KHI SWS đã báo HSI. Nếu vẫn đang chạy PLL 72 MHz
       mà bỏ ÷2 của APB1 (lên 72 MHz, vượt mức tối đa 36 MHz) hoặc bỏ wait state Flash
       thì chip chạy sai. Quá giờ thì giữ nguyên: bộ chia và latency hiện tại vẫn an toàn cho 72 MHz. */
    if ((RCC->CFGR & RCC_CFGR_SWS) == RCC_CFGR_SWS_HSI)
    {
        /* Bộ chia về ÷1 như sau reset (APB1 = 8 MHz) */
        RCC->CFGR &= ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2);

        /* Ở 8 MHz Flash không cần wait state; giảm latency SAU khi đã về tần số thấp */
        FLASH->ACR &= ~FLASH_ACR_LATENCY;
    }

    SystemCoreClockUpdate();                        /* SystemCoreClock = 8000000 */
}

uint8_t clock_init(void)
{
    uint32_t count;

    /* ⓪ Đã chạy PLL rồi (ví dụ gọi clock_init lần thứ hai): không cấu hình lại,
       vì PLLSRC/PLLMULL chỉ ghi được khi PLL đang tắt. Chỉ cập nhật SystemCoreClock. */
    if ((RCC->CFGR & RCC_CFGR_SWS) == RCC_CFGR_SWS_PLL)
    {
        SystemCoreClockUpdate();
        return CLOCK_OK;
    }

    /* ① Bật HSE, chờ HSERDY có giới hạn */
    RCC->CR |= RCC_CR_HSEON;
    count = 0;
    while ((RCC->CR & RCC_CR_HSERDY) == 0)
    {
        count++;
        if (count > CLOCK_HSE_TIMEOUT)
        {
            clock_fallback_hsi();
            return CLOCK_ERR_HSE;                    /* thạch anh không dao động */
        }
    }

    /* ② Flash: 2 wait state + prefetch, làm TRƯỚC khi tăng lên 72 MHz.
       Chú ý: LATENCY là trường 3 bit, 2 WS = 0b010 = (2U << Pos).
       KHÔNG dùng FLASH_ACR_LATENCY_2: macro đó là bit 2 (= 0b100). */
    FLASH->ACR = (FLASH->ACR & ~FLASH_ACR_LATENCY) | (2U << FLASH_ACR_LATENCY_Pos) | FLASH_ACR_PRFTBE;

    /* ③ Bộ chia: AHB ÷1, APB1 ÷2 (36 MHz, tối đa cho phép), APB2 ÷1 */
    RCC->CFGR = (RCC->CFGR & ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2))
              | RCC_CFGR_HPRE_DIV1 | RCC_CFGR_PPRE1_DIV2 | RCC_CFGR_PPRE2_DIV1;

    /* ④ PLL: nguồn HSE (PLLSRC = 1), HSE không chia (PLLXTPRE = 0), ×9.
       Xoá cả 3 trường cũ trước rồi mới ghi giá trị mới. */
    RCC->CFGR &= ~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLXTPRE | RCC_CFGR_PLLMULL);
    RCC->CFGR |= RCC_CFGR_PLLSRC | RCC_CFGR_PLLMULL9;

    /* ⑤ Bật PLL, chờ PLLRDY có giới hạn */
    RCC->CR |= RCC_CR_PLLON;
    count = 0;
    while ((RCC->CR & RCC_CR_PLLRDY) == 0)
    {
        count++;
        if (count > CLOCK_PLL_TIMEOUT)
        {
            clock_fallback_hsi();
            return CLOCK_ERR_PLL;                    /* PLL không khoá */
        }
    }

    /* ⑥ Chuyển SYSCLK sang PLL, chờ SWS = PLL có giới hạn */
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_PLL;
    count = 0;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL)
    {
        count++;
        if (count > CLOCK_SW_TIMEOUT)
        {
            clock_fallback_hsi();
            return CLOCK_ERR_SWITCH;                 /* không chuyển được sang PLL */
        }
    }

    /* ⑦ Cập nhật biến SystemCoreClock (= 72000000) cho SysTick_Config */
    SystemCoreClockUpdate();
    return CLOCK_OK;
}
