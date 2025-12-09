#!/usr/bin/env bash
set -euo pipefail

# 项目根目录（脚本所在目录）
SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

COMPILER=${CC:-gcc}
CFLAGS=${CFLAGS:-"-O2 -fPIC -I."}
LDFLAGS=${LDFLAGS:-"-shared"}
LIBS=${LIBS:-"-ldl -lpthread"}
TARGET=USB_G2X.so
# 源文件清单（保持与项目一致，如有新增 .c 记得补充）
SRCS=(
  usb_application.c
  usb_middleware.c
  usb_device.c
  usb_protocol.c
  usb_log.c
  usb_spi.c
  usb_bootloader.c
  usb_power.c
  usb_gpio.c
  usb_i2s.c
  usb_pwm.c
  usb_uart.c
  usb_audil.c
)

usage() {
  cat <<EOF
Usage: $0 [build|clean|rebuild]
  build    直接用 gcc 编译并生成 USB_G2X.so (默认)，不依赖 Makefile
  clean    清理构建产物（删除 USB_G2X.so）
  rebuild  先清理再重新编译
EOF
}

cmd=${1:-build}
case "$cmd" in
  build)
    echo "[BUILD] $TARGET"
    if "$COMPILER" $CFLAGS $LDFLAGS -o "$TARGET" "${SRCS[@]}" $LIBS; then
      echo "[SUCCESS] Build succeeded: $TARGET"
    else
      rc=$?
      echo "[FAIL] Build failed with code $rc"
      exit $rc
    fi
    ;;
  clean)
    echo "[CLEAN] $TARGET"
    rm -f -- "$TARGET"
    ;;
  rebuild)
    "$0" clean
    "$0" build
    ;;
  -h|--help|help)
    usage
    ;;
  *)
    echo "未知命令: $cmd" >&2
    usage
    exit 1
    ;;
 esac
