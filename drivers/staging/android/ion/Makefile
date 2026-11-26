# SPDX-License-Identifier: GPL-2.0
obj-$(CONFIG_ION)	+= ion.o ion_buffer.o ion_dma_buf.o ion_heap.o
# Explicit single-object CI target; not linked into ion or Image.
extra-y += cp3nv1_abi_check.o
CFLAGS_ion_buffer.o 	= -I$(src)
obj-y			+= heaps/
obj-$(CONFIG_ION_SPRD)  += sprd/
