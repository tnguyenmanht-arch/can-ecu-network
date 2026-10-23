/*
 * can_port_f103.c — Phần riêng chip của driver bxCAN cho STM32F103.
 * Chuyển nguyên văn bước ①②③ từ Can_Init cũ trong drivers/bxcan/bxcan.c (code của Thắng).
 * Chân: PB8 = CAN_RX, PB9 = CAN_TX (remap).
 */
#include "can_port.h"

void can_port_init(void)
{
    /* ① Bật clock */
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN | RCC_APB2ENR_AFIOEN;   /* GPIOB, AFIO */
    RCC->APB1ENR |= RCC_APB1ENR_CAN1EN;                        /* CAN1 */

    /* ② Remap CAN1 -> PB8/PB9: xoá trường CAN_REMAP rồi đặt = 10 (nhị phân) */
    AFIO->MAPR &= ~AFIO_MAPR_CAN_REMAP;
    AFIO->MAPR |= AFIO_MAPR_CAN_REMAP_REMAP2;                  /* = (0x2U << 13) */

    /* ③ Chân: PB8 = input pull-up (0x8), PB9 = AF push-pull 50 MHz (0xB) */
    GPIOB->CRH &= ~(0xFFU << 0);                               /* xoá 8 bit của PB8, PB9 */
    GPIOB->CRH |= (0x8U << 0) | (0xBU << 4);
    GPIOB->ODR |= (1U << 8);                                   /* pull-up cho PB8 */
}
