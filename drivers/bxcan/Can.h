/*
 * Can.h — API MỚI của driver bxCAN, đặt tên theo module Can của AUTOSAR.
 * Dùng chung F103 và F407; phần riêng chip nằm ở drivers/mcu/<chip>/can_port_<chip>.c.
 * bxcan.h cũ GIỮ NGUYÊN để các test GĐ 4 vẫn build; chuyển sang file này ở GĐ 5.
 * File này CHỈ khai báo.
 */
#ifndef CAN_H
#define CAN_H

#include <stdint.h>
#include "Std_Types.h"

/* Can_Write: mailbox đang bận, thử lại ở lần gọi sau */
#define CAN_BUSY  2u

/* Khởi tạo bxCAN: clock + chân (can_port_init), vào init, ghi BTR, thoát init.
 * Ai gọi: main.
 * Chạy ở: khởi động, trước StartOS (ECU1) hoặc trước vòng lặp chính (F103).
 * Trả về: E_OK; E_NOT_OK nếu kẹt khi vào hoặc thoát chế độ init. */
Std_ReturnType Can_Init(uint32_t btr);

/* Đặt bộ lọc phần cứng: chỉ nhận các ID 11 bit trong danh sách ids[0..n-1].
 * Ai gọi: CanIf_Init.
 * Chạy ở: khởi động.
 * Trả về: E_OK; E_NOT_OK nếu n vượt số ID mà các bank lọc chứa được. */
Std_ReturnType Can_SetRxFilter(const uint16_t *ids, uint8_t n);

/* Bật ngắt nhận: FMPIE0 trong CAN1->IER + bật IRQ CAN RX0 ở NVIC.
 * Ai gọi: StartupHook (ECU1), main (F103).
 * Chạy ở: khởi động, sau Can_SetRxFilter.
 * Trả về: không. */
void Can_EnableRxIrq(void);

/* Nạp 1 khung ID chuẩn vào mailbox trống rồi yêu cầu gửi (không chờ gửi xong).
 * Ai gọi: CanIf_Transmit.
 * Chạy ở: task.
 * Trả về: E_OK; CAN_BUSY nếu không còn mailbox trống. */
Std_ReturnType Can_Write(uint16_t id, const uint8_t *data, uint8_t dlc);

/* Đọc HẾT khung trong FIFO0; mỗi khung gọi CanIf_RxIndication rồi nhả FIFO.
 * Ai gọi: ISR(CanRxIsr).
 * Chạy ở: ISR loại 2 (CAN1_RX0).
 * Trả về: không. */
void Can_IsrRx(void);

#endif /* CAN_H */
