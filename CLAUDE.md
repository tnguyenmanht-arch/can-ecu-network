# CLAUDE.md — can-ecu-network (ĐAKS) | HUST Automotive Engineering

> Đồ án tốt nghiệp — Chương trình Kỹ sư chuyên sâu Ô tô số (Automotive Digital Engineering)
> Đại học Bách Khoa Hà Nội (HUST) · Việt Nam

---

## 1. Tổng quan

**Mô phỏng mạng ECU ô tô trên bàn qua CAN**, phần mềm chia tầng theo kiểu AUTOSAR. Hướng đồ án chốt với GVHD ngày 30/9/2026.

- **ECU1 VCU**: board Hiwonder ROS Robot Controller (STM32F407VET6) chạy **Trampoline (OSEK)**. Nhận ga/phanh/số, chạy mô hình xe đơn giản, gửi tốc độ, vòng quay mô-tơ, % pin. Buzzer PA8 kêu khi cửa mở lúc đang chạy. Giám sát timeout các ECU khác
- **ECU2 Pedal**, **ECU3 Body**: STM32F103C8T6 + SN65HVD230, bare-metal HAL
- **Jetson Orin Nano**: gateway/cockpit, SocketCAN qua header J17 + SN65HVD230. Giải mã theo DBC, gửi sang Android **theo tên tín hiệu**
- **Android HMI**: app Kotlin, thuộc **ĐACN**, repo riêng, chưa làm

Chi tiết kiến trúc, nguyên tắc module, dải ID, CAN matrix, cách đấu bus, cấu trúc repo: **`docs/can-network-architecture.md`**. Hợp đồng duy nhất giữa các ECU: **`can/vehicle.dbc`**.

**Không thuộc repo này:** xe AMR (ROS2, SLAM, Nav2, firmware F411 + DRV8871) nằm ở repo **`amr_ws`**. Xe là dự án cá nhân, không thuộc đồ án nào. Repo này tách ra từ `amr_ws` ngày 30/9/2026 (commit nguồn `b8f7731`).

---

## 2. Quy tắc làm việc cho Claude (bắt buộc)

### Chế độ học
- ⭐ **User là người mới, muốn TỰ học và TỰ làm để báo cáo được.** Thứ tự: đọc → hiểu → thiết kế (Claude giảng) → code (user viết, Claude review) → test
- **Claude KHÔNG tự viết code hay tự chuyển sang bước sau khi user chưa yêu cầu.** Mỗi đầu việc có một mức: **Tự làm** (Claude chỉ gợi ý) / **Làm cùng** (Claude giải thích hoặc viết khung, user viết phần chính) / **Claude làm, user giải thích lại** (chỉ cho phần hạ tầng ít giá trị học)
- Giải thích cho người mới: định nghĩa thuật ngữ, ví dụ cụ thể từ chính dự án
- **Sổ tay làm việc (Claude Docs):** https://claude.ai/code/artifact/b8999f1f-6937-4e05-b481-7cafc3363337. Có 3 tab: Lộ trình, Thuật ngữ & ghi chú (có mục "Hỏi đáp với Claude"), Nhật ký. Viết lại theo hướng mạng CAN ngày 1/10/2026: **10 giai đoạn (GĐ 0–9), 2 mốc go/no-go**; câu hỏi tự kiểm tra đánh số theo GĐ (0.1, 1.1...)

### An toàn
- **Mọi bước nạp code, hoặc làm cơ cấu chấp hành hoạt động, phải có xác nhận của user ở lượt ngay trước đó.** Không gộp "chuẩn bị" và "chạy" vào một tin nhắn
- Chỉ **MỘT** đường nối giữa board và **một** máy tại một thời điểm. Cắm/rút cáp khi board tắt. Laptop rút sạc khi nối vào board hở. Cấp nguồn các node CAN từ cùng một nguồn/hub, GND chung (bài học từ các lần hỏng board ở dự án xe)

### Quy ước code
- Comment tiếng Việt cho logic quan trọng. Comment trong file DBC viết **không dấu** (công cụ DBC đọc theo cp1252)
- STM32: prefix `APP_` cho tầng ứng dụng; driver CAN đặt tên theo module Can của AUTOSAR (`Can_Init`, `Can_Write`, `Can_IsrRx`...). BSW theo tên AUTOSAR (`Com_`, `CanIf_`, `Rte_`, `SchM_`)
- COM và CanIf là **C thuần, KHÔNG gọi HAL**, dùng chung F407 và F103. Chỉ driver phụ thuộc chip
- File trong `gen/` do comgen sinh ra, **commit vào git, KHÔNG sửa tay**

### Git
- **Chỉ merge vào `main` khi Thắng nói rõ "merge".** Các câu như "đã kiểm tra xong", "đạt" không tính là đồng ý merge

---

## 3. Trạng thái (cập nhật thủ công)

