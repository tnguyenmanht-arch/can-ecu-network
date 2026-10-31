# bus2_f407_rx — test bus thật 2 board, phía NHẬN (Hiwonder V1.2, Trampoline)

Mốc 1 bước 2. Đi cặp với [`../bus2_f103_tx`](../bus2_f103_tx/README.md) (F103 gửi 0x080 mỗi 100 ms).

## Mục tiêu

```
F103 gửi 0x080 (byte 0 = số thứ tự) → bus → VP230 → PD0 → FIFO0 → ngắt CAN1_RX0 (IRQ 20)
  → ISR can_rx_isr (loại 2): can_receive = đọc + nhả FIFO0, ActivateTask(Task_CanRx)
  → Task_CanRx: ID 0x080 → g_rx_ok++, so byte 0 với lần trước + 1 (lệch → g_rx_gap++), đảo LED PE10
Alarm_Esr (100 tick) → Task_EsrCopy: g_esr = CAN1->ESR
```

- Khung từ `f407_can_loopback`: giữ `main` (LED, bíp, `DBG_SLEEP`, `FMPIE0`, `NVIC_EnableIRQ`), ISR2 `can_rx_isr`, `Task_CanRx`; bỏ `Task_CanTx` và `Alarm_CanTx`
- CAN1 **chế độ thường**: `Can_Init(0x001A0005U)` = BRP 5 (chia 6), TS1 10 (11 tq), TS2 1 (2 tq), LBKM = 0 → 42 MHz / 6 / 14 = **500 kbit/s**, mẫu 85,7 %
- bxCAN F407 tự **ACK** mọi khung đúng trên bus (ở phần cứng, trước bộ lọc), nên F103 gửi xong được khi F407 đã thoát init
- `g_esr` có vì thanh ghi ngoại vi đọc HOTPLUG lúc IDLE ngủ `wfe` ra rác (`CLAUDE.md` mục 4); task chép ra RAM thì đọc không cần halt

## Build

```bash
scripts/trampoline_build.sh tests/target/bus2_f407_rx
```

Build 30/10: 0 lỗi, chỉ có cảnh báo RWX của linker script Trampoline (như mọi app trước); file app + `bxcan.c` + `can_port_f407.c` biên dịch riêng với `-Wall -Wextra`: 0 cảnh báo. text 7375 / data 2808 / bss 1069; `.bin` 10196 byte.

## Nạp (ST-Link ở H1) — CHỈ khi đã xác nhận

Đấu như `f407_blink`: H1 chân 3 SWCLK, chân 5 GND, chân 7 SWDIO, không nối 3.3V; board cấp điện qua USB-C (cổng download); công tắc 20 OFF, không motor/servo. Đấu bus: xem README bên gửi.

```bash
"C:/ST/STM32CubeIDE_2.1.1/STM32CubeIDE/plugins/com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.win32_2.2.400.202601091506/tools/bin/STM32_Programmer_CLI.exe" \
  -c port=SWD -w tests/target/bus2_f407_rx/bus2_f407_rx_exe.bin 0x08000000 -v -rst
```

## Đọc khi đang chạy

RAM đọc HOTPLUG không cần halt:

```bash
STM32_Programmer_CLI.exe -c port=SWD mode=HOTPLUG -r32 0x20000B00 0x14 -r32 0x200000F0 4
```

Thanh ghi ngoại vi phải halt chớp nhoáng:

```bash
STM32_Programmer_CLI.exe -c port=SWD mode=HOTPLUG -halt -r32 0x40006404 4 -r32 0x4000640C 4 -r32 0x40006414 4 -r32 0x40006418 4 -r32 0x4000641C 4 -r32 0xE000E100 4 -run
```

Địa chỉ (từ `bus2_f407_rx.map`, build 30/10; build lại thì kiểm lại):

| Biến | Địa chỉ | Kích thước | Ý nghĩa |
|---|---|---|---|
| `g_esr` | `0x20000B00` | 4 | bản sao `CAN1->ESR`, cập nhật mỗi 100 ms |
| `g_rx_gap` | `0x20000B04` | 4 | số lần byte 0 ≠ lần trước + 1 |
| `g_rx_ok` | `0x20000B08` | 4 | số khung 0x080 Task_CanRx đã xử lý |
| `g_rx_isr` | `0x20000B0C` | 4 | số lần ISR đọc được khung (mọi ID) |
| `g_init_err` | `0x20000B10` | 1 | byte thấp của word; 0 OK, 1 kẹt vào init, 2 kẹt thoát init. Byte kế: `s_have_last`, `s_last_seq`, `s_rx_dlc` |
| `tpl_time_counter` | `0x200000F0` | 4 | tăng ≈ 1000/s |
| `CAN1->MSR` / `RF0R` / `IER` / `ESR` / `BTR` | `0x40006404` / `0C` / `14` / `18` / `1C` | 4 | cần `-halt`. BTR phải = `0x001A0005` (bit 30 LBKM = 0) |
| `NVIC->ISER0` | `0xE000E100` | 4 | bit 20 = CAN1_RX0 |

Giải mã ESR: xem README bên gửi (REC [31:24], TEC [23:16], LEC [6:4], BOFF/EPVF/EWGF bit 2/1/0).

## Tiêu chí đạt (chung với bus2_f103_tx)

