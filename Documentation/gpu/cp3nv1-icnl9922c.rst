CP3NV1 ICNL9922C panel bring-up
=============================

This is experimental native Unisoc panel support, not a tested flashable build.
The existing sprd-mipi-panel-drv consumes the LCD device-tree description; a
separate mainline drm_panel driver is not interchangeable with this host, which
uses container_of(..., struct sprd_panel, base) in DSI, DPU and DSC code.

Sources and provenance
----------------------

The input U-Boot partition is 3145728 bytes, SHA-256::

  bf776cfe56116951dbcea4c9096586b22ee40386f1d229ca9458e23bf9f325ac

AArch64 code starts at file offset 0x200. Static VA = file offset + 0xb4fffe00.
The panel registration at file 0x1abb20 has ID 0x9922c and points to the driver
at 0x1c78c8. Its info at 0x1c78d8 names LCD_ICNL9922C_MIPI_SHARP.
The read-ID callback reads one byte of DCS register 0xda and accepts 0xf9;
0x9922c is a registration ID, not a three-byte value read from the panel.

All 39 initialization records are transcribed byte-for-byte from file offsets
[0x1c7438, 0x1c78c6). Their 1166 bytes have SHA-256::

  4801fb3c20c5d27160c4d2e3d1a736d412155b7527e65d3da214d03ba4c1b9c5

The U-Boot zero sentinel and alignment are deliberately excluded: the kernel
parser consumes the property length. Headers are type, wait_ms, length_be16.
Use sprd,use-dcs-write: the kernel ignores individual type tags and chooses
DCS versus generic writes for the entire table. Sleep Out and Display On,
including 120 ms and 20 ms delays, remain in INIT because normal enable sends
INIT, not the separately parsed sleep-out property. The shutdown sequence
(Display Off, 20 ms; Enter Sleep, 120 ms) is a standard DCS bring-up default,
not recovered vendor shutdown code; validate it on hardware.

The supplied Chipone ICNL9922C V0.00 PDF (July 2024, seven pages) is a feature
summary, not a panel-specific register/programming specification. It confirms
TDDI, RGB formats, DSC 1.2a support, and up to four D-PHY lanes, but does not
supply this module's reset, porch, bias or register sequence. It does not prove
the CP3NV1 supports every refresh rate advertised for the IC.

The external chipone_tddi repository is a vendor touchscreen driver port for a
KONKR SM8650 device targeting mainline-based 7.0.x, not an upstream LCD driver.
Its I2C address, GPIOs, axis inversion and 1080x1920 touch coordinates are NOT
CP3NV1 board evidence and have not been imported.

Native integration
------------------

The source is arch/arm64/boot/dts/sprd/lcd/lcd_icnl9922c_mipi_sharp.dtsi,
following the existing lcd_nt36672e_truly_mipi_fhd.dtsi format. An isolated
cp3nv1-icnl9922c-panel-overlay.dts adds only /lcds data. It does not change the
UMS9620 public SoC tree or select a reference board's wiring.

After board validation, include the LCD dtsi in the actual CP3NV1 board overlay,
or merge the isolated panel-data overlay into the complete device tree. The
bootloader must supply exactly::

  lcd_name=LCD_ICNL9922C_MIPI_SHARP

The generic driver looks up /lcds/<lcd_name>; the IC is not independently
matched through a new compatible. Loading the data overlay alone is NOT a
complete board integration or proof the screen will light up.

Recovered mode and DSC
----------------------

* Active pixels: 1080 x 2408; pixel-clock request: 384000000 Hz.
* HFP/HBP/HSYNC: 102/102/4; VFP/VBP/VSYNC: 2520/32/4.
* PHY bit-rate request: 1100000 kHz, four data lanes.
* DSC enabled; slice width 540, height 8; output component depth 8.
* No guessed physical width/height, VRR or enabled ESD polling.

