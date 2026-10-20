# CMSIS cho STM32F103 (chép nguyên, KHÔNG sửa)

Chép từ gói **STM32Cube_FW_F1 V1.8.7** do STM32CubeIDE tải về máy
(`~/STM32Cube/Repository/STM32Cube_FW_F1_V1.8.7/Drivers/CMSIS/`), ngày 20/10/2026.
Phần `device/` là cùng bộ file với repo chính thức `STMicroelectronics/cmsis_device_f1`.

| Thư mục | Nội dung | Phiên bản | Giấy phép |
|---|---|---|---|
| `core/Include/` | `core_cm3.h`, `cmsis_compiler.h`, `cmsis_gcc.h`, `cmsis_version.h`, `mpu_armv7.h` | CMSIS Core V5.0.8 (`core_cm3.h`) | Apache-2.0, `core/LICENSE.txt` |
| `device/Include/` | `stm32f1xx.h`, `stm32f103xb.h`, `system_stm32f1xx.h` | CMSIS Device F1 V4.3.5 | Apache-2.0, `device/LICENSE.txt` |
| `device/Source/` | `system_stm32f1xx.c`, `startup_stm32f103xb.s` (bản GCC) | như trên | như trên |

- Chỉ chép những file cần cho F103 dòng medium-density (`STM32F103xB`: C8 và CB).
- Không dùng bản CMSIS core trong submodule Trampoline (V5.2.0), để build F103 không phụ thuộc Trampoline.
- `system_stm32f1xx.c` giữ nguyên: `SystemInit()` không cấu hình PLL; clock do code dự án tự cấu hình.
