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
