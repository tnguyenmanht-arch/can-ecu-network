# f407_can_loopback — CAN1 loopback bằng ISR loại 2 trên Hiwonder V1.2 (ECU1)

## Mục tiêu

Kiểm tra trọn chuỗi "gửi → ngắt nhận → task xử lý" trên Trampoline, trước khi ra bus thật:

```
Alarm_CanTx (100 tick) → Task_CanTx: can_send(0x080, 8 byte)
  → [bxCAN loopback, LBKM = 1] → FIFO0 có khung → ngắt CAN1_RX0 (IRQ 20, ô vector 36)
  → ISR can_rx_isr (loại 2): can_receive = đọc + nhả FIFO0, ActivateTask(Task_CanRx)
  → Task_CanRx: chép bộ đệm trong Suspend/ResumeOSInterrupts, kiểm id/dlc/data[1], đảo LED PE10
```

- CAN1: 500 kbit/s, APB1 = 42 MHz, `Can_Init(0x401A0005U)`: BRP = 5 (chia 6), TS1 = 10 (11 tq), TS2 = 1 (2 tq), LBKM = 1 → 14 tq, sample point 85,7 %
- Driver chung `drivers/bxcan/bxcan.c` + phần riêng chip `drivers/mcu/stm32f407/can_port_f407.c` (PD0 = RX, PD1 = TX, AF9). Chọn chip bằng include path `-I drivers/mcu/stm32f407`
- `main` giữ phần của `f407_blink`: LED PE10, bíp PA8 trước `StartOS`, `DBG_SLEEP` để ST-Link đọc được khi IDLE ngủ `wfe`

## Build

```bash
scripts/trampoline_build.sh tests/target/f407_can_loopback
```

⚠️ `build.py` của Trampoline đặt file `.o` tại `build/<đường dẫn APP_SRC>.o`. Với `APP_SRC = "../../../drivers/..."` thì object của `bxcan.c` và `can_port_f407.c` bị ghi ra **`tests/drivers/...`** (ngoài `build/`). Không commit thư mục đó; xoá được bất cứ lúc nào.

## Nạp (ST-Link ở H1) — CHỈ khi đã xác nhận

Đấu như `f407_blink`: H1 chân 3 SWCLK, chân 5 GND, chân 7 SWDIO, không nối 3.3V; board cấp điện qua USB-C (cổng download); công tắc 20 OFF, không motor/servo.

```bash
"C:/ST/STM32CubeIDE_2.1.1/STM32CubeIDE/plugins/com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.win32_2.2.400.202601091506/tools/bin/STM32_Programmer_CLI.exe" \
  -c port=SWD -w tests/target/f407_can_loopback/f407_can_loopback_exe.bin 0x08000000 -v -rst
```

Ở chế độ loopback, khung vẫn được đẩy ra chân TX (PD1 → VP230 → CANH/CANL), nhưng không cần node nào ACK.

## Đọc khi đang chạy

Biến RAM đọc HOTPLUG không cần halt (xem `CLAUDE.md` mục 4); thanh ghi ngoại vi (CAN1, GPIO) phải `-halt ... -run`.

```bash
STM32_Programmer_CLI.exe -c port=SWD mode=HOTPLUG -r32 0x20000B00 0x18
```

Địa chỉ (build 23/10, `arm-none-eabi-nm f407_can_loopback_exe`; build lại thì kiểm lại):

| Biến | Địa chỉ | Kích thước |
|---|---|---|
| `g_rx_mismatch` | `0x20000B00` | 4 |
| `g_rx_ok` | `0x20000B04` | 4 |
| `g_rx_isr` | `0x20000B08` | 4 |
| `g_tx_busy` | `0x20000B0C` | 4 |
| `g_tx_ok` | `0x20000B10` | 4 |
| `g_init_err` | `0x20000B14` | 1 (byte thấp của word) |
| `tpl_time_counter` | `0x200000F0` | 4, tăng ≈ 1000/s |
| `SystemCoreClock` | `0x20000000` | 4, = 168000000 |

