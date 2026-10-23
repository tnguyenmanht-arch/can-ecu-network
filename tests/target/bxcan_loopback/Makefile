# Target test loopback cho driver bxCAN (STM32F103). CHỈ BUILD, không nạp.
#   scripts/f103_build.sh tests/target/bxcan_loopback          -> build
#   scripts/f103_build.sh tests/target/bxcan_loopback clean    -> xoá build/

REPO_ROOT := ../../..
TARGET    := bxcan_loopback

SRCS := main.c \
        $(REPO_ROOT)/drivers/bxcan/bxcan.c \
        $(REPO_ROOT)/drivers/mcu/stm32f103/can_port_f103.c

INCS := -I$(REPO_ROOT)/drivers/bxcan

include $(REPO_ROOT)/drivers/mcu/stm32f103/stm32f103.mk
