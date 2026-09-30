# f407_bringup — firmware kiểm tra board Hiwonder khi hàng về

Project STM32CubeIDE cho board Hiwonder "ROS Robot Controller" (STM32F407VET6).
**Tên project trong CubeIDE vẫn là `amr_stm32f407`** (giữ nguyên để không phải sửa `.ioc`,
`.cproject`, `.mxproject`), nên file build ra là `Debug/amr_stm32f407.elf`.

**Đây không phải firmware ECU1.** Firmware ECU1 sẽ viết mới trên Trampoline (thư mục `ecu1_vcu/`).

Nguồn gốc: firmware superloop của xe AMR, port sang board này ngày 29/9/2026, lúc còn định lắp
board lên xe. Giữ lại vì 2 phần dùng được cho đồ án:
- **Chế độ bring-up**: giữ nút K1 khi reset để vào chế độ kiểm tra tương tác (`Core/Src/bringup.c`),
  xuất ra cả USART1 (cổng USB số 7) lẫn USART3 (cổng số 4). Dùng theo checklist `docs/bringup-f407.md`.
- **Hạ tầng đo DWT** (`Core/Src/dwt_log.c/.h`), dùng với `tools/dwt/`.

Phần motor, encoder, servo, giao thức `$VEL`/`$ODO` thuộc xe AMR, không dùng cho đồ án.

Mở trong CubeIDE: File → Import → Existing Projects into Workspace → chọn thư mục này.
Nếu workspace còn project `amr_stm32f407` cũ (trỏ vào repo `amr_ws`) thì xoá project đó khỏi
workspace trước (chọn **không** xoá nội dung trên đĩa), rồi mới import.
