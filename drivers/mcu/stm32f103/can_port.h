/*
 * can_port.h — Phần riêng chip của driver bxCAN, bản STM32F103.
 * bxcan.c include file này (KHÔNG include header chip trực tiếp); chọn chip bằng
 * include path: -I drivers/mcu/stm32f103 hoặc -I drivers/mcu/stm32f407.
 */
#ifndef CAN_PORT_H
#define CAN_PORT_H

#include "stm32f1xx.h"

/* Bật clock GPIOB + AFIO + CAN1, remap CAN1 ra PB8 (RX) / PB9 (TX), cấu hình 2 chân */
void can_port_init(void);

#endif /* CAN_PORT_H */
