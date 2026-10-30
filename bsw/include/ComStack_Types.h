/*
 * ComStack_Types.h — Kiểu dùng chung giữa COM và CanIf (tên theo AUTOSAR).
 * PDU (Protocol Data Unit) = một khung CAN nhìn từ phía phần mềm: số hiệu + dữ liệu.
 */
#ifndef COMSTACK_TYPES_H
#define COMSTACK_TYPES_H

#include <stdint.h>

/* Số hiệu PDU: chỉ số trong bảng cấu hình do comgen sinh, KHÔNG phải CAN ID */
typedef uint8_t PduIdType;

/* Dữ liệu một PDU: con trỏ tới bộ đệm và số byte (CAN cổ điển: tối đa 8) */
typedef struct
{
    uint8_t *SduDataPtr;
    uint8_t  SduLength;
} PduInfoType;

#endif /* COMSTACK_TYPES_H */
