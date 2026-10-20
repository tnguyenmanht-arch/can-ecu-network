#!/usr/bin/env bash
# Build 1 target STM32F103 (bare-metal, Makefile) tren Windows (Git Bash). CHI BUILD, khong nap.
#
#   scripts/f103_build.sh tests/target/bxcan_loopback          -> build
#   scripts/f103_build.sh tests/target/bxcan_loopback clean    -> xoa build/
#
# Dung arm-none-eabi-gcc va make di kem STM32CubeIDE (khong can cai them).
set -euo pipefail

APP_DIR="${1:?Cach dung: $0 <thu muc target co Makefile> [target make...]}"
shift

REPO="$(cd "$(dirname "$0")/.." && pwd)"
CUBE="${CUBEIDE_PLUGINS:-/c/ST/STM32CubeIDE_2.1.1/STM32CubeIDE/plugins}"
ARM_BIN="${ARM_BIN:-$(ls -d "$CUBE"/com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.*/tools/bin | head -1)}"
MAKE_BIN="${MAKE_BIN:-$(ls -d "$CUBE"/com.st.stm32cube.ide.mcu.externaltools.make.*/tools/bin | head -1)}"

for p in "$ARM_BIN/arm-none-eabi-gcc.exe" "$MAKE_BIN/make.exe"; do
  [ -f "$p" ] || { echo "Khong tim thay: $p" >&2; exit 1; }
done
export PATH="$ARM_BIN:$MAKE_BIN:$PATH"

make -C "$REPO/$APP_DIR" "$@"
