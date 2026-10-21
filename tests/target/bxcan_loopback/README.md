# bxcan_loopback — test driver bxCAN trên STM32F103 (GĐ 4)

Chương trình gửi khung `Pedal` (ID 0x080, 8 byte) mỗi 100 ms qua mailbox 0. bxCAN ở chế độ
**loopback** (LBKM = 1) nên tự nhận lại khung đó vào FIFO0. Chương trình so khung nhận với khung
gửi rồi tăng bộ đếm. Không cần transceiver hay node thứ hai.

Đầu `main`, `clock_init()` (trong `drivers/mcu/stm32f103/clock_f103.c`) đưa chip lên 72 MHz từ thạch anh
8 MHz: **APB1 = 36 MHz, bus 500 kbit/s** đúng với BTR trong `Can_Init`.

## Build

```bash
scripts/f103_build.sh tests/target/bxcan_loopback          # build
scripts/f103_build.sh tests/target/bxcan_loopback clean    # xoá build/
```

Kết quả nằm trong `build/`: `bxcan_loopback.elf`, `.hex`, `.bin`, `.map`. Script dùng
`arm-none-eabi-gcc` và `make` đi kèm STM32CubeIDE.

**Script chỉ build, KHÔNG tự nạp.** Việc nạp board do người làm đồ án tự làm (ST-Link + CubeIDE
hoặc `STM32_Programmer_CLI`).

## Kiểm tra phần cứng trước khi nạp

- **Thạch anh HSE:** chữ in trên thạch anh phải là **8.000 MHz**, khớp `HSE_VALUE = 8000000`
  (`third_party/cmsis_f1/device/Source/system_stm32f1xx.c`) và hệ số PLL ×9 trong `clock_init()`.
  Thạch anh khác 8 MHz thì SYSCLK, `SystemCoreClock`, SysTick và tốc độ bus CAN đều sai.
- **Mã chip:** đọc chữ in trên chip là **STM32F103C8** hay **STM32F103CB**. Linker script đặt
  64 KB Flash nên chạy được cả hai; ghi lại mã thật vào tài liệu.

## Tiêu chí đạt

Chạy ít nhất 10 giây, đọc các biến bằng Live Expressions trong CubeIDE:

| Biến | Giá trị mong đợi sau 10 s |
|---|---|
| `g_clk_err` | 0 |
| `SystemCoreClock` | 72000000 |
| `RCC->CFGR`, trường SWS (bit 3:2) | `10` = nguồn SYSCLK là PLL (đọc `RCC->CFGR & 0xC` = `0x8`) |
| `g_init_err` | 0 |
| `g_tx_ok` | ≈ 100 |
| `g_rx_ok` | ≈ 100 |
| `g_tx_busy` | 0 |
| `g_rx_timeout` | 0 |
| `g_rx_mismatch` | 0 |

**Live Expressions:** `RCC` là macro C nên có thể không đọc được `RCC->CFGR` trong CubeIDE. Dùng
`*(unsigned int*)0x40021004` (địa chỉ của `RCC->CFGR`); kiểm SWS bằng `*(unsigned int*)0x40021004 & 0xC`, phải ra `0x8`.
Cách khác: cửa sổ SFRs → RCC → CFGR → SWS.

## Nạp và đọc kết quả bằng CLI

Chỉ nạp khi người làm đồ án đã xác nhận. CLI đi kèm CubeIDE:
`C:\ST\STM32CubeIDE_2.1.1\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.win32_2.2.400.202601091506\tools\bin\STM32_Programmer_CLI.exe`

```bash
# Nạp: chỉ ghi đúng file ELF (xoá các trang Flash nó chiếm), verify, rồi reset.
# KHÔNG xoá toàn chip (-e), KHÔNG ghi option byte (-ob).
STM32_Programmer_CLI -c port=SWD -w tests/target/bxcan_loopback/build/bxcan_loopback.elf -v -rst

# Đọc khi chip đang chạy: BẮT BUỘC mode=HOTPLUG (mode mặc định dừng core, đọc ra số sai)
STM32_Programmer_CLI -c port=SWD mode=HOTPLUG -r32 0x20000000 0x4 -r32 0x20000020 0x1C -r32 0x40021004 0x4
```

