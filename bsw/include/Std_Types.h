/*
 * Std_Types.h — Kiểu dùng chung cho mọi module BSW (tên theo AUTOSAR).
 * C thuần, không phụ thuộc chip.
 */
#ifndef STD_TYPES_H
#define STD_TYPES_H

#include <stdint.h>

/* Kết quả trả về chung của các hàm API */
typedef uint8_t Std_ReturnType;

#define E_OK      0u   /* thành công */
#define E_NOT_OK  1u   /* thất bại */

#endif /* STD_TYPES_H */
