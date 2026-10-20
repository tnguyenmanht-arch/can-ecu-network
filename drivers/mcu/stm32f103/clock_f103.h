/*
 * clock_f103.h — Cấu hình clock STM32F103 bằng thanh ghi (CMSIS, không HAL)
 *
 *   HSE 8 MHz → PLL ×9 → SYSCLK 72 MHz
 *   AHB ÷1 = 72 MHz, APB2 ÷1 = 72 MHz, APB1 ÷2 = 36 MHz (BTR của bxCAN tính theo APB1)
 *
 * Bảng giá trị trả về của clock_init():
 *   0 = OK, đang chạy 72 MHz từ PLL
 *   1 = HSE không lên (kiểm thạch anh 8 MHz)
 *   2 = PLL không khoá
 *   3 = không chuyển được SYSCLK sang PLL
 * Khi lỗi (1, 2, 3): quay về HSI 8 MHz, mọi bộ chia ÷1, để chip vẫn chạy.
 */
#ifndef CLOCK_F103_H
#define CLOCK_F103_H

#include <stdint.h>

/* Số vòng chờ tối đa khi đợi phần cứng. Mỗi vòng vài chu kỳ ở HSI 8 MHz,
   nên 100000 vòng cỡ vài chục ms — dư so với thời gian khởi động thạch anh (~ms). */
#define CLOCK_HSE_TIMEOUT  100000U   /* chờ HSERDY */
#define CLOCK_PLL_TIMEOUT  100000U   /* chờ PLLRDY */
#define CLOCK_SW_TIMEOUT   100000U   /* chờ SWS đổi nguồn */

#define CLOCK_OK           0U
#define CLOCK_ERR_HSE      1U
#define CLOCK_ERR_PLL      2U
#define CLOCK_ERR_SWITCH   3U

uint8_t clock_init(void);

#endif /* CLOCK_F103_H */