**23/10/2026 — GĐ 2 xong (22/10); GĐ 4 (driver bxCAN loopback) xong trên F103 Yahboom (21/10); bring-up Hiwonder V1.2 xong (22/10); CAN1 loopback qua ISR loại 2 trên Trampoline F407 đạt (23/10).**
- ✅ GĐ 4: driver bxCAN + `clock_init` 72 MHz chạy đạt ở chế độ loopback trên board Yahboom YB-MSV01 (STM32F103C8 chính hãng), 21/10. Kết quả và lệnh nạp/đọc: `tests/target/bxcan_loopback/README.md`
- ✅ GĐ 2 (hiểu Trampoline qua code) xong, 22/10
- ✅ **DBC 1.0** (`can/vehicle.dbc`, chốt 30/10): 7 message. Bản 0.2 (5/10) đã có khoá cửa trung tâm `LockStatus` 0x310 (ECU3 → Jetson), `LockCommand` 0x500 (Jetson → ECU3, gửi theo sự kiện; gateway chỉ được gửi dải 0x500–0x5FF) và `GearActual` trong 0x100; bản 1.0 thêm **`VcuStatus` 0x110** (ECU1, 100 ms: `IgnitionOn`, `ReadyState`, `PedalLost`, `BodyLost`, `DoorWarnLevel`) và chốt comment. Mọi thay đổi sau phải tăng phiên bản và báo ĐACN. Kiểm bằng cantools `strict=True`: hợp lệ. Tải bus lý thuyết ở 500 kbit/s: **3,6 %** theo chu kỳ (132 khung/s), **5,7 %** xấu nhất (`BodyStatus`, `LockStatus` gửi theo sự kiện tối đa 50 khung/s)
- ✅ Tài liệu kiến trúc; cấu trúc repo và 4 quyết định thiết kế **đã chốt** (mục 7 của tài liệu kiến trúc): repo riêng + Trampoline qua fork/submodule; commit `gen/`; RTE sinh thẳng từ DBC; một driver bxCAN mức thanh ghi dùng chung
- ✅ Trampoline + goil build được trên Windows, đã vá hard-float + `USEFLOAT`; `tests/target/fpu_check` build đạt (chưa nạp). Xem `docs/trampoline-setup.md`
- ✅ `tests/target/f407_blink` (23/10): app Trampoline đầu tiên cho Hiwonder V1.2, task `blink` đảo LED PE10 theo alarm 500 tick, bíp 1 lần trước `StartOS`. **Nạp và chạy đạt 23/10** (ST-Link): 999 tick/s, `g_blink_count` +10 trong 5 s, ODR bit 10 đổi đúng. Kết quả và cách đọc: `tests/target/f407_blink/README.md`. Header CMSIS của target là bản cũ (StdPeriph V1.6.0): GPIO không có `BSRR` mà tách `BSRRL` (set) / `BSRRH` (reset), mỗi nửa 16 bit
- ✅ Driver bxCAN tách phần chung / phần riêng chip (23/10): `drivers/bxcan/bxcan.c` chỉ chạm thanh ghi `CAN1`, `Can_Init(uint32_t btr)` gọi `can_port_init()` ở `drivers/mcu/<chip>/can_port_<chip>.c`; chọn chip bằng include path (`-I drivers/mcu/<chip>`, mỗi thư mục có `can_port.h` riêng). F103: PB8/PB9 remap, BTR `0x40050008`; F407: PD0/PD1 AF9, BTR `0x401A0005` (APB1 42 MHz, 14 tq, 85,7 %). Tên thanh ghi CAN giống hệt giữa CMSIS F1 và header F4 cũ
- ✅ `tests/target/f407_can_loopback` (23/10): Alarm 100 ms → `Task_CanTx` gửi 0x080 → loopback → ISR loại 2 `can_rx_isr` (`SOURCE = CAN1_RX0_IRQ`) → `ActivateTask(Task_CanRx)` → kiểm khung, đảo LED. **Nạp và chạy đạt 23/10**: +100 khung/10 s ở cả tx / rx_isr / rx_ok, 0 sai, 0 bận, ESR = 0. Kết quả: `tests/target/f407_can_loopback/README.md`. ⚠️ **Trampoline `stm32f407` KHÔNG tự bật NVIC cho nguồn ISR2 không phải EXTI** (chỉ đặt priority; `ISER` chỉ được ghi trong `ResumeOSInterrupts`) → app phải gọi `NVIC_EnableIRQ(<IRQn>)` trước `StartOS`. Nguồn ngắt CAN không có `ACK` → ISR phải tự nhả FIFO
- ✅ `f407_bringup/`: firmware kiểm tra board Hiwonder khi hàng về (port từ xe, giữ K1 khi reset để vào chế độ bring-up). **Tên project CubeIDE vẫn là `amr_stm32f407`.** Checklist: `docs/bringup-f407.md`
- ✅ Board Hiwonder đã về (silkscreen **V1.2**), 22/10. Bring-up xong 22/10 (`f407_bringup` bản HSE 16 MHz, nạp qua cổng serial): clock, LED, buzzer, K1/K2, PD3, I2C2 (QMI8658 @ 0x6A) đạt; USART3 (cổng số 4) chưa thử. Kết quả: `docs/bringup-f407.md`
- ✅ **ST-Link hoạt động lại với Hiwonder** (23/10). Đấu ở H1: chân 3 SWCLK, chân 5 GND, chân 7 SWDIO, không nối 3.3V; board cấp điện qua USB-C cổng download. `STM32_Programmer_CLI -c port=SWD mode=HOTPLUG`: ST-LINK FW V2J46S7, Device ID 0x413, Flash 512 KB. Đọc khi `f407_bringup` đang chạy: `RCC->PLLCFGR` = 0x07405410 (PLLM 16, N 336, P ÷2, nguồn HSE, Q 7), `RCC->CFGR` = 0x0000940A (SWS = PLL, AHB ÷1, APB1 ÷4, APB2 ÷2) → **SYSCLK 168 MHz, APB1 42 MHz, APB2 84 MHz**. Lần thử đầu 23/10 báo `No STM32 target found!` do **board chưa cấp nguồn (quên cắm USB-C)**: ST-Link không cấp điện cho chip vì không nối chân 3.3V; 22/10 laptop không thấy ST-Link ("No debug probe detected")
- ✅ **Mốc 1 bước 2 đạt 31/10: bus thật F103 ↔ F407.** `tests/target/bus2_f103_tx` (F103 gửi 0x080 mỗi 100 ms, chế độ thường) + `tests/target/bus2_f407_rx` (Hiwonder nhận bằng ISR2). Trong ~10 s: F103 +102 khung có ACK, 0 bận; F407 +103 khung, không mất khung; ESR 2 phía = 0; CANH–CANL 53,8 Ω. Gỡ lỗi 30–31/10: đầu dupont dây CAN RX → PB8 bị loe gây LEC = 5 + bus-off; driver chưa bật ABOM (bật ở GĐ 5). Chi tiết: `tests/target/bus2_f103_tx/README.md`
- ⏳ Cần mua: dây xoắn đôi. SN65HVD230 đã về (31/10). Hiwonder có sẵn VP230 + R23 120 Ω nên ECU1 nằm ở một đầu bus, không cần SN65HVD230 cho ECU1
- 📝 Đề cương mới (Claude Doc, 1/10, chờ GVHD duyệt): https://claude.ai/code/artifact/af6e2b14-e170-4eb3-9d70-5dee7526f16d. Bản cũ 28/9 (`reference/ĐA/DAKS...pdf`) viết theo xe AMR, không dùng nữa. Đề xuất ĐACN (1/10): `reference/ĐA/Đề xuất ĐACN — HMI cụm đồng hồ trên Android.pdf`, chốt gói JSON Jetson → app qua WebSocket và ngưỡng cửa mở 3 km/h dùng chung với buzzer VCU
- Tài liệu lý thuyết core: chuẩn OSEK OS 2.2.3 (`reference/osek/os223.pdf`), Trampoline Handbook (`third_party/trampoline/documentation/manual/main.pdf`), 2 bảng tra `documentation/qrdc/`. **Từ hướng mới, COM nằm trong phạm vi**: `reference/osek/OSEKCOM303.pdf` (OSEK COM) và chuẩn AUTOSAR SWS COM

