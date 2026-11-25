#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-only
# Repack only; never flash. Stock ramdisk/DTB/vendor_boot remain unchanged.
set -euo pipefail
ROOT=$(git rev-parse --show-toplevel)
MAGISKBOOT=$(realpath "${1:?usage: repack-cp3nv1-boot.sh MAGISKBOOT [IMAGE] [OUTPUT]}")
IMAGE=$(realpath "${2:-$ROOT/arch/arm64/boot/Image}")
OUTPUT=$(realpath -m "${3:-$ROOT/out-boot/boot.img}")
STOCK="$ROOT/boot/stock/boot.img"
echo "4c8b8719f7a6a93f45b998ffd5ea7c8b87fcdff7e6aec7a43484a55ae17e948e  $STOCK" | sha256sum -c -
test -s "$IMAGE"
WORK=$(mktemp -d)
# Keep temporary evidence on failure, rather than recursively deleting paths.
cd "$WORK"
mkdir original verify
cd original
"$MAGISKBOOT" unpack -h "$STOCK"
# magiskboot transparently decompresses/recompresses the kernel using stock format.
cp "$IMAGE" kernel
"$MAGISKBOOT" repack "$STOCK" "$WORK/new-boot.img"
test -s "$WORK/new-boot.img"
test "$(stat -c %s "$WORK/new-boot.img")" -le "$(stat -c %s "$STOCK")"
cd ../verify
"$MAGISKBOOT" unpack -h "$WORK/new-boot.img"
cmp "$IMAGE" kernel
# Compare every unpacked component other than the deliberately replaced kernel.
for component in "$WORK"/original/*; do
    name=$(basename "$component")
    [ "$name" = kernel ] && continue
    [ -f "$component" ] || continue
    cmp "$component" "$WORK/verify/$name"
done
mkdir -p "$(dirname "$OUTPUT")"
cp "$WORK/new-boot.img" "$OUTPUT"
sha256sum "$OUTPUT" > "$OUTPUT.sha256"
printf '%s\n' \
    'Repacked with magiskboot v28.1; only the kernel was replaced.' \
    'Re-unpack verified kernel equality and unchanged stock components/header.' \
    'No Magisk ramdisk patch, AVB signing, vendor_boot modification or flashing performed.' \
    'Unsigned experimental boot: bootloader acceptance and hardware operation unverified.' \
    > "$(dirname "$OUTPUT")/README.txt"
echo "PASS: $OUTPUT (work evidence: $WORK)"
