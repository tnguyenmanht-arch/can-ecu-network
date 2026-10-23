# f407_blink — app Trampoline đầu tiên trên Hiwonder V1.2 (ECU1)

## Mục tiêu

Kiểm tra chuỗi Trampoline (goil → build → nạp) chạy đúng trên board thật, trước khi viết task CAN:

- Clock 168 MHz từ HSE 16 MHz (target `stm32f407` đã sửa `HSE_VALUE`, submodule commit `1c7e434`)
- SysTick của Trampoline = 1 tick/ms (`SystemCounter`)
- Alarm `blink_al` (ALARMTIME 500, CYCLETIME 500) kích task `blink` mỗi 500 ms
- Task `blink` đảo LED PE10 (sáng khi mức THẤP) và tăng `g_blink_count`
- `main` bíp buzzer PA8 một lần trước `StartOS` ("main đã chạy tới đây")

## Build

```bash
scripts/trampoline_build.sh tests/target/f407_blink
```

Kết quả: `f407_blink_exe` (ELF, không có đuôi) và `f407_blink_exe.bin`. Cảnh báo duy nhất `LOAD segment with RWX permissions` đến từ linker script của Trampoline, bỏ qua được.

⚠️ Header CMSIS của target là bản cũ (StdPeriph V1.6.0): GPIO **không có `BSRR`**, mà tách `BSRRL` (ghi 1 → chân lên 1) và `BSRRH` (ghi 1 → chân về 0), mỗi nửa 16 bit. Viết `BSRRH = (1U << pin)`, **không** cộng 16.

## Nạp (ST-Link ở H1) — CHỈ khi đã xác nhận

Đấu: H1 chân 3 SWCLK, chân 5 GND, chân 7 SWDIO, **không** nối 3.3V. Board **phải** được cấp điện qua USB-C (cổng download). Công tắc 20 OFF, không motor/servo.

`STM32_Programmer_CLI` nhận dạng file theo đuôi, nên nạp file `.bin` kèm địa chỉ:

```bash
"C:/ST/STM32CubeIDE_2.1.1/STM32CubeIDE/plugins/com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.win32_2.2.400.202601091506/tools/bin/STM32_Programmer_CLI.exe" \
  -c port=SWD -w tests/target/f407_blink/f407_blink_exe.bin 0x08000000 -v -rst
```

## Đọc khi đang chạy

Task IDLE của Trampoline ngủ bằng `wfe` gần như suốt thời gian, nên:

1. `main` phải có `DBGMCU->CR |= DBGMCU_CR_DBG_SLEEP;` trước `StartOS`. Thiếu dòng này, HOTPLUG báo `Unable to get core ID`
2. **RAM** đọc HOTPLUG bình thường, không làm dừng CPU:
   ```bash
   STM32_Programmer_CLI.exe -c port=SWD mode=HOTPLUG -r32 0x20000600 4 -r32 0x20000094 4
   ```
3. **Ngoại vi** (GPIO, RCC...) đọc HOTPLUG khi CPU đang ngủ ra **rác** mà CLI không báo lỗi (mọi thanh ghi cùng ra `0x080013E5`, địa chỉ hàm `CallTerminateTask`). Phải halt chớp nhoáng (~0,1 s) rồi cho chạy lại:
   ```bash
   STM32_Programmer_CLI.exe -c port=SWD mode=HOTPLUG -halt -r32 0x40021014 4 -run
   ```

Địa chỉ (lấy bằng `arm-none-eabi-nm f407_blink_exe`; build lại thì kiểm lại):

| Biến / thanh ghi | Địa chỉ | Mong đợi |
|---|---|---|
| `SystemCoreClock` | `0x20000000` | 168000000 |
| `tpl_time_counter` (tăng mỗi SysTick) | `0x20000094` | ≈ 1000/s |
| `g_blink_count` | `0x20000600` | ≈ 2/s, = ⌊tick / 500⌋ |
| `DBGMCU->CR` | `0xE0042004` | bit 0 = 1 |
| `GPIOE->MODER` (cần `-halt`) | `0x40021000` | bit 21:20 = 01 |
| `RCC->AHB1ENR` (cần `-halt`) | `0x40023830` | bit 0 và bit 4 = 1 |
| `GPIOE->ODR` (cần `-halt`) | `0x40021014` | bit 10 đổi mỗi 500 ms |

## Kết quả 23/10/2026 — ĐẠT

| Phép đo | Số liệu | Mong đợi | |
|---|---|---|---|
| Nạp | Device ID 0x413, `Download verified successfully` | | ✅ |
| `SystemCoreClock` | 168000000 | 168000000 | ✅ |
| `DBGMCU->CR` | 0x00000001 | bit 0 = 1 | ✅ |
| Tốc độ tick | 11:53:51.416 tick 8660 → 11:53:56.472 tick 13706: 5046 tick / 5,051 s = **999,1 tick/s**. Lần sau: 999,5 tick/s | ≈ 1000/s | ✅ |
| `g_blink_count` | 12:01:58.978 = 25 (tick 12682) → 12:02:04.014 = 35 (tick 17719): **+10 trong 5,039 s = 1,98 lần/s** | ≈ 10 | ✅ |
| Alarm khớp tick | 25 = ⌊12682/500⌋, 35 = ⌊17719/500⌋ | ALARMTIME 500, CYCLETIME 500 | ✅ |
| MODER / AHB1ENR (`-halt`) | 0x00100000 / 0x00100011 | bit 21:20 = 01 / bit 0, 4 = 1 | ✅ |
| ODR bit 10 (`-halt`, 4 lần cách ~300 ms) | 0 → 1 → 1 → 0 | đổi mỗi 500 ms | ✅ |

Sai số tick −0,1 % nằm trong sai số đo (mỗi lần gọi CLI mất ~0,1 s, lấy mốc giữa lần gọi).
