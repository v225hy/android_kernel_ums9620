# CP3NV1 stock-module compatibility fixes

These are targeted fixes, not proof of general ABI compatibility between the
stock 5.4.210 modules and this 5.4.302 kernel. No device flashing is performed by
CI. Keep the known-working vendor_boot unchanged for the next controlled test.

## Display SG-table failure

Captured logs show `sprd_iommu_map ... get sg error ... ret 0 table NULL` followed
by `sprd_dpu_prepare_fb ... failed to map iommu address`. DRM card0/DSI-1 exist.
The saved stock `unisoc-iommu.ko` disassembly of `sprd_iommu_map` contains:

```
1c08: ldr x3, [x21, #8]      // data->buf
1c14: ldr x24, [x3, #104]    // ion_buffer.sg_table
```

Stock CONFIG_E_SHOW_MEM is disabled, while sprd_qogirn6pro_defconfig enables it.
In include/linux/ion.h it changes the first ion_buffer member from a 16-byte
list_head to a union containing a 24-byte rb_node, shifting subsequent fields.
CI disables this debug option and compiles cp3nv1_abi_check.c against the actual
kernel headers to assert offsetof(ion_buffer, sg_table) == 104. This repairs the
identified layout mismatch without changing the stock module or adding unsafe
hard-coded pointer offsets. Other kernel/module interfaces remain unverified.

## zram/LZ4 kernel stack overflow

Pstore reports `Comm: mkswap`, kernel stack overflow in memset / LZ4 compression,
through crypto_compress and zram_bvec_write. The bundled userspace-style LZ4
stateless entry allocates its roughly 16 KiB state on the kernel task stack.
Crypto and F2FS callers already allocate compression contexts. Reuse those via
LZ4 extState APIs, with compile-time workspace size checks; do not allocate
sleeping memory in the compression hot path or enlarge the task stack as a
workaround. Preserve the existing stateless exported API; unrelated external
callers of that API are not made stack-safe by fixing these in-tree callers.

## Acceptance checks on device (not performed by this patch)

- Confirm Image version/config and stock vendor modules actually in use.
- /data mounted read-write; no unknown age_extent_cache option rejection.
- zram writes and swap initialization survive beyond the former panic window.
- No SG-table NULL errors; valid IOVA mappings and successful DRM commits.
- Verify a visible picture, brightness changes, suspend/resume and reboot.

The age_extent_cache parser change from the earlier commit is only a warned
compatibility no-op, not an upstream age-cache implementation backport.
