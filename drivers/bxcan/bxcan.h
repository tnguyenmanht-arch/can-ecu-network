/*
 * bxcan.h — Driver bxCAN tối giản cho STM32F103 (GĐ 4 ĐAKS)
 * Code do Thắng viết qua các bài 10–13, đã chấm đạt.
 *
 * Bảng giá trị trả về:
 *   can_enter_init()        0 = đã vào init      | 1 = quá giờ chờ INAK lên 1
 *   can_exit_init()         0 = đã thoát init    | 1 = quá giờ chờ INAK về 0
 *   can_filter_accept_all() không trả về (void)
 *   can_send()              0 = đã nạp mailbox 0 | 1 = mailbox 0 đang bận
 *   can_receive()           0 = đọc được 1 khung | 1 = FIFO0 rỗng
 *   Can_Init()              0 = OK | 1 = kẹt khi vào init | 2 = kẹt khi thoát init
 */
#ifndef BXCAN_H
#define BXCAN_H

#include <stdint.h>

/* Số vòng chờ tối đa khi đợi phần cứng (vào/thoát init) */
#define CAN_INIT_TIMEOUT 10000U

uint8_t Can_Init(void);

uint8_t can_enter_init(void);
uint8_t can_exit_init(void);
void    can_filter_accept_all(void);

/* data phải là mảng 8 byte, kể cả khi dlc < 8 */
uint8_t can_send(uint16_t id, const uint8_t *data, uint8_t dlc);
uint8_t can_receive(uint16_t *id, uint8_t *data, uint8_t *dlc);

#endif /* BXCAN_H */