Địa chỉ của bản build ngày 21/10/2026. **Sửa code thì địa chỉ có thể đổi**: lấy lại bằng
`arm-none-eabi-nm -S build/bxcan_loopback.elf | grep -E " g_|SystemCoreClock"`.

| Địa chỉ | Biến |
|---|---|
| 0x20000000 | `SystemCoreClock` |
| 0x20000020 | `g_rx_timeout` |
| 0x20000024 | `g_rx_mismatch` |
| 0x20000028 | `g_rx_ok` |
| 0x2000002C | `g_tx_busy` |
| 0x20000030 | `g_tx_ok` |
| 0x20000034 | byte thấp = `g_init_err`, byte kế = `g_clk_err` (hai biến 1 byte chung một word) |
| 0x20000038 | `g_tick_ms` |
| 0x40021004 | `RCC->CFGR` |

Tốc độ = (`g_tx_ok` lần B − lần A) / thời gian thực giữa hai lần đọc; phải ≈ 10 khung/s.

## Kết quả trên board (21/10/2026)

Board Yahboom YB-MSV01 V1.0, chip **STM32F103C8 chính hãng** (CPUID `0x411FC231` = Cortex-M3 r1p1, Device ID
0x410, Flash 64 KB), ST-Link V2 clone (FW V2J46S7), nạp qua SWD. Reset lúc 15:22:11.

| Biến | Lần A (15:22:37, sau reset 26 s) | Lần B (15:22:47, sau A 10,06 s) | Mong đợi | Đạt? |
|---|---|---|---|---|
| `g_clk_err` | 0 | 0 | 0 | ✅ |
| `g_init_err` | 0 | 0 | 0 | ✅ |
| `SystemCoreClock` | 72000000 | 72000000 | 72000000 | ✅ |
| `RCC->CFGR` | 0x001D040A | 0x001D040A | SWS (& 0xC) = 0x8 | ✅ (0x8) |
| `g_tx_ok` | 261 | 362 | tăng ≈ 10/s | ✅ +101 → 10,04 /s |
| `g_rx_ok` | 261 | 362 | bằng `g_tx_ok` | ✅ |
| `g_tx_busy` | 0 | 0 | 0 | ✅ |
| `g_rx_timeout` | 0 | 0 | 0 | ✅ |
| `g_rx_mismatch` | 0 | 0 | 0 | ✅ |
| `g_tick_ms` | 26174 | 36231 | tăng ≈ 1000/s | ✅ 999,98 /s |

`RCC->CFGR = 0x001D040A` giải mã: SW = SWS = PLL, AHB ÷1, APB1 ÷2, APB2 ÷1, nguồn PLL = HSE không chia,
hệ số ×9. `g_tick_ms` đi khớp đồng hồ máy tính (999,98 /s) nên SysTick và clock 72 MHz đúng.

Lần A đọc ở giây 26 thay vì giây 10 vì chuẩn bị script đọc chậm; không ảnh hưởng tới phép tính tốc độ.

## Bảng chẩn đoán

| Triệu chứng | Nghi ngờ |
|---|---|
| `g_clk_err` = 1 | HSE không lên: kiểm thạch anh 8 MHz trên board. Chip vẫn chạy ở HSI 8 MHz |
| `g_clk_err` = 2 hoặc 3 | Kiểm cấu hình PLL (nguồn, hệ số ×9) và Flash latency. Chip vẫn chạy ở HSI 8 MHz |
| `g_init_err` = 1 | INAK không lên: quên CAN1EN hoặc chưa xoá SLEEP |
| `g_init_err` = 2 | Kẹt khi thoát init: chân PB8 / remap sai |
| `tx_ok` tăng, `rx_timeout` = `tx_ok` | Khung không vào FIFO0: filter (FACT0, FINIT chưa xoá, gán FIFO1) |
| `rx_ok` = 1, `rx_mismatch` tăng | Đọc lại khung cũ: quên nhả FIFO (RFOM0) |
| Mọi bộ đếm = 0, `g_init_err` = 0 | SysTick không chạy (SysTick_Config sai) |

Ghi chú cho dòng `g_init_err` = 2: ở chế độ loopback, bxCAN nối TX vào RX ngay bên trong chip, nên
chân PB8 ít ảnh hưởng tới việc thoát init. Nguyên nhân "chân PB8 / remap sai" chủ yếu đúng khi ra bus thật
(bỏ LBKM).
