/*
 * Com.h — Module COM: đóng gói / bóc tách tín hiệu theo DBC, gửi theo chu kỳ,
 * giám sát timeout. C thuần, KHÔNG gọi HAL, dùng chung mọi ECU.
 * Bảng tín hiệu, PDU, chu kỳ do comgen sinh (gen/Com_Cfg.c/.h). File này CHỈ khai báo.
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
#ifndef COM_H
#define COM_H

#include <stdint.h>
#include "Std_Types.h"
#include "ComStack_Types.h"

/* Số hiệu tín hiệu: chỉ số trong bảng do comgen sinh */
typedef uint8_t Com_SignalIdType;

/* Com_ReceiveSignal: PDU chứa tín hiệu đã quá timeout */
#define COM_E_TIMEOUT  2u

/* Xoá bộ đệm PDU về giá trị ban đầu, đặt lại bộ đếm chu kỳ và timeout.
 * Ai gọi: main (hoặc StartupHook ở ECU1).
 * Chạy ở: khởi động.
 * Trả về: không. */
void Com_Init(void);

/* Ghi giá trị thô của 1 tín hiệu vào bộ đệm PDU gửi; chưa gửi ngay.
 * Ai gọi: RTE (Rte_Write_<tín hiệu>).
 * Chạy ở: task.
 * Trả về: E_OK; E_NOT_OK nếu signalId sai. */
Std_ReturnType Com_SendSignal(Com_SignalIdType signalId, int32_t rawValue);

/* Bóc tách 1 tín hiệu từ bộ đệm PDU nhận; tín hiệu có dấu phải ra số âm đúng.
 * Ai gọi: RTE (Rte_Read_<tín hiệu>).
 * Chạy ở: task.
 * Trả về: E_OK; COM_E_TIMEOUT nếu PDU đã quá timeout (*rawValue vẫn là giá trị cũ). */
Std_ReturnType Com_ReceiveSignal(Com_SignalIdType signalId, int32_t *rawValue);

/* CHỈ chép 8 byte vào bộ đệm PDU nhận và đánh dấu "có khung mới"; không bóc tách.
 * Ai gọi: CanIf (CanIf_RxIndication).
 * Chạy ở: ISR loại 2.
 * Trả về: không. */
void Com_RxIndication(PduIdType rxPduId, const uint8_t *data);

/* Kiểm timeout từng PDU nhận: có khung mới thì đặt lại đồng hồ, quá hạn thì đánh dấu timeout.
 * Ai gọi: Task_10ms.
 * Chạy ở: task, mỗi 10 ms.
 * Trả về: không. */
void Com_MainFunctionRx(void);

/* Đếm chu kỳ từng PDU gửi; PDU tới hạn thì đóng gói và gọi CanIf_Transmit.
 * Ai gọi: Task_10ms.
 * Chạy ở: task, mỗi 10 ms.
 * Trả về: không. */
void Com_MainFunctionTx(void);

#endif /* COM_H */
