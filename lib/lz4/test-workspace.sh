#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-only
# Host regression for the real freestanding LZ4 implementation, not liblz4.so.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
tmp=$(mktemp -d)
# Only delete the directory returned by mktemp, after verifying its location.
case "$tmp" in /tmp/tmp.*) trap 'rm -rf -- "$tmp"' EXIT HUP INT TERM ;; *) exit 1 ;; esac
mkdir -p "$tmp/linux"
printf '#include <stddef.h>\n#include <stdint.h>\n' > "$tmp/linux/types.h"
printf '#include <stddef.h>\n#include <stdint.h>\n' > "$tmp/linux/kernel.h"
printf '#define EXPORT_SYMBOL(x)\n' > "$tmp/linux/module.h"
printf '#include <string.h>\n' > "$tmp/linux/string.h"
printf '#include <limits.h>\n' > "$tmp/linux/limits.h"
cat > "$tmp/test.c" <<'EOF'
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/lz4.h>
_Static_assert(LZ4_MEM_COMPRESS >= sizeof(LZ4_stream_t), "workspace size");
struct guarded_state {
    unsigned long before;
    LZ4_stream_t state;
    unsigned long after;
};
static struct guarded_state states[2];
static char src[262144], dst[263200], decoded[262144];
int main(void)
{
    static const int sizes[] = { 0, 1, 12, 4096, 65535, 65536, 262144 };
    unsigned int seed = 1;
    unsigned int round, n, i;
    for (round = 0; round < 16; round++) {
        for (i = 0; i < sizeof(src); i++) {
            seed = seed * 1664525u + 1013904223u;
            src[i] = round & 1 ? seed >> 24 : i % 31;
        }
        for (n = 0; n < sizeof(sizes) / sizeof(sizes[0]); n++) {
            int size = sizes[n], len;
            struct guarded_state *s = &states[round & 1];
            s->before = 0x1234abcd;
            s->after = 0x5678abcd;
            /* extState must initialize stale/nonzero state on every call. */
            memset(&s->state, 0xa5, sizeof(s->state));
            len = LZ4_compress_fast_extState(&s->state, src, dst,
                    size, LZ4_compressBound(size), 1);
            if (len <= 0 || LZ4_decompress_safe(dst, decoded, len, size) != size ||
                memcmp(src, decoded, size) || s->before != 0x1234abcd ||
                s->after != 0x5678abcd)
                return 1;
            if (size > 12 && LZ4_compress_fast_extState(&s->state, src,
                    dst, size, 1, 1) != 0)
                return 2;
        }
    }
    printf("PASS: 112 roundtrips, bounds failures, reused contexts, canaries; workspace=%zu bytes\n",
           sizeof(LZ4_stream_t));
    return 0;
}
EOF
"${CC:-cc}" -std=gnu11 -O2 -fstack-usage -I"$tmp" -I"$root/include" \
    -c "$root/lib/lz4/lz4.c" -o "$tmp/lz4.o"
"${CC:-cc}" -std=gnu11 -O2 -I"$tmp" -I"$root/include" \
    "$tmp/test.c" "$tmp/lz4.o" -o "$tmp/test"
"$tmp/test"
# Fail if the context-taking API regresses to a page-sized stack frame.
awk -F '\t' '/:LZ4_compress_fast_extState\t/ { print "extState stack: " $2 " bytes"; seen=1; if ($2 >= 4096) exit 1 } END { if (!seen) exit 1 }' "$tmp/lz4.su"
