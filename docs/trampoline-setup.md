# Trampoline (OSEK) trên Windows: cài đặt và build

Trampoline là hệ điều hành OSEK chạy trên ECU1 (board Hiwonder, STM32F407). Tài liệu này hướng dẫn cài đặt và build. Tất cả chỉ là **build**, chưa nạp vào board.

## 1. Thành phần

| Thành phần | Vị trí | Ghi chú |
|---|---|---|
| Trampoline | **submodule `third_party/trampoline`**, trỏ tới fork `tnguyenmanht-arch/trampoline`, branch **`amr-f407`** | Upstream `ff28702` cộng 2 commit vá (mục 4). Mỗi repo dùng Trampoline (repo này, và sau này `amr_ws`) ghim một commit của fork |
| goil 3.1.16 | `third_party/trampoline/goil/makefile-unix/goil.exe` | Tự build từ source (mục 2). Không commit vào git |
| GCC cho máy tính | WinLibs GCC 16.1, `C:\winlibs\...\mingw64\bin` | goil cần C++17 (MinGW 6.3 cũ không build được) và cần DLL runtime của WinLibs khi chạy |
| GCC cho ARM | GNU Tools for STM32 14.3 (có sẵn trong CubeIDE 2.1.1) | Dùng chung với `f407_bringup` |

## 2. Clone repo lần đầu

```bash
git clone https://github.com/tnguyenmanht-arch/can-ecu-network.git
cd can-ecu-network
git submodule update --init third_party/trampoline
# Trampoline có nhiều submodule nặng (tflite-micro, lwip...): KHÔNG dùng --recursive, chỉ lấy CMSIS_5
git -C third_party/trampoline submodule update --init --depth 1 machines/cortex-m/CMSIS_5
# Build goil (khoảng 2,5 phút), dùng GCC của WinLibs:
PATH="/c/winlibs/<thư mục winlibs>/mingw64/bin:$PATH" python third_party/trampoline/goil/makefile-unix/build.py all
```

Thiếu `CMSIS_5` thì khi build sẽ báo lỗi `core_cm4.h: No such file`.

Gặp `Filename too long` / `Unable to checkout ... in submodule path` trên Windows là do đường dẫn vượt 260 ký tự (Trampoline có file nằm rất sâu). Sửa bằng `git config --global core.longpaths true`, hoặc clone vào thư mục có đường dẫn ngắn. Đã kiểm tra 30/9: clone theo các lệnh trên vào `Documents/` chạy đúng, submodule khoảng 54 MB.

**Sửa Trampoline** (thêm bản vá, port target mới): commit trong `third_party/trampoline` trên branch `amr-f407`, push lên fork, rồi commit con trỏ submodule ở repo này (`git add third_party/trampoline`).

## 3. Build một app

```bash
scripts/trampoline_build.sh tests/target/fpu_check
```

Script đặt PATH, chạy goil, rồi chạy `make.py`. Kết quả là `<app>_exe` (ELF) và `<app>_exe.bin`.
Mỗi app mới cần thêm dòng `<app>/<app>/` vào `.gitignore` cạnh app (thư mục code do goil sinh ra).

## 4. Các bản vá so với upstream (commit trên branch `amr-f407` của fork)

1. **`make.py` dùng `--target=` thay cho `-t=`**: khi make.py gọi goil từ Python, bản goil Windows bỏ qua `-t=` và báo "No target platform given".
2. **Startup stm32f407 bỏ lời gọi `_exit()`**: `script.ld` định nghĩa `_exit = .` (chỉ là một địa chỉ, không phải hàm). ld mới trong toolchain ST 14.3 từ chối lệnh `BL` tới symbol đó. `StartOS()` không bao giờ trả về nên dòng này vốn không chạy tới.
3. **Hard float + lưu ngữ cảnh FPU** (chép cách làm của port `stm32l432`):
   - `-mfloat-abi=hard`.
   - Template `process_specific` sinh `arm_float_context` cho task/ISR có `USEFLOAT = TRUE`.
   - Startup tắt ASPEN/LSPEN, bật CP10/11.

   Lý do: bản upstream của port F407 là **soft-float**. Mô hình xe trong VCU tính float, mà CPU F407 có FPU; bỏ phí FPU thì thời gian thực thi phình ra nhiều lần.

## 5. Quy tắc khi viết app OSEK

- Task nào dùng float (ví dụ mô hình xe) phải khai **`USEFLOAT = TRUE`**. Nếu không, task ưu tiên cao hơn cắt ngang sẽ ghi đè thanh ghi FPU của nó.
- **ISR không được dùng float**, vì đã tắt tự động lưu FPU khi vào ngắt. Mỗi lần thêm code thì kiểm tra lại:
  ```bash
  arm-none-eabi-objdump -d <app>_exe | awk '/^[0-9a-f]+ <.*>:$/{fn=$2} /\tv[a-z]+/{c[fn]++} END{for(f in c) print c[f], f}'
  ```
  Chỉ được phép thấy các hàm task và `tpl_save/load_context*`.
- `fpu_check` (2026-09-30): đạt kiểm tra trên. Lệnh VFP chỉ nằm trong `slow`/`fast` và 4 hàm chuyển ngữ cảnh. Trampoline có ví dụ chính thức làm cùng việc này: `third_party/trampoline/examples/cortex-m/armv7em/stm32l432/Nucleo-32/FPU_test/`.

## 6. Việc tiếp theo cho ECU1

- Thêm board Hiwonder: thay BSP Discovery (LED PE10, buzzer PA8).
- Bỏ StdPeriph. Hướng đang đề xuất là **không ghép HAL vào Trampoline**: driver CAN bxCAN và GPIO viết ở mức thanh ghi, chỉ dùng CMSIS (xem `docs/can-network-architecture.md`, mục 7). Như vậy tránh được các xung đột: header CMSIS trùng nhau, `HAL_GetTick()` khi SysTick đã thuộc về OS, clock cấu hình ở hai nơi.
- ISR2 cho CAN1 RX, Alarm gửi message theo chu kỳ, Resource bảo vệ dữ liệu dùng chung.
