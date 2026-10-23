/*
 * can_port.h — Phần riêng chip của driver bxCAN, bản STM32F407 (ECU1, Hiwonder V1.2).
 * bxcan.c include file này (KHÔNG include header chip trực tiếp); chọn chip bằng
 * include path: -I drivers/mcu/stm32f103 hoặc -I drivers/mcu/stm32f407.
 * stm32f4xx.h ở đây là bản CMSIS cũ đi kèm target Trampoline (StdPeriph V1.6.0).
 */
#ifndef CAN_PORT_H
#define CAN_PORT_H

#include "stm32f4xx.h"

/* Bật clock GPIOD + CAN1, đặt PD0 (RX) / PD1 (TX) sang AF9 = CAN1 */
void can_port_init(void);

#endif /* CAN_PORT_H */