Đọc lần A, chờ 10 s, đọc lần B:

| Biến | Mong đợi |
|---|---|
| `g_init_err` | 0 |
| `g_rx_ok` | +~100 |
| `g_rx_isr` | +~100, bằng `g_rx_ok` (chỉ có 1 node gửi 0x080) |
| `g_rx_gap` | 0 (cộng 1 nếu F103 bị reset giữa chừng: số thứ tự quay về 0) |
| `g_esr` | TEC = REC = 0 |
| LED PE10 | nháy ~5 lần/s (đảo mỗi khung, 10 khung/s) |
| F103 | `g_tx_done` +~100, `g_tx_busy` +0, TEC = REC = 0 (xem README bên gửi) |

## Chẩn đoán

| Triệu chứng | Nghi ở đâu | Kiểm tiếp |
|---|---|---|
| `g_init_err` = 2 | PD0 không thấy 11 bit lặn nên không thoát init | Bus bị giữ trội (CANH chập CANL, dây TX/RX của transceiver nối sai) |
| F103 `g_tx_busy` tăng, `g_tx_done` đứng; ở đây mọi bộ đếm = 0 | **Không ai ACK** → F407 không thấy khung | F407 có chạy không (nghe bíp, `tpl_time_counter` tăng); dây CANH/CANL, GND chung; BTR 2 bên. Hiwonder: CANH/CANL ra ở header H1, đối chiếu board thật |
| F103 đạt (`g_tx_done` tăng) mà `g_rx_isr` = 0 | Khung tới bxCAN (đã ACK) nhưng **ngắt không tới** | `-halt`: `RF0R` FMP0 > 0 → khung nằm trong FIFO mà không ngắt: `IER` bit 1, `ISER0` bit 20. FMP0 = 0 → bộ lọc không cho qua: kiểm `can_filter_accept_all` trong `Can_Init` |
| `g_rx_isr` tăng mà `g_rx_ok` = 0 | Task_CanRx không chạy, hoặc ID khác 0x080 | `ActivateTask` trả lỗi (STATUS = EXTENDED); `s_rx_id` (0x20000B1C) |
| `g_rx_gap` > 0 và tăng dần | Mất khung | ISR ghi đè bộ đệm 1 khung trước khi task kịp chép (ACTIVATION = 1); hoặc FIFO tràn (`RF0R` FOVR0) |
| `g_rx_isr` tăng vọt, `tpl_time_counter` gần như đứng | Bão ngắt: FIFO không được nhả | ISR phải gọi `can_receive` (ghi `RFOM0`) |
| TEC/REC > 0 tăng dần | Lỗi lẻ tẻ trên dây | Trở đầu cuối (≈ 60 Ω giữa CANH–CANL khi tắt nguồn), dây xoắn, tiếp xúc |

## Kết quả 31/10/2026 — ĐẠT

Bus thật 2 node: F103 **board 1** (`bus2_f103_tx`) + SN65HVD230, **bộ dây mới** ↔ Hiwonder V1.2 (`bus2_f407_rx`). Điện trở CANH–CANL đo khi tắt nguồn: **53,8 Ω** (2 trở 120 Ω song song, mong đợi ≈ 60 Ω). Đọc RAM bằng HOTPLUG, không halt; mỗi lần chỉ một ST-Link nên 2 phía đo lệch giờ nhau.

**Phía gửi — F103 board 1** (`-r32 0x20000020 0x18`):

| | Giờ | `g_tx_req` | `g_tx_done` | `g_tx_busy` | `g_tick_ms` | `g_esr` |
|---|---|---|---|---|---|---|
| A | 12:26:10.077 | 600 | 599 | 0 | 60009 | `0x00000000` |
| B | 12:26:20.364 | 702 | 701 | 0 | 70264 | `0x00000000` |
| Hiệu | | **+102** ✅ | **+102** ✅ | **+0** ✅ | +10255 ms | TEC = REC = 0, LEC = 0 ✅ |

`g_init_err` = 0, `g_clk_err` = 0. `g_tx_busy` = 0 từ lúc khởi động: có ACK ngay từ khung đầu.

**Phía nhận — F407** (`-r32 0x20000B00 0x14`):

| | Giờ | `g_rx_ok` | `g_rx_isr` | `g_rx_gap` | `s_last_seq` | `g_esr` |
|---|---|---|---|---|---|---|
| A | 12:29:55.870 | 461 | 461 | 0 | 204 | `0x00000000` |
| B | 12:30:06.180 | 564 | 564 | 0 | 51 | `0x00000000` |
| Hiệu | 10,31 s | **+103** ✅ | **+103** ✅ | **0** ✅ | 204 + 103 = 307 → 307 − 256 = 51 ✅ | TEC = REC = 0, LEC = 0 ✅ |

`g_init_err` = 0. Số thứ tự khớp tuyệt đối (qua cả lúc quay vòng 255 → 0), `g_rx_isr` = `g_rx_ok` (mọi khung là 0x080). **LED PE10 nháy đều** ✅.

## Gỡ lỗi 30–31/10

Lỗi nằm ở phía gửi (dây CAN RX → PB8 của F103 tiếp xúc chập chờn), phía F407 không phải sửa gì. Chi tiết: [`../bus2_f103_tx/README.md`](../bus2_f103_tx/README.md#gỡ-lỗi-3031-10).
