/*
 * CanIf.h — CAN Interface: ánh xạ PDU (phía COM) <-> CAN ID (phía driver).
 * C thuần, KHÔNG gọi HAL, dùng chung mọi ECU. File này CHỈ khai báo.
 *
 * Chuỗi gọi chiều gửi:
 *   SWC -> Rte_Write_<tín hiệu> -> Com_SendSignal
 *     (COM chỉ ghi bộ đệm)
 *   Task_10ms: Com_MainFunctionTx
 *     -> CanIf_Transmit -> Can_Write -> mailbox
 *
 * Chuỗi gọi chiều nhận:
 *   ISR(CanRxIsr): Can_IsrRx
 *     -> CanIf_RxIndication (CAN ID -> PDU)
 *     -> Com_RxIndication (chép 8 byte)
 *   ---------------- hết ISR ----------------
 *   Task_10ms: Com_MainFunctionRx (kiểm timeout)
 *   SWC -> Rte_Read_<tín hiệu> -> Com_ReceiveSignal
 */
#ifndef CANIF_H
#define CANIF_H

#include <stdint.h>
#include "Std_Types.h"
#include "ComStack_Types.h"

/* Nạp bảng PDU <-> CAN ID; đặt bộ lọc driver theo các ID mà ECU này nhận.
 * Ai gọi: main (hoặc StartupHook ở ECU1), sau Can_Init.
 * Chạy ở: khởi động.
 * Trả về: không. */
void CanIf_Init(void);

/* Tra CAN ID của PDU txPduId rồi gửi qua Can_Write.
 * Ai gọi: COM (Com_MainFunctionTx).
 * Chạy ở: task.
 * Trả về: kết quả Can_Write (E_OK, CAN_BUSY); E_NOT_OK nếu txPduId sai. */
Std_ReturnType CanIf_Transmit(PduIdType txPduId, const PduInfoType *pduInfo);

/* Đổi CAN ID -> PDU rồi gọi Com_RxIndication; ID lạ thì bỏ.
 * Ai gọi: driver (Can_IsrRx), mỗi khung một lần.
 * Chạy ở: ISR loại 2.
 * Trả về: không. */
void CanIf_RxIndication(uint16_t canId, const uint8_t *data, uint8_t dlc);

#endif /* CANIF_H */