DSC is not inferred from the product name: panel-info byte +66 enters the
U-Boot DSC setup at VA 0xb512b3d8 / 0xb512c428. Fields +68/+72 feed slice
geometry and +76 feeds DSC depth. These match sprd_dsc.c's existing interface.
The DSC payload in the initialization stream is preserved rather than replaced
with the NT36672E template's commands.

Video burst mode and 20000 kHz escape clock are framework/template defaults,
not independently confirmed U-Boot enum mappings or measured frequencies.
They must be checked before deployment. Do not copy the raw U-Boot video enum
into sprd,dsi-work-mode without interpreting the two different enums.

The DPU mode implies 384000000/(1288*4964) = approximately 60.06 Hz. However,
the traced DSI path writes VFP 2520 to a 10-bit field (472 after masking), while
DPU preserves 2520. This unresolved discrepancy MUST NOT be silently corrected
or described as a measured 60 Hz mode. Inspect actual host registers and active
execution paths before testing a newly built boot image.

Power, reset and backlight prerequisites
---------------------------------------

U-Boot drives GPIO192 high (10 ms), GPIO191 high (20 ms), then GPIO11 high
(5 ms), low (5 ms), high (20 ms). These are U-Boot numbers, not yet verified
Linux GPIO phandles. Only the reset levels/delays are encoded in the LCD data.
The reset-off callback only drives GPIO11 low for 5 ms; it does not disable the
other GPIOs.

After a successful panel ID, U-Boot writes two I2C messages on software bus 6,
address 0x3e: 00 14 and 01 14. The bias IC and conversion from 0x14 to voltage
are unknown. Do not infer a supply voltage from the IC feature summary.

The built-in generic panel driver obtains power-supply, avdd/avee/reset-gpios
and sprd,backlight from the DSI child node, not from /lcds. Its built-in power
sequence uses 5 ms per enable GPIO and has no I2C bias implementation. It is
therefore not equivalent to U-Boot on its own.

Sequencing implementation
-------------------------

drivers/regulator/sm5109-regulator.c provides a board-specific sequencing
supply that replays the recovered U-Boot behaviour in one place: ENP high plus
10 ms, ENN high plus 20 ms, reset high 5 ms, low 5 ms, high 20 ms, then raw
writes 0x00=0x14 and 0x01=0x14 over I2C. It intentionally exposes no voltage
operations, because the register encoding is not documented by the available
one-page SM5109 feature summary and must not be guessed as "6 V".

The opt-in board overlay cp3nv1-icnl9922c-board-overlay.dtso enables I2C6,
instantiates the supply at address 0x3e, owns ENP/ENN/reset GPIOs 192/191/11 and
points the DSI child's power-supply at it while setting
sprd,external-power-sequencer. In that mode the generic panel driver relinquishes
the shared TDDI reset line, so exactly one driver owns each GPIO. Without the
overlay the previously committed panel data behaves as before.

Known gaps: the GPIO numbers are U-Boot global indices and their Linux mapping,
polarity and pinmux are still unverified; no regulator-boot-on handoff is
claimed, so an already-lit panel left by the bootloader is re-sequenced when the
panel is next prepared; the reverse-order rail shutdown in the driver is an
engineering choice, not recovered behaviour; the ID read that U-Boot performs
between reset and the bias writes cannot be issued from this I2C driver.
Backlight wiring and the out-of-tree display module's external bias symbols
remain outside this change.

Validation
----------

Run the self-contained static test from the kernel repository root::

  python3 scripts/check-cp3nv1-panel.py

Optionally provide the original U-Boot to compare the initialization stream::

  python3 scripts/check-cp3nv1-panel.py --uboot /path/to/uboot_a.img

The test compiles the standalone overlay using cpp and dtc in a temporary
directory, verifies record boundaries, hashes, delays, DSC geometry and timing,
and optionally compares the source image byte range. It does not compile the
kernel or exercise hardware. No first-boot, suspend/resume, bias or touch test
has been performed. The current source baseline is 5.4.302; stock boot is
5.4.210 and vendor module ABI compatibility remains a separate task.
