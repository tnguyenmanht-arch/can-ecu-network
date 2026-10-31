# bus2_f103_tx — test bus thật 2 board, phía GỬI (STM32F103)

Mốc 1 bước 2. Đi cặp với [`../bus2_f407_rx`](../bus2_f407_rx/README.md) (Hiwonder nhận bằng ISR2).

## Mục tiêu

- F103 gửi khung **0x080**, 8 byte, mỗi **100 ms**; byte 0 = số thứ tự 0–255 quay vòng, byte 1–7 = 0
- CAN1 **chế độ thường** (LBKM = 0): `Can_Init(0x00050008U)` = BRP 8, TS1 5, TS2 0 → APB1 36 MHz / 9 / 8 tq = **500 kbit/s**, mẫu 87,5 %
- Khác loopback: khung chỉ gửi xong khi **có node khác ACK** (ở đây là bxCAN của F407). Không ai ACK thì bxCAN tự gửi lại mãi, mailbox 0 không trống → `g_tx_busy` tăng
- Driver cũ (`bxcan.h`: `Can_Init`, `can_send`); chưa dùng `Can.h` mới

## Build

```bash
scripts/f103_build.sh tests/target/bus2_f103_tx
```

Ra `build/bus2_f103_tx.elf` / `.hex` / `.bin` / `.map`. Build 30/10: 0 cảnh báo (`-Wall -Wextra`), text 1640 / data 4 / bss 1588.

## Đấu bus

```
F103 PB9 (TX) ─┐ SN65HVD230 ── CANH ════ CANH ── VP230 ── PD1/PD0 Hiwonder (R23 120 Ω)
F103 PB8 (RX) ─┘  (3,3 V)    ── CANL ════ CANL
GND chung giữa 2 board (cùng hub/nguồn)
```

- 2 đầu bus mỗi đầu một trở 120 Ω: R23 trên Hiwonder + trở trên module SN65HVD230 (nếu module có; đo giữa CANH–CANL khi tắt nguồn ≈ 60 Ω là đúng)
- Module SN65HVD230 cấp **3,3 V** từ F103, không cấp 5 V
- Theo `CLAUDE.md` mục 2: cắm/rút dây khi các board tắt; cấp nguồn 2 board từ cùng hub, GND chung

## Nạp — CHỈ khi đã xác nhận

```bash
"C:/ST/STM32CubeIDE_2.1.1/STM32CubeIDE/plugins/com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.win32_2.2.400.202601091506/tools/bin/STM32_Programmer_CLI.exe" \
  -c port=SWD -w tests/target/bus2_f103_tx/build/bus2_f103_tx.elf -v -rst
```

## Đọc khi đang chạy

F103 chạy vòng lặp bận, không ngủ, nên HOTPLUG đọc được cả RAM lẫn thanh ghi:

```bash
STM32_Programmer_CLI.exe -c port=SWD mode=HOTPLUG -r32 0x20000020 0x18 -r32 0x40006408 4 -r32 0x40006418 4
```

Địa chỉ (từ `build/bus2_f103_tx.map`, build 30/10; build lại thì kiểm lại):

| Biến | Địa chỉ | Kích thước | Ý nghĩa |
|---|---|---|---|
| `g_esr` | `0x20000020` | 4 | bản sao `CAN1->ESR`, cập nhật mỗi 100 ms |
| `g_tx_done` | `0x20000024` | 4 | số khung TSR báo TXOK0 (đã có node ACK). Đếm trễ 1 khung: kiểm ở đầu chu kỳ sau |
| `g_tx_busy` | `0x20000028` | 4 | số lần mailbox 0 còn bận |
| `g_tx_req` | `0x2000002C` | 4 | số khung đã nạp vào mailbox |
| `g_init_err` | `0x20000030` | 1 | 0 OK, 1 kẹt vào init, 2 kẹt thoát init |
| `g_clk_err` | `0x20000031` | 1 | 0 OK (72 MHz) |
| `g_tick_ms` | `0x20000034` | 4 | tăng 1000/s |
| `CAN1->TSR` | `0x40006408` | 4 | bit 1 TXOK0, bit 26 TME0 |
| `CAN1->ESR` | `0x40006418` | 4 | xem bảng giải mã ESR dưới |

**Giải mã ESR:** bit [31:24] REC, [23:16] TEC, [6:4] LEC (mã lỗi cuối: 0 không lỗi, 1 stuff, 2 form, **3 ACK**, 4 bit lặn, 5 bit trội, 6 CRC), bit 2 BOFF (bus-off), bit 1 EPVF (error passive, TEC hoặc REC > 127), bit 0 EWGF (cảnh báo, > 95).

⚠️ Chỉ có **một ST-Link**: đọc xong board này mới chuyển sang board kia. Đếm theo hiệu A/B nên không cần đọc 2 board cùng lúc.