Thanh ghi CAN1 hữu ích (cần `-halt`): `CAN1->MSR` `0x40006404`, `CAN1->TSR` `0x40006408`, `CAN1->RF0R` `0x4000640C`, `CAN1->IER` `0x40006414` (bit 1 = FMPIE0), `CAN1->ESR` `0x40006418`, `CAN1->BTR` `0x4000641C` (= `0x401A0005`). NVIC: `ISER0` `0xE000E100` (bit 20 = CAN1_RX0).

## Tiêu chí đạt

Đọc lần A, chờ 10 s, đọc lần B:

| Biến | Mong đợi |
|---|---|
| `g_init_err` | 0 |
| `g_tx_ok` | +100 (≈ 10 khung/s) |
| `g_rx_isr` | +100, bằng `g_tx_ok` (sai lệch ≤ 1 do thời điểm đọc) |
| `g_rx_ok` | +100, bằng `g_rx_isr` |
| `g_rx_mismatch` | 0 |
| `g_tx_busy` | 0 |
| LED PE10 | nháy ~5 lần/s (đảo mỗi khung, 10 khung/s) |

## Chẩn đoán theo mắt xích

| Triệu chứng | Mắt xích hỏng | Kiểm tiếp |
|---|---|---|
| `g_tx_ok`, `g_rx_isr`, `g_rx_ok` đều = 0 | OS / alarm / `Can_Init` | `g_init_err` (1 = kẹt vào init, 2 = kẹt thoát init); `tpl_time_counter` có tăng không; nghe bíp chưa (main chạy tới `StartOS`) |
| `g_tx_ok` tăng, `g_rx_isr` = 0 | Ngắt không tới ISR | `CAN1->RF0R` (`-halt`): FMP0 > 0 nghĩa là khung đã vào FIFO nhưng không có ngắt → kiểm `CAN1->IER` bit 1 và `NVIC->ISER0` bit 20. FMP0 = 0 → khung không về: kiểm `CAN1->BTR` bit 30 (LBKM), `CAN1->ESR` |
| `g_rx_isr` tăng, `g_rx_ok` = 0 và `g_rx_mismatch` = 0 | ISR không kích được Task_CanRx | `ActivateTask` trả lỗi (STATUS = EXTENDED); tên task trong `DeclareTask` và OIL |
| `g_rx_isr` tăng, `g_rx_mismatch` tăng | Khung sai | So `s_rx_id`, `s_rx_dlc`, `s_rx_data` với khung gửi (0x080, 8 byte, data[1] = 0xAA) |
| `g_rx_isr` tăng vọt (hàng nghìn/s), `g_tx_ok` đứng | Bão ngắt: FIFO không được nhả | ISR phải gọi `can_receive` (ghi `RFOM0`); xem `CAN1->RF0R` |
| `g_tx_busy` tăng | Mailbox 0 không trống | Khung trước chưa gửi xong: `CAN1->TSR`, `CAN1->ESR` (lỗi bus, bus-off) |

## ⚠️ Dự đoán trước khi nạp (phân tích ELF 23/10, chưa kiểm trên board)

Trampoline target `stm32f407` **không tự bật NVIC cho nguồn ISR2 không phải EXTI** lúc khởi động:

- `tpl_init_machine` chỉ gọi `tpl_init_it_priority` (đặt priority IRQ 20 = 0xF0), **không** ghi `NVIC->ISER`
- Lệnh ghi `NVIC->ISER[0] = 0x100000` (bit 20) chỉ có trong `tpl_enable_os_interrupts`, mà hàm này chỉ được gọi từ `tpl_resume_os_interrupts_service`, và chỉ khi trước đó đã có `SuspendOSInterrupts` (bộ đếm khoá ≠ 0)
- Ví dụ `readbutton_isr` chạy được vì nguồn EXTI có template riêng gọi `NVIC_EnableIRQ`

→ Dự kiến kẹt ở dòng "`g_tx_ok` tăng, `g_rx_isr` = 0", FIFO0 đầy 3 khung (FMP0 = 3, rồi FOVR0). Cần bật IRQ 20 trước `StartOS` (quyết định cách sửa trước khi nạp).
