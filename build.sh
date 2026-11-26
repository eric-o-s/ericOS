#!/usr/bin/env bash
# Build ericOS into a deployable UF2.
#
#   ./build.sh            compile, report size and verify the UF2
#   ./build.sh --deploy   compile, then copy to a mounted RP2350 drive
#   ./build.sh --clean    clear the arduino-cli build cache first
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SKETCH="$HERE/EricOS"
export PATH="$PATH:$HOME/.local/bin"

FQBN="rp2040:rp2040:rpipico2w:flash=4194304_1048576,freq=200,opt=Optimize2"

# Clear the stock family so picotool emits a plain RP2350_RISCV image rather than
# prepending an ARM safest-block stub. This matches the verified release layout.
UF2_FAMILY=""
BUILD_DIR="$HERE/build"

ARGS=(compile --fqbn "$FQBN" --warnings default
      --build-property "build.uf2family=$UF2_FAMILY")
[[ "${1:-}" == "--clean" ]] && ARGS+=(--clean)

mkdir -p "$BUILD_DIR"

echo "==> Compiling ericOS for RP2350"
arduino-cli "${ARGS[@]}" \
  --build-path "$BUILD_DIR/obj" \
  --export-binaries \
  --output-dir "$BUILD_DIR" \
  "$SKETCH"

UF2="$BUILD_DIR/EricOS.ino.uf2"
[[ -f "$UF2" ]] || UF2="$BUILD_DIR/EricOS.uf2"
if [[ ! -f "$UF2" ]]; then
  echo "!! No UF2 produced" >&2
  exit 1
fi

echo
# wc -c works on both GNU (Linux) and BSD (macOS); stat flags differ between them.
echo "==> UF2: $UF2 ($(wc -c < "$UF2" | tr -d ' ') bytes)"
python3 "$HERE/verify_uf2.py" "$UF2"

if [[ "${1:-}" == "--deploy" ]]; then
  TARGET="${RPI_RP2:-}"
  if [[ -z "$TARGET" ]]; then
    # The RP2350 boot ROM mounts as "RP2350"; older boards use "RPI-RP2".
    # macOS mounts drives under /Volumes, Linux under /run/media or /media.
    TARGET="$(find /Volumes /run/media /media -maxdepth 3 -type d \
      \( -name 'RP2350*' -o -name 'RPI-RP2*' \) 2>/dev/null | head -1 || true)"
  fi
  if [[ -z "$TARGET" || ! -d "$TARGET" ]]; then
    echo "!! Board drive not found. Enter BOOTSEL mode," >&2
    echo "   or set RPI_RP2=/path/to/board" >&2
    exit 1
  fi
  cp "$UF2" "$TARGET/ericOS.uf2"
  sync
  echo "==> Deployed to $TARGET/ericOS.uf2"
  echo "    The board reboots automatically when the drive remounts."
fi
