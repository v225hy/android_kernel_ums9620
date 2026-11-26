// SPDX-License-Identifier: GPL-2.0-only
/* CI-only compile probe; deliberately not linked into the kernel.
 * Stock CP3NV1 unisoc-iommu.ko sprd_iommu_map loads ion_buffer.sg_table
 * with `ldr x24, [x3, #104]`. Its config has E_SHOW_MEM disabled.
 * This checks that narrow interface, not general vendor module ABI safety.
 */
#include <linux/build_bug.h>
#include <linux/ion.h>
#include <linux/stddef.h>

void cp3nv1_check_stock_ion_layout(void)
{
	BUILD_BUG_ON(offsetof(struct ion_buffer, sg_table) != 104);
}
