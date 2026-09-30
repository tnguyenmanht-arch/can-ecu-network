# can-ecu-network

Mô phỏng một mạng ECU ô tô trên bàn qua CAN, phần mềm chia tầng theo kiểu AUTOSAR.
Đồ án tốt nghiệp, chương trình Kỹ sư chuyên sâu Ô tô số, Đại học Bách khoa Hà Nội.

```
 ECU1 VCU            ECU2 Pedal          ECU3 Body           Jetson Orin Nano
 STM32F407           STM32F103           STM32F103           Linux, SocketCAN
 Trampoline (OSEK)   bare-metal          bare-metal          gateway → HMI Android
     │                   │                   │                   │
═════╧═══════════════════╧═══════════════════╧═══════════════════╧═════  CAN 500 kbit/s
```

- Hợp đồng giữa các ECU là file [`can/vehicle.dbc`](can/vehicle.dbc).
- Mỗi ECU chia tầng SWC → RTE → COM → CanIf → driver. COM và CanIf viết bằng C thuần, dùng chung cho mọi chip.
- Kiến trúc chi tiết: [`docs/can-network-architecture.md`](docs/can-network-architecture.md).

**Trạng thái:** đang thiết kế. Đã có DBC nháp, tài liệu kiến trúc, toolchain Trampoline build được. Chưa có firmware mạng CAN.

## Clone

Trampoline (hệ điều hành OSEK của ECU1) là submodule trỏ tới
[fork có vá](https://github.com/tnguyenmanht-arch/trampoline/tree/amr-f407). **Không dùng `--recursive`**:
Trampoline có nhiều submodule nặng mà dự án không cần.

```bash
git clone https://github.com/tnguyenmanht-arch/can-ecu-network.git
cd can-ecu-network
git submodule update --init third_party/trampoline
git -C third_party/trampoline submodule update --init --depth 1 machines/cortex-m/CMSIS_5
```

Build goil và app Trampoline: [`docs/trampoline-setup.md`](docs/trampoline-setup.md).

## Thư mục

| Thư mục | Nội dung |
|---|---|
| `can/` | File DBC |
| `docs/` | Kiến trúc mạng CAN, cài đặt Trampoline, checklist kiểm tra board. `docs/archive/` là tài liệu của hướng đề tài cũ |
| `f407_bringup/` | Firmware kiểm tra board Hiwonder (STM32F407) khi mới nhận hàng. Không phải firmware ECU1 |
| `tests/target/` | Chương trình chạy trên board, ví dụ `fpu_check` (kiểm tra lưu ngữ cảnh FPU trong Trampoline) |
| `tools/dwt/` | Đo thời gian thực thi bằng bộ đếm DWT của Cortex-M |
| `scripts/` | Script build |
| `third_party/trampoline/` | Submodule Trampoline |

Các thư mục `bsw/`, `drivers/`, `ecu1_vcu/`, `ecu2_pedal/`, `ecu3_body/`, `jetson_gateway/`, `tools/comgen/`
sẽ có khi bắt đầu viết code (xem mục 7 của tài liệu kiến trúc).