### Quy tắc OSEK (Trampoline)
- Task dùng float phải khai `USEFLOAT = TRUE`
- **ISR KHÔNG được dùng float** (đã tắt lazy stacking ASPEN/LSPEN). Kiểm bằng lệnh objdump trong `docs/trampoline-setup.md`
- Không dùng lệnh `git clone --recursive` cho Trampoline (kéo cả tflite-micro, lwip...); chỉ init `CMSIS_5`

---

## 4. Lệnh hay dùng (Windows, Git Bash)

```bash
# Build một app Trampoline (chỉ build, không nạp)
scripts/trampoline_build.sh tests/target/fpu_check

# Kiểm tra DBC
python -c "import cantools; db=cantools.database.load_file('can/vehicle.dbc', strict=True); print(db.messages)"

# Build f407_bringup headless (project phải được import vào workspace trước)
"C:/ST/STM32CubeIDE_2.1.1/STM32CubeIDE/stm32cubeidec.exe" --launcher.suppressErrors -nosplash \
  -application org.eclipse.cdt.managedbuilder.core.headlessbuild \
  -data "c:/Users/admin/STM32CubeIDE/workspace_2.1.1" -cleanBuild amr_stm32f407

# Nạp qua ST-Link ở header H1 (PA13 = SWDIO chân 7, PA14 = SWCLK chân 3, GND) — CHỈ khi user đã xác nhận
"C:/ST/STM32CubeIDE_2.1.1/STM32CubeIDE/plugins/com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.win32_2.2.400.202601091506/tools/bin/STM32_Programmer_CLI.exe" \
  -c port=SWD -w "<file .elf>" -v -rst

# Đọc thanh ghi khi board đang chạy: BẮT BUỘC mode=HOTPLUG (mode mặc định tự halt core, đọc ra toàn 0 giả)
STM32_Programmer_CLI.exe -c port=SWD mode=HOTPLUG -r32 <địa chỉ> 1

# App Trampoline: task IDLE ngủ bằng `wfe` gần như suốt thời gian. Hai việc bắt buộc:
#  (a) Trong main, trước StartOS: DBGMCU->CR |= DBGMCU_CR_DBG_SLEEP;  -> thiếu dòng này HOTPLUG báo "Unable to get core ID"
#  (b) RAM (0x2000xxxx) đọc HOTPLUG bình thường. NGOẠI VI (0x4000xxxx: GPIO, RCC...) đọc HOTPLUG khi CPU đang ngủ ra RÁC
#      (CLI không báo lỗi, in lại giá trị cũ, ví dụ 0x080013E5 = địa chỉ CallTerminateTask ở MỌI thanh ghi).
#      Phải halt chớp nhoáng (~0,1 s) rồi cho chạy lại. Đã kiểm 23/10 với f407_blink: ODR/MODER/AHB1ENR ra đúng.
STM32_Programmer_CLI.exe -c port=SWD mode=HOTPLUG -halt -r32 0x40021014 4 -run
#      Muốn không làm dừng CPU thì cho task chép trạng thái vào 1 biến RAM rồi đọc biến đó (ví dụ g_blink_count)

# Nạp Hiwonder qua cổng download (cổng USB số 7, CH9102, USART1, ví dụ COM16) khi không có ST-Link — CHỈ khi user đã xác nhận
# 1) Tạo .bin từ ELF (objcopy đi kèm CubeIDE)
arm-none-eabi-objcopy -O binary f407_bringup/Debug/amr_stm32f407.elf f407_bringup/Debug/amr_stm32f407.bin
# 2) Xoá + ghi + verify; -R -B là cực tính đúng cho mạch tự nạp Hiwonder (mặc định KHÔNG vào được bootloader).
#    Cuối lệnh stm32loader tự kéo BOOT0 thấp rồi reset -> chip chạy chương trình từ Flash
python -m stm32loader -p COM16 -f F4 -R -B -e -w -v -a 0x08000000 f407_bringup/Debug/amr_stm32f407.bin
#    Chỉ đọc ID chip (không ghi): python -m stm32loader -p COM16 -f F4 -R -B   -> mong đợi 0x413

# Mở terminal AN TOÀN trên cổng download: giữ DTR=1 (không reset), RTS=0 (BOOT0 thấp).
# Thiếu 2 cờ này thì bấm reset có thể vào bootloader thay vì chạy chương trình
python -m serial.tools.miniterm --dtr 1 --rts 0 COM16 115200
```

