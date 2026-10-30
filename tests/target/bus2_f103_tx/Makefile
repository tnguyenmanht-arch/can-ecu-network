# Test bus thật 2 board, phía GỬI (STM32F103). CHỈ BUILD, không nạp.
#   scripts/f103_build.sh tests/target/bus2_f103_tx          -> build
#   scripts/f103_build.sh tests/target/bus2_f103_tx clean    -> xoá build/

REPO_ROOT := ../../..
TARGET    := bus2_f103_tx

SRCS := main.c \
        $(REPO_ROOT)/drivers/bxcan/bxcan.c \
        $(REPO_ROOT)/drivers/mcu/stm32f103/can_port_f103.c

INCS := -I$(REPO_ROOT)/drivers/bxcan

include $(REPO_ROOT)/drivers/mcu/stm32f103/stm32f103.mk