## Tiêu chí đạt (chung với bus2_f407_rx)

Đọc lần A, chờ 10 s, đọc lần B:

| Biến | Mong đợi |
|---|---|
| `g_init_err` | 0 |
| `g_tx_done` | +~100 |
| `g_tx_req` | +~100 |
| `g_tx_busy` | +0 |
| `g_esr` | TEC = REC = 0 (`g_esr & 0xFFFF0000` = 0), LEC = 0 |
| F407 | `g_rx_ok` +~100, `g_rx_gap` = 0, TEC = REC = 0 (xem README bên nhận) |

## Chẩn đoán

| Triệu chứng | Nghi ở đâu | Kiểm tiếp |
|---|---|---|
| `g_init_err` = 2 | PB8 (RX) không thấy 11 bit lặn liên tiếp nên không thoát init | Module SN65HVD230 chưa có nguồn 3,3 V; dây RX/TX đảo; bus bị giữ trội (CANH chập CANL) |
| `g_tx_busy` tăng, `g_tx_done` đứng | **Không ai ACK** | ESR: LEC = 3 (ACK), TEC lên ≥ 128 (EPVF = 1). Kiểm: F407 đã chạy và `g_init_err` = 0 chưa; dây CANH/CANL (đảo?), GND chung; nguồn module; bit timing 2 bên (cùng 500 kbit/s: F103 `0x00050008`, F407 `0x001A0005`) |
| `g_tx_done` tăng nhưng TEC/REC > 0 và tăng dần | Có lỗi lẻ tẻ trên đường dây | Trở đầu cuối (đo ≈ 60 Ω), dây không xoắn/quá dài, tiếp xúc lỏng; LEC cho biết loại lỗi |
| ESR BOFF = 1 | Bus-off (TEC > 255) | Lỗi kéo dài; sau khi sửa phải reset F103 (driver chưa bật ABOM) |
| F103 đạt mà F407 `g_rx_ok` = 0 | Phía nhận | Có ACK nghĩa là bxCAN F407 đã thấy khung (ACK làm ở phần cứng, không phụ thuộc bộ lọc) → nghi **ngắt** (FMPIE0, NVIC IRQ 20) hoặc **bộ lọc**. Xem bảng chẩn đoán ở README bên nhận |

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

**Triệu chứng:**

| Quan sát | Số liệu |
|---|---|
| F103: lỗi bit trội rồi bus-off | `g_esr` = `0x00F80057`: LEC = 5 (bit dominant error), BOFF = 1, TEC vượt 255; `g_tx_busy` tăng đều, mailbox 0 kẹt (TSR TME0 = 0, TERR0 = 1) |
| Số khung chạy được trước khi hỏng thay đổi mỗi lần | 41 khung / 5 khung / 0 khung |
| Chạm vào dây là hỏng | — |

**Khoanh vùng:**

| Bước | Làm gì | Kết quả |
|---|---|---|
| 1 | Đọc phía F407 (`-halt`): MSR, RF0R, IER, ESR, NVIC ISER0/ISPR0 | Phía nhận sẵn sàng và rảnh: FIFO0 rỗng, FMPIE0 = 1, IRQ 20 bật, ESR = 0 → lỗi không ở F407 |
| 2 | F103 đứng một mình (tách CANH/CANL khỏi bus) | Vẫn LEC = 5 + bus-off. Node có transceiver tốt đứng một mình chỉ được lỗi ACK (LEC = 3), TEC dừng ~128 → lỗi nằm trong vòng F103 → module → F103 |
| 3 | Dừng CPU, ép PB9 = 0 / 1 bằng ST-Link (ghi CRH, BRR, BSRR), đọc IDR; đo chân D, R của module và CANH–CANL bằng đồng hồ | PB9 đổi 0 ↔ 1 nhưng **PB8 luôn đọc 1** (61 lần đọc trong 30 s) |
| 4 | Đổi sang F103 board 2 + bộ dây mới | Đạt (+102 / +102 / +0, ESR = 0) |
| 5 | Quay lại board 1 + bộ dây mới | Đạt, giống hệt board 2 → board 1 không hỏng |

**Thủ phạm:** đầu dupont cũ của dây CAN RX → PB8 bị loe, tiếp xúc chập chờn.

**Bài học:**
- bxCAN **cần đường RX để gửi**: mỗi bit phát ra đều được đọc lại để so. RX đứt thì node tự báo lỗi bit (LEC = 5) dù TX vẫn tốt
- Driver hiện **chưa bật ABOM** (tự thoát bus-off), nên chỉ một đợt nhiễu đủ dài là node im lặng vĩnh viễn tới khi reset. Sẽ bật ABOM ở GĐ 5
- **Thay dây trước khi nghi board.** Dây dupont cũ, đầu loe là nghi phạm đầu tiên khi lỗi chập chờn và số khung chạy được thay đổi mỗi lần
