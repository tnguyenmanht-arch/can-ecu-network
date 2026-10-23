/*
 * can_port_f407.c — Phần bxCAN riêng cho STM32F407 (ECU1, Hiwonder V1.2)
 * Code do Thắng viết (Mốc 1 phần 2, 23/10/2026), đã chấm đạt.
 * Vị trí: drivers/mcu/stm32f407/can_port_f407.c
 *
 * Khác F103: F4 không có AFIO/remap; mỗi chân tự chọn chức năng bằng MODER (10 = AF)
 * và AFR (AF9 = CAN1). Board Hiwonder nối sẵn transceiver VP230 vào PD0 (RX) / PD1 (TX).
 * Clock hệ thống (APB1 = 42 MHz) do SystemInit của Trampoline đặt, ở đây không đụng tới.
 */
#include "can_port.h"

#define CAN1_RX_PIN   (0U)   /* PD0 */
#define CAN1_TX_PIN   (1U)   /* PD1 */
#define GPIO_AF9_CAN1 (9U)   /* AF9 = CAN1 (datasheet F407, bảng Alternate function) */

void can_port_init(void)
{
    /* ① Bật clock: GPIOD trên AHB1 (bit 3), CAN1 trên APB1 (bit 25, giống F103) */
    RCC->AHB1ENR |= (1U << 3U);
    RCC->APB1ENR |= (1U << 25U);

    /* ② MODER: 2 bit/chân, 10 = alternate function; xoá trước rồi ghi */
    GPIOD->MODER &= ~((3U << (2U * CAN1_RX_PIN)) | (3U << (2U * CAN1_TX_PIN)));
    GPIOD->MODER |=  ((2U << (2U * CAN1_RX_PIN)) | (2U << (2U * CAN1_TX_PIN)));

    /* ③ AFRL: 4 bit/chân, chọn AF9 cho cả 2 chân → AFR[0] bit [7:0] = 0x99 */
    GPIOD->AFR[0] &= ~((0xFU << (4U * CAN1_RX_PIN)) | (0xFU << (4U * CAN1_TX_PIN)));
    GPIOD->AFR[0] |=  ((GPIO_AF9_CAN1 << (4U * CAN1_RX_PIN)) | (GPIO_AF9_CAN1 << (4U * CAN1_TX_PIN)));
}