- CubeMX 6.16.1 bản rời chạy dòng lệnh: `%LOCALAPPDATA%\Programs\STM32CubeMX\jre\bin\java.exe -jar STM32CubeMX.exe -q script.txt` (script: `config load <ioc>` / `project generate` / `exit`). ⚠️ Gọi thẳng `STM32CubeMX.exe -q` sẽ tách tiến trình và treo chờ hộp thoại
- Import project headless: `-import "C:\...\f407_bringup"` phải dùng **dấu `\`**; dạng `c:/...` bị Eclipse hiểu nhầm là URI
- ⚠️ Khi ghép code vào khối `USER CODE BEGIN 3`: dấu `}` đóng `while(1)` do CubeMX sinh nằm **bên trong** khối này

---

## 5. Bài học STM32 mang từ dự án xe (chi tiết ở CLAUDE.md của repo `amr_ws`)

1. **Project CubeMX mới KHÔNG tự bật NVIC cho ngắt** (UART, CAN...). Phải bật trong tab NVIC Settings, nếu không `*_Receive_IT` không bao giờ gọi callback
2. **SYS Debug phải là "Serial Wire"**. Để "No Debug" sẽ khoá SWD sau lần nạp đầu. Trên board Hiwonder còn bắt buộc hơn, vì PA15/PB3/PB4 được dùng làm chân khác
3. **Không trộn `HAL_UART_Transmit` (chờ gửi xong) với `HAL_UART_Receive_IT` trên cùng một UART**. Khoá HAL làm RX chết âm thầm. Gửi bằng ghi thanh ghi trực tiếp
4. **Tràn số `uint32_t` khi so thời gian**: không dùng mốc thời gian chụp ở đầu vòng lặp để so với mốc được cập nhật ở giữa vòng lặp; đọc lại `HAL_GetTick()` ngay tại chỗ so sánh
5. **Lệnh dừng / trạng thái an toàn phải nằm NGOÀI vòng phản hồi**, không phụ thuộc cảm biến
6. **Đo ở mức thấp nhất trước khi đổ lỗi phần cứng** (đếm byte thô, đọc thanh ghi qua SWD). Soft reset qua ST-Link có khi không gỡ được trạng thái kẹt; phải rút nguồn cắm lại
7. **Kiểm tra nguồn và dây trước tiên** khi test hỏng: nhiều lần "lỗi phức tạp" hoá ra là quên cấp nguồn hoặc tiếp xúc lỏng

---

## 6. Board Hiwonder "ROS Robot Controller" (STM32F407VET6) — ECU1

MCU STM32F407VET6, 100 chân, Flash 512 KB, RAM 192 KB, SYSCLK **168 MHz**. **Board V1.2 dùng thạch anh HSE 16 MHz (tụ 10 pF) và IMU QMI8658** theo tài liệu V1.2 (`reference/hiwonder_ros_controller/v1.2/`, file Schematic Explanation tr.5 và tr.10); V1.0/V1.1 dùng HSE 8 MHz và MPU-6050. **HSE 16 MHz đã xác nhận trên board (22/10).** `f407_bringup` đã sửa cho 16 MHz (PLLM 16, commit `ba6591e`). Target `stm32f407` của Trampoline cũng đã sửa `HSE_VALUE` = 16 MHz (`stm32f4xx_conf.h:29`, commit `1c7e434` trong submodule, nhánh `amr-f407`, đã push fork 23/10) → `PLL_M` = 16, SYSCLK 168 MHz. ⚠️ Target này **không còn đúng cho STM32F4-Discovery** (thạch anh 8 MHz). Có sẵn transceiver CAN VP230 + trở 120 Ω (R23, hàn cố định → ECU1 nằm ở một đầu bus).

> Nội dung bên dưới chuyển nguyên văn từ `amr_ws` (viết ngày 29/9, lúc còn định lắp board lên xe). **Phần nguồn, quy trình kết nối, sơ đồ chân, rủi ro vẫn đúng.** Các dòng về motor (M1–M4, YX-4055AM), encoder, servo và "đề xuất dùng M1 + M2 cho 2 bánh sau" **không dùng cho đồ án**. Các chỗ nhắc `reference/...` là file trong thư mục `reference/` của repo này (không commit).

Tài liệu gốc (đã đọc 2026-09-29):
- `reference/hiwonder_ros_controller/1. ROS Robot Controller Hardware Introduction.pdf`: đánh số cổng 1–23 dùng bên dưới
- `reference/hiwonder_ros_controller/2. ROS Robot Controller Schematic Explanation.pdf`: § = mục trong file này
- ⭐ **`reference/hiwonder_ros_controller/Ros Robot ControllerV1.1.pdf`**: schematic đầy đủ 3 trang (S1 = MCU/ngoại vi, S2 = CH9102/MPU/CAN/servo, S3 = nguồn/driver/encoder/bus servo), bản V1.1 ngày 2023.03.12. **Khi schematic và file Explanation mâu thuẫn thì tin schematic** (Explanation có ít nhất 3 lỗi, ghi ở bảng dưới)

> ⚠️ **Board thật là V1.2 (22/10/2026), schematic V1.1 chỉ để tham khảo, bảng chân cần xác nhận qua bring-up.** Chưa có schematic V1.2.

Chưa có source firmware Hiwonder cho board này.

**Không trùng board MiniROS Controller** (kế hoạch cũ, đã tạm dừng). Cùng hãng, cùng họ MCU, nhưng là 2 board khác nhau:

| | Board MỚI: "Ros Robot Controller" V1.0/V1.1 | MiniROS (cũ): "Ros Robot Controller **Mini**" V1.0/V2.0 |
|---|---|---|
| Driver motor | **YX-4055AM** ×4 | SA8870 ×4 |
| IMU | **MPU-6050** (I2C2) | QMI8658 (I2C2) |
| Buffer bus servo | **SN74LVC2G125** | 74HC125 |
| CAN | **VP230 + trở 120 Ω** | không ghi nhận |
| Chung | STM32F407VET6, CH9102F, bus servo trên USART6 | |

→ Các ghi chú trong mục "MiniROS Controller — kế hoạch thay thế" **không áp dụng trực tiếp** cho board này.

**Lý do đổi:**
- Có sẵn CAN (VP230 trên PD0/PD1, trở đầu cuối 120 Ω), nên CAN chuyển từ "tùy chọn" thành "làm được" cho ĐATN
- Cùng chip với target `stm32f407` chính thức của Trampoline (168 MHz), nhưng cần đổi PLL_M cho HSE 16 MHz
- Có cổng bus servo cấp VIN, không cần board BusLinker nữa

### Nguồn (đã đối chiếu tài liệu)
- **Một ngõ vào duy nhất**: terminal số 21 (J3, KF301-2P) → net VIN_SW → công tắc nguồn số 20 (**SW4** trên schematic S3) → VIN. Nhận DC 5–12.6 V (Hardware Intro #20, #21)
- VIN → 4× YX-4055AM (chân VDD), mỗi ngõ ra có **cầu chì 2000 mA** (schematic S3 ghi "Fuse 2000mA"; file Explanation §23 ghi 1500 mA → tin schematic, kiểm tra ký hiệu trên linh kiện thật). VIN → cổng servo PWM J4/J5 và cổng bus servo P6
- VIN → buck RT8289 (U7) → 5 V → LDO RT9013-33 → 3V3, sau LDO có cầu chì BSMD0603-050 + TVS SMBJ3.3A. Buck RT8289 thứ hai (U6) cấp cổng "5V 5A" (số 2). Cả hai buck bật chung qua `P_EN` (lấy từ VIN), nên **cổng 5V 5A luôn có điện khi bật công tắc 20**
- **Đo điện áp pin**: VIN → R27 100 kΩ / R26 10 kΩ → **PB0** (ADC1_IN8), tỉ lệ 1/11 (12.6 V → 1.15 V). Dùng được để cảnh báo pin yếu
- **Quy tắc:**
  - Chỉ dùng pin **LiPo 3S** (tối đa 12.6 V). KHÔNG dùng ắc quy chì hay pin 4S
  - Thêm cầu chì ~5 A trên dây từ pin vào terminal 21
  - Jetson dùng nguồn riêng (19 V) như hiện tại. **KHÔNG** lấy nguồn Jetson từ cổng 5V 5A (số 2)
- **Mặt sau board ghi V1.0**: theo Hardware Intro FAQ 4, trên V1.0 hai chân **VREF+ và VDDA phải được nối** (bản V1.0 xuất xưởng đã có cầu nối này). Chỉ ảnh hưởng việc đọc điện áp pin qua ADC. Nếu là V1.0 mà không thấy cầu nối thì hàn nối; V1.1 không cần. **Không áp dụng cho V1.2** (board thật)
- ⚠️ **Đã đo (22/10, V1.2): rail VIN BỊ nuôi ngược ~4,2 V khi chỉ cắm USB** (công tắc 20 OFF, không pin; đọc bằng ADC PB0 của firmware bring-up, chưa đối chiếu đồng hồ). Cả 2 cổng USB-C đưa VBUS vào rail 5V qua diode 1N5819 (§12, §14), nên rail 5V và cả VIN đều có điện từ laptop. **Quy tắc: KHÔNG cắm motor/servo khi chỉ có USB**

### Quy trình kết nối (bắt buộc, rút kinh nghiệm vụ F411 hỏng lần 3)
| Việc | Cổng | Trạng thái nguồn |
|---|---|---|
| Nạp code | Cổng USB số 7 (serial 1/download) → laptop *(xem ghi chú: đề xuất dùng SWD)* | Tắt công tắc 20; laptop cấp 5 V qua USB cho phần logic |
| Chạy thật + debug + truyền với Jetson | Cổng USB số 4 (serial 2, CH9102) → Jetson hoặc laptop | Bật pin |

- Chỉ **MỘT** đường nối giữa board và **một** máy tại một thời điểm. Không dùng CP2102 ở PA2/PA3 song song với cổng số 4
- Cắm/rút cáp khi board đang tắt (công tắc 20 ở OFF)
- Laptop phải rút sạc (chạy pin) khi nối vào board
- Nên chen bộ cách ly USB **ADuM3160** (loại có B0505S) giữa Jetson và cổng số 4
- Nếu node ROS đang giữ cổng serial thì phải tắt node trước khi đọc log trực tiếp
- 📝 **Đề xuất (chưa chốt):** nạp/debug bằng **ST-Link qua header H1** (chân 7 = PA13/SWDIO, chân 3 = PA14/SWCLK, GND; không nối chân 3.3 V của ST-Link khi board đã có nguồn) thay vì cổng số 7. Lý do:
  - (a) Cổng số 7 nạp qua bootloader ROM được (đã xác minh bằng schematic), nhưng không debug/đọc RAM được
  - (b) Đo DWT cần **dump RAM qua SWD `mode=HOTPLUG`**, bootloader serial không làm được
  - (c) Giữ nguyên toolchain hiện tại (CubeIDE debug, `STM32_Programmer_CLI`)
  - ST-Link và cổng số 4 cùng cắm vào **cùng một laptop** thì vẫn tính là một máy

### Sơ đồ chân — trạng thái xác minh
✔ = đã thấy trong tài liệu Hiwonder. ❓ = chưa chắc. ⚠️ = tài liệu mâu thuẫn. Cột "Ngoại vi" là suy ra từ bảng alternate function của F407, **chưa kiểm tra bằng CubeMX**.

| Chức năng | Chân | Ngoại vi dự kiến | Trạng thái |
|---|---|---|---|
| Clock | **V1.2: HSE 16 MHz, 10 pF** (V1.1: 8 MHz, 22 pF) | → 168 MHz (PLLM 16, N 336, P 2, Q 7) | ✔ **Đã xác nhận trên board 22/10**: firmware cấu hình HSE 16 MHz, UART 115200 ra chữ rõ. (Bài Lesson 5 và 2 file `.ioc` ví dụ trong bộ V1.2 ghi 8 MHz là sai với board này) |
| SWD | PA13 (SWDIO), PA14 (SWCLK); có trên H1 chân 7/3 | — | ✔ §10, §11 |
| Cổng USB số 4 "serial 2" (CH9102F U3) | PD8, PD9 | USART3 (PD8=TX, PD9=RX) | ✔ S2 "串口3电路". Là cổng số 4 **suy ra bằng loại trừ** (cổng số 7 là USART1, xem dòng dưới) → vẫn xác nhận nhanh bằng firmware echo |
| Cổng USB số 7 "serial 1/download" (CH9102F U13) | PA9 (TX), PA10 (RX) | USART1 + mạch tự nạp: DTR/RTS → Q5 SS8050/Q6 SS8550/D5 1N4148 → NRST và BOOT0 | ✔ S2 "串口1/下载接口电路". Nạp bằng **bootloader ROM qua USART1** (mạch tự kéo BOOT0 + reset). PA9/PA10 **không** dính motor |
| PWM driver M1 | PE13, PE14 | TIM1_CH3 / TIM1_CH4 | ✔ S3 (U5, cầu chì F1) |
| PWM driver M2 | PE9, PE11 | TIM1_CH1 / TIM1_CH2 | ✔ S3 (U10, F2). ⚠️ File Explanation §23 ghi "PA9, PA11" là **SAI**, đúng là PE9, PE11 |
| PWM driver M3 | PE5, PE6 | TIM9_CH1 / TIM9_CH2 | ✔ S3 (U8, F3) |
| PWM driver M4 | PB8, PB9 | TIM10_CH1 / TIM11_CH1 (tránh TIM4_CH3/CH4 vì TIM4 là encoder M3) | ✔ S3 (U12, F4) |
| Chân YX-4055AM | BI, FI (vào), FO×2, BO×2 (ra), VDD = VIN, GND | — | ✔ S3. ❓ Trong mỗi cặp chân, chân nào là BI và chân nào là FI chưa đọc rõ từ ảnh (chỉ làm đổi chiều quay, đằng nào cũng phải đo). ❓ Bảng chân lý (0/0, 1/1) chưa có datasheet |
| Encoder M1 | PA0 / PA1 | TIM5_CH1/CH2 (32-bit) | ✔ S3, ảnh FAQ 4 ghi TIM5_CH1/CH2 |
| Encoder M2 | PA15 / PB3 | TIM2_CH1/CH2 (32-bit) | ✔ S3. ⚠️ PA15 = JTDI, PB3 = JTDO → **SYS Debug phải là "Serial Wire", KHÔNG được "JTAG"**, nếu không encoder M2 chết |
| Encoder M3 | PB6 / PB7 | TIM4_CH1/CH2 (16-bit) | ✔ S3 |
| Encoder M4 | PB4 / PB5 | TIM3_CH1/CH2 (16-bit) | ✔ S3. PB4 = NJTRST, cùng lưu ý JTAG như trên |
| Header motor (6 chân, HDR 2.0 mm) | M_B, GND, A, B, 5V, M_F (thứ tự đọc từ ảnh) | — | ✔ S3 có đủ các net này. ❓ Thứ tự chân chính xác cần đối chiếu board thật trước khi làm dây cho JGB37-520 |
| Nguồn encoder | **5 V** trên header | — | ✔ S3. ⚠️ Tín hiệu A/B lên tới 5 V (khác lựa chọn 3.3 V trên F411) → tra cột "FT" trong datasheet F407 (DS8626) cho 8 chân encoder |
| CAN1 | PD0 (RX) / PD1 (TX) | CAN1, transceiver VP230, R23 = 120 Ω hàn cố định | ✔ §16 |
| IMU (**V1.2: QMI8658**; V1.1: MPU-6050) | PB10 (SCL) / PB11 (SDA), INT = PB12 | I2C2, pull-up 10 kΩ trên board | ✔ §13. ✔ **Bring-up 22/10: quét I2C2 thấy 0x6A (QMI8658)**. Chưa có driver QMI8658; `bringup.c` chỉ in "chua ho tro" |
| Cổng I2C mở rộng (số 5) | 5V, GND, SDA, SCL (chung bus I2C2) | — | ✔ §9. Có thể gắn BNO055 ở đây (địa chỉ 0x28/0x29, không trùng MPU 0x68) — cổng cấp **5 V**, kiểm tra module chịu được |
| Bus servo | TX_EN = **PE7**, RX_EN = **PE8**, TX = **PC6**, RX = **PC7** | USART6 + buffer SN74LVC2G125 (U14), SERVO_SIGNAL kéo lên 5 V qua R14 1 kΩ | ✔ S3. ⚠️ Explanation §24 ghi "PG6_TX" là lỗi đánh máy: F407VET6 bản 100 chân **không có port G** → PC6. Chân OE của 74LVC2G125 theo datasheet TI là **tích cực mức THẤP** → TX_EN/RX_EN = 0 là bật; xác nhận khi viết `servo_buslinker.c`. ❓ P6 chắc chắn có VIN; P7 nhiều khả năng cũng VIN (đọc ảnh chưa chắc) → **đo trước khi cắm HTS-20H** (servo cần 9.6–12.6 V) |
| Servo PWM | J1 = PA11 (5V), J2 = PA12 (5V), J4 = PC8 (VIN), J5 = PC9 (VIN) | — | ✔ S2 (PA11 chỉ là servo J1, không dính motor) |
| LED người dùng | PE10, **tích cực mức THẤP** | GPIO | ✔ S1 |
| Buzzer | **PA8**, tích cực mức CAO (qua S8050) | GPIO (**không** đặt PA8 làm TIM1_CH1, vì TIM1 dành cho motor) | ✔ S2. ⚠️ Explanation §17 ghi PA4 → tin schematic, kiểm tra bằng 1 tiếng bíp khi bring-up |
| Đo điện áp pin | PB0 | ADC1_IN8, cầu chia 1/11 | ✔ S3 |
| MPU-6050 AD0 | kéo xuống GND qua R19 10 kΩ | → địa chỉ I2C **0x68** | ✔ S2 |
| Nút bấm K1/K2 | PE1 / PE0, kéo lên 10 kΩ, nhấn = mức THẤP | GPIO | ✔ §6 |
| Công tắc enable (số 1) | PD3, kéo lên 10 kΩ, gạt về GND | GPIO input | ✔ §18. ✔ Bring-up 22/10: PD3 = 0 khi gạt công tắc số 1 về phía cổng USB-C (vị trí ON) — tích cực mức thấp. ⚠️ **Chỉ là đầu vào GPIO, không cắt nguồn motor bằng phần cứng.** Firmware tự viết phải tự đọc PD3. Nếu dùng làm e-stop thì phải đặt NGOÀI vòng PID (cùng nguyên tắc "lệnh dừng không phụ thuộc cảm biến") |
| SBUS | PD2 (qua transistor đảo) | UART5_RX | ✔ §15 |
| Bluetooth | PD5 / PD6 | USART2 | ✔ §8 |
| OLED | PB13, PC3, PD11–PD14 | — | ✔ §7 |
| USB host | PB14 / PB15 | OTG_HS (chế độ FS) | ✔ §25 |
| Header H1 (26 chân) | 3V3, 5V×2, GND, PA2, PA3, PA6, PA7, PA13, PA14, PC0, PC1, PC2, PC5, PC10, PC11, PC12, PD4, PD10, PD15, PE2, PE3, CANH, CANL | — | ✔ §11. PA2/PA3 = USART2 (trùng USART2 với Bluetooth PD5/PD6, chỉ chọn một) |

⭐ **Đề xuất dùng M1 + M2 cho 2 bánh sau** (đã xác minh bằng schematic): PWM cả 2 motor nằm trên **một timer TIM1** (CH1–CH4, APB2 168 MHz → 20 kHz ứng với ARR = 8399), encoder nằm trên 2 timer **32-bit** (TIM5, TIM2) → không cần cộng dồn tràn số. TIM1 là timer advanced, phải bật MOE (`HAL_TIM_PWM_Start` tự làm).

### Rủi ro cần kiểm tra khi có board
1. **Cầu chì motor 2 A (theo schematic) vs dòng stall ~2.3 A** của JGB37-520 → đo dòng motor thật (kê bánh lên, có xác nhận của user) trước khi tin vào driver trên board. Chưa rõ loại cầu chì. Nếu là **PTC tự phục hồi** thì quá tải sẽ ngắt rồi tự đóng lại, triệu chứng giống hệt "giật cục" → gặp mất lực chập chờn khi tải nặng thì nghi cầu chì trước khi nghi firmware
2. **YX-4055AM chưa có datasheet**: chưa biết bảng chân lý (IN1/IN2 = 0/0 là thả trôi hay phanh, 1/1 là gì) → xác minh trước khi dùng lại kiểu "PWM một chân, chân kia = 0" của `motor_driver.c`
3. Encoder cấp 5 V (xem bảng chân)
4. Công tắc enable PD3 không phải ngắt cứng (xem bảng chân)
5. ~~Cổng nạp số 7 và mâu thuẫn chân PA9/PA11~~ → **đã giải quyết bằng schematic V1.1** (cổng số 7 = USART1 + mạch tự nạp; motor M2 là PE9/PE11)
6. Rail VIN có thể bị nuôi ngược từ USB (xem mục Nguồn)
7. **MPU-6050 không có từ kế** → yaw tích phân từ gyro sẽ trôi. Nếu cần heading (ĐATN đo sai số heading, EKF sau này) thì BNO055 vẫn là lựa chọn tốt hơn, gắn qua cổng I2C số 5

