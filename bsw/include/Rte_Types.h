/*
 * Rte_Types.h — Mã trả về của các hàm Rte_Read_<tín hiệu> / Rte_Write_<tín hiệu>.
 * Tên mượn theo AUTOSAR RTE, giá trị số do dự án tự đặt (không lấy theo chuẩn).
 */
#ifndef RTE_TYPES_H
#define RTE_TYPES_H

#include "Std_Types.h"

#define RTE_E_OK                0u   /* có giá trị hợp lệ */
#define RTE_E_INVALID           1u   /* tín hiệu đang là SNA (bên gửi báo không có dữ liệu) */
#define RTE_E_MAX_AGE_EXCEEDED  2u   /* mất khung quá timeout (GenMsgTimeoutTime trong DBC) */

#endif /* RTE_TYPES_H */
