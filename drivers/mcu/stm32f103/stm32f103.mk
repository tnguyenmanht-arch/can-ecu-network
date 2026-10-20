# stm32f103.mk — Cấu hình build dùng chung cho mọi target STM32F103 (bare-metal, KHÔNG HAL)
#
# Makefile của target khai báo trước khi include file này:
#   REPO_ROOT  đường dẫn tương đối tới gốc repo (ví dụ ../../..)
#   TARGET     tên file đầu ra (không đuôi)
#   SRCS       danh sách file .c của target (kể cả driver dùng chung)
#   INCS       (tuỳ chọn) thêm -I...
#
# Gọi qua scripts/f103_build.sh để có sẵn arm-none-eabi-gcc và make của STM32CubeIDE.

# ---- Công cụ ----
CROSS   ?= arm-none-eabi-
CC      := $(CROSS)gcc
OBJCOPY := $(CROSS)objcopy
SIZE    := $(CROSS)size

# ---- Đường dẫn dùng chung ----
MCU_DIR   := $(REPO_ROOT)/drivers/mcu/stm32f103
CMSIS_DIR := $(REPO_ROOT)/third_party/cmsis_f1
LDSCRIPT  := $(MCU_DIR)/STM32F103C8_FLASH.ld
BUILD     ?= build

# File của ST: SystemInit/SystemCoreClockUpdate và bảng vector + Reset_Handler
SRCS += $(CMSIS_DIR)/device/Source/system_stm32f1xx.c
ASRCS := $(CMSIS_DIR)/device/Source/startup_stm32f103xb.s

INCS += -I$(CMSIS_DIR)/core/Include -I$(CMSIS_DIR)/device/Include -I$(MCU_DIR)

# ---- Cờ biên dịch ----
MCUFLAGS := -mcpu=cortex-m3 -mthumb
DEFS     := -DSTM32F103xB
# Build debug: -Og -g3 để đọc biến volatile bằng Live Expressions
CFLAGS   := $(MCUFLAGS) $(DEFS) $(INCS) -Og -g3 -Wall -Wextra \
            -ffunction-sections -fdata-sections -MMD -MP
ASFLAGS  := $(MCUFLAGS) -g3 -x assembler-with-cpp
LDFLAGS  := $(MCUFLAGS) -T$(LDSCRIPT) --specs=nano.specs \
            -Wl,--gc-sections -Wl,-Map=$(BUILD)/$(TARGET).map -Wl,--print-memory-usage

# ---- File đầu ra: mọi .o dồn vào $(BUILD)/ (tên file nguồn không được trùng nhau) ----
OBJS := $(addprefix $(BUILD)/,$(notdir $(SRCS:.c=.o) $(ASRCS:.s=.o)))
vpath %.c $(sort $(dir $(SRCS)))
vpath %.s $(sort $(dir $(ASRCS)))

ELF := $(BUILD)/$(TARGET).elf
HEX := $(BUILD)/$(TARGET).hex
BIN := $(BUILD)/$(TARGET).bin

.PHONY: all clean size

all: $(ELF) $(HEX) $(BIN) size

$(BUILD):
	mkdir -p $@

$(BUILD)/%.o: %.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: %.s | $(BUILD)
	$(CC) $(ASFLAGS) -c $< -o $@

$(ELF): $(OBJS) $(LDSCRIPT)
	$(CC) $(LDFLAGS) $(OBJS) -o $@

$(HEX): $(ELF)
	$(OBJCOPY) -O ihex $< $@

$(BIN): $(ELF)
	$(OBJCOPY) -O binary $< $@

size: $(ELF)
	$(SIZE) $<

clean:
	rm -rf $(BUILD)

-include $(wildcard $(BUILD)/*.d)
